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
	@file xbee/platform_posix.h
	Header for POSIX platforms (e.g., Linux, BSD, Mac OS X).

	This file is automatically included by xbee/platform.h.

	@todo need a configure script to find location of endian.h?
	@todo figure out a way to handle XBEE_NATIVE_64BIT
	@todo better way to determine timer resolution?
*/

#ifndef __XBEE_PLATFORM_POSIX
#define __XBEE_PLATFORM_POSIX
	#include <strings.h>

	#define strcmpi		strcasecmp
	#define strncmpi		strncasecmp

	// Load platform's endian header to learn whether we're big or little.
	#include <sys/types.h>

	// macro used to declare a packed structure (no alignment of elements)
	#define PACKED_STRUCT		struct __attribute__ ((__packed__))

	#define _f_memcpy		memcpy
	#define _f_memset		memset

	// stdint.h for int8_t, uint8_t, int16_t, etc. types
	#include <stdint.h>

	typedef int bool_t;

	// inttypes.h for PRIx16, PRIx32, etc. macros
	#include <inttypes.h>

	// compiler natively supports 64-bit integers
	#define XBEE_NATIVE_64BIT

// Elements needed to keep track of serial port settings.  Must have a
// baudrate memember, other fields are platform-specific.
typedef struct xbee_serial_t {
	uint32_t		baudrate;
	int			fd;
	char			device[40];		// /dev/ttySxx
} xbee_serial_t;

// Unix epoch is 1/1/1970
#define ZCL_TIME_EPOCH_DELTA	ZCL_TIME_EPOCH_DELTA_1970

// Per POSIX standard, "The resolution of the system clock is unspecified."
// We assume the millisecond timer has at least a 10ms resolution.
#define XBEE_MS_TIMER_RESOLUTION 10

#endif		// __XBEE_PLATFORM_POSIX
