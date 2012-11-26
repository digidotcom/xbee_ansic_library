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

#define TEST_ADDR		0xABCD

// We don't actually care about the attributes, we're just testing the ZDO
// layer which processes the cluster list.
const zcl_attribute_base_t master_attributes[] =
{
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

void compare_buffers( const void *actual, size_t actual_length,
	const void *expected, size_t expected_length, const char *error)
{
	int passed;

	passed = actual_length == expected_length &&
						(memcmp( actual, expected, expected_length) == 0);

	if (test_bool( passed, error) && test_verbose)
	{
		puts( "actual:");
		hex_dump( actual, actual_length, HEX_DUMP_FLAG_TAB);

		puts( "expected:");
		hex_dump( expected, expected_length, HEX_DUMP_FLAG_TAB);
	}
}

const uint16_t empty_clusters[] = {
	WPAN_CLUSTER_END_OF_LIST
};

const uint16_t some_clusters[] = {
	TEST_CLUSTER,
	TEST_CLUSTER + 1,
	TEST_CLUSTER + 2,
	TEST_CLUSTER + 3,
	WPAN_CLUSTER_END_OF_LIST
};

const uint16_t other_clusters[] = {
	TEST_CLUSTER + 4,
	TEST_CLUSTER + 5,
	TEST_CLUSTER + 6,
	WPAN_CLUSTER_END_OF_LIST
};

uint8_t buffer[128];

void _t_common( const void *in, const void *out,
	const uint8_t *expected, size_t expected_size)
{
	int i, retval;

	for (i = 0; i < 2; ++i)
	{
		memset( buffer, 0xFF, sizeof buffer);
		retval = zdo_match_desc_request( buffer, expected_size - i, TEST_ADDR,
			TEST_PROFILE, in, out);

		if (i)
		{
			if (test_compare( retval, -ENOSPC, "%ld",
						"supposedly fit in small buffer"))
			{
				if (retval > 0)
				{
					printf( "returned %d bytes instead of -ENOSPC:", retval);
					hex_dump( buffer, retval, HEX_DUMP_FLAG_TAB);
				}
			}
		}
		else if (! test_bool( retval > 0, "retval indicates error"))
		{
			compare_buffers( buffer, retval,
				expected, expected_size, "unexpected response");

			test_compare( buffer[expected_size], 0xFF, NULL, "buffer overflow");
		}
	}
}

const uint8_t no_clusters_expected[] = {
	0xFF,							// sequence number (unchanged)
	_LE16( TEST_ADDR),		// network address
	_LE16( TEST_PROFILE),	// profile ID
	0,								// number of input clusters
	0,								// number of output clusters
};

void t_no_clusters( void)
{
	int retval;

	retval = zdo_match_desc_request( buffer, sizeof no_clusters_expected,
		TEST_ADDR, TEST_PROFILE, empty_clusters, empty_clusters);

	test_compare( retval, -ENOSPC, NULL,
		"unexpected response when asked to match no clusters");
}

const uint8_t input_only_expected[] = {
	0xFF,							// sequence number (unchanged)
	_LE16( TEST_ADDR),		// network address
	_LE16( TEST_PROFILE),	// profile ID
	4,								// number of input clusters
	_LE16( TEST_CLUSTER),
	_LE16( TEST_CLUSTER + 1),
	_LE16( TEST_CLUSTER + 2),
	_LE16( TEST_CLUSTER + 3),
	0,								// number of output clusters
};

void t_input_only( void)
{
	_t_common( some_clusters, empty_clusters,
		input_only_expected, sizeof input_only_expected);
}

const uint8_t output_only_expected[] = {
	0xFF,							// sequence number (unchanged)
	_LE16( TEST_ADDR),		// network address
	_LE16( TEST_PROFILE),	// profile ID
	0,								// number of input clusters
	4,								// number of output clusters
	_LE16( TEST_CLUSTER),
	_LE16( TEST_CLUSTER + 1),
	_LE16( TEST_CLUSTER + 2),
	_LE16( TEST_CLUSTER + 3),
};

void t_output_only( void)
{
	_t_common( empty_clusters, some_clusters,
		output_only_expected, sizeof output_only_expected);
}

const uint8_t both_expected[] = {
	0xFF,							// sequence number (unchanged)
	_LE16( TEST_ADDR),		// network address
	_LE16( TEST_PROFILE),	// profile ID
	4,								// number of input clusters
	_LE16( TEST_CLUSTER),
	_LE16( TEST_CLUSTER + 1),
	_LE16( TEST_CLUSTER + 2),
	_LE16( TEST_CLUSTER + 3),
	3,								// number of output clusters
	_LE16( TEST_CLUSTER + 4),
	_LE16( TEST_CLUSTER + 5),
	_LE16( TEST_CLUSTER + 6),
};

void t_both( void)
{
	_t_common( some_clusters, other_clusters,
		both_expected, sizeof both_expected);
}

void t_null_buffer( void)
{
	int retval;

	retval = zdo_match_desc_request( NULL, sizeof both_expected,
		TEST_ADDR, TEST_PROFILE, some_clusters, other_clusters);

	test_compare( retval, -EINVAL, NULL, "function accepted NULL buffer");
}

int main( int argc, char *argv[])
{
	int failures = 0;

	failures += DO_TEST( t_null_buffer);
	failures += DO_TEST( t_no_clusters);
	failures += DO_TEST( t_input_only);
	failures += DO_TEST( t_output_only);
	failures += DO_TEST( t_both);

	return test_exit( failures);
}
