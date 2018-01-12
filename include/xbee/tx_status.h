/*
 * Copyright (c) 2017 Digi International Inc.,
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
	@addtogroup xbee_tx_status
	@{
	@file xbee/tx_status.h
*/

#ifndef __XBEE_TX_STATUS_H
#define __XBEE_TX_STATUS_H

#include "xbee/device.h"
#include "xbee/delivery_status.h"

#if (!XBEE_WIFI_ENABLED) && (!XBEE_CELLULAR_ENABLED)
	#error "At least one of XBEE_WIFI_ENABLED and XBEE_CELLULAR_ENABLED " \
		"must be defined as non-zero to use this header."
#endif

/// Indicates the success or failure of a transmit operation. [Wi-Fi, Cellular]
#define XBEE_FRAME_TX_STATUS 0x89

XBEE_BEGIN_DECLS

typedef PACKED_STRUCT xbee_frame_tx_status_t {
	uint8_t			frame_type;			//< XBEE_FRAME_TX_STATUS (0x89)
	uint8_t			frame_id;
	uint8_t			delivery; /// See xbee/delivery_status.h
} xbee_frame_tx_status_t;



/**
	@brief
	Frame handler for 0x89 (XBEE_FRAME_TX_STATUS) frames -- dumps transmit
	status to STDOUT for debugging purposes.

	View the documentation of xbee_frame_handler_fn() for this function's
	parameters and return value.
*/
int xbee_frame_dump_tx_status( xbee_dev_t *xbee,
	const void FAR *frame, uint16_t length, void FAR *context);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_tx_status.c"
#endif

#endif /* __XBEE_TX_STATUS_H */