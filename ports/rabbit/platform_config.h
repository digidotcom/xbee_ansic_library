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
	@addtogroup hal_rabbit
	@{
	@file xbee/platform_rabbit.h
	Header for Rabbit platform (using Dynamic C).

	This file is automatically included by xbee/platform.h.

*/
#ifndef __XBEE_PLATFORM_RABBIT
#define __XBEE_PLATFORM_RABBIT

	// Load platform's endian header to learn whether we're big or little.
	#include <endian.h>

// On the Rabbit platform, some pointers are "far".  Many platforms just have
// a single pointer type and will define FAR to nothing.

#if 1
	#define FAR				__far
	#define CAST_FAR_TO_NEAR(p)	((void *)(uint16_t)(uint32_t)(p))
// Following in the standard set by inttypes.h, use fprintf macro PRIsFAR for
// printing far strings (will be just "s" on most platforms).
	#define PRIsFAR			"Fs"
	#define PRIpFAR			"Fp"
#else
	#define PRIsFAR			"s"
	#define PRIpFAR			"p"
#endif

	#include <stdint.h>
	#include <inttypes.h>

	// This type isn't in stdint.h
	typedef int					bool_t;

// Elements needed to keep track of serial port settings.  Must have a
// baudrate member, other fields are platform-specific.
typedef struct xbee_serial_t {
	uint32_t		baudrate;
	int			port;
} xbee_serial_t;

// Rabbit doesn't have separate functions for opening a serial port and setting
// its baudrate, so just map xbee_ser_baudrate() to xbee_ser_open().
#define xbee_ser_baudrate( serial, baudrate) xbee_ser_open( serial, baudrate)

// Rabbit epoch is 1/1/1980.
#define ZCL_TIME_EPOCH_DELTA	ZCL_TIME_EPOCH_DELTA_1980

// our millisecond timer has a 1ms resolution
#define XBEE_MS_TIMER_RESOLUTION 1

// use an inline-assembly version of _xbee_checksum()
#define _xbee_checksum( bytes, length, initial)	\
	_xbee_checksum_inline( bytes, length, initial)

#use "xbee_platform_rabbit.c"
#use "hexdump.c"

#endif		// __XBEE_PLATFORM_RABBIT

//@}
