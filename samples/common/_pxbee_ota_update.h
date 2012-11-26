/*
 * Copyright (c) 2010 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#ifndef _PXBEE_OTA_UPDATE_H
#define _PXBEE_OTA_UPDATE_H

#include "xbee/platform.h"
#include "wpan/aps.h"
#include "xbee/ota_client.h"

typedef struct supported_profile_t {
	uint16_t 	profile_id;
	uint8_t		flags;				// envelope options
} supported_profile_t;

extern const supported_profile_t *current_profile;

typedef struct basic_value_t {
	uint8_t	app_ver;
	uint8_t	stack_ver;
	char		model_id[33];
	char		datecode[17];
} basic_value_t;

typedef struct target_t
{
	addr64			ieee;
	basic_value_t	basic;
} target_t;

#define TARGET_COUNT 40
extern target_t target_list[TARGET_COUNT];
extern int target_index;				// number of targets in target_list

struct _endpoints {
	wpan_endpoint_table_entry_t		zdo;
	wpan_endpoint_table_entry_t		zcl;
	wpan_endpoint_table_entry_t		digi_data;
	uint8_t									end_of_list;
};

extern struct _endpoints sample_endpoints;
extern wpan_cluster_table_entry_t digi_data_clusters[];
extern xbee_ota_t xbee_ota;

int xbee_ota_find_devices( wpan_dev_t *dev, wpan_response_fn callback,
	const void FAR *context);
int xbee_found( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope);

void print_target( int index);

#endif

