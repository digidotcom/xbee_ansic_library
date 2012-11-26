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
	@addtogroup hal_hcs08
	@{
	@file xbee_platform_hcs08.c
	Platform-specific functions for use by the XBee Driver on Freescale HCS08
	platform.

	Note that on the Programmable XBee, you need to call InitRTC() from main
	and assign vRTC to the interrupt in the project's .PRM file.
*/

#include "xbee/platform.h"
#include "rtc.h"

#ifdef XBEE_CW10
#define SEC_TIMER uptime
#define MS_TIMER ms_uptime
#endif

// These function declarations have extra parens around the function name
// because we define them as macros referencing the global directly.
uint32_t (xbee_seconds_timer)( void)
{
	return SEC_TIMER;
}

uint32_t (xbee_millisecond_timer)( void)
{
	return MS_TIMER;
}

#pragma MESSAGE DISABLE C1404		// Return expected

/**
	Swap the byte order of a 16-bit value.

	@param[in]	value	value to swap
	@return		new 16-bit value with opposite endianness of \a value
*/
#pragma NO_ENTRY
#pragma NO_EXIT
#pragma NO_FRAME
#pragma NO_RETURN
uint16_t swap16( uint16_t value)
{
	asm {
		PSHH			; push MSB
		PSHX			; push LSB
		PULH			; pop LSB into H
		PULX			; pop MSB into X
		RTS
	}
}

/**
	Swap the byte order of a 32-bit value.

	@param[in]	value	value to swap
	@return		new 32-bit value with opposite endianness of \a value
*/
#pragma NO_ENTRY
#pragma NO_EXIT
#pragma NO_FRAME
#pragma NO_RETURN
uint32_t swap32( uint32_t value)
{
	/* Stack offset:
		1: MSB of return address
		2: LSB of return address
		3: byte3 (MSB) param1
		4: byte2 of param1
		5: byte1 of param1
		6: byte0 (LSB) of param1
		7: MSB of address of return value
		8: LSB of address of return value
	*/
	asm {
		LDHX	7, SP			; H:X is index to return value
		LDA	3, SP			; load byte 3 of parameter
		STA	3, X			; save as byte 0 of return
		LDA	4, SP			; load byte 2 of parameter
		STA	2, X			; save as byte 1 of return
		LDA	5, SP			; load byte 1 of parameter
		STA	1, X			; save as byte 2 of return
		LDA	6, SP			; load byte 0 of parameter
		STA	0, X			; save as byte 3 of return
		RTS
	}
}

#pragma MESSAGE DEFAULT C1404

/**
	Function to pass to xbee_dev_init() to control the EM250's /RESET pin
	on PTCD4.
*/
void xbee_reset_radio( struct xbee_dev_t *xbee, bool_t asserted)
{
	PTCD_PTCD4 = 0;
	PTCD_PTCD4 = asserted;
}

#ifdef XBEE_CW6
// include support for 64-bit integers, linked on CW10
#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#include "../util/jslong.c"
#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif
