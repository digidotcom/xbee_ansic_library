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
	@addtogroup xbee_ota_client
	@{
	@file xbee/ota_client.h

	Support code for over-the-air (OTA) firmware updates of application code
	on Programmable XBee target.
*/

#ifndef XBEE_OTA_CLIENT_H
#define XBEE_OTA_CLIENT_H

#include "xbee/platform.h"
#include "xbee/xmodem.h"
#include "wpan/aps.h"
#include "xbee/cbuf.h"
#include "xbee/transparent_serial.h"

XBEE_BEGIN_DECLS

#define XBEE_OTA_MAX_AUTH_LENGTH		64

/// Structure for tracking state of over-the-air update.
typedef struct xbee_ota_t {
	wpan_dev_t				*dev;		///< local device to send updates through
	addr64					target;	///< network device to update
	uint16_t					flags;	///< combination of XBEE_OTA_FLAG_* values
		/// Send data with APS encryption
		#define XBEE_OTA_FLAG_APS_ENCRYPT		0x0001

	union {
		xbee_cbuf_t				cbuf;	///< track state of circular buffer
		/// buffer for xbee_cbuf_t structure and bytes received from target
		uint8_t					raw[255 + XBEE_CBUF_OVERHEAD];
	} rxbuf;
	xbee_xmodem_state_t	xbxm;		///< track state of Xmodem transfer

	/// Payload used to initiate update
	uint8_t					auth_data[XBEE_OTA_MAX_AUTH_LENGTH];
	uint8_t					auth_length;		///< Number of bytes in \c auth_data
} xbee_ota_t;

/**
	@brief
	Initialize an xbee_ota_t structure to send firmware updates to \c target
	using \c dev.

	Calls xbee_xmodem_tx_init and xbee_xmodem_set_stream
	(therefore unnecessary to do so in main program).

	Note that when performing OTA updates, you MUST use XBEE_XMODEM_FLAG_64
	(64-byte xmodem blocks) if calling xbee_xmodem_tx_init directly.

	Sends frames to the target to have it start the update cycle.

	@param[in,out]	ota		state-tracking structure for sending update
	@param[in]		dev		device to use for sending update
	@param[in]		target	64-bit address of target device

	@retval		0			successfully initialized
	@retval		-EINVAL	invalid parameter passed in
*/
int xbee_ota_init( xbee_ota_t *ota, wpan_dev_t *dev, const addr64 *target);

/**
	@internal
	@brief
	Cluster handler for "Digi Transparent Serial" cluster.

	Used in the cluster
	table of #WPAN_ENDPOINT_DIGI_DATA (0xE8) for cluster DIGI_CLUST_SERIAL
	(0x0011).

	This is a preliminary API and WILL change in a future release.  This
	version is dedicated to processing responses in OTA (over-the-air)
	firmware updates and has not been generalized for other uses.

	@param[in]		envelope	information about the frame (addresses, endpoint,
									profile, cluster, etc.)
	@param[in,out]	context	pointer to xbee_ota_t structure

	@retval	0	handled data
	@retval	!0	some sort of error processing data

	@sa wpan_aps_handler_fn()
*/
int _xbee_ota_transparent_rx( const wpan_envelope_t FAR *envelope,
	void FAR *context);

/**
	@brief
	Macro for adding the OTA receive cluster (Digi Transparent Serial) to
	the cluster list for WPAN_ENDPOINT_DIGI_DATA.

	@param[in]	ota	pointer to an xbee_ota_t structure for tracking update
							state
	@param[in]	flags	additional flags for the cluster; typically 0 or
							#WPAN_CLUST_FLAG_ENCRYPT
*/
#define XBEE_OTA_DATA_CLIENT_CLUST_ENTRY(ota, flags)	\
	{ DIGI_CLUST_SERIAL, _xbee_ota_transparent_rx, ota, 		\
		WPAN_CLUST_FLAG_INOUT | WPAN_CLUST_FLAG_NOT_ZCL | flags }

// client cluster used to kick off OTA updates
// flag should be WPAN_CLUST_FLAG_NONE or WPAN_CLUST_FLAG_ENCRYPT
#define XBEE_OTA_CMD_CLIENT_CLUST_ENTRY(handler, context, flag)		\
	{	DIGI_CLUST_PROG_XBEE_OTA_UPD, handler,									\
		context, (flag) | WPAN_CLUST_FLAG_CLIENT | WPAN_CLUST_FLAG_NOT_ZCL }

XBEE_BEGIN_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_ota_client.c"
#endif

#endif		// XBEE_OTA_CLIENT_H defined
