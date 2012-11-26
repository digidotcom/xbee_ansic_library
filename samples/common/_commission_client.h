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

#ifndef _COMMISSION_CLIENT_H
#define _COMMISSION_CLIENT_H

#include "xbee/platform.h"
#include "wpan/aps.h"

#define SAMPLE_COMMISION_ENDPOINT 1

typedef struct basic_value_t {
	uint8_t	app_ver;
	uint8_t	stack_ver;
	char		model_id[33];
	char		datecode[17];
} basic_value_t;

typedef struct target_t
{
	addr64			ieee;
	uint8_t			endpoint;
	basic_value_t	basic;
} target_t;

#define TARGET_COUNT 40
extern target_t target_list[TARGET_COUNT];
extern int target_index;				// number of targets in target_list

struct _endpoints {
	wpan_endpoint_table_entry_t		zdo;
	wpan_endpoint_table_entry_t		zcl;
	uint8_t									end_of_list;
};

extern struct _endpoints sample_endpoints;
int xbee_found( wpan_conversation_t FAR *conversation,
	const wpan_envelope_t FAR *envelope);

void print_target( int index);

#endif
