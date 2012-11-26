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
#include "zcl_test_common.h"

wpan_dev_t	dev;
int response_count;
const uint8_t *expected_response;
int expected_length;

zcl_attribute_base_t attributes[20];

const zcl_attribute_tree_t attribute_tree[] =
			{ { ZCL_MFG_NONE, attributes, NULL } };

const wpan_cluster_table_entry_t master_clusters[] =
{
	{ TEST_CLUSTER,
		&zcl_general_command,
		attribute_tree,
		WPAN_CLUST_FLAG_SERVER | WPAN_CLUST_FLAG_ENCRYPT
	},

	WPAN_CLUST_ENTRY_LIST_END
};
wpan_cluster_table_entry_t cluster_table[100];

wpan_ep_state_t	test_ep_state = { 0 };
wpan_ep_state_t	zdo_ep_state = { 0 };

const wpan_endpoint_table_entry_t endpoint_table[] = {
	ZDO_ENDPOINT(zdo_ep_state),
	{ TEST_ENDPOINT,						// endpoint
		TEST_PROFILE,						// profile ID
		zcl_invalid_cluster,				// endpoint handler
		&test_ep_state,					// context
		TEST_DEVICE,						// device ID
		TEST_DEVICE_VER,					// version
		cluster_table						// clusters
	},

	WPAN_ENDPOINT_TABLE_END
};

uint8_t test_payload[200];

const wpan_envelope_t envelope_master =
{
	&dev,										// wpan_dev_t
	{ { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } },	// ieee_address
	0xD161,									// network_address
	TEST_PROFILE,							// profile_id
	TEST_CLUSTER,							// cluster_id
	SOURCE_ENDPOINT,						// source_endpoint
	TEST_ENDPOINT,							// dest_endpoint
	WPAN_ENVELOPE_RX_APS_ENCRYPT,		// options
	test_payload,							// payload
	0											// length
};
wpan_envelope_t envelope;


int test_tick( wpan_dev_t *dev)
{
	test_info( "wpan dev called tick...\n");
	return 0;
}

int test_send( const wpan_envelope_t FAR *envelope, uint16_t flags)
{
	int passed;

	++response_count;
	passed = (envelope->length == expected_length
		&& memcmp( envelope->payload, expected_response, expected_length) == 0);

	if (test_bool( passed, "received unexpected payload") && test_verbose)
	{
		printf( "unexpected %d-byte response:\n", envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);

		printf( "should be %d-byte response:\n", expected_length);
		hex_dump( expected_response, expected_length, HEX_DUMP_FLAG_TAB);
	}

	return 0;
}

void reset_common( const uint8_t *payload, int payload_length,
	const uint8_t *response, int response_length)
{
	dev.tick = test_tick;
	dev.endpoint_send = test_send;
	dev.endpoint_table = endpoint_table;
	dev.address.network = NETWORK_ADDR;

	memcpy( attributes, master_attributes, sizeof attributes);
	response_count = 0;
	expected_response = response;
	expected_length = response_length;
	envelope = envelope_master;
	envelope.length = payload_length;
	memset( test_payload, 0, sizeof test_payload);
	memcpy( test_payload, payload, payload_length);

	memcpy( cluster_table, master_clusters, sizeof master_clusters);
}

