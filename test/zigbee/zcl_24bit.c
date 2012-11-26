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
/*
	Tom Collins
	<Tom.Collins@digi.com>
	August 2010

	Unit tests for writing to 24-bit values.  Special case since host uses
	a 32-bit variable to store the value.  Driver must sign-extend negative
	values on write, but only for ZCL_TYPE_SIGNED_24BIT.

	Tests:

	t_write_negative: Write a negative value to SIGNED_24BIT.
	t_write_positive: Write a positive value to SIGNED_24BIT.
	t_write_unsigned: Write a "negative" value (high bit set) to UNSIGNED_24BIT.
	t_short_request: Same as t_write_negative, but leave off bytes from end of
							request.
*/

#include "zcl_test_common.h"

#define TEST_ATTRIBUTE	0x0001
#define TEST_SEQUENCE	0x21

// starting value for dummy attribute
#define DUMMY_START		0x00345678

int32_t dummy_var;

const zcl_attribute_base_t master_attributes[] =
{
	{ TEST_ATTRIBUTE,								// id
		ZCL_ATTRIB_FLAG_NONE,					// flags
		ZCL_TYPE_SIGNED_24BIT,					// type
		&dummy_var,									// address
	},
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

// Write 0x80000 to TEST_ATTRIBUTE
const uint8_t payload_master[] =
{
	ZCL_FRAME_TYPE_PROFILE,			// frame control
	TEST_SEQUENCE,						// transaction sequence (random)
	ZCL_CMD_WRITE_ATTRIB,			// command
	_LE16(TEST_ATTRIBUTE),			// attribute ID
	ZCL_TYPE_SIGNED_24BIT,			// attribute type
	_LE24(0x800000),					// attribute value
};

void reset( const uint8_t *response, int length)
{
	reset_common( payload_master, sizeof(payload_master), response, length);
	dummy_var = DUMMY_START;
}


const uint8_t write_success[] =
{
	ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
	TEST_SEQUENCE,
	ZCL_CMD_WRITE_ATTRIB_RESP,
	ZCL_STATUS_SUCCESS
};

void t_write_negative()
{
	// use variable for proper sign extension (in test_compare) w/64-bit platform
	int32_t expected = 0xFF800000;
	reset( write_success, sizeof(write_success));

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, expected, "0x%08lx", "dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_write_positive()
{
	reset( write_success, sizeof(write_success));

	// change the WRITE value to 0x7F0000
	test_payload[8] = 0x7F;

	dummy_var = 0xFFFFFFFF;		// make sure MSB cleared to 0x00

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, 0x007F0000, "0x%08lx",  "dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_write_unsigned()
{
	reset( write_success, sizeof(write_success));

	// change the type of attribute 0, and change it in the request as well
	attributes[0].type = test_payload[5] = ZCL_TYPE_UNSIGNED_24BIT;

	dummy_var = 0xFFFFFFFF;		// make sure MSB cleared to 0x00

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, 0x00800000, "0x%08lx",  "dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

/**
	Shorten a legitimate request by multiple bytes so write attribute record is
	incomplete (not enough bytes in value, no type, missing MSB of attribute ID)
	and ultimately until it's a request without any attribute records.
*/
void t_short_request()
{
	int i;
	char buffer[40];
	const uint8_t err_malformed[] =
	{
		ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
		TEST_SEQUENCE,
		ZCL_CMD_WRITE_ATTRIB_RESP,
		ZCL_STATUS_MALFORMED_COMMAND,
		_LE16(TEST_ATTRIBUTE),			// attribute ID
	};

	for (i = 1; i < sizeof(payload_master) - 2; ++i)
	{
		sprintf( buffer, "%d byte(s) short", i);
		test_info( buffer);

		if (i == sizeof(payload_master) - 3)
		{
			reset( write_success, sizeof(write_success));
		}
		else
		{
			reset( err_malformed, sizeof(err_malformed));
		}
		envelope.length -= i;

		wpan_envelope_dispatch( &envelope);

		test_compare( dummy_var, DUMMY_START, "0x%08lx", "dummy_var modified");
		test_compare( response_count, 1, NULL, "didn't receive response");
	}
}

int main()
{
	int failures = 0;

	reset( NULL, 0);

	failures += DO_TEST( t_write_negative);
	failures += DO_TEST( t_write_positive);
	failures += DO_TEST( t_write_unsigned);
	failures += DO_TEST( t_short_request);

	return test_exit( failures);
}
