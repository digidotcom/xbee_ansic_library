/*
 * Copyright (c) 2010-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */
/**
	@addtogroup hal_dos
	@{
	@file xbee_platform_dos.c
	Platform-specific functions for use by the XBee Driver on DOS target.
*/

#include <ctype.h>
#include <stdio.h>
#include <time.h>

#include <conio.h>

#include "xbee/platform.h"

uint32_t xbee_seconds_timer()
{
	return time(NULL);
}

uint32_t xbee_millisecond_timer()
{
	return clock() * (1000 / CLOCKS_PER_SEC);
}

#ifdef __WATCOMC__

#include <i86.h>

void gotoxy00_bios( int col, int row)
{
	// Move cursor to (col,row) - zero based coordinates.
    union REGS  regs;

    regs.h.dh = row; 		// DH=row
    regs.h.dl = col; 		// DL=col
    regs.h.bh = 0; 			// Page number
    regs.h.ah = 0x02; 		// 02 = move cursor

    int86( 0x10, &regs, &regs );
}

void getxy_bios( int * col, int * row)
{
	// Set parameters to current cursor col,row
    union REGS  regs;

    regs.h.bh = 0; 			// Page number
    regs.h.ah = 0x03; 		// 03 = get cursor

    int86( 0x10, &regs, &regs );

    *row = regs.h.dh; 		// DH=row
    *col = regs.h.dl; 		// DL=col
}

void clrscr_bios( void)
{
	// Clear screen and reset cursor position to (0,0)
    union REGS  regs;

    regs.w.cx = 0; 			// Upper left
#ifdef HANDHELD
    regs.w.dx = 0x1018; 	// Lower right (of 16x24)
#else
    regs.w.dx = 0x1850; 	// Lower right (of 24x80)
#endif
    regs.h.bh = 7; 			// Blank lines attribute (white text on black)
    regs.w.ax = 0x0600; 	// 06 = scroll up, AL=00 to clear

    int86( 0x10, &regs, &regs );

    gotoxy00(0, 0);
}

#endif

void gotoxy_ansi( int col, int row)
{
	printf( "\x1B[%d;%dH", row, col);
	fflush( stdout);
}

void clrscr_ansi( void)
{
	printf( "\x1B[2J");		// erase screen and move to home position
	fflush( stdout);
}


#define XBEE_READLINE_STATE_INIT				0
#define XBEE_READLINE_STATE_START_LINE		1
#define XBEE_READLINE_STATE_CONTINUE_LINE	2

// See xbee/platform.h for function documentation.
int xbee_readline( char *buffer, int length)
{
	static int state = XBEE_READLINE_STATE_INIT;
	int c;
	static char *cursor;

	if (buffer == NULL || length < 1)
	{
		return -EINVAL;
	}

	switch (state)
	{
		default:
		case XBEE_READLINE_STATE_INIT:
			// no initialization necessary, fall through to start of new line

		case XBEE_READLINE_STATE_START_LINE:			// start of new line
			cursor = buffer;
			*buffer = '\0';	// reset string
			state = XBEE_READLINE_STATE_CONTINUE_LINE;
			// fall through to continued input

		case XBEE_READLINE_STATE_CONTINUE_LINE:		// continued input
			if (! kbhit())
			{
				delay( 1);	// sleep to reduce CPU usage
				return -EAGAIN;
			}
			c = getch();
			switch (c)
			{
				case 0x7F:				// backspace (Win32)
				case '\b':				// supposedly backspace...
					if (buffer != cursor)
					{
						fputs("\b \b", stdout);		// back up, erase last character
						cursor--;
					}
					break;

				case '\n':
				case '\r':
					putchar('\n');
					state = XBEE_READLINE_STATE_START_LINE;
					return cursor - buffer;

				default:
					if (isprint( c) && (cursor - buffer < length - 1))
					{
						*cursor++= c;
						putchar(c);
					}
               else
               {
                  putchar( '\x07');     // error beep -- bad char
               }
					break;
			}
			*cursor = 0;		// keep string null-terminated
			fflush(stdout);
	}

	return -EAGAIN;
}


// include support for 64-bit integers
#include "../util/jslong.c"
