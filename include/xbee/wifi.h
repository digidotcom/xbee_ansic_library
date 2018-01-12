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
	@addtogroup xbee_wifi
	@{
	@file xbee/wifi.h
	Features related to the Wi-Fi XBee module (S6B).
*/

#include "xbee/platform.h"

#if !XBEE_WIFI_ENABLED
	#error "XBEE_WIFI_ENABLED must be defined as non-zero to use this header."
#endif

// Values for ATEE:
#define XBEE_WIFI_ENCRYPTION_OPEN			0x00
#define XBEE_WIFI_ENCRYPTION_WPA				0x01
#define XBEE_WIFI_ENCRYPTION_WPA2			0x02
#define XBEE_WIFI_ENCRYPTION_WEP				0x03

const char *xbee_wifi_encryption_name( uint8_t type);
