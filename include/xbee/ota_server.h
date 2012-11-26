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
	@addtogroup xbee_ota_server
	@{
	@file xbee/ota_server.h

	Code to add an OTA Server Cluster to a device.  It receives notification
	to start an update, and then calls back to the bootloader to receive
	that update.
*/

#ifndef XBEE_OTA_SERVER_H
#define XBEE_OTA_SERVER_H

#include "wpan/aps.h"
#include "zigbee/zcl.h"

XBEE_BEGIN_DECLS

/**
	@brief Cluster command to initiate firmware updates.

	Verifies that APS encryption
	was used (if cluster is configured as such) before handing off to
	implementation-provided function xbee_update_firmware_ota().

	@see wpan_aps_handler_fn
*/
int xbee_ota_server_cmd( const wpan_envelope_t FAR *envelope,
	void FAR *context);

/**
	@brief
	Application needs to provide this function as a method of
	receiving firmware updates over-the-air with Xmodem protocol.

	See xbee/ota_client.h for details on sending updates.

	Your application can support password-protected updates by checking the
	payload of the request.  If the payload is a valid request to initiate
	an update, this function should enter an "XMODEM receive" mode and
	start sending 'C' to the sender of the request, indicating that it
	should start sending 64-byte XMODEM packets with the new firmware.

	On Digi's Programmable XBee platform, this function would exit to the
	bootloader so it can receive the new application firmware.

	@param[in]	envelope		command sent to start update -- function may want
									to use the payload for some sort of password
									verification
	@param[in,out]	context	user context (from cluster table)

	@retval	NULL	do not respond to request
	@retval	!NULL	respond to request with error message

*/
const char *xbee_update_firmware_ota( const wpan_envelope_t FAR *envelope,
	void FAR *context);

/**
	@brief
	Macro to add the OTA cluster to the Digi Data Endpoint.

	@param[in]	flag	set to WPAN_CLUST_FLAG_NONE or WPAN_CLUST_FLAG_ENCRYPT
*/
#define XBEE_OTA_CMD_SERVER_CLUST_ENTRY(flag)								\
	{	DIGI_CLUST_PROG_XBEE_OTA_UPD,	xbee_ota_server_cmd, NULL,			\
		(flag) | WPAN_CLUST_FLAG_SERVER | WPAN_CLUST_FLAG_NOT_ZCL }

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_ota_server.c"
#endif

#endif		// XBEE_OTA_SERVER_H defined
