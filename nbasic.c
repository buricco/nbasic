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
 BE_UP /* Last Error + 1 */
} b_err;

char *ermsg[]={
 "Break",
 "Syntax error",
 "Undefined line number",
 "Out of memory",
 "String too long",
 NULL
};

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

int b_run (void)
{
 /* Forward declaration */
 int b_exec (uint32_t line);
 
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

int b_list (void)
{
 /* Forward declaration */
 void list (uint32_t start, uint32_t end);
 
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

struct cmdtok cmdtok[]={
 {"END",     b_end},
 {"FOR",     b_nop},
 {"NEXT",    b_nop},
 {"DATA",    b_nop},
 {"INPUT",   b_nop},
 {"DIM",     b_nop},
 {"READ",    b_nop},
 {"LET",     b_nop},
 {"GOTO",    b_nop},
 {"RUN",     b_run},
 {"IF",      b_nop},
 {"RESTORE", b_nop},
 {"GOSUB",   b_nop},
 {"RETURN",  b_nop},
 {"REM",     b_nop},
 {"STOP",    b_nop},
 {"ON",      b_nop},
 {"LOAD",    b_nop},
 {"SAVE",    b_nop},
 {"DEF",     b_nop},
 {"PRINT",   b_nop},
 {"CONT",    b_nop},
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
  printf ("Unprintable error");
 else
  printf ("%s", ermsg[b_err]);
 if (curlin!=0xFFFFFFFF)
  printf (" at line %lu", curlin);
 printf ("\n");
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
   printf ("%c", *(q++));
  }
  else if (*q>0x80)
  {
   int t;
   
   t=*(q++);
   if ((t-0x81)>=maxtok)
    putchar (t);
   else
    printf (" %s ", cmdtok[t-0x81].content);
  }
  else
   putchar(*(q++));
 }
 putchar('\n');
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
   printf (" %10lu ", l);
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
  if (!n) return;
  
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
  if (trace)
  {
   if (curlin!=0xFFFFFFFF) printf ("[%lu]", curlin);
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
 uint32_t lp, np;
 
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
  e=b_do(&(RAM[8]));
  if (e) return e;
  lp=np;
 }

 curlin=0xFFFFFFFF;
 return 0;
}

int main (int argc, char **argv)
{
 char *cmd;
 
 printf ("This version of NBASIC built " __DATE__ " " __TIME__ "\n\n");
 
 /* If we can't get enough memory, display an error and die. */
 if (!mkram())
 {
  fprintf (stderr, "%s\n", ermsg[BE_OM]);
  return -1;
 }
 
 printf ("%lu bytes free\n", ramlen);
 
 prgtop=0;

 for (maxtok=0; cmdtok[maxtok].content; maxtok++);
 
 cmd=0;
 die=trace=0;
 while (!die)
 {
  int e;
  size_t l;
  
  curlin=0xFFFFFFFF;
  printf ("Ready\n");

  /* Don't show the "Ready" prompt if there's no input to begin with */
another:  
  putchar('>');
  e=getline(&cmd, &l, stdin);
  if (e<0) break;
  if (cmd[strlen(cmd)-1]=='\n') cmd[strlen(cmd)-1]=0;
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
 
 free(RAM);
 return 0;
}
