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
	@addtogroup wpan_types
	@{
	@file wpan_types.c
	Data types and macros used by all WPAN (802.15.4) devices.

*/

/*** BeginHeader */
#include <ctype.h>
#include <string.h>
#include "wpan/types.h"
/*** EndHeader */

/*** BeginHeader _WPAN_IEEE_ADDR_UNDEFINED */
/*** EndHeader */
/// @internal address pointed to by macro #WPAN_IEEE_ADDR_UNDEFINED
const addr64 _WPAN_IEEE_ADDR_UNDEFINED =
								{ { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF } };

/*** BeginHeader _WPAN_IEEE_ADDR_BROADCAST */
/*** EndHeader */
/// @internal address pointed to by macro #WPAN_IEEE_ADDR_BROADCAST
const addr64 _WPAN_IEEE_ADDR_BROADCAST =
								{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF } };

/*** BeginHeader _WPAN_IEEE_ADDR_COORDINATOR */
/*** EndHeader */
/// @internal address pointed to by macro #WPAN_IEEE_ADDR_COORDINATOR
const addr64 _WPAN_IEEE_ADDR_COORDINATOR =
								{ { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 } };


/*** BeginHeader addr64_format */
/*** EndHeader */
/** @brief
		Format a 64-bit address as a null-terminated, printable string
		(e.g., "00-13-A2-01-23-45-67").

		To change the default separator ('-'), define
		ADDR64_FORMAT_SEPARATOR to any character.  For example:

			#define ADDR64_FORMAT_SEPARATOR ':'

@param[out]	buffer	Pointer to a buffer of at least #ADDR64_STRING_LENGTH
					(8 2-character bytes + 7 separators + 1 null = 24) bytes.

@param[in]	address	64-bit address to format.

@return	\a address as a printable string (stored in \a buffer).

	@todo Add a parameter to indicate big or little endian and update code
			to work with either.  (for little-endian, b starts at address->b + 8
			and is decremented)

			add a parameter for other formats/flags
			- uppercase vs. lowercase hex
			- compact format (0013a200-405e0ef0)
			- format used by the Python framework (with [!]?)

*/
char FAR *addr64_format( char FAR *buffer, const addr64 FAR *address)
{
	int i;
	const uint8_t FAR *b;
	char FAR *p;
	uint_fast8_t ch;

	// format address into buffer
	p = buffer;
	b = address->b;
	for (i = 8; ; )
	{
		ch = *b++;
		*p++ = "0123456789abcdef"[ch >> 4];
		*p++ = "0123456789abcdef"[ch & 0x0F];
		if (--i)
		{
			*p++ = ADDR64_FORMAT_SEPARATOR;
		}
		else
		{
			*p = '\0';
			break;
		}
	}

	// return start of buffer
	return buffer;
}

/*** BeginHeader addr64_equal */
/*** EndHeader */
/** @brief
	Compare two 64-bit addresses for equality.

	@param[in]	addr1		address to compare
	@param[in]	addr2		address to compare

	@retval	TRUE	\a addr1 and \a addr2 are not NULL and point to
						identical addresses
	@retval	FALSE	NULL parameter passed in, or addresses differ
*/
bool_t addr64_equal( const addr64 FAR *addr1, const addr64 FAR *addr2)
{
	// This is marginally faster than calling memcmp.  Make sure neither
	// parameter is NULL and then do two 4-byte compares.
	return (addr1 && addr2 &&
		addr1->l[0] == addr2->l[0] && addr1->l[1] == addr2->l[1]);
}


/*** BeginHeader addr64_is_zero */
/*** EndHeader */
/** @brief
	Test a 64-bit address for zero.

	@param[in]	addr	address to test

	@retval TRUE	\c addr is NULL or points to an all-zero address
	@retval FALSE	\c addr points to a non-zero address

	@see WPAN_IEEE_ADDR_ALL_ZEROS
*/
bool_t addr64_is_zero( const addr64 FAR *addr)
{
	return ! (addr && (addr->l[0] || addr->l[1]));
}

/*
	Do we need functions to return the high and low 32-bit halves of an addr64
	in host byte order?

	addr64_high32h and addr64_low32h?

	ntoh64 and hton64?

	I would prefer to ALWAYS store the bytes in an addr64 structure in network
	byte order to reduce the possibility of us passing a struct with the wrong
	byte order.  That doesn't mean we shouldn't provide helper functions to
	get or set the 32-bit halves in host byte order.

*/

/*** BeginHeader addr64_parse */
/*** EndHeader */
/**
	@brief	Parse a text string into a 64-bit IEEE address.

	Converts a text string with eight 2-character hex values, with an optional
	separator between any two values.  For example, the following formats are
	all valid:
		- 01-23-45-67-89-ab-cd-ef
		- 01234567-89ABCDEF
		- 01:23:45:67:89:aB:Cd:EF
		- 0123 4567 89AB cdef

	@param[out]	address	converted address (stored big-endian)
	@param[in]	str		string to convert, starting with first hex character

	@retval	-EINVAL	invalid parameters passed to function; if \a address is
							not NULL, it will be set to all zeros
	@retval	0			string converted
*/
int addr64_parse( addr64 *address_be, const char FAR *str)
{
	uint_fast8_t i;
	uint8_t *b;
	int ret;

	i = 8;			// bytes to convert
	if (str != NULL && address_be != NULL)
	{
		// skip over leading spaces
		while (isspace( (uint8_t) *str))
		{
			++str;
		}
		for (b = address_be->b; i; ++b, --i)
		{
			ret = hexstrtobyte( str);
			if (ret < 0)
			{
				break;
			}
			*b = (uint8_t)ret;
			str += 2;					// point past the encoded byte

			// skip over any separator, if present
			if (*str && ! isxdigit( (uint8_t) *str))
			{
				++str;
			}
		}
	}

	if (i == 0)			// successful conversion
	{
		return 0;
	}

	// conversion not complete
	if (address_be != NULL)
	{
		*address_be = *WPAN_IEEE_ADDR_ALL_ZEROS;	// zero out address on errors
	}
	return -EINVAL;
}




