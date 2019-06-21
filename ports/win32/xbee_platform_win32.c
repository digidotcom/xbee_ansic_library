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
	@addtogroup hal_win32
	@{
	@file xbee_platform_win32.c
	Platform-specific functions for use by the XBee Driver on Win32/gcc target.
*/

#include "xbee/platform.h"
#include <time.h>

uint32_t xbee_seconds_timer()
{
	return time(NULL);
}

uint32_t xbee_millisecond_timer()
{
	return GetTickCount();
}

#include <ctype.h>			// isprint()
#include <stdio.h>			// fputs(), fflush(), getchar(), putchar()
#include <conio.h>			// kbhit()
#include <unistd.h>			// usleep()

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
				usleep( 1000);	// sleep 1ms to reduce CPU usage
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
			break;
	}

	return -EAGAIN;
}
