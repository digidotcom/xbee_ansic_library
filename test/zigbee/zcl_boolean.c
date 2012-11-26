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

	Unit tests for reading and writing to boolean values.

	Tests:
		t_write_true:	write TRUE to a boolean attribute
		t_write_false:	write TRUE to a boolean attribute
		t_write_invalid:	write an invalid value (other than 0/1) to a
								boolean attribute
		t_read:  read boolean attribute where underlying uint8_t is set
					to 0 (ZCL_BOOL_FALSE), 1 (ZCL_BOOL_TRUE) and other values
		t_short_request: same as t_write, but leaves off bytes from end of
								request.
*/

#include "zcl_test_common.h"

#define TEST_ATTRIBUTE	0x0001
#define TEST_SEQUENCE	0x21

#define DUMMY_START 0x55
uint8_t dummy_var;

const zcl_attribute_base_t master_attributes[] =
{
	{ TEST_ATTRIBUTE,								// id
		ZCL_ATTRIB_FLAG_NONE,					// flags
		ZCL_TYPE_LOGICAL_BOOLEAN,				// type
		&dummy_var,									// address
	},
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

// Write TRUE to TEST_ATTRIBUTE
const uint8_t payload_master[] =
{
	ZCL_FRAME_TYPE_PROFILE,			// frame control
	TEST_SEQUENCE,						// transaction sequence (random)
	ZCL_CMD_WRITE_ATTRIB,			// command
	_LE16(TEST_ATTRIBUTE),			// attribute ID
	ZCL_TYPE_LOGICAL_BOOLEAN,		// attribute type
	ZCL_BOOL_TRUE,						// attribute value
};

const uint8_t payload_read[] =
{
	ZCL_FRAME_TYPE_PROFILE,			// frame control
	TEST_SEQUENCE,						// transaction sequence (random)
	ZCL_CMD_READ_ATTRIB,				// command
	_LE16(TEST_ATTRIBUTE),			// attribute ID
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

void t_write_true()
{
	reset( write_success, sizeof(write_success));

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, ZCL_BOOL_TRUE, "%lu",  "dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_write_false()
{
	reset( write_success, sizeof(write_success));

	// change the WRITE value to FALSE
	test_payload[6] = ZCL_BOOL_FALSE;

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, ZCL_BOOL_FALSE, "%lu",  "dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_write_invalid()
{
	const uint8_t err_invalid[] =
	{
		ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
		TEST_SEQUENCE,
		ZCL_CMD_WRITE_ATTRIB_RESP,
		ZCL_STATUS_INVALID_VALUE,
		_LE16(TEST_ATTRIBUTE),			// attribute ID
	};
	reset( err_invalid, sizeof(err_invalid));

	// change the WRITE value to an invalid value
	test_payload[6] = 0xAA;

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, DUMMY_START, "%lu",  "dummy_var modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

// Make sure read converts non-zero values to ZCL_BOOL_TRUE

uint8_t read_success[] =
{
	ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
	TEST_SEQUENCE,
	ZCL_CMD_READ_ATTRIB_RESP,
	_LE16(TEST_ATTRIBUTE),			// attribute ID
	ZCL_STATUS_SUCCESS,
	ZCL_TYPE_LOGICAL_BOOLEAN,		// attribute type
	ZCL_BOOL_TRUE,						// attribute value
};

void t_read()
{
	for (dummy_var = 0xFF; dummy_var != 10; ++dummy_var)
	{
		reset_common( payload_read, sizeof payload_read,
							read_success, sizeof read_success);

		// any non-zero value should read as TRUE
		read_success[7] = dummy_var ? ZCL_BOOL_TRUE : ZCL_BOOL_FALSE;

		wpan_envelope_dispatch( &envelope);

		test_compare( response_count, 1, NULL, "didn't receive response");
	}
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

		test_compare( dummy_var, DUMMY_START, "%lu", "dummy_var modified");
		test_compare( response_count, 1, NULL, "didn't receive response");
	}
}


int main()
{
	int failures = 0;

	reset( NULL, 0);

	failures += DO_TEST( t_write_true);
	failures += DO_TEST( t_write_false);
	failures += DO_TEST( t_write_invalid);
	failures += DO_TEST( t_read);
	failures += DO_TEST( t_short_request);

	return test_exit( failures);
}
