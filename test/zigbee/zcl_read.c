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

	Unit tests for reading attributes.

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
	_LE32(0x0A0B0C0D),				// attribute value
};

void reset( const uint8_t *response, int length)
{
	reset_common( payload_master, sizeof(payload_master), response, length);
	dummy_var = DUMMY_START;
}


int main()
{
	int failures = 0;

	reset( NULL, 0);

	return test_exit( failures);
}
