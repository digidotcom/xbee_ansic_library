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
	December 2010

	Unit tests of the zcl_type_info[] array.

	Tests:
		t_reportable_sizes:
			Make sure reportable types have a size of 1, 2, 3, 4, 5, 6, 7, 8
			or 16 bytes.

		t_signed:
			Makes sure only signed types are marked signed, and that they are
			also marked as analog.

		t_invalid:
			If a type is set as "invalid", make sure upper bits aren't set.

*/

#include <stdio.h>
#include "zigbee/zcl_types.h"

#include "../unittest.h"

// Make sure reportable types have a size of 1, 2, 3, 4, 5, 6, 7, 8 or 16
// bytes.
void t_reportable_sizes()
{
	int i;
	int size;
	char errstr[80];

	for (i = 0; i < 256; ++i)
	{
		if (ZCL_TYPE_IS_REPORTABLE(i))
		{
			size = zcl_sizeof_type(i);
			sprintf( errstr, "type 0x%02x has invalid size %d", i, size);
			test_bool( size == 16 || (size >= 1 && size <= 8), errstr);
		}
	}
}

// Makes sure only signed types are marked signed, and that they are
// also marked as analog.
void t_signed()
{
	int i;
	char errstr[80];
	bool_t is_signed;

	for (i = 0; i < 256; ++i)
	{
		is_signed = FALSE;
		if (i >= 0x28 && i <= 0x2F)		// signed 8-bit to 64-bit
		{
			is_signed = TRUE;
		}
		if (i >= 0x38 && i <= 0x3A)		// floating point types
		{
			is_signed = TRUE;
		}
		sprintf( errstr, "type 0x%02x should %s signed", i,
			is_signed ? "be" : "not be");
		test_bool( (ZCL_TYPE_IS_SIGNED(i) != 0) == is_signed, errstr);

		if (is_signed)
		{
			sprintf( errstr, "signed type 0x%02x should be analog", i);
			test_bool( ZCL_TYPE_IS_ANALOG(i), errstr);
		}
	}
}

// If a type is set as "invalid", make sure upper bits aren't set.
void t_invalid()
{
	int i;
	char errstr[80];

	for (i = 0; i < 256; ++i)
	{
		if (zcl_sizeof_type(i) == ZCL_SIZE_INVALID)
		{
			sprintf( errstr, "invalid type 0x%02x has attributes set", 	i);
			test_compare( zcl_type_info[i] & 0xF0, 0, "0x%02x", errstr);
		}
	}
}

int main()
{
	int failures = 0;

	failures += DO_TEST( t_reportable_sizes);
	failures += DO_TEST( t_signed);
	failures += DO_TEST( t_invalid);

	return test_exit( failures);
}
