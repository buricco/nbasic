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
 * This code is not finished.
 * In particular, the TTY output code has not yet been written.
 */

#include <stdint.h>
#include <SDL.h>
#include "tty.h"

static unsigned char font[]={
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7E, 0x81, 0xA5, 0x81, 0x81, 0xBD,
 0x99, 0x81, 0x81, 0x7E, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7E, 0xFF, 0xDB, 0xFF, 0xFF, 0xC3,
 0xE7, 0xFF, 0xFF, 0x7E, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x6C, 0xFE, 0xFE, 0xFE,
 0xFE, 0x7C, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x7C, 0xFE,
 0x7C, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x18, 0x3C, 0x3C, 0xE7, 0xE7,
 0xE7, 0x99, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x18, 0x3C, 0x7E, 0xFF, 0xFF,
 0x7E, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x3C,
 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xE7, 0xC3,
 0xC3, 0xE7, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x3C, 0x66, 0x42,
 0x42, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC3, 0x99, 0xBD,
 0xBD, 0x99, 0xC3, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 0x00, 0x00, 0x1E, 0x0E, 0x1A, 0x32, 0x78, 0xCC,
 0xCC, 0xCC, 0xCC, 0x78, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C,
 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3F, 0x33, 0x3F, 0x30, 0x30, 0x30,
 0x30, 0x70, 0xF0, 0xE0, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7F, 0x63, 0x7F, 0x63, 0x63, 0x63,
 0x63, 0x67, 0xE7, 0xE6, 0xC0, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x18, 0x18, 0xDB, 0x3C, 0xE7,
 0x3C, 0xDB, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFE, 0xF8,
 0xF0, 0xE0, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x02, 0x06, 0x0E, 0x1E, 0x3E, 0xFE, 0x3E,
 0x1E, 0x0E, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18,
 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
 0x66, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7F, 0xDB, 0xDB, 0xDB, 0x7B, 0x1B,
 0x1B, 0x1B, 0x1B, 0x1B, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x7C, 0xC6, 0x60, 0x38, 0x6C, 0xC6, 0xC6,
 0x6C, 0x38, 0x0C, 0xC6, 0x7C, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0xFE, 0xFE, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18,
 0x18, 0x7E, 0x3C, 0x18, 0x7E, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18,
 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18,
 0x18, 0x7E, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x0C, 0xFE,
 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x60, 0xFE,
 0x60, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0, 0xC0, 0xC0,
 0xC0, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x28, 0x6C, 0xFE,
 0x6C, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x38, 0x7C,
 0x7C, 0xFE, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xFE, 0xFE, 0x7C, 0x7C,
 0x38, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x3C, 0x3C, 0x3C, 0x18, 0x18,
 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x66, 0x66, 0x66, 0x24, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x6C, 0x6C, 0xFE, 0x6C, 0x6C,
 0x6C, 0xFE, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00,
 0x18, 0x18, 0x7C, 0xC6, 0xC2, 0xC0, 0x7C, 0x06,
 0x86, 0xC6, 0x7C, 0x18, 0x18, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0xC2, 0xC6, 0x0C, 0x18,
 0x30, 0x60, 0xC6, 0x86, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x38, 0x6C, 0x6C, 0x38, 0x76, 0xDC,
 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x30, 0x30, 0x30, 0x60, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x18, 0x0C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x0C,
 0x0C, 0x0C, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x3C, 0xFF,
 0x3C, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x7E,
 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x18, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x02, 0x06, 0x0C, 0x18,
 0x30, 0x60, 0xC0, 0x80, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xD6, 0xD6,
 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x38, 0x78, 0x18, 0x18, 0x18,
 0x18, 0x18, 0x18, 0x7E, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0x06, 0x0C, 0x18, 0x30,
 0x60, 0xC0, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0x06, 0x06, 0x3C, 0x06,
 0x06, 0x06, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x0C, 0x1C, 0x3C, 0x6C, 0xCC, 0xFE,
 0x0C, 0x0C, 0x0C, 0x1E, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFE, 0xC0, 0xC0, 0xC0, 0xFC, 0x0E,
 0x06, 0x06, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x38, 0x60, 0xC0, 0xC0, 0xFC, 0xC6,
 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFE, 0xC6, 0x06, 0x06, 0x0C, 0x18,
 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7C, 0xC6,
 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0x7E, 0x06,
 0x06, 0x06, 0x0C, 0x78, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00,
 0x00, 0x18, 0x18, 0x30, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x06, 0x0C, 0x18, 0x30, 0x60,
 0x30, 0x18, 0x0C, 0x06, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0x00,
 0x00, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x60, 0x30, 0x18, 0x0C, 0x06,
 0x0C, 0x18, 0x30, 0x60, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0x0C, 0x18, 0x18,
 0x18, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xDE, 0xDE,
 0xDE, 0xDC, 0xC0, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6, 0xC6, 0xFE,
 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x66,
 0x66, 0x66, 0x66, 0xFC, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3C, 0x66, 0xC2, 0xC0, 0xC0, 0xC0,
 0xC0, 0xC2, 0x66, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xF8, 0x6C, 0x66, 0x66, 0x66, 0x66,
 0x66, 0x66, 0x6C, 0xF8, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68,
 0x60, 0x62, 0x66, 0xFE, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFE, 0x66, 0x62, 0x68, 0x78, 0x68,
 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3C, 0x66, 0xC2, 0xC0, 0xC0, 0xDE,
 0xC6, 0xC6, 0x66, 0x3A, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xFE, 0xC6,
 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x18,
 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
 0xCC, 0xCC, 0xCC, 0x78, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xE6, 0x66, 0x6C, 0x6C, 0x78, 0x78,
 0x6C, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xF0, 0x60, 0x60, 0x60, 0x60, 0x60,
 0x60, 0x62, 0x66, 0xFE, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6,
 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xE6, 0xF6, 0xFE, 0xDE, 0xCE,
 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0xC6,
 0xC6, 0xC6, 0x6C, 0x38, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x60,
 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6,
 0xC6, 0xD6, 0xDE, 0x7C, 0x0C, 0x0E, 0x00, 0x00,
 0x00, 0x00, 0xFC, 0x66, 0x66, 0x66, 0x7C, 0x6C,
 0x66, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7C, 0xC6, 0xC6, 0x60, 0x38, 0x0C,
 0x06, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x7E, 0x7E, 0x5A, 0x18, 0x18, 0x18,
 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6,
 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6,
 0xC6, 0x6C, 0x38, 0x10, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xD6,
 0xD6, 0xFE, 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xC6, 0xC6, 0x6C, 0x6C, 0x38, 0x38,
 0x6C, 0x6C, 0xC6, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18,
 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xFE, 0xC6, 0x86, 0x0C, 0x18, 0x30,
 0x60, 0xC2, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3C, 0x30, 0x30, 0x30, 0x30, 0x30,
 0x30, 0x30, 0x30, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0x70, 0x38,
 0x1C, 0x0E, 0x06, 0x02, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C,
 0x0C, 0x0C, 0x0C, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x00,
 0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x78, 0x0C, 0x7C,
 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0xE0, 0x60, 0x60, 0x78, 0x6C, 0x66,
 0x66, 0x66, 0x66, 0xDC, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC0,
 0xC0, 0xC0, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x1C, 0x0C, 0x0C, 0x3C, 0x6C, 0xCC,
 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xFE,
 0xC0, 0xC0, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x38, 0x6C, 0x64, 0x60, 0xF0, 0x60,
 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC,
 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xCC, 0x78, 0x00,
 0x00, 0x00, 0xE0, 0x60, 0x60, 0x6C, 0x76, 0x66,
 0x66, 0x66, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x18, 0x00, 0x38, 0x18, 0x18,
 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x06, 0x06, 0x00, 0x0E, 0x06, 0x06,
 0x06, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C, 0x00,
 0x00, 0x00, 0xE0, 0x60, 0x60, 0x66, 0x6C, 0x78,
 0x78, 0x6C, 0x66, 0xE6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x18,
 0x18, 0x18, 0x18, 0x3C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xEC, 0xFE, 0xD6,
 0xD6, 0xD6, 0xD6, 0xD6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66,
 0x66, 0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0xC6,
 0xC6, 0xC6, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x66, 0x66,
 0x66, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x76, 0xCC, 0xCC,
 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0x0C, 0x1E, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xDC, 0x76, 0x62,
 0x60, 0x60, 0x60, 0xF0, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0xC6, 0x60,
 0x38, 0x0C, 0xC6, 0x7C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x10, 0x30, 0x30, 0xFC, 0x30, 0x30,
 0x30, 0x30, 0x36, 0x1C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0xCC,
 0xCC, 0xCC, 0xCC, 0x76, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x66, 0x66, 0x66,
 0x66, 0x66, 0x3C, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xC6,
 0xD6, 0xD6, 0xFE, 0x6C, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0x6C, 0x38,
 0x38, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xC6, 0xC6, 0xC6,
 0xC6, 0xC6, 0xC6, 0x7E, 0x06, 0x0C, 0xF8, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0xFE, 0xCC, 0x18,
 0x30, 0x60, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x0E, 0x18, 0x18, 0x18, 0x70, 0x18,
 0x18, 0x18, 0x18, 0x0E, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x00, 0x18,
 0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x70, 0x18, 0x18, 0x18, 0x0E, 0x18,
 0x18, 0x18, 0x18, 0x70, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x76, 0xDC, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x10, 0x38, 0x6C, 0xC6,
 0xC6, 0xC6, 0xFE, 0x00, 0x00, 0x00, 0x00, 0x00
};

static SDL_Window *screen;
static SDL_Renderer *renderer;
static SDL_Texture *texture;
static SDL_TimerID tick;

static uint32_t *display;
static uint32_t bg, fg;
static int tty_alive=0;
static uint8_t csrx, csry;
static uint8_t blink=0;
static uint8_t keyring[256], keybufr=0, keybufw=0;

static uint8_t vram[2000];

/* Forward declarations */
static void tty_deinit (void);

static void pset (uint16_t x, uint16_t y, uint32_t c)
{
 c&=0x00FFFFFF;
 
 if (x>639) return;
 if (y>399) return;
 display[(y*640)+x]=c|0xFF000000;
}

static void paintchar (uint8_t x, uint8_t y, uint8_t c)
{
 uint16_t xx, yy, cc, t;
 
 if ((x>79)||(y>24)) return;
  
 xx=x; xx<<=3;
 yy=y; yy<<=4;
 cc=(c&0x7F)*16;
 
 for (t=0; t<16; t++)
 {
  uint8_t b;
  
  b=font[cc+t];
  if ((csrx==x)&&(csry==y)&&(blink<25)) b=255-b;
  if (c&0x80) b=255-b;
  pset (xx+0, yy+t, (b&0x80)?fg:bg);
  pset (xx+1, yy+t, (b&0x40)?fg:bg);
  pset (xx+2, yy+t, (b&0x20)?fg:bg);
  pset (xx+3, yy+t, (b&0x10)?fg:bg);
  pset (xx+4, yy+t, (b&0x08)?fg:bg);
  pset (xx+5, yy+t, (b&0x04)?fg:bg);
  pset (xx+6, yy+t, (b&0x02)?fg:bg);
  pset (xx+7, yy+t, (b&0x01)?fg:bg);
 }
}

static void refresh (void)
{
 uint8_t x, y;
 uint16_t yoff;
 
 for (y=0; y<24; y++)
 {
  yoff=y*80;
  for (x=0; x<80; x++)
  {
   paintchar (x, y, vram[yoff+x]);
  }
 }
 blink++;
 while (blink>=50) blink-=50;

 /* Update window */
 SDL_UpdateTexture(texture, 0, display, 640*sizeof(uint32_t));
 SDL_RenderClear(renderer);
 SDL_RenderCopy(renderer, texture, 0, 0);
 SDL_RenderPresent(renderer);
}

static void keypoll (void)
{
 SDL_Event event;
 
 if (!tty_alive) return;
 
 while (SDL_PollEvent(&event))
 {
  int k;
  
  k=-1;
  switch (event.type)
  {
   static char *shiftnums = ")!@#$%^&*(";
   SDL_Keymod m;
   
   case SDL_QUIT:
    tty_deinit();
   case SDL_KEYDOWN:
    k=event.key.keysym.sym;
    
    m=SDL_GetModState();
    if (m&KMOD_CTRL)
    {
     if (k=='[')  k=0x1B;
     if (k=='\\') k=0x1C;
     if (k==']')  k=0x1D;
     if (k=='-')  k=0x1F;
    }
    if (m&KMOD_SHIFT)
    {
     if (k=='`') k='~';
     if (k=='-') k='_';
     if (k=='=') k='+';
     if (k=='[') k='{';
     if (k==']') k='}';
     if (k=='\\') k='|';
     if (k==';') k=':';
     if (k=='\'') k='"';
     if (k==',') k='<';
     if (k=='.') k='>';
     if (k=='/') k='?';
    }
    if ((k>='a') && (k<='z'))
    {
     if (m&KMOD_CAPS) k^=32;
     if (m&KMOD_SHIFT) k^=32;
     if (m&KMOD_CTRL) k&=0x1F;
    }
    else if ((k>='0')&&(k<='9'))
    {
     if (m&KMOD_CTRL)
     {
      if (k=='2') k=0x00; else if (k=='6') k=0x1E;
     }
     else
     {
      if (m&KMOD_SHIFT) k=shiftnums[k&0x0F];
     }
    }
    
    /* Nope. */
    if (k>255) k=-1;
    break;
  }
  
  if (k>=0) keyring[keybufw++]=k;
 }
}

static uint8_t tty_getch (void)
{
 /* Timer interrupt will take care of this */
 while (keybufr==keybufw) keypoll();
 
 return keyring[keybufr++];
}

static int tty_keypoll (void)
{
 return (keybufr!=keybufw);
}

static unsigned tock (unsigned interval, void *timerid)
{
 /* In case we are called before initialization is complete */
 if (!tty_alive) return 20;

 keypoll(); 
 if (!tty_alive) return 0; /* We got the axe. */
 refresh();
 
 return 20;
}

static void tty_clr (void)
{
 csrx=csry=0;
 memset(vram, 32, 2000);
}

static void tty_deinit (void)
{
 if (!tty_alive) return;
 
 SDL_RemoveTimer(tick);
 SDL_DestroyWindow(screen);
 if (display) free(display);
 tty_alive=0;
}

static int tty_init (const char *title, uint32_t f, uint32_t b)
{
 /* Don't do it twice. */
 if (tty_alive) return 0;
 
 display=calloc(640*400, 4);
 
 tick=SDL_AddTimer(20, tock, "50 Hz tick");
 if (!tick) return -1;
 
 screen=SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
                         SDL_WINDOWPOS_UNDEFINED, 640, 350, 0);
 if (!screen) return -1;
 
 renderer=SDL_CreateRenderer(screen, -1, 0);
 if (!renderer) return -1;
 
 texture=SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                           SDL_TEXTUREACCESS_STREAMING, 640, 350);
 if (!texture) return -1;
 
 tty_alive=1;
 fg=f?f:0xCCCCCC;
 bg=b;
 tty_clr();
 refresh();
 return 0;
}

int i_getch (void)
{
 return tty_getch();
}

int i_putch (int c)
{
 
}

void i_deinitty (void)
{
 tty_deinit();
}

int i_initty (void)
{
 return tty_init("NBASIC",0,0);
}