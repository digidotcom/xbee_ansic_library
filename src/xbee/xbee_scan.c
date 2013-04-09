/*
 * Copyright (c) 2009-2013 Digi International Inc.,
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
	@file xbee_scan.c
	Code related to ATAS (Active Scan) responses.
*/

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/scan.h"
#include "wpan/types.h"
#if XBEE_WIFI_ENABLED
	#include "xbee/wifi.h"
#endif

int xbee_scan_dump_response( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t as = { { 'A', 'S' } };
	const xbee_frame_local_at_resp_t FAR *resp = raw;
	const xbee_scan_response_t FAR *atas = (const void FAR *)resp->value;
	char buffer[ADDR64_STRING_LENGTH];
	bool_t hexdump = FALSE;
	int scan_length = length - offsetof( xbee_frame_local_at_resp_t, value);

	if (resp->header.command.w == as.w)
	{
		if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
		{
			// this is a successful ATAS response
			if (scan_length < 1)
			{
				puts( "Scan complete");
			}
			else switch (atas->as_type)
			{
#if XBEE_WIFI_ENABLED
				case XBEE_SCAN_TYPE_WIFI:
				{
					int ssid_length = scan_length
							- offsetof( xbee_scan_response_t, wifi.ssid);
					if (ssid_length < 1)
					{
						printf( "Short (%u-byte) response:\n", scan_length);
						hexdump = TRUE;
					}
					else
					{
						printf( "CH%2u  Enc: %-4s  LM:%3udBm  ID:[%.*s]\n",
							atas->wifi.channel,
							xbee_wifi_encryption_name( atas->wifi.security_type),
							atas->wifi.link_margin,
							ssid_length,
							atas->wifi.ssid);
					}
					break;
				}
#endif
				case XBEE_SCAN_TYPE_ZIGBEE:
					if (scan_length < sizeof atas->zigbee)
					{
						printf( "Short (%u-byte) response:\n", scan_length);
						hexdump = TRUE;
					}
					else
					{
						printf( "CH%2u PAN:0x%04X %" PRIsFAR " J%u SP%u LQI:%3u RSSI:%d\n",
							atas->zigbee.channel, be16toh( atas->zigbee.pan_be),
							addr64_format( buffer, &atas->zigbee.extended_pan_be),
							atas->zigbee.allow_join, atas->zigbee.stack_profile,
							atas->zigbee.lqi, atas->zigbee.rssi);
					}
					break;
				default:
					printf( "Unknown scan type 0x%02X\n", atas->as_type);
					hexdump = TRUE;
					break;
			}
		}
		else
		{
			printf( "ATAS status 0x%02X\n", resp->header.status);
			hexdump = TRUE;
		}

		if (hexdump)
		{
			hex_dump( resp->value, scan_length, HEX_DUMP_FLAG_OFFSET);
		}
	}

	return 0;
}
