/*
	Test zcl_encode_attribute_value() with encoding arrays, structures
	and other complex data types.

*/

#include <stdio.h>
#include "zigbee/zcl.h"
#include "../unittest.h"

// structure for tests to run -- holds the attribute to encode, and information on how
// it's expected to encode
typedef struct test_data_t {
	zcl_attribute_base_t		attr;
	uint8_t						encoded_length;
	uint8_t						encoded[256];
} test_data_t;

const uint8_t tod_slots[8] = { 1, 2, 3, 4, 5, 6, 7, 8 };
const zcl_array_t tod_slots_array_attr =
	{ 8, 8, 1, ZCL_TYPE_UNSIGNED_8BIT, tod_slots };
const zcl_array_t tod_slots_array_empty_attr =
	{ 8, 0, 1, ZCL_TYPE_UNSIGNED_8BIT, tod_slots };

const zcl_array_t big_nothing_array =
	{ 0xABCD, 0xABCD, 0, ZCL_TYPE_NO_DATA, "" };

const struct {
	uint8_t		x;
	uint16_t		y;
} big_slots[8] = {
	{ 1, 2 }, { 3, 4 }, { 5, 6 }, { 7, 8 }
};

const zcl_array_t big_slots_u8_attr =
	{ 4, 4, sizeof(big_slots[0]), ZCL_TYPE_UNSIGNED_8BIT, &big_slots[0].x };
const zcl_array_t big_slots_u16_attr =
	{ 4, 4, sizeof(big_slots[0]), ZCL_TYPE_UNSIGNED_16BIT, &big_slots[0].y };

typedef struct test_struct_value_t {
	uint16_t				u16;
	uint8_t				u8;
	bool_t				b;
	uint32_t				u32;
	zcl_array_t			array;
} test_struct_value_t;

const zcl_struct_element_t test_struct_elements[] =
{
	{ ZCL_TYPE_UNSIGNED_8BIT,		offsetof( test_struct_value_t, u8) },
	{ ZCL_TYPE_UNSIGNED_16BIT,		offsetof( test_struct_value_t, u16) },
	{ ZCL_TYPE_UNSIGNED_32BIT,		offsetof( test_struct_value_t, u32) },
	{ ZCL_TYPE_LOGICAL_BOOLEAN,	offsetof( test_struct_value_t, b) },
	{ ZCL_TYPE_NO_DATA,				0 },
};

const zcl_struct_element_t test_struct_with_array[] =
{
	{ ZCL_TYPE_UNSIGNED_8BIT,		offsetof( test_struct_value_t, u8) },
	{ ZCL_TYPE_ARRAY,					offsetof( test_struct_value_t, array) },
	{ ZCL_TYPE_UNSIGNED_32BIT,		offsetof( test_struct_value_t, u32) },
	{ ZCL_TYPE_LOGICAL_BOOLEAN,	offsetof( test_struct_value_t, b) },
	{ ZCL_TYPE_NO_DATA,				0 },
};

const test_struct_value_t test_struct_value =
{	0x1234, 0x55, TRUE, 0x0ABCDEF0,
	{ 4, 4, sizeof(big_slots[0]), ZCL_TYPE_UNSIGNED_8BIT, &big_slots[0].x } };

const zcl_struct_t test_struct_meta =
	{ 5, &test_struct_value, test_struct_elements };

const zcl_struct_t test_struct_with_array_meta =
	{ 5, &test_struct_value, test_struct_with_array };

const test_data_t test_data[] = {
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_ARRAY, &tod_slots_array_attr },
		11, { ZCL_TYPE_UNSIGNED_8BIT, 0x08, 0x00, 1, 2, 3, 4, 5, 6, 7, 8 }
	},
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_ARRAY, &tod_slots_array_empty_attr },
		3, { ZCL_TYPE_UNSIGNED_8BIT, 0x00, 0x00 }
	},
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_ARRAY, &big_slots_u8_attr },
		7, { ZCL_TYPE_UNSIGNED_8BIT, 0x04, 0x00, 1, 3, 5, 7 }
	},
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_ARRAY, &big_slots_u16_attr },
		11, { ZCL_TYPE_UNSIGNED_16BIT, 0x04, 0x00, 2, 0, 4, 0, 6, 0, 8, 0 }
	},
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_ARRAY, &big_nothing_array },
		3, { ZCL_TYPE_NO_DATA, 0xCD, 0xAB }
	},
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_STRUCT, &test_struct_meta },
		15, { 0x05, 0x00,
				ZCL_TYPE_UNSIGNED_8BIT, 0x55,
				ZCL_TYPE_UNSIGNED_16BIT, 0x34, 0x12,
				ZCL_TYPE_UNSIGNED_32BIT, 0xF0, 0xDE, 0xBC, 0x0A,
				ZCL_TYPE_LOGICAL_BOOLEAN, TRUE,
				ZCL_TYPE_NO_DATA }
	},
	{	{	0x1234, ZCL_ATTRIB_FLAG_NONE, ZCL_TYPE_STRUCT, &test_struct_with_array_meta },
		20, { 0x05, 0x00,
				ZCL_TYPE_UNSIGNED_8BIT, 0x55,
				ZCL_TYPE_ARRAY, ZCL_TYPE_UNSIGNED_8BIT, 0x04, 0x00, 1, 3, 5, 7,
				ZCL_TYPE_UNSIGNED_32BIT, 0xF0, 0xDE, 0xBC, 0x0A,
				ZCL_TYPE_LOGICAL_BOOLEAN, TRUE,
				ZCL_TYPE_NO_DATA }
	},
};
#define TEST_COUNT (sizeof test_data / sizeof *test_data)

void t_encode( void)
{
	uint8_t	buffer[256];
	char		errstr[80];
	int		i;
	int		retval;
	bool_t	match;

	for (i = 0; i < TEST_COUNT; ++i)
	{
		// make buffer one byte too small
		memset( buffer, 0, sizeof buffer);
		retval = zcl_encode_attribute_value( buffer,
			test_data[i].encoded_length - 1, &test_data[i].attr);
		sprintf( errstr, "case #%u should fail with small buffer", i);
		test_compare( retval, -ZCL_STATUS_INSUFFICIENT_SPACE, "%ld", errstr);

		// make buffer correct size
		memset( buffer, 0, sizeof buffer);
		retval = zcl_encode_attribute_value( buffer,
			test_data[i].encoded_length, &test_data[i].attr);
		sprintf( errstr, "case #%u had wrong return value", i);
		if (! test_compare( retval, test_data[i].encoded_length, "%ld", errstr))
		{
			sprintf( errstr, "case #%u encoded incorrectly", i);
			match = 0 == memcmp( buffer, test_data[i].encoded,
																test_data[i].encoded_length);
			if (! test_bool( match, errstr))
			{
				continue;		// passed all tests for this case
			}
		}

		if (retval > 0)
		{
			printf( "good:");
			hex_dump( test_data[i].encoded, test_data[i].encoded_length, HEX_DUMP_FLAG_TAB);
			printf( "bad:");
			hex_dump( buffer, retval, HEX_DUMP_FLAG_TAB);
		}
	}

	sprintf( errstr, "should have passed %lu tests",
		(unsigned long) TEST_COUNT * 3);
	test_info( errstr);
}

int main( int argc, char *argv[])
{
	int failures = 0;

	failures += DO_TEST( t_encode);

	return test_exit( failures);
}