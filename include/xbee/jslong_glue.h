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

// glue for jslong.h/jslong.c
// automatically included at the end of xbee/platform.h
// *** Do not #include this file directly. **

#ifndef XBEE_JSLONG_GLUE_H
#define XBEE_JSLONG_GLUE_H

#ifdef XBEE_NATIVE_64BIT
	#define JS_HAVE_LONG_LONG
	// NOTE: Your code should never access these types as integral types
	// (i.e., don't use unary or binary operators on them) since some
	// platforms implement them as structures.
	typedef int64_t	JSInt64;
	typedef uint64_t	JSUint64;
#else
	// NOTE: Your code should never access .lo and .hi directly, since some
	// platforms will be using an actual 64-bit integer.
	typedef struct {
		#if BYTE_ORDER == LITTLE_ENDIAN
			uint32_t lo, hi;
		#else
			uint32_t hi, lo;
		#endif
	} JSInt64;
	typedef JSInt64 JSUint64;
#endif

typedef int32_t	JSInt32;
typedef uint32_t	JSUint32;

#if (BYTE_ORDER == LITTLE_ENDIAN)
	#define IS_LITTLE_ENDIAN
#endif

#include "xbee/jslong.h"

// xbee_platform_rabbit.c includes the #use for jslong.c, no need for it here

#endif		// ! defined XBEE_JSLONG_GLUE_H
