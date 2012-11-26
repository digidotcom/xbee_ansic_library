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

#include "zcl_test_common.h"

#define TEST_SEQUENCE	0x21

// We don't actually care about the attributes, we're just testing the ZDO
// layer which processes the cluster list.
const zcl_attribute_base_t master_attributes[] =
{
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

const uint8_t payload_master[] =
{
	TEST_SEQUENCE,				// transaction
	_LE16( NETWORK_ADDR),	// network_addr_le
	TEST_ENDPOINT,				// endpoint
};

const wpan_envelope_t zdo_envelope =
{
	&dev,										// wpan_dev_t
	{ { 0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF } },	// ieee_address
	NETWORK_ADDR,							// network_address
	WPAN_PROFILE_ZDO,						// profile_id
	ZDO_SIMPLE_DESC_REQ,					// cluster_id
	WPAN_ENDPOINT_ZDO,					// source_endpoint
	WPAN_ENDPOINT_ZDO,					// dest_endpoint
	WPAN_CLUST_FLAG_NONE,				// options
	test_payload,							// payload
	0											// length
};
void reset( const uint8_t *response, int length)
{
	reset_common( payload_master, sizeof(payload_master), response, length);
	envelope = zdo_envelope;
}

const uint8_t basic_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_SUCCESS,		// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	8 + 2 * 1,					// length
	TEST_ENDPOINT,				// endpoint
	_LE16( TEST_PROFILE),	// profile_id_le
	_LE16( TEST_DEVICE),		// device_id_le
	TEST_DEVICE_VER,			// device_version
	1,								// input cluster count
	_LE16( TEST_CLUSTER),	// input clusters
	0,								// output cluster count
	// output clusters (none)
};

void t_basic( void)
{
	reset( basic_response, sizeof basic_response);

	wpan_envelope_dispatch( &envelope);

	test_compare( response_count, 1, NULL, "didn't receive response");
}

const uint8_t output_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_SUCCESS,		// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	8 + 2 * 1,					// length
	TEST_ENDPOINT,				// endpoint
	_LE16( TEST_PROFILE),	// profile_id_le
	_LE16( TEST_DEVICE),		// device_id_le
	TEST_DEVICE_VER,			// device_version
	0,								// input cluster count
	// input clusters (none)
	1,								// output cluster count
	_LE16( TEST_CLUSTER),	// output clusters
};

void t_output( void)
{
	reset( output_response, sizeof output_response);

	// toggle from SERVER to CLIENT
	cluster_table[0].flags = WPAN_CLUST_FLAG_CLIENT;

	wpan_envelope_dispatch( &envelope);

	test_compare( response_count, 1, NULL, "didn't receive response");
}

// helper function for building a cluster table with lots of clusters
void _lots_of_clusters( int count, uint8_t flags)
{
	int i;

	if (count >= _TABLE_ENTRIES( cluster_table))
	{
		test_fail( __func__);
		return;
	}
	for (i = 0; i < count; ++i)
	{
		cluster_table[i] = master_clusters[0];
		cluster_table[i].cluster_id = TEST_CLUSTER + i;
		cluster_table[i].flags = flags;
	}
	cluster_table[count].cluster_id = WPAN_CLUSTER_END_OF_LIST;
}

const uint8_t big_output_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_SUCCESS,		// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	8 + 2 * 39,					// length
	TEST_ENDPOINT,				// endpoint
	_LE16( TEST_PROFILE),	// profile_id_le
	_LE16( TEST_DEVICE),		// device_id_le
	TEST_DEVICE_VER,			// device_version
	0,								// input cluster count
	// input clusters (none)
	39,							// output cluster count
	_LE16( TEST_CLUSTER),	// output clusters
	_LE16( TEST_CLUSTER + 1),
	_LE16( TEST_CLUSTER + 2),
	_LE16( TEST_CLUSTER + 3),
	_LE16( TEST_CLUSTER + 4),
	_LE16( TEST_CLUSTER + 5),
	_LE16( TEST_CLUSTER + 6),
	_LE16( TEST_CLUSTER + 7),
	_LE16( TEST_CLUSTER + 8),
	_LE16( TEST_CLUSTER + 9),
	_LE16( TEST_CLUSTER + 10),
	_LE16( TEST_CLUSTER + 11),
	_LE16( TEST_CLUSTER + 12),
	_LE16( TEST_CLUSTER + 13),
	_LE16( TEST_CLUSTER + 14),
	_LE16( TEST_CLUSTER + 15),
	_LE16( TEST_CLUSTER + 16),
	_LE16( TEST_CLUSTER + 17),
	_LE16( TEST_CLUSTER + 18),
	_LE16( TEST_CLUSTER + 19),
	_LE16( TEST_CLUSTER + 20),
	_LE16( TEST_CLUSTER + 21),
	_LE16( TEST_CLUSTER + 22),
	_LE16( TEST_CLUSTER + 23),
	_LE16( TEST_CLUSTER + 24),
	_LE16( TEST_CLUSTER + 25),
	_LE16( TEST_CLUSTER + 26),
	_LE16( TEST_CLUSTER + 27),
	_LE16( TEST_CLUSTER + 28),
	_LE16( TEST_CLUSTER + 29),
	_LE16( TEST_CLUSTER + 30),
	_LE16( TEST_CLUSTER + 31),
	_LE16( TEST_CLUSTER + 32),
	_LE16( TEST_CLUSTER + 33),
	_LE16( TEST_CLUSTER + 34),
	_LE16( TEST_CLUSTER + 35),
	_LE16( TEST_CLUSTER + 36),
	_LE16( TEST_CLUSTER + 37),
	_LE16( TEST_CLUSTER + 38),
};

void t_big_output( void)
{
	reset( big_output_response, sizeof big_output_response);

	// create maximum of 39 clusters in cluster list
	_lots_of_clusters( 39, WPAN_CLUST_FLAG_OUTPUT);

	wpan_envelope_dispatch( &envelope);

	test_compare( response_count, 1, NULL, "didn't receive response");
}

const uint8_t big_split_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_SUCCESS,		// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	8 + 2 * 38,					// length
	TEST_ENDPOINT,				// endpoint
	_LE16( TEST_PROFILE),	// profile_id_le
	_LE16( TEST_DEVICE),		// device_id_le
	TEST_DEVICE_VER,			// device_version
	19,							// input cluster count
	_LE16( TEST_CLUSTER),	// input clusters
	_LE16( TEST_CLUSTER + 1),
	_LE16( TEST_CLUSTER + 2),
	_LE16( TEST_CLUSTER + 3),
	_LE16( TEST_CLUSTER + 4),
	_LE16( TEST_CLUSTER + 5),
	_LE16( TEST_CLUSTER + 6),
	_LE16( TEST_CLUSTER + 7),
	_LE16( TEST_CLUSTER + 8),
	_LE16( TEST_CLUSTER + 9),
	_LE16( TEST_CLUSTER + 10),
	_LE16( TEST_CLUSTER + 11),
	_LE16( TEST_CLUSTER + 12),
	_LE16( TEST_CLUSTER + 13),
	_LE16( TEST_CLUSTER + 14),
	_LE16( TEST_CLUSTER + 15),
	_LE16( TEST_CLUSTER + 16),
	_LE16( TEST_CLUSTER + 17),
	_LE16( TEST_CLUSTER + 18),
	19,							// output cluster count
	_LE16( TEST_CLUSTER),	// output clusters
	_LE16( TEST_CLUSTER + 1),
	_LE16( TEST_CLUSTER + 2),
	_LE16( TEST_CLUSTER + 3),
	_LE16( TEST_CLUSTER + 4),
	_LE16( TEST_CLUSTER + 5),
	_LE16( TEST_CLUSTER + 6),
	_LE16( TEST_CLUSTER + 7),
	_LE16( TEST_CLUSTER + 8),
	_LE16( TEST_CLUSTER + 9),
	_LE16( TEST_CLUSTER + 10),
	_LE16( TEST_CLUSTER + 11),
	_LE16( TEST_CLUSTER + 12),
	_LE16( TEST_CLUSTER + 13),
	_LE16( TEST_CLUSTER + 14),
	_LE16( TEST_CLUSTER + 15),
	_LE16( TEST_CLUSTER + 16),
	_LE16( TEST_CLUSTER + 17),
	_LE16( TEST_CLUSTER + 18),
};

void t_big_split( void)
{
	reset( big_split_response, sizeof big_split_response);

	// 38 clusters split equally between input and output
	_lots_of_clusters( 19, WPAN_CLUST_FLAG_INOUT);

	wpan_envelope_dispatch( &envelope);

	test_compare( response_count, 1, NULL, "didn't receive response");
}

const uint8_t too_big_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_INSUFFICIENT_SPACE,		// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	0,								// length
};

// request more clusters than our implementation can send in a response
void t_too_big_output( void)
{
	int i;

	// run three passes
	for (i = 0; i < 3; ++i)
	{
		reset( too_big_response, sizeof too_big_response);

		// go beyond maximum of 39 clusters in cluster list
		switch (i)
		{
			case 0:
				_lots_of_clusters( 40, WPAN_CLUST_FLAG_OUTPUT);
			case 1:
				_lots_of_clusters( 40, WPAN_CLUST_FLAG_INPUT);
			case 2:
				_lots_of_clusters( 20, WPAN_CLUST_FLAG_INOUT);
		}

		wpan_envelope_dispatch( &envelope);

		test_compare( response_count, 1, NULL, "didn't receive response");
	}
}

const uint8_t invalid_ep_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_INVALID_EP,	// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	0,								// length
};

// request invalid cluster IDs (0x00 and 0xFF)
void t_invalid( void)
{
	int i;

	// run two passes
	for (i = 0; i < 2; ++i)
	{
		reset( invalid_ep_response, sizeof invalid_ep_response);

		test_payload[3] = i ? 0 : 0xFF;

		wpan_envelope_dispatch( &envelope);

		test_compare( response_count, 1, NULL, "didn't receive response");
	}
}

const uint8_t inactive_ep_response[] =
{
	TEST_SEQUENCE,				// transaction
	ZDO_STATUS_NOT_ACTIVE,	// status
	_LE16( NETWORK_ADDR),	// network_addr_le
	0,								// length
};

void t_inactive( void)
{
	reset( inactive_ep_response, sizeof inactive_ep_response);

	test_payload[3] = TEST_ENDPOINT ^ 0xFF;

	wpan_envelope_dispatch( &envelope);

	test_compare( response_count, 1, NULL, "didn't receive response");
}

int main( int argc, char *argv[])
{
	int failures = 0;

	reset( NULL, 0);

	failures += DO_TEST( t_basic);
	failures += DO_TEST( t_output);
	failures += DO_TEST( t_big_output);
	failures += DO_TEST( t_big_split);

	// test some error conditions
	failures += DO_TEST( t_too_big_output);
	failures += DO_TEST( t_invalid);
	failures += DO_TEST( t_inactive);

	return test_exit( failures);
}
