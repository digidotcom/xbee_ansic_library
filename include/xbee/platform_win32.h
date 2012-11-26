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
	@file xbee/platform_win32.h
	Header for 32-bit Windows (Win32) platform (using cygwin and gcc).

	This file is automatically included by xbee/platform.h.
*/

#ifndef __XBEE_PLATFORM_WIN32
#define __XBEE_PLATFORM_WIN32

	// Load platform's endian header to learn whether we're big or little.
	#ifdef __MINGW32__
		#include <sys/param.h>
		// MinGW's length-limited case-insensitive string comparison has a
		// slightly different name.
		#define strncmpi strnicmp
	#else
		#include <endian.h>
	#endif

	// debugging defines
//	#define XBEE_SERIAL_VERBOSE
//	#define XBEE_DEVICE_VERBOSE
//	#define XBEE_ATCMD_VERBOSE
//	#define XBEE_SERIAL_DEBUG
//	#define XBEE_XMODEM_VERBOSE
//	#define XBEE_FIRMWARE_VERBOSE
//	#define XBEE_APS_VERBOSE
//	#define XBEE_ZDO_VERBOSE
//	#define XBEE_ZCL_VERBOSE
//	#define ZCL_TIME_VERBOSE

	// macro used to declare a packed structure (no alignment of elements)
	// "__gcc_struct__" added to fix problems with gcc 4.7.0 in MinGW/MSYS
	#define PACKED_STRUCT	struct __attribute__ ((__packed__, __gcc_struct__))

	#define _f_memcpy		memcpy
	#define _f_memset		memset

	// stdint.h for int8_t, uint8_t, int16_t, etc. types
	#include <stdint.h>

	typedef int bool_t;

	// inttypes.h for PRIx16, PRIx32, etc. macros
	#include <inttypes.h>

	// for HANDLE type used in xbee_serial_t
	#include <wtypes.h>

	// compiler natively supports 64-bit integers
	#define XBEE_NATIVE_64BIT

	// size of OS-level serial buffers
	#define XBEE_SER_TX_BUFSIZE	512
	#define XBEE_SER_RX_BUFSIZE	4096

// Elements needed to keep track of serial port settings.  Must have a
// baudrate memember, other fields are platform-specific.
typedef struct xbee_serial_t {
	uint32_t		baudrate;
	int			comport;		// COMx:
	HANDLE		hCom;
} xbee_serial_t;

// Win32 epoch is 1/1/1970
#define ZCL_TIME_EPOCH_DELTA	ZCL_TIME_EPOCH_DELTA_1970

// Per MSDN, "The resolution of the GetTickCount function is limited to the
// resolution of the system timer, which is typically in the range of 10
// milliseconds to 16 milliseconds."
#define XBEE_MS_TIMER_RESOLUTION 16

#endif		// __XBEE_PLATFORM_WIN32


