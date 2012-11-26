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
	@addtogroup hal
	@{
	@file xbee/platform.h
	Common header for Hardware Abstraction Layer

	This file should be included by all other files in the library.

	@todo find pre-processor macro to identify POSIX platform
*/

#ifndef __XBEE_PLATFORM
#define __XBEE_PLATFORM

/**
	@def XBEE_BEGIN_DECLS
		Macro defined to @code extern "C" { @endcode when used by a C++ compiler.
		Used (along with XBEE_END_DECLS) for header file declarations to allow
		linking C-compiled code into C++ projects.
	@def XBEE_END_DECLS
		Macro defined to @code } @endcode when used by a C++ compiler.
		Used (along with XBEE_BEGIN_DECLS) for header file declarations to allow
		linking C-compiled code into C++ projects.
*/
#if defined(__cplusplus)
	#define XBEE_BEGIN_DECLS	extern "C" {
	#define XBEE_END_DECLS		}
#else
	#define XBEE_BEGIN_DECLS
	#define XBEE_END_DECLS
#endif

#include <errno.h>
#include <stddef.h>

/**
	@name
	These error names are used throughout the library.  Some platforms don't
	define them in errno.h, so we define them here using arbitrary values.
	@{

	@def E2BIG
		argument list too long (POSIX.1)
	@def EACCES
		permission denied (POSIX.1)
	@def EAGAIN
		resource temporarily unavailable (POSIX.1)
	@def EBADMSG
		bad message (POSIX.1)
	@def EBUSY
		device or resource busy (POSIX.1)
	@def ECANCELED
		operation canceled (POSIX.1)
	@def EEXIST
		file exists (POSIX.1)
	@def EILSEQ
		illegal byte sequence (POSIX.1, C99)
	@def EINVAL
		invalid argument (POSIX.1)
	@def EIO
		input/output error (POSIX.1)
	@def EMSGSIZE
		message too long (POSIX.1)
	@def ENODATA
		no message is available on the STREAM head read queue (POSIX.1)
	@def ENOENT
		no such file or directory (POSIX.1)
	@def ENOSPC
		no space left on device (POSIX.1)
	@def ENOSYS
		function not implemented (POSIX.1)
	@def ENOTSUP
		operation not supported (POSIX.1)
	@def EPERM
		operation not permitted (POSIX.1)
	@def ETIMEDOUT
		connection timed out (POSIX.1)
*/
#ifndef ENODATA
	#define ENODATA	20000
#endif
#ifndef EINVAL
	#define EINVAL		20001
#endif
#ifndef EIO
	#define EIO			20002
#endif
#ifndef EBUSY
	#define EBUSY		20003
#endif
#ifndef EEXIST
	#define EEXIST		20004
#endif
#ifndef ENOSPC
	#define ENOSPC		20005
#endif
#ifndef ENOENT
	#define ENOENT		20006
#endif
#ifndef E2BIG
	#define E2BIG		20007
#endif
#ifndef EBADMSG
	#define EBADMSG	20010
#endif
#ifndef ENOTSUP
	#define ENOTSUP	20011
#endif
#ifndef ETIMEDOUT
	#define ETIMEDOUT	20012
#endif
#ifndef EILSEQ
	#define EILSEQ		20013
#endif
#ifndef EAGAIN
	#define EAGAIN		20014
#endif
#ifndef ENOSYS
	#define ENOSYS		20015
#endif
#ifndef EACCES
	#define EACCES		20016
#endif
#ifndef ECANCELED
	#define ECANCELED	20017
#endif
#ifndef EMSGSIZE
	#define EMSGSIZE	20018
#endif
#ifndef EPERM
	#define EPERM		20019
#endif
// Note to developers: if possible, only add POSIX.1-2001 macros to this list.
//@}


/**
	@ingroup platform_specific
	Documentation for things that must be defined in the platform-specific
	header files.
@{

	@def XBEE_PLATFORM_HEADER
	Name of the platform-specific header file that xbee/platform.h should
	#include.  If not defined, this file will try to determine the platform
	and automatically include the necessary file.  This macro is useful for
	adding new platforms without having to modify platform.h.

	@typedef xbee_serial_t
	Must be a structure with uint32_t member \c baudrate and any additional
	members required by the functions in xbee/serial.h.

	@def XBEE_UNUSED_PARAMETER(p)
	Functions with unused parameters (common due to the use of function
	pointers) can use this macro to dismiss compiler warnings about unused
	parameters.  Defaults to [ (void) p ].

	@def ZCL_TIME_EPOCH_DELTA
	Number of seconds between 1/1/2000 and the target's epoch.  Used by the
	\ref zcl_time "ZCL Time Cluster" to convert from device's RTC to time
	base used by ZCL.

	@def __FUNCTION__
	Name of the current function.  Rabbit supports this macro directly; on
	Win32 it's defined to \c __func__.  On platforms with neither
	keyword available, define to \c "xbee".  Used to prefix debugging messages
	with the name of the current function.

	@def PACKED_STRUCT
	A replacement for [struct] when defining a packed structure where the
	elements are not aligned based on type.  The offsetof any given element
	will always be the offsetof plus sizeof the previous element.  On
	platforms with gcc, it's defined as [struct __attribute__ ((__packed__))].
	Defaults to just [struct]

	@def INTERRUPT_DISABLE
	Disable CPU interrupts (at the level of the serial port driver, at least).

	@def INTERRUPT_ENABLE
	Enable CPU interrupts (at the level of the serial port driver, at least).
	INTERRUPT_DISABLE/INTERRUPT_ENABLE are used without nesting in the library,
	so these can be low-level functions or in-line assembler.  Note that uses
	of these macros add a semicolon.

	@def XBEE_MS_TIMER_RESOLUTION
	The maximum number of milliseconds between consecutive calls to
	xbee_millisecond_timer().
@}

	@name platform_endian
	@ingroup platform_specific
	Macros typically defined in <endian.h>, define manually if endian.h is not
	available on this platform.
@{
	@def BIG_ENDIAN
	Typically defined as 1234.

	@def LITTLE_ENDIAN
	Typically defined as 4321.

	@def BYTE_ORDER
	Must be defined as \c LITTLE_ENDIAN or \c BIG_ENDIAN to match the target's
	byte order.
@}

	@name platform_far
	@ingroup platform_specific
	Macros related to supporting far (>16-bit) pointers.
@{
	@def FAR
	On platforms with "far" pointers, define to the proper keyword; empty
	definition if not required.

	@def CAST_FAR_TO_NEAR
	On platforms with "far" pointers, define to a series of explicit casts to
	convert a "far" pointer back to "near".  Only appropriate for pointers that
	are known to be near.

	@def PRIsFAR
	Literal string format specifier for printing a far string, typically \c "s".

	@def PRIpFAR
	Literal string format specifier for printing a far pointer, typically \c "p".

	@def _f_memcpy
	Version of memcpy() that accepts far pointers.

	@def _f_memset
	Version of memset() that accepts far pointers.
@}

	@name platform_stdint
	@ingroup platform_specific
	Types typically defined in <stdint.h>, define manually if stdint.h is not
	available on this platform.
@{
	@typedef bool_t
	Variable that can hold 0 or 1, may be an \c int for speed purporses or
	\c uint8_t for size optimization.

	@typedef int8_t
	8-bit signed integer

	@typedef uint8_t
	8-bit unsigned integer

	@typedef int16_t
	16-bit signed integer

	@typedef uint16_t
	16-bit unsigned integer

	@typedef int32_t
	32-bit signed integer

	@typedef uint32_t
	32-bit unsigned integer
@}

	@name platform_inttypes
	@ingroup platform_specific
	Macros typically defined in <inttypes.h>, define manually if inttypes.h is
	not available on this platform.
@{
	@def PRIu16
	Format specifier for 16-bit unsigned (usually \c "u" or \c "hu")

	@def PRIu32
	Format specifier for 32-bit unsigned (usually \c "lu" or \c "u")

	@def PRIx16
	Format specifier for 16-bit lowercase hex (usually \c "x" or \c "hx")

	@def PRIx32
	Format specifier for 32-bit lowercase hex (usually \c "lx" or \c "x")

	@def PRIX16
	Format specifier for 16-bit uppercase hex (usually \c "X" or \c "hX")

	@def PRIX32
	Format specifier for 32-bit uppercase hex (usually \c "lX" or \c "X")
@}
*/

/// For 1/1/1980 epoch (Rabbit), add 20 years, plus 5 leap days (1980,
/// 1984, 1988, 1992, 1996) to get to ZigBee epoch of 1/1/2000.
#define ZCL_TIME_EPOCH_DELTA_1980	((UINT32_C(20) * 365 + 5) * 24 * 60 * 60)

/// For 1/1/1970 epoch (Win32, Unix), add 30 years, plus 7 leap days (1972,
/// 1976, 1980, 1984, 1988, 1992, 1996) to get to ZigBee epoch of 1/1/2000.
#define ZCL_TIME_EPOCH_DELTA_1970	((UINT32_C(30) * 365 + 7) * 24 * 60 * 60)

#ifdef XBEE_PLATFORM_HEADER
	#include XBEE_PLATFORM_HEADER
#elif defined __DC__
	#include "xbee/platform_rabbit.h"
#elif defined POSIX
	#include "xbee/platform_posix.h"
#elif defined __DOS__
	// Note: at present, only Open Watcom compiler supported
	// (can test for __WATCOMC__ preprocessor symbol, or
	// __BORLANDC__ for Borland C++)
	#include "xbee/platform_dos.h"
#elif defined WIN32 || defined _WIN32 || defined _WIN32_ || defined __WIN32__ \
	|| defined __CYGWIN32__ || defined MINGW32
	#include "xbee/platform_win32.h"
#elif defined __MWERKS__ && defined __HC08__
	#include "xbee/platform_hcs08.h"
#else
	#error "Unknown target"
#endif

#ifndef TRUE
	#define TRUE 1
#endif
#ifndef FALSE
	#define FALSE 0
#endif

#ifndef PACKED_STRUCT
	#define PACKED_STRUCT struct
#endif

// default is for FAR to be ignored
#ifndef FAR
	#define FAR
#endif
// on platforms without the concept of "FAR", cast to near is unnecessary
#ifndef CAST_FAR_TO_NEAR
	#define CAST_FAR_TO_NEAR(p)	(p)
#endif
#ifndef INTERRUPT_ENABLE
	#define INTERRUPT_ENABLE
#endif

#ifndef INTERRUPT_DISABLE
	#define INTERRUPT_DISABLE
#endif

// legacy macro -- applications should remove conditional compilation
// tests that use this macro
#define XBEE_SERIAL_POLLED

// Default method for specifying an unused parameter (to avoid compiler
// warnings) is to cast it to void.  If this method doesn't work for a platform
// or actually generates code, that platform's header file should define it
// as nothing.
#ifndef XBEE_UNUSED_PARAMETER
	#define XBEE_UNUSED_PARAMETER(p)		(void) p
#endif

// Following in the standard set by inttypes.h, use fprintf macro PRIsFAR for
// printing far strings (will be just "s" on most platforms).
// default settings for various macros
#ifndef PRIsFAR
	#define PRIsFAR			"s"
#endif
#ifndef PRIpFAR
	#define PRIpFAR			"p"
#endif
#ifndef PRId16
	#define PRId16				"hd"
#endif
#ifndef PRId32
	#define PRId32				"ld"
#endif
#ifndef PRIu16
	#define PRIu16				"hu"
#endif
#ifndef PRIu32
	#define PRIu32				"lu"
#endif
#ifndef PRIx16
	#define PRIx16				"hx"
#endif
#ifndef PRIx32
	#define PRIx32				"lx"
#endif
#ifndef PRIX16
	#define PRIX16				"hX"
#endif
#ifndef PRIX32
	#define PRIX32				"lX"
#endif


/// Helper macro for calculating the number of entries in an array.
#define _TABLE_ENTRIES(array)		((sizeof (array)) / (sizeof (*array)))

XBEE_BEGIN_DECLS

/**
	@brief
	Platform-specific function to return the number of elapsed seconds

	On some platforms, this is the equivalent of an "uptime".

	On other platforms, it may reflect the value of the RTC, which might
	not be synchronized with a time server and might include leap seconds.

	Regardless, it should consistently report elapsed time and not jump on
	clock synchronization.

	In addition to determining timeouts, the \ref zcl_time "ZCL Time Cluster"
	makes use of it to report current time.

	(Function name wrapped in parentheses so platforms can use a macro function
	of the same name.)

	@return Number of elapsed seconds.
*/
uint32_t (xbee_seconds_timer)( void);

/**
	@brief
	Platform-specific function to return the number of elapsed milliseconds.

	OK for this counter to rollover.  Used for timing and should have a
	resolution of at least 60ms.

	- Rabbit has 1ms resolution.
	- HCS08 has 4ms resolution.
	- DOS has 18 ticks/second (55.55ms resolution)

	(Function name wrapped in parentheses so platforms can use a macro function
	of the same name.)

	@return Number of elapsed milliseconds.

	@sa XBEE_MS_TIMER_RESOLUTION
*/
uint32_t (xbee_millisecond_timer)( void);

/**
	@brief
	Converts two hex characters (0-9A-Fa-f) to a byte.

	@param[in] p	String of hex characters to convert.

	@retval	-1	Error (invalid character or string less than 2 bytes).
	@retval	0-255	The byte represented by the first two characters of \a p.

	\par Examples
	- hexstrtobyte("FF") returns 255
	- hexstrtobyte("0") returns -1 (error because < 2 characters)
	- hexstrtobyte("ABCDEF") returns 0xAB (ignores additional chars)

*/
int hexstrtobyte( const char FAR *p);


/**
	@brief
	This function is a non-blocking version of gets(), used to read a line of
	input from the user.

	It waits for a string from stdin terminated by a return.  It should be
	called repeatedly, until it returns a value other than -EAGAIN.  The input
	string, stored in \a buffer is null-terminated without the return.

	The user should make sure only one process calls this function at a time.

	@param[in,out]	buffer	buffer to store input from user
	@param[in]		length	size of \a buffer

	@retval	>=0		User ended the input with a newline, return value is
							number of bytes written.
	@retval	-EAGAIN	User has not completed a line.
	@retval	-EINVAL	NULL buffer or length is less than 1.
*/
int xbee_readline( char *buffer, int length);

/**
	Helper function for printing a hex dump of memory to stdout.  A reference
	implementation is provided in as util/hexdump.c.  Dumps data in
	hex/printable format, 16 bytes to a line, to stdout.

	@param[in]	address	Address of data to dump.

	@param[in]	length	Number of bytes to dump.

	@param[in]	flags		One of
		- #HEX_DUMP_FLAG_NONE
		- #HEX_DUMP_FLAG_OFFSET
		- #HEX_DUMP_FLAG_ADDRESS
		- #HEX_DUMP_FLAG_TAB
*/
void hex_dump( const void FAR *address, uint16_t length, uint16_t flags);
/**
	@name
	Flags to pass to hex_dump().
	@{
*/
/// Default settings (no prefix).
#define HEX_DUMP_FLAG_NONE			0x0000
/// Prefix each line with the memory offset (0000: xx xx xx).
#define HEX_DUMP_FLAG_OFFSET		0x0001
/// Prefix each line with the address (uses %p specifier) (000000: xx xx xx).
#define HEX_DUMP_FLAG_ADDRESS		0x0002
/// Prefix each line with a tab character.
#define HEX_DUMP_FLAG_TAB			0x0004
//@}


/**
	@brief Test whether a block of memory is set to a single byte value.

	@param[in]	src		starting address
	@param[in]	c			value to compare each byte to
	@param[in]	length	number of bytes to compare

	@retval	0	\a length bytes from \a src are set to \a c
	@retval	<0	\a length bytes from \a src would sort before \a length
					bytes of \a c
	@retval	>0	\a length bytes from \a src would sort before \a length
					bytes of \a c

	@sa memcmp, memset
*/
int memcheck( const void FAR *src, int c, size_t length);

/**
    Compare two uint16_t timer values, with proper handling of rollover
    (e.g., 0xFFFF < 0x0000).

    @param[in]	a	first timer value (cast to uint16_t)
    @param[in] op	operand (> or <) to use when comparing two timers
    @param[in] b	second timer value (cast to uint16_t)

    @return	The equivalent to "a op b".

    Note that when using the millisecond timer, you can only compare times
    within 32 seconds of each other.

    @code
    	 uint16_t t0;

    	 t0 = (uint16_t) xbee_millisecond_timer();
    	 do {
    	 	 some_stuff_here();
    	 	 delay = update_delay();
    	 } while (XBEE_TIMER_COMPARE( t0 + delay, >, xbee_millisecond_timer()));
    @endcode
*/
#define XBEE_TIMER_COMPARE( a, op, b) \
	( ((int16_t)((uint16_t)(a) - (uint16_t)(b))) op 0 )

/**
	Macro used to load a variable with an expiration time, later used with
	XBEE_CHECK_TIMEOUT_MS() macro.  For timeouts longer than 32767 milliseconds
	(with less accuracy), use XBEE_SET_TIMEOUT_SEC()/XBEE_CHECK_TIMEOUT_SEC().

	@param[in]	delay		Timeout in milliseconds, from 1 to 32767.

	@return	Value to assign to an unsigned 16-bit variable (uint16_t)
				for use with CHECK_TIMEOUT_MS().

	@sa	XBEE_CHECK_TIMEOUT_MS(), XBEE_SET_TIMEOUT_SEC(),
			XBEE_CHECK_TIMEOUT_SEC()

	@code
		uint16_t t;

		t = XBEE_SET_TIMEOUT_MS(5000);		// 5 second timeout
		for (;;)
		{
			// do stuff or wait for something to happen

			if (XBEE_CHECK_TIMEOUT_MS(t))
			{
				// 5 seconds elapsed
				break;
			}
		}
	@endcode
*/
#define XBEE_SET_TIMEOUT_MS(delay)	\
	((uint16_t)xbee_millisecond_timer() + (delay))

/**
	Macro used to check a timer set by XBEE_SET_TIMEOUT_MS().  See the
	documentation for XBEE_SET_TIMEOUT_MS() for a code example on proper
	use of this macro.

	@param[in]	timer		16-bit variable set by XBEE_SET_TIMEOUT_MS().

	@retval	TRUE		Time specified in call to XBEE_SET_TIMEOUT_MS() has
							elapsed.
	@retval	FALSE		Time specified has not yet elapsed.

	@sa XBEE_SET_TIMEOUT_MS(), XBEE_SET_TIMEOUT_SEC(), XBEE_CHECK_TIMEOUT_SEC()
*/
#define XBEE_CHECK_TIMEOUT_MS(timer) \
	((int16_t)((uint16_t)xbee_millisecond_timer() - (timer)) >= 0)


/**
	Macro used to load a variable with an expiration time, later used with
	XBEE_CHECK_TIMEOUT_SEC() macro.  For a more precise timeout, for values of
	30 seconds or less, use XBEE_SET_TIMEOUT_MS()/XBEE_CHECK_TIMEOUT_MS()
	instead.

	@param[in]	delay		Timeout in seconds, from 1 to 32767.

	@return	Value to assign to an unsigned 16-bit variable (uint16_t) for use
				with XBEE_CHECK_TIMEOUT_SEC().

	@sa XBEE_CHECK_TIMEOUT_SEC(), XBEE_SET_TIMEOUT_MS(), XBEE_CHECK_TIMEOUT_MS()

	@code
		uint16_t t;

		t = XBEE_SET_TIMEOUT_SEC(600);		// 10 minute timeout
		for (;;)
		{
			// do stuff or wait for something to happen

			if (XBEE_CHECK_TIMEOUT_SEC(t))
			{
				// 10 minutes elapsed
				break;
			}
		}
	@endcode
*/
#define XBEE_SET_TIMEOUT_SEC(delay) ((uint16_t)xbee_seconds_timer() + (delay))

/**
	Macro used to check a timer set by XBEE_SET_TIMEOUT_SEC().  See the
	documentation for XBEE_SET_TIMEOUT_SEC() for a code example on proper use
	of this macro.

	@param[in]	timer		16-bit variable set by XBEE_SET_TIMEOUT_SEC().

	@retval	TRUE		Time specified in call to XBEE_SET_TIMEOUT_SEC() has
							elapsed.
	@retval	FALSE		Time specified has not yet elapsed.

	@sa XBEE_SET_TIMEOUT_SEC(), XBEE_SET_TIMEOUT_MS(), XBEE_CHECK_TIMEOUT_MS()
*/
#define XBEE_CHECK_TIMEOUT_SEC(timer) \
	((int16_t)((uint16_t)xbee_seconds_timer() - (timer)) >= 0)

// include support for 64-bit integers
#include "xbee/jslong_glue.h"

XBEE_END_DECLS

#endif


