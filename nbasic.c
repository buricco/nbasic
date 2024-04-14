/*
 * (C) Copyright 2024 S. V. Nickolas.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.
 *
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * By default, this program uses "ttystdio.c", which provides a reasonable
 * I/O library for any system where the <stdio.h> I/O routines work.  But on
 * some GUI operating systems, this may not be reasonable.  In this case, it
 * is possible to write a custom console library.  A primitive example of this
 * is included in "ttysdl.c"; to use this you will need to link in SDL the
 * usual way and #define CUSTOM_GETLINE to use our line input routine instead
 * of the system routine.
 * 
 * This may be desirable for other reasons, e.g., to provide UTF-8 support on
 * older systems.
 */

#include <ctype.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * This is intended to be a vaguely Microsoft-like dialect of BASIC.
 * That said, there is no need for bug-compatibility with any particular
 * subdialect and we are free to raise some of Microsoft's limits.
 * stderr should only be used during the initialization phase, or if due to
 * some error condition the interpreter itself must die screaming.
 * Programs should be stored in memory in a crunched form.
 */

static char *copyright=
 "@(#) (C) Copyright 2024 S. V. Nickolas\n";

#include "tty.h"

/* Necessary when building with some MS-DOS compilers. */
#ifdef __MSDOS__
#define EXTGETLINE
#define MOVERAISE
#endif

/*
 * If compiling somewhere where getline(3) is unavailable, take it from
 * NetBSD (/usr/src/tools/compat/getline.c) and #define EXTGETLINE.
 * For example, this is needed when building for DOS with Watcom C.
 * (This does not work on 16-bit DOS.  You don't want this on 16-bit DOS.)
 */
#ifdef EXTGETLINE
extern ssize_t getline(char **, size_t *, FILE *);
#endif

/*
 * If you get a link error about raise (e.g., DJGPP), #define MOVERAISE.
 */
#ifdef MOVERAISE
#define raise i_raise
#endif

/*
 * The RAM available for BASIC program space.
 * ramlen will contain the length of the buffer.
 * The following macros can be overridden to define the specifications by
 * which RAM is allocated to BASIC (in bytes):
 *   MINRAM - minimum amount of RAM below which a malloc() failure is fatal
 *   MAXRAM - maximum amount of RAM nbasic will attempt to malloc()
 *   INCRAM - amount of RAM decremented between malloc() failures
 */
uint8_t *RAM;
uint32_t ramlen;
#ifndef MINRAM
#define MINRAM  4096
#endif
#ifndef MAXRAM
#define MAXRAM  1048576
#endif
#ifndef INCRAM
#define INCRAM  1024
#endif

/* Length of the stack. */
#ifndef BASSTK
#define BASSTK  2048
#endif

uint8_t basstk[BASSTK];
uint16_t stkptr;

/* Line pointer.  Set to 0xFFFFFFFF when in immediate mode. */
uint32_t curlin;

uint8_t *cmdptr;
uint8_t cmdlin[256];
uint32_t prgtop;
uint8_t *myptr;

uint8_t *pointer_to_nothing = "";

int trace;
int die;
uint32_t lp, np;

#ifdef CUSTOM_GETLINE
uint8_t cmd[256];
#endif

/*
 * While not currently used, this should be set by a SIGINT handler.
 * Periodically, this value should be checked, and if found, do a STOP.
 */
int brkraised;
uint32_t brklin;
uint8_t *brkptr;

struct cmdtok {
 uint8_t *content;
 int (*function)(void);
};

enum B_ERROR {
 BE_BK,
 BE_SN,
 BE_UL,
 BE_OM,
 BE_LS,
 BE_NE,
 BE_AD,
 BE_IO,
 BE_ND,
 BE_WP,
 BE_DF,
 BE_CN,
 BE_UP /* Last Error + 1 */
} b_err;

char *ermsg[]={
 "Break",
 "Syntax error",
 "Undefined line number",
 "Out of memory",
 "String too long",
 "File not found",
 "Access denied",
 "I/O error",
 "No such file system",
 "Read-only file system",
 "Disk full",
 "Cannot continue",
 NULL
};

/* Forward declarations */
int       b_exec   (uint32_t line);
uint32_t  findptr  (uint32_t line);
void      list     (uint32_t start, uint32_t end);
void      raise    (enum B_ERROR e);

/* Helper functions for printing various types. */

void i_puts (char *s)
{
 char *p;
 
 for (p=s; *p; p++) i_putch(*p);
}

void i_putu (uint32_t x, int align)
{
 uint8_t t[11];
 
 sprintf(t, align?"%10lu":"%lu", x);
 i_puts(t);
}

#ifdef CUSTOM_GETLINE
int i_getln (char *prompt)
{
 uint8_t l;
 
top:
 memset(cmd, 0, 256);
 l=0;
 i_puts(prompt);
 while (1)
 {
  int c;
  
  c=i_getch();
  if (c<0) return -1;
  if ((c==8)||(c==127)) /* ^H / Del */
  {
   if (!l)
   {
    i_putch(7);
    continue;
   }
   i_putch(8);
   i_putch(' ');
   i_putch(8);
   l--;
   continue;
  }
  if ((c==10)||(c==13))
  {
   cmd[l]=10;
   i_putch('\n');
   return l;
  }
  if (c==24) /* ^X */
  {
   i_puts("@\n");
   l=0;
   break;
  }
  if (l==254)
  {
   i_putch(7);
   continue;
  }
  i_putch(c);
  cmd[l++]=c;
 }
 goto top;
}
#endif

uint8_t thischr (void)
{
 if (*myptr==':') return 0;
 return *myptr;
}

uint8_t nextchr (void)
{
 myptr++;
 return thischr();
}

int b_end (void)
{
 curlin=0xFFFFFFFF;
 return 0;
}

int b_nop (void)
{
 return 0;
}

int b_clear (void)
{
 memset(&(RAM[prgtop]), 0, ramlen-prgtop);
 return 0;
}

int b_new (void)
{
 memset(RAM, 0, ramlen);
 return 0;
}

/* This needs work */
int b_cont (void)
{
 if (!brkptr) return BE_CN;
 myptr=brkptr;
 curlin=brklin;
 brklin=0;
 brkptr=NULL;
 return 0;
}

int b_stop (void)
{
 if (curlin==0xFFFFFFFF)
 {
  brklin=0;
  brkptr=NULL;
 }
 else
 {
  brklin=curlin;
  brkptr=myptr;
 }
 
 raise(BE_BK);
 curlin=0xFFFFFFFF;
 return BE_BK;
}

int b_run (void)
{
 char *p;
 uint32_t l;
 
 if (*myptr)
 {
  errno=0;
  l=strtoul(myptr, &p, 10);
  myptr=p;
  if (errno) return BE_SN;
 }
 else
 {
  l=0;
 }
 
 b_clear();
 return b_exec(l);
}

int b_goto (void)
{
 char *p;
 uint32_t l, ll;
 
 errno=0;
 l=strtoul(myptr, &p, 10);
 if (errno||((*p)&&(*p!=':'))) return BE_SN;
 ll=findptr(l);
 if (ll=0xFFFFFFFF) return BE_UL;
 curlin=l;
 lp=ll;
 myptr=&(RAM[ll])+8;
 return 0;
}

int b_list (void)
{
 uint32_t s, e;
 char *t;
 
 s=e=0;
 
 if (*myptr)
 {
  if ((!isdigit(*myptr))&&(*myptr!=',')) return BE_SN;
  if (*myptr!=',')
  {
   errno=0;
   s=strtoul(myptr, &t, 10);
   if (errno) return BE_SN;
   myptr=t;
   if (!*myptr)
   {
    list (s, s);
    return 0;
   }
   if (*myptr!=',') return BE_SN;
   myptr++;
   if (*myptr)
   {
    e=strtoul(myptr, &t, 10);
    if (errno) return BE_SN;
    myptr=t;
   }
   else
   {
    e=0;
   }
  }
 }
 list(s,e);
 return 0;
}

int b_tron (void)
{
 trace=1;
 return 0;
}

int b_troff (void)
{
 trace=0;
 return 0;
}

int b_system (void)
{
 die=1;
 return 0;
}

int maxtok;

enum bastok {
 TK_END = 0x81,
 TK_FOR,
 TK_NEXT,
 TK_DATA,
 TK_INPUT,
 TK_DIM,
 TK_READ,
 TK_LET,
 TK_GOTO,
 TK_RUN,
 TK_IF,
 TK_RESTORE,
 TK_GOSUB,
 TK_RETURN,
 TK_REM,
 TK_STOP,
 TK_ON,
 TK_LOAD,
 TK_SAVE,
 TK_DEF,
 TK_PRINT,
 TK_CONT,
 TK_LIST,
 TK_CLEAR,
 TK_NEW,
 TK_TRON,
 TK_TROFF,
 TK_SYSTEM,
 TK_LASTTOKEN
};

struct cmdtok cmdtok[]={
 {"END",     b_end},
 {"FOR",     b_nop},
 {"NEXT",    b_nop},
 {"DATA",    b_nop},
 {"INPUT",   b_nop},
 {"DIM",     b_nop},
 {"READ",    b_nop},
 {"LET",     b_nop},
 {"GOTO",    b_goto},
 {"RUN",     b_run},
 {"IF",      b_nop},
 {"RESTORE", b_nop},
 {"GOSUB",   b_nop},
 {"RETURN",  b_nop},
 {"REM",     b_nop},
 {"STOP",    b_stop},
 {"ON",      b_nop},
 {"LOAD",    b_nop},
 {"SAVE",    b_nop},
 {"DEF",     b_nop},
 {"PRINT",   b_nop},
 {"CONT",    b_cont},
 {"LIST",    b_list},
 {"CLEAR",   b_clear},
 {"NEW",     b_new},
 {"TRON",    b_tron},
 {"TROFF",   b_troff},
 {"SYSTEM",  b_system},
 {NULL,      NULL}
};

void raise (enum B_ERROR e)
{
 b_err=e;
 if (e>BE_UP)
  i_puts ("Unprintable error");
 else
  i_puts (ermsg[b_err]);
 if (curlin!=0xFFFFFFFF)
 {
  i_puts (" at line ");
  i_putu (curlin, 0);
 }
 i_putch('\n');
 curlin=0xFFFFFFFF;
}

int mvup (uint32_t from, uint32_t size)
{
 if (from>prgtop) return BE_SN;
 if ((prgtop+size)>=ramlen) return BE_OM;
 memmove(&(RAM[from+size]), &(RAM[from]), prgtop-from);
 memset(&(RAM[from]), 0, size);
 prgtop+=size;
 return 0;
}

int mvdn (uint32_t to, uint32_t from)
{
 uint32_t diff, size;
 
 if (to>from) return BE_SN;
 if (to>prgtop) return BE_SN;
 diff=from-to;
 size=prgtop-from;
 memmove(&(RAM[to]), &(RAM[from]), size);
 memset(&(RAM[prgtop-diff]), 0, diff);
 prgtop-=diff;
 return 0;
}

int match (char *main, char *sub)
{
 int x;
 
 if (strlen(main)<strlen(sub)) return 0;
 for (x=0; x<strlen(sub); x++)
  if (toupper(main[x])!=toupper(sub[x])) return 0;
 return 1;
}

/*
 * If the line becomes too long after parsing (may happen if there are a lot
 * of extended characters; limit is presently 254), "string too long" will be
 * raised.  Some dialects silently ignore lines considered too long.
 */
int crunch (char *p)
{
 int l;
 char *q;
 uint8_t *r;
 int mode;

 mode=0;
 q=p;
 
 memset(cmdlin, 0, 256);
 r=cmdlin;
 
 while (*q)
 {
  int n, t;
  
  t=l=0;
  
  if (mode==1)
  {
   if (*q=='"') mode=0;
  }
  else if (mode==2)
  {
   if (*q=='"') mode=4;
   if (*q==':') mode=0;
  }
  else if (mode==3)
  {
   if (*q=='"') mode=2;
  }
  else if (!mode)
  {
   if (isspace(*q))
   {
    q++;
    continue;
   }
   if (*q=='?')
   {
    for (n=0; strcmp(cmdtok[n].content, "PRINT"); n++) ;
    q++;
    *(r++)=0x81+n;
    continue;
   }
   if (*q=='"') mode=1;
   for (n=0; cmdtok[n].content; n++)
   {
    if (match(q, cmdtok[n].content))
    {
     l++;
     if (l==255) return BE_LS;
     t=1;
     if (!strcmp(cmdtok[n].content, "REM")) mode=3;
     if (!strcmp(cmdtok[n].content, "DATA")) mode=2;
     q+=strlen(cmdtok[n].content);
     *(r++)=0x81+n;
     break;
    }
   }
   if (t) continue;
  }
  
  t=*q;
  if ((t>=0x80)&&(t<(maxtok+0x81)))
  {
   if ((l++)==255) return BE_LS;
   *(r++)=0x80;
  }
  if ((l++)==255) return BE_LS;
  if (!mode) *q=toupper(*q);
  *(r++)=*(q++);
 }
 
 return 0;
}

/* Detokenize a string.  Used by LIST. */
void uncrunch (unsigned char *p)
{
 unsigned char *q;
 
 q=p;
 
 while (*q)
 {
  if (*q==0x80)
  {
   q++;
   i_putch (*(q++));
  }
  else if (*q>0x80)
  {
   int t;
   
   t=*(q++);
   if ((t-0x81)>=maxtok)
    i_putch (t);
   else
   {
    i_putch(' ');
    i_puts(cmdtok[t-0x81].content);
    i_putch(' ');
   }
  }
  else
   i_putch(*(q++));
 }
 i_putch('\n');
}

/*
 * Pack and unpack 32-bit DWORDs in an implementation-independent manner.
 * In this case, the manner is 8086/low endian format.
 */

void dwpak (uint8_t *y, uint32_t x)
{
 y[0]=x&0x000000FF;
 y[1]=(x&0x0000FF00)>>8;
 y[2]=(x&0x00FF0000)>>16;
 y[3]=(x&0xFF000000)>>24;
}

uint32_t dwunpak (uint8_t *p)
{
 uint32_t n;
 
 n=p[3];
 n<<=8;
 n|=p[2];
 n<<=8;
 n|=p[1];
 n<<=8;
 n|=p[0];
 return n;
}

void list (uint32_t start, uint32_t end)
{
 uint32_t p;
 
 if (end==0) end=0xFFFFFFFF;
 
 p=0;
 while (p<ramlen)
 {
  uint32_t l, n;
  
  /* next pointer=0 = EOF */
  n=dwunpak(&(RAM[p]));
  if (!n) return;
  
  l=dwunpak(&(RAM[p+4]));
  if ((l>=start)&&(l<=end))
  {
   i_putch(' ');
   i_putu(l, 1);
   i_putch(' ');
   uncrunch(&(RAM[p+8]));
  }
  p=n;
 }
}

/*
 * As a line is entered into the program, the pointers will be invalidated.
 * Iterate through program space, making each line point to the next, and fix
 * the end-of-program pointer.
 */
void fixlinks (void)
{
 uint32_t p, n;
 
 p=0;
 
 while (p<ramlen)
 {
  n=dwunpak(&(RAM[p]));
  if (!n)
  {
   prgtop=p+9;
   return;
  }
  
  n=p+8;
  while (RAM[n++]) ;
  dwpak(&(RAM[p]), n);
  p=n;
 }
}

/* Return offset to requested line or 0xFFFFFFFF if not found. */
uint32_t findptr (uint32_t line)
{
 uint32_t p, n;
 
 p=0;
 
 while (p<ramlen)
 {
  n=dwunpak(&(RAM[p]));
  if (!n) return 0xFFFFFFFF;
  if (dwunpak(&(RAM[p+4]))==line) return p;
  p=n;
 }
 
 return 0xFFFFFFFF;
}

/* Return best location for requested line or top of program space. */
uint32_t findptr2 (uint32_t line)
{
 uint32_t p, n;
 
 p=0;
 
 while (p<ramlen)
 {
  n=dwunpak(&(RAM[p]));
  if (!n) return p;
  if (dwunpak(&(RAM[p+4]))>line) return p;
  p=n;
 }
 
 return 0xFFFFFFFF;
}

int dellin (uint32_t line)
{
 uint32_t p, n;
 
 p=findptr(line);
 if (p==0xFFFFFFFF) return BE_UL;
 
 n=p+8;
 while (RAM[n++]);
 mvdn(p, n);
 fixlinks();
 return 0;
}

int addlin (uint32_t line, char *content)
{
 int e;
 uint32_t p;
 
 e=dellin(line);
 if (!*content) return e;
 p=findptr2(line);
 if (p==0xFFFFFFFF) return BE_OM;
 mvup(p, strlen(content)+9);
 memset(&(RAM[p]), 0xFF, 4);
 dwpak(&(RAM[p+4]), line);
 strcpy(&(RAM[p+8]), content); /* we checked for overrun already. */
 fixlinks();
 return 0;
}

/* Allocate BASIC program space.  Return 0 upon failure. */
uint32_t mkram (void)
{
 ramlen=MAXRAM;
 
 RAM=0;
 while (1)
 {
  RAM=malloc(ramlen);
  if (RAM)
  {
   memset(RAM, 0, ramlen);
   return ramlen;
  }
  ramlen-=INCRAM;
  if (ramlen<MINRAM) return 0;
 }
}

/*
 * If the command is only a line number:
 *   If the line exists, delete it; otherwise raise UL.
 * If there is content after the line number:
 *   Add or replace the line in the program.
 */
int mklin (char *cmd)
{
 char *p;
 int e;
 uint32_t l;
 
 errno=0;
 l=strtoul(cmd, &p, 10);
 if (errno||(!l)||(l==0xFFFFFFFF))
  return BE_SN;
 
 e=crunch(p);
 if (e) return e;
 
 e=addlin(l, cmdlin);
 
 curlin=0xFFFFFFFF; /* Return to immediate mode */
 return e;
}

/*
 * Command execution loop:
 *   Start at the beginning of the given line, and run commands separated by a
 *   colon (:).
 */
int b_do (char *ptr)
{
 int e;
 char f;
 
 myptr=ptr;
 while (*myptr)
 {
  if (trace&&(curlin!=0xFFFFFFFF))
  {
   i_putch('[');
   i_putu(curlin, 0);
   i_putch(']');
  }
  if ((*myptr>=0x81)&&(*myptr<(0x81+maxtok)))
  {
   f=(*myptr)-0x81;
   myptr++;
   e=(cmdtok[f].function)();
   if (e) return e;
   if (!(*myptr)) return 0;
   if (*myptr!=':') return BE_SN;
   continue;
  }
  return BE_SN;
 }
 return 0;
}

/*
 * Program execution loop:
 *   Start at the beginning of the specified line, or by default the beginning
 *   of the program; then point the command execution loop at each line.
 */
int b_exec (uint32_t line)
{
 int e;
 
 if (!line)
 {
  lp=0;
 }
 else
 {
  lp=findptr(line);
  if (lp=0xFFFFFFFF) return BE_UL;
 }
 
 while (0!=(np=dwunpak(&(RAM[lp]))))
 {
  curlin=dwunpak(&(RAM[lp+4]));
  e=b_do(&(RAM[lp+8]));
  if (e) return e;
  lp=np;
 }

 curlin=0xFFFFFFFF;
 return 0;
}

void basver (void)
{
 i_puts ("This version of NBASIC built " __DATE__ " " __TIME__ "\n");
}

/*
 * Load and save programs.
 *   ALERT: Programs are stored in TOKENIZED FORMAT.
 *          Changing the tokens will break compatibility with earlier versions
 *          (this is acceptable for now, we haven't set the token table in
 *           stone yet; but by version 1.0 we need to do that).
 */

int i_load (char *filename)
{
 FILE *file;
 uint32_t e, l;

 /*
  * Open the file.
  * If this returns "No such file or directory", raise NE (file not found).
  * Otherwise raise AD (access denied).
  */ 
 file=fopen(filename, "rb");
 if (!file)
 {
  switch (errno)
  {
   case EIO:
    return BE_IO;
   case ENODEV:
    return BE_ND;
   case ENOENT:
    return BE_NE;
   default:
    return BE_AD;
  }
 }
 
 /* Check whether there is enough room for the program. */
 fseek(file, 0, SEEK_END);
 l=ftell(file);
 fseek(file, 0, SEEK_SET);
 if (l>=ramlen)
 {
  fclose(file);
  return BE_OM;
 }
 
 /* Clear the memory space, then load the program into it. */
 b_new();
 e=fread(RAM, 1, l, file);
 fclose(file);
 if (e<l) return BE_IO; /* Raise I/O error for short read */
  
 /* Correct line pointers. */
 fixlinks();
 return 0;
}

int i_save (char *filename)
{
 FILE *file;
 uint32_t e;
 
 /*
  * Open the file;
  * if it failed, try to figure out why and raise a relevant error code.
  */
 file=fopen(filename, "wb");
 if (!file)
 {
  switch (errno)
  {
   case EIO:
    return BE_IO;
   case ENODEV:
    return BE_ND;
   case ENOENT:
    return BE_NE;
   case EROFS:
    return BE_WP;
   default:
    return BE_AD;
  }
 }
 
 /*
  * Write the file.
  * If it worked, return success.
  * If not, see whether it was "disk full" and raise it.
  * If not "disk full", raise "I/O error".
  */
 e=fwrite(RAM, 1, prgtop, file);
 fclose(file);
 if (e<(prgtop))
 {
  if (errno==ENOSPC) return BE_DF;
  return BE_IO;
 }
 return 0;
}

/*
 * Metacommands are commands which affect the internal operation of the
 * interpreter, but cannot be executed in a program.  They must be entered at
 * the command line.  These commands begin with a star (*) and may take
 * parameters (which must be literals).
 * 
 * The following metacommands are currently implemented:
 *   *BYE - Hard exit, optionally leaving a return code.  (SYSTEM cannot
 *          currently leave return codes.)
 *   *FREE - Display memory usage statistics.  (Some of this will eventually
 *           make its way into the FRE() function.)
 *   *LOAD - Load a program file into memory (see i_load() above).
 *   *SAVE - Save a program file to disk (see i_save() above).
 *   *VER - Display the version/build information for NBASIC (same as at
 *          initial signon).
 * 
 * Some functions initially implemented through metacommands will eventually
 * be migrated to actual commands, but as they require the use of strings,
 * support for variables, and in particular garbage-collected Pascal strings,
 * will need to be implemented for them to work.
 * 
 * When invoked, args will point to a valid string; may be empty.  cmd will be
 * upcased and the star will be removed.  metacmd should return 0 for success,
 * and a failure will raise the code supplied (usually this should be BE_SN).
 */
int metacmd (char *cmd, char *args)
{
 if (!strcmp(cmd, "BYE")) /* NO RETURN */
 {
  int e;
  
  i_deinitty();
  
  if (*args)
   exit(atoi(args));
  else
   exit(0);
 }
 if (!strcmp(cmd, "FREE")) /* XXX: add variable space when implemented */
 {
  if (*args) return BE_SN;
  
  i_puts ("Memory used:  ");
  i_putu (prgtop, 1);
  i_puts ("bytes\nMemory free:  ");
  i_putu (ramlen-prgtop, 1);
  i_puts ("bytes\n");
  return 0;
 }
 if (!strcmp(cmd, "LOAD"))
 {
  if (!*args) return BE_SN;
  return i_load(args);
 }
 if (!strcmp(cmd, "SAVE"))
 {
  if (!*args) return BE_SN;
  return i_save(args);
 }
 if (!strcmp(cmd, "VER"))
 {
  if (*args) return BE_SN;
  basver();
  return 0;
 }
 return BE_SN;
}

int main (int argc, char **argv)
{
#ifndef CUSTOM_GETLINE
 char *cmd;
#endif
 
 if (i_initty())
 {
  fprintf (stderr, "Could not initialize tty\n");
  return -1;
 }
 
 basver();
 i_putch('\n');
 
 /* If we can't get enough memory, display an error and die. */
 if (!mkram())
 {
  fprintf (stderr, "%s\n", ermsg[BE_OM]);
  return -1;
 }
 
 i_putu(ramlen, 0);
 i_puts(" bytes free\n");
 
 prgtop=0;

 for (maxtok=0; cmdtok[maxtok].content; maxtok++);
 
#ifndef CUSTOM_GETLINE
 cmd=0;
#endif
 brkraised=die=trace=0;
 brkptr=NULL;
 brklin=0;
 while (!die)
 {
  int e;
  size_t l;
  
  curlin=0xFFFFFFFF;
  i_puts ("Ready\n");

  /* Don't show the "Ready" prompt if there's no input to begin with */
another:  
#ifdef CUSTOM_GETLINE
  e=i_getln(">");
#else
  i_putch('>');
  e=getline(&cmd, &l, stdin);
#endif
  if (e<0) break;
  if (cmd[strlen(cmd)-1]=='\n') cmd[strlen(cmd)-1]=0;
  if (*cmd=='*')
  {
   char *p;
   
   for (p=cmd+1; (*p)&&(*p!=' '); p++) *p=toupper(*p);
   p=strchr(cmd, ' ');
   if (p)
    *(p++)=0;
   else
    p=pointer_to_nothing;
   e=metacmd(cmd+1, p);
   if (e)
    raise(e);
   continue;
  }
  cmdptr=cmd;
  while (isspace(*cmdptr)) cmdptr++;
  if (!*cmdptr) goto another;
  
  /* Start with a number?  It's a program line; add it */
  if (isdigit(*cmdptr))
  {
   e=mklin(cmdptr);
   if (e)
   {
    raise(e);
    continue;
   }
   goto another;
  }

  e=crunch(cmdptr);
  if (e)
  {
   raise(e);
   continue;
  }
  cmdptr=cmdlin;
  e=b_do(cmdptr);
  if (e) raise(e);
 }
 
 i_deinitty();
 free(RAM);
 return 0;
}
