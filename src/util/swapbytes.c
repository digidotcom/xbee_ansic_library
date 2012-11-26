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
	@addtogroup util_byteorder
	@{
	@file swapbytes.c
	For platforms without swap32() and swap16() functions for swapping byte
	order for 32 and 16-bit values.

	@note Should be written in assembly for embedded targets.
*/

#include "xbee/platform.h"

#ifndef HAVE_SWAP_FUNCS

/**
	@brief Swap the byte order of a 32-bit value.

	@param[in]	value	value to swap
	@return		new 32-bit value with opposite endianness of \a value
*/
uint32_t swap32( uint32_t value)
{
	return  (value & 0x000000FF) << 24
			| (value & 0x0000FF00) << 8
			| (value & 0x00FF0000) >> 8
			| (value & 0xFF000000) >> 24;
}

/**
	@brief Swap the byte order of a 16-bit value.

	@param[in]	value	value to swap
	@return		new 16-bit value with opposite endianness of \a value
*/
uint16_t swap16( uint16_t value)
{
	return ((value & 0x00FF) << 8) | (value >> 8);
}

#endif /* HAVE_SWAP_FUNCS */

