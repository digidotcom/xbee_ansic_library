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
	@addtogroup hal_posix
	@{
	@file posix/xbee_readline.c

	ANSI C xbee_readline() implementation that works for POSIX platforms
	(Win32, Linux, BSD, Mac OS X).

	Function documentation appears in xbee/platform.h.
*/

#include <stdio.h>
#include <ctype.h>

#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#define NB_ENABLE 0
#define NB_DISABLE 1

int kbhit()
{
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
    select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}

struct termios _ttystate_orig;
void _restore_tty( void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &_ttystate_orig);
}

void nonblock(int state)
{
    static int init = 1;
    struct termios ttystate;

	 if (init)
	 {
        init = 0;
		  tcgetattr( STDIN_FILENO, &_ttystate_orig);
		  atexit( _restore_tty);
	 }

    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);

    if (state==NB_ENABLE)
    {
        //turn off canonical mode AND ECHO
        ttystate.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
        //minimum of number input read.
        ttystate.c_cc[VMIN] = 1;
    }
    else if (state==NB_DISABLE)
    {
        //turn on canonical mode
        ttystate.c_lflag |= ICANON;
    }
    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);

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
		case XBEE_READLINE_STATE_INIT:		// first time through, init terminal
			nonblock(NB_ENABLE);
			state = XBEE_READLINE_STATE_START_LINE;
			// fall through to start of new line

		case XBEE_READLINE_STATE_START_LINE:			// start of new line
			cursor = buffer;
			*buffer = '\0';	// reset string
			state = XBEE_READLINE_STATE_CONTINUE_LINE;
			// fall through to start of new line

		case XBEE_READLINE_STATE_CONTINUE_LINE:		// continued input
			if (! kbhit())
			{
				usleep( 1000);	// sleep 1ms to reduce CPU usage
				return -EAGAIN;
			}
			c = getchar();
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


