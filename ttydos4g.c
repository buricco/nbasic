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

#include <stdint.h>
#include <string.h>

char *vram;

char *i_ttydrv="DOS/4GW";

uint8_t csrx, csry;

int i_ckbrk (void)
{
 uint8_t c;
 
 _asm {
  mov ah, 0x01;
  int 0x16;
  jz ckbrk_1;
  xor al, al;
ckbrk_1:
  mov c, al;
 };
 return (c==3);
}

int i_getch (void)
{
 uint8_t c, s;
 
 _asm {
  mov ah, 0x02;
  xor bh, bh;
  mov dh, csry;
  mov dl, csrx;
  int 0x10;
  mov ah, 0x01;
  mov cx, 0x000F;
  int 0x10;
  xor ah, ah;
  int 0x16;
  mov s, ah;
  mov c, al;
  mov ah, 0x01;
  mov ch, 0x3F;
  int 0x10;
 };
 return c?c:(-s);
}

int i_putch (int c)
{
 uint16_t t;
 
 if ((c&0x7F)<0x20)
 {
  switch (c&0x7F)
  {
   case 0x07:
    /* XXX: bell */
    return 7;
   case 0x08:
    if (csrx)
    {
     csrx--;
    }
    else if (csry)
    {
     csrx=80;
     csry--;
    }
    return 8;
   case 0x09:
    while (csrx&7) i_putch(0x20|(c&0x80));
    return 9;
   case 0x0A:
    csrx=0;
    if (csry<24)
    {
     csry++;
     return 10;
    }
    csry=24;
    memmove(vram, vram+160, 3840);
    for (t=3840; t<4000; t+=2)
    {
     vram[t]=' ';
     vram[t+1]=0x07;
    }
    return 10;
   case 0x0D:
    csrx=0;
    return 13;
  }
 }
 
 t=csrx+(csry*80);
 t<<=1;
 vram[t]=c;
 vram[t+1]=0x07;
 csrx++;
 if (csrx==80)
  i_putch(0x10);
 return c;
}

void i_deinitty (void)
{
 _asm {
  mov ax, 0x0003;
  int 0x10;
 };
}

int i_initty (void)
{
 vram=(void *)0x000B8000;
 _asm {
  mov ax, 0x0003;
  int 0x10;
  mov ah, 0x01;
  mov ch, 0x3F;
  int 0x10;
 };
 return 0;
}
