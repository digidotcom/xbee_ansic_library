/*
 * Copyright (c) 2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */
/*
	Platform-specific console support functions for samples/common/_xbee_term.c.

	Functions documented in _xbee_term.h.
*/
#include "../common/_xbee_term.h"

#include <conio.h>			// kbhit()
#include <unistd.h>			// usleep()
#include <wincon.h>

static WORD wAttributes;

void xbee_term_console_restore( void)
{
	SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), wAttributes);
}

void xbee_term_console_init( void)
{
	CONSOLE_SCREEN_BUFFER_INFO		bufferInfo;

	GetConsoleScreenBufferInfo( GetStdHandle(STD_OUTPUT_HANDLE), &bufferInfo);
	wAttributes = bufferInfo.wAttributes;

	atexit( xbee_term_console_restore);
}

void xbee_term_set_color( enum char_source source)
{
	WORD color;

	// match X-CTU's terminal of blue for keyboard text and red for serial text
	// (actually, CYAN for keyboard since BLUE is too dark for black background)
	switch (source)
	{
		case SOURCE_KEYBOARD:
			color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case SOURCE_STATUS:
			color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case SOURCE_SERIAL:
			color = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		default:
			color = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
			break;
	}
	SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), color);
}

int xbee_term_getchar( void)
{
	if (kbhit())
	{
		return getch();
	}
	usleep( 500);
	return -EAGAIN;
}

