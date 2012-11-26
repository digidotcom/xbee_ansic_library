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

	Unit tests for writing attributes.

	Tests:
	t_write:	simple Write Attributes request
	t_write_noresponse: same as t_write, but Write Attributes No Response
	t_write_undivided: same as t_write, but Write Attributes Undivided
				(simple test since it's only writing one attribute, and
				doing so successfully)
	t_read_only: try to write to a read-only attribute
	t_wrong_type: Write Attributes request with the wrong type
	t_wrong_id: Write Attributes request with the wrong ID
	t_wrong_direction: direction bit of frame control field is wrong
	t_mfg_specific: request is for manufacturer-specific attribute, but
				DUT's attribute is general
	t_short_request: same as t_write, but leaves off bytes from end of request

	need to write:
		general write when attribute is manufacturer-specific
		set cluster-specific bit?

*/
/*
	Work in progress for unit testing Write Attribute family of ZCL commands.

	Let's write some unit tests...

	Create an endpoint with a single read-only object and then try to set it.

	Design of Unit Test framework...

	Two xbee_dev_t devices, one with attributes and handlers, the other without.
	Both attached to a single PC with two serial ports.  One is coordinator, other
	is router, both are joined to each other.

	One device (TESTER) sends frames to the other (DUT).

	----------

	TODO: check parameters to function for "-q" for "quiet" and turn off verbose output

	----------

	Should be possible to run multiple variants on this test by modifying single
	bytes.
		Set direction incorrectly.
		Set the type wrong (profile/general vs. cluster).
		Set the read-only flag on the attribute.
		Use the wrong type when trying to set the attribute.

	----------

	Write another set of tests for each device type, to be run on the
	coordinator.

	Smart Meter
		- make sure read-only attributes return error
		- make sure all mandatory attributes are present and return acceptable
			values

*/

#include "zcl_test_common.h"

#define TEST_ATTRIBUTE	0x0001
#define TEST_SEQUENCE	0x21

// starting value for dummy attribute
#define DUMMY_START		0x12345678

uint32_t dummy_var;

const zcl_attribute_base_t master_attributes[] =
{
	{ TEST_ATTRIBUTE,								// id
		ZCL_ATTRIB_FLAG_NONE,					// flags
		ZCL_TYPE_UNSIGNED_32BIT,				// type
		&dummy_var,									// address
	},
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

// Write 0x0A0B0C0D to TEST_ATTRIBUTE
const uint8_t payload_master[] =
{
	ZCL_FRAME_TYPE_PROFILE,			// frame control
	TEST_SEQUENCE,						// transaction sequence (random)
	ZCL_CMD_WRITE_ATTRIB,			// command
	_LE16(TEST_ATTRIBUTE),			// attribute ID
	ZCL_TYPE_UNSIGNED_32BIT,		// attribute type
	_LE32(0x0A0B0C0D),				// attribute value (0x0A0B0C0D)
};

// Bug found during MSEDCL testing in August 2011.  When write request contains
// the wrong type, stack was using size of actual attribute, instead of size of
// type in request.  This test should just be an INCORRECT_TYPE response on the
// attribute, but will be two INCORRECT_TYPE errors if parsed incorrectly.
// As an extra test, we have a valid write at the end to ensure request was
// parsed correctly.
const uint8_t payload_bigwrongtype[] =
{
	ZCL_FRAME_TYPE_PROFILE,			// frame control
	TEST_SEQUENCE,						// transaction sequence (random)
	ZCL_CMD_WRITE_ATTRIB,			// command
	// record with the incorrect type...
	_LE16(TEST_ATTRIBUTE),			// attribute ID
	ZCL_TYPE_UNSIGNED_64BIT,		// attribute type
	_LE32(0x0A0B0C0D),				// attribute value (0x0A0B0C0D)

	// second write record inside of attribute value from first record
	_LE16(TEST_ATTRIBUTE + 1),
	ZCL_TYPE_UNSIGNED_8BIT,
	0xFF,

	// And finally, the valid request...
	_LE16(TEST_ATTRIBUTE),			// attribute ID
	ZCL_TYPE_UNSIGNED_32BIT,		// attribute type
	_LE32(0x0A0B0C0D),				// attribute value (0x0A0B0C0D)
};

const uint8_t response_bigwrongtype[] =
{
	ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
	TEST_SEQUENCE,
	ZCL_CMD_WRITE_ATTRIB_RESP,
	ZCL_STATUS_INVALID_DATA_TYPE,
	_LE16(TEST_ATTRIBUTE),			// attribute ID
};



const uint8_t write_success[] =
{
	ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
	TEST_SEQUENCE,
	ZCL_CMD_WRITE_ATTRIB_RESP,
	ZCL_STATUS_SUCCESS
};

const uint8_t write_error_master[] =
{
	ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_DISABLE_DEF_RESP,
	TEST_SEQUENCE,
	ZCL_CMD_WRITE_ATTRIB_RESP,
	ZCL_STATUS_READ_ONLY,
	_LE16(TEST_ATTRIBUTE),			// attribute ID
};
uint8_t write_error[sizeof write_error_master];

void _expect_error( uint8_t error_code)
{
	write_error[3] = error_code;
	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, DUMMY_START, "0x%08lx", "dummy_var modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void reset( const uint8_t *response, int length)
{
	reset_common( payload_master, sizeof(payload_master), response, length);
	memcpy( write_error, write_error_master, sizeof write_error_master);
	dummy_var = DUMMY_START;
}

void t_write()
{
	reset( write_success, sizeof(write_success));

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, 0x0A0B0C0D, "0x%08lx",  "dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_write_undivided()
{
	reset( write_success, sizeof(write_success));
	test_payload[2] = ZCL_CMD_WRITE_ATTRIB_UNDIV;

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, 0x0A0B0C0D, "0x%08lx",
		"dummy_var not modified");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_write_noresponse()
{
	reset( NULL, 0);
	test_payload[2] = ZCL_CMD_WRITE_ATTRIB_NORESP;

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, 0x0A0B0C0D, "0x%08lx",
		"dummy_var not modified");
	test_compare( response_count, 0, NULL, "received unexpected response");
}

void t_read_only()
{
	reset( write_error, sizeof write_error);
	attributes[0].flags = ZCL_ATTRIB_FLAG_READONLY;
	_expect_error( ZCL_STATUS_READ_ONLY);
}

void t_wrong_type()
{
	reset( write_error, sizeof write_error);
	attributes[0].type = ZCL_TYPE_SIGNED_32BIT;
	_expect_error( ZCL_STATUS_INVALID_DATA_TYPE);
}

void t_big_wrong_type()
{
	reset_common( payload_bigwrongtype, sizeof payload_bigwrongtype,
		response_bigwrongtype, sizeof response_bigwrongtype);
	memcpy( write_error, write_error_master, sizeof write_error_master);

	dummy_var = DUMMY_START;

	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, 0x0A0B0C0D, "0x%08lx", "dummy_var unchanged");
	test_compare( response_count, 1, NULL, "didn't receive response");
}

void t_wrong_id()
{
	reset( write_error, sizeof write_error);
	attributes[0].id = TEST_ATTRIBUTE + 1;
	_expect_error( ZCL_STATUS_UNSUPPORTED_ATTRIBUTE);
}

// With the direction bit set incorrectly, attribute should not match.
void t_wrong_direction()
{
	reset( write_error, sizeof write_error);
	// toggle direction bit in both request and expected response
	test_payload[0] ^= ZCL_FRAME_DIRECTION;
	write_error[0] ^= ZCL_FRAME_DIRECTION;
	_expect_error( ZCL_STATUS_UNSUPPORTED_ATTRIBUTE);
}

// Shouldn't match with a manufacturer-specific bit (and id field).
#define MFGID	0x1234
uint8_t mfg_write_error[] =
{
	ZCL_FRAME_TYPE_PROFILE | ZCL_FRAME_SERVER_TO_CLIENT
		| ZCL_FRAME_DISABLE_DEF_RESP | ZCL_FRAME_MFG_SPECIFIC,
	_LE16(MFGID),
	TEST_SEQUENCE,
	ZCL_CMD_WRITE_ATTRIB_RESP,
	ZCL_STATUS_UNSUPPORTED_ATTRIBUTE,
	_LE16(TEST_ATTRIBUTE),			// attribute ID
};
void t_mfg_specific()
{
	reset( mfg_write_error, sizeof mfg_write_error);

	// set manufacturer-specific bit and add manufacturer ID to request
	test_payload[0] = payload_master[0] | ZCL_FRAME_MFG_SPECIFIC;
	test_payload[1] = _LSB(MFGID);
	test_payload[2] = _MSB(MFGID);
	memcpy( &test_payload[3], &payload_master[1], sizeof(payload_master) - 1);
	envelope.length += 2;
	wpan_envelope_dispatch( &envelope);

	test_compare( dummy_var, DUMMY_START, "0x%08lx", "dummy_var modified");
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
			reset( write_error, sizeof write_error);
			write_error[3] = ZCL_STATUS_MALFORMED_COMMAND;
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

	failures += DO_TEST( t_write);
	failures += DO_TEST( t_write_noresponse);
	failures += DO_TEST( t_write_undivided);
	failures += DO_TEST( t_read_only);
	failures += DO_TEST( t_wrong_type);
	failures += DO_TEST( t_big_wrong_type);
	failures += DO_TEST( t_wrong_id);
	failures += DO_TEST( t_wrong_direction);
	failures += DO_TEST( t_mfg_specific);
	failures += DO_TEST( t_short_request);

	return test_exit( failures);
}
