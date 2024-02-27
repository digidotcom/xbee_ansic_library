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

#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>

#include "../common/_xbee_term.h"

static struct termios _ttystate_orig;
void xbee_term_console_restore( void)
{
   xbee_term_set_color(SOURCE_UNKNOWN);
   tcsetattr(STDIN_FILENO, TCSANOW, &_ttystate_orig);
}

void xbee_term_console_init( void)
{
    static int init = 1;
    struct termios ttystate;

    if (init)
    {
        init = 0;
        tcgetattr( STDIN_FILENO, &_ttystate_orig);
        atexit( xbee_term_console_restore);
    }

    //get the terminal state
    tcgetattr(STDIN_FILENO, &ttystate);

    //turn off canonical mode AND ECHO
    ttystate.c_lflag &= ~(ECHO | ECHONL | ICANON | IEXTEN);
    //minimum of number input read.
    ttystate.c_cc[VMIN] = 0;

    //set the terminal attributes.
    tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
}

void xbee_term_set_color( enum char_source source)
{
   const char *color;
   
   // match X-CTU's terminal of blue for keyboard text and red for serial text
   // (actually, CYAN for keyboard since BLUE is too dark for black background)
   switch (source)
   {
      case SOURCE_KEYBOARD:
         color = "\x1B[36;1m";      // bright cyan
         break;
      case SOURCE_STATUS:
         color = "\x1B[32;1m";      // bright green
         break;
      case SOURCE_SERIAL:
         color = "\x1B[31;1m";      // bright red
         break;
      default:
         color = "\x1B[0m";         // All attributes off
         break;
   }
   
   fputs( color, stdout);
   fflush( stdout);
}

int xbee_term_getchar(void)
{
   int c = getc(stdin);

   if (c == EOF && feof(stdin)) {
      clearerr(stdin);
      usleep(500);
      return -EAGAIN;
   }

   if (c == EOF && ferror(stdin)) {
      return -EIO;
   }

   return c;
}
