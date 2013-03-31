/*
 * Copyright (c) 2008-2013 Digi International Inc.,
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
	@addtogroup xbee_atcmd
	@{
	@file xbee/scan.h
	Structures for the ATAS (Active Scan) API responses.
*/

#include "xbee/platform.h"
#include "xbee/device.h"
#include "wpan/types.h"

typedef PACKED_STRUCT {
	uint8_t	as_type;
	uint8_t	channel;
	uint16_t	pan_be;
	addr64	extended_pan_be;
	uint8_t	allow_join;
	uint8_t	stack_profile;
	uint8_t	lqi;
	int8_t	rssi;
} xbee_scan_zigbee_response_t;

#if XBEE_WIFI_ENABLED
typedef PACKED_STRUCT {
	uint8_t	as_type;
	uint8_t	channel;
	uint8_t	security_type;
	uint8_t	link_margin;
	uint8_t	ssid[31];				// variable length, not null-terminated
} xbee_scan_wifi_response_t;
#endif

typedef union {
	uint8_t								as_type;
	xbee_scan_zigbee_response_t	zigbee;
#if XBEE_WIFI_ENABLED
	xbee_scan_wifi_response_t		wifi;
#endif
} xbee_scan_response_t;

// values for the as_type field of the ATAS response
#define XBEE_SCAN_TYPE_ZIGBEE				0x01
#if XBEE_WIFI_ENABLED
#define XBEE_SCAN_TYPE_WIFI				0x02
#endif

int xbee_scan_dump_response( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context);
