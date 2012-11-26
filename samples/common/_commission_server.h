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

#ifndef _COMMISSION_SERVER_H
#define _COMMISSION_SERVER_H

#include "xbee/platform.h"
#include "wpan/aps.h"

#define SAMPLE_COMMISION_ENDPOINT 1

struct _endpoints {
	wpan_endpoint_table_entry_t		zdo;
	wpan_endpoint_table_entry_t		zcl;
	uint8_t									end_of_list;
};

extern struct _endpoints sample_endpoints;

#endif
