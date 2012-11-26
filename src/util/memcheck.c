/*
 * Copyright (c) 2008-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/*** BeginHeader */
#include <stddef.h>
#include "xbee/platform.h"
/*** EndHeader */

/*** BeginHeader memcheck */
/*** EndHeader */
// documented in xbee/platform.h
int memcheck( const void FAR *src, int ch, size_t length)
{
	const uint8_t FAR *s;
	uint8_t c = (ch & 0xFF);

	for (s = src; length--; ++s)
	{
		if (*s != c)
		{
			return (*s > c) ? 1 : -1;
		}
	}

	return 0;
}

