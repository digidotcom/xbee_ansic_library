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

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "zigbee/zdo.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_client.h"
#include "zigbee/zcl_identify.h"
#include "wpan/aps.h"

#include "_commission_server.h"
#include "zigbee/zcl_commissioning.h"

#define ZCL_APP_VERSION			0x01
#define ZCL_MANUFACTURER_NAME	"Digi International"
#define ZCL_MODEL_IDENTIFIER	"ZCL Commissioning Server"
#define ZCL_DATE_CODE			"20110630 dev"
#define ZCL_POWER_SOURCE		ZCL_BASIC_PS_SINGLE_PHASE
#include "zigbee/zcl_basic_attributes.h"

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };

/// Tracks state of ZCL endpoint.
wpan_ep_state_t zcl_ep_state = { 0 };

const wpan_cluster_table_entry_t zcl_cluster_table[] =
{
	ZCL_CLUST_ENTRY_BASIC_SERVER,
	ZCL_CLUST_ENTRY_IDENTIFY_SERVER,
	ZCL_CLUST_ENTRY_COMMISSIONING_SERVER,

	WPAN_CLUST_ENTRY_LIST_END
};

// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee/wpan.h"			// for XBEE_FRAME_HANDLE_RX_EXPLICIT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_HANDLE_RX_EXPLICIT,
	XBEE_FRAME_MODEM_STATUS_DEBUG,
	XBEE_FRAME_TABLE_END
};

struct _endpoints sample_endpoints = {
	ZDO_ENDPOINT(zdo_ep_state),

	{	SAMPLE_COMMISION_ENDPOINT,		// endpoint
		0,										// profile ID (filled in later)
		NULL,									// endpoint handler
		&zcl_ep_state,						// ep_state
		0x0000,								// device ID
		0x00,									// version
		zcl_cluster_table,				// clusters
	},

	WPAN_ENDPOINT_END_OF_LIST
};

