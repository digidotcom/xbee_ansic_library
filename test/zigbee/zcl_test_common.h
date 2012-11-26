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
#include "wpan/aps.h"
#include "zigbee/zcl.h"
#include "zigbee/zdo.h"

#include "../unittest.h"

#define _LSB(x)	((x) & 0xFF)
#define _MSB(x)	(((x) >> 8) & 0xFF)
#define _LE16(x)	_LSB(x), _MSB(x)
#define _LE24(x)	_LE16((x) & 0xFFFF), _LSB((x) >> 16)
#define _LE32(x)	_LE16((x) & 0xFFFF), _LE16((x) >> 16)
#define SOURCE_ENDPOINT	0x55
#define TEST_ENDPOINT	0x01
#define TEST_CLUSTER		0x1234
#define TEST_PROFILE		0xAABB
#define TEST_DEVICE		0x5566
#define TEST_DEVICE_VER	0x77
#define NETWORK_ADDR		0x8899

// variables that each test must provide
extern const zcl_attribute_base_t master_attributes[];

// variables in zcl_test_common.c
extern int response_count;
extern wpan_envelope_t envelope;
extern uint8_t test_payload[200];
extern zcl_attribute_base_t attributes[20];
extern wpan_dev_t	dev;
extern const wpan_cluster_table_entry_t master_clusters[];
extern wpan_cluster_table_entry_t cluster_table[100];

void reset_common( const uint8_t *payload, int payload_length,
	const uint8_t *response, int response_length);
