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
	@addtogroup zcl_client
	@{
	@file zigbee/zcl_client.h

	Code to support ZCL client clusters.
*/
#ifndef __ZCL_CLIENT_H
#define __ZCL_CLIENT_H

#include "wpan/aps.h"
#include "zigbee/zcl.h"

XBEE_BEGIN_DECLS

typedef struct zcl_client_read
{
	// include ieee, network and dest endpoint?
	// discovery has those set to broadcast, response handler fills them in?
	const wpan_endpoint_table_entry_t		*ep;
	uint16_t											mfg_id;
		// ZCL_MFG_NONE for standard
	uint16_t											cluster_id;
	const uint16_t							FAR	*attribute_list;
} zcl_client_read_t;

int zcl_find_and_read_attributes( wpan_dev_t *dev, const uint16_t *clusters,
	const zcl_client_read_t FAR *cr);
int zcl_process_read_attr_response( zcl_command_t *zcl,
	const zcl_attribute_base_t FAR *attr_table);
int _zcl_send_read_from_zdo_match( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope);
int zdo_send_match_desc( wpan_dev_t *dev, const uint16_t *clusters,
	uint16_t profile_id, wpan_response_fn callback, const void FAR *context);
int zcl_client_read_attributes( wpan_envelope_t FAR *envelope,
	const zcl_client_read_t *client_read);

int zcl_create_attribute_records( void FAR *buffer,
	uint8_t bufsize, const zcl_attribute_base_t FAR **p_attr_list);
int zcl_send_write_attributes( wpan_envelope_t *envelope,
	const zcl_attribute_base_t FAR *attr_list);

int zcl_print_attribute_value( uint8_t type, const void *value,
	int value_length);
int zcl_print_array_value( const void *value, int value_length);
int zcl_print_struct_value( const void *value, int value_length);

typedef const char *(*zcl_format_utctime_fn)( char dest[40],
	zcl_utctime_t utctime);
extern zcl_format_utctime_fn zcl_format_utctime;

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "zcl_client.c"
#endif

#endif	// __ZCL_CLIENT_H
