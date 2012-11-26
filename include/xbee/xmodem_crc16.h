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
	@addtogroup util_xmodem
	@{
	@file xmodem_crc16.h
	Header for crc16_calc() function implemented in xmodem_crc16.c.
*/

#ifndef XBEE_XMODEM_CRC16_H
#define XBEE_XMODEM_CRC16_H

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

uint16_t crc16_calc( const void FAR *data, uint16_t length, uint16_t current);

// Rabbit platform has crc16_calc() in crc16.lib; no need to #use xmodem_crc16.c

XBEE_END_DECLS

#endif		// XBEE_XMODEM_CRC16_H defined
