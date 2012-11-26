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
	@addtogroup zcl_identify
	@{
	@file zigbee/zcl_identify.h
	Header for implementation of the ZigBee Identify Cluster
	(ZCL Spec section 3.5).

	Numbers in comments refer to sections of ZigBee Cluster Library
	specification (Document 075123r02ZB).
*/

#ifndef ZIGBEE_ZCL_IDENTIFY_H
#define ZIGBEE_ZCL_IDENTIFY_H

#include "zigbee/zcl.h"

XBEE_BEGIN_DECLS

extern const zcl_attribute_tree_t zcl_identify_attribute_tree[];

#define ZCL_IDENTIFY_ATTR_IDENTIFY_TIME	0x0000
										///< 3.5.2.2, IdentifyTime Attribute

// client->server commands
#define ZCL_IDENTIFY_CMD_IDENTIFY			0x00
										///< 3.5.2.3.1, Identify Command
#define ZCL_IDENTIFY_CMD_QUERY				0x01
										///< 3.5.2.3.2, Identify Query Command

// server->client responses
#define ZCL_IDENTIFY_CMD_QUERY_RESPONSE	0x00
										///< 3.5.2.4.1, Identify Query Response Command

int zcl_identify_command( const wpan_envelope_t FAR *envelope,
	void FAR *context);

uint16_t zcl_identify_isactive( void);

/// Macro to add an Identify Server cluster to an endpoint's cluster table.
#define ZCL_CLUST_ENTRY_IDENTIFY_SERVER	\
	{ ZCL_CLUST_IDENTIFY,						\
		&zcl_identify_command,					\
		zcl_identify_attribute_tree,			\
		WPAN_CLUST_FLAG_SERVER }

XBEE_END_DECLS

#ifdef __DC__
	#use "zcl_identify.c"
#endif

#endif		// ZIGBEE_ZCL_IDENTIFY_H
