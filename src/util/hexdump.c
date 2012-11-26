/*
 * Copyright (c) 2009-2012 Digi International Inc.,
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
	@addtogroup util
	@{
	@file util/hexdump.c

	ANSI C hex_dump() implementation if not available natively on a given platform.
*/
/*** BeginHeader hex_dump */
/*** EndHeader */
#include "xbee/platform.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>

// See xbee/platform.h for function documentation.
void hex_dump( const void FAR *address, uint16_t length, uint16_t flags)
{
	char linebuf[80];
	char *p, *q, *hex, *chars;
   unsigned char ch;
   uint16_t i;
   const char FAR *data = address;

   hex = linebuf;
   if (flags & HEX_DUMP_FLAG_OFFSET)
   {
		hex += 6;			// 0000:<sp>
   }
   else if (flags & HEX_DUMP_FLAG_ADDRESS)
   {
		hex += 8;			// 000000:<sp>
   }
   else if (flags & HEX_DUMP_FLAG_TAB)
   {
		*hex++ = '\t';
   }
   // start printing ASCII characters at position <chars>
   chars = hex + (16 * 3 + 3);

   for(i = 0; i < length; )
   {
   	if (flags & HEX_DUMP_FLAG_OFFSET)
   	{
			sprintf( linebuf, "%04x: ", i);
   	}
		else if (flags & HEX_DUMP_FLAG_ADDRESS)
		{
			sprintf( linebuf, "%" PRIpFAR ": ", data);
		}
   	p = hex;
   	q = chars;
      do {
      	ch = *data++;
      	if ((i & 15) == 8)
      	{
      		// insert space between two sets of 8 bytes
				*p++ = ' ';
				*q++ = ' ';
      	}
      	p[0] = "0123456789abcdef"[ch >> 4];
      	p[1] = "0123456789abcdef"[ch & 0x0F];
      	p[2] = ' ';
      	p += 3;
         *q++ = isprint(ch) ? ch : '.';
      } while ((++i < length) && (i & 15));
		// add missing spaces between hex and printed chars
		memset( p, ' ', chars - p);
		#ifdef __DC__
			q[0] = '\n';
			q[1] = '\0';							// null terminate ASCII characters
			fputs( linebuf, stdout);
			// only necessary to flush stdout on Rabbit platform
			fflush( stdout);
		#else
			*q = '\0';								// null terminate ASCII characters
			puts( linebuf);
		#endif
   }
}
