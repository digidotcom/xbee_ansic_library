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

/**
	@addtogroup zcl_types
	@{
	@file zcl_types.c

	Information on each ZCL datatype; see zcl_type_info[] for additional
	documentation.
*/

/*** BeginHeader */
#include "zigbee/zcl_types.h"

#ifndef __DC__
	#define zcl_types_debug
#elif defined ZCL_TYPES_DEBUG
	#define zcl_types_debug		__debug
#else
	#define zcl_types_debug		__nodebug
#endif
/*** EndHeader */

/*** BeginHeader zcl_type_info */
/*** EndHeader */
/**
	@brief
	Table to store information on each ZCL datatype.

	Lower 4 bits encode size (0 to 8 octets, 16
	octets, one-byte size prefix, two-byte size prefix), upper 4 bits encode
	additional information.

	Need to represent 0 to 8 octets, 16 octets, size in first octet, size in
	first two octets.  12 possible values.

	Using this table, it may be possible to simplify the encode/decode functions
	greatly -- just use memcpy or swapcpy to copy the given number of bytes.

	Special case for floating point values if the platform doesn't use IEEE
	floats, and to convert from 2-byte semi-precision float to 4-byte float.

*/
const uint8_t zcl_type_info[256] =
{
	0,														// 0x00 ZCL_TYPE_NO_DATA
	ZCL_T_INVALID,										// 0x01
	ZCL_T_INVALID,										// 0x02
	ZCL_T_INVALID,										// 0x03
	ZCL_T_INVALID,										// 0x04
	ZCL_T_INVALID,										// 0x05
	ZCL_T_INVALID,										// 0x06
	ZCL_T_INVALID,										// 0x07

	1 | ZCL_T_DISCRETE,								// 0x08 ZCL_TYPE_GENERAL_8BIT
	2 | ZCL_T_DISCRETE,								// 0x09 ZCL_TYPE_GENERAL_16BIT
	3 | ZCL_T_DISCRETE,								// 0x0A ZCL_TYPE_GENERAL_24BIT
	4 | ZCL_T_DISCRETE,								// 0x0B ZCL_TYPE_GENERAL_32BIT
	5 | ZCL_T_DISCRETE,								// 0x0C ZCL_TYPE_GENERAL_40BIT
	6 | ZCL_T_DISCRETE,								// 0x0D ZCL_TYPE_GENERAL_48BIT
	7 | ZCL_T_DISCRETE,								// 0x0E ZCL_TYPE_GENERAL_56BIT
	8 | ZCL_T_DISCRETE,								// 0x0F ZCL_TYPE_GENERAL_64BIT

	1 | ZCL_T_DISCRETE,								// 0x10 ZCL_TYPE_LOGICAL_BOOLEAN
	ZCL_T_INVALID,										// 0x11
	ZCL_T_INVALID,										// 0x12
	ZCL_T_INVALID,										// 0x13
	ZCL_T_INVALID,										// 0x14
	ZCL_T_INVALID,										// 0x15
	ZCL_T_INVALID,										// 0x16
	ZCL_T_INVALID,										// 0x17

	1 | ZCL_T_DISCRETE,								// 0x18 ZCL_TYPE_BITMAP_8BIT
	2 | ZCL_T_DISCRETE,								// 0x19 ZCL_TYPE_BITMAP_16BIT
	3 | ZCL_T_DISCRETE,								// 0x1A ZCL_TYPE_BITMAP_24BIT
	4 | ZCL_T_DISCRETE,								// 0x1B ZCL_TYPE_BITMAP_32BIT
	5 | ZCL_T_DISCRETE,								// 0x1C ZCL_TYPE_BITMAP_40BIT
	6 | ZCL_T_DISCRETE,								// 0x1D ZCL_TYPE_BITMAP_48BIT
	7 | ZCL_T_DISCRETE,								// 0x1E ZCL_TYPE_BITMAP_56BIT
	8 | ZCL_T_DISCRETE,								// 0x1F ZCL_TYPE_BITMAP_64BIT

	1 | ZCL_T_ANALOG,									// 0x20 ZCL_TYPE_UNSIGNED_8BIT
	2 | ZCL_T_ANALOG,									// 0x21 ZCL_TYPE_UNSIGNED_16BIT
	3 | ZCL_T_ANALOG,									// 0x22 ZCL_TYPE_UNSIGNED_24BIT
	4 | ZCL_T_ANALOG,									// 0x23 ZCL_TYPE_UNSIGNED_32BIT
	5 | ZCL_T_ANALOG,									// 0x24 ZCL_TYPE_UNSIGNED_40BIT
	6 | ZCL_T_ANALOG,									// 0x25 ZCL_TYPE_UNSIGNED_48BIT
	7 | ZCL_T_ANALOG,									// 0x26 ZCL_TYPE_UNSIGNED_56BIT
	8 | ZCL_T_ANALOG,									// 0x27 ZCL_TYPE_UNSIGNED_64BIT

	1 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x28 ZCL_TYPE_SIGNED_8BIT
	2 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x29 ZCL_TYPE_SIGNED_16BIT
	3 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x2A ZCL_TYPE_SIGNED_24BIT
	4 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x2B ZCL_TYPE_SIGNED_32BIT
	5 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x2C ZCL_TYPE_SIGNED_40BIT
	6 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x2D ZCL_TYPE_SIGNED_48BIT
	7 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x2E ZCL_TYPE_SIGNED_56BIT
	8 | ZCL_T_ANALOG | ZCL_T_SIGNED,				// 0x2F ZCL_TYPE_SIGNED_64BIT

	1 | ZCL_T_DISCRETE,								// 0x30 ZCL_TYPE_ENUM_8BIT
	2 | ZCL_T_DISCRETE,								// 0x31 ZCL_TYPE_ENUM_16BIT
	ZCL_T_INVALID,										// 0x32
	ZCL_T_INVALID,										// 0x33
	ZCL_T_INVALID,										// 0x34
	ZCL_T_INVALID,										// 0x35
	ZCL_T_INVALID,										// 0x36
	ZCL_T_INVALID,										// 0x37

	2 | ZCL_T_FLOAT | ZCL_T_ANALOG | ZCL_T_SIGNED,	// 0x38 ZCL_TYPE_FLOAT_SEMI
	4 | ZCL_T_FLOAT | ZCL_T_ANALOG | ZCL_T_SIGNED,	// 0x39 ZCL_TYPE_FLOAT_SINGLE
	8 | ZCL_T_FLOAT | ZCL_T_ANALOG | ZCL_T_SIGNED,	// 0x3A ZCL_TYPE_FLOAT_DOUBLE
	ZCL_T_INVALID,										// 0x3B
	ZCL_T_INVALID,										// 0x3C
	ZCL_T_INVALID,										// 0x3D
	ZCL_T_INVALID,										// 0x3E
	ZCL_T_INVALID,										// 0x3F

// Note that although the String, Array, Struct, Set and Bag types are
// discrete, we're using that flag as part of our "reportable" flag.
// Perhaps we should make discrete == !analog, and use the extra bit
// to store "reportable".
	ZCL_T_INVALID,										// 0x40
	ZCL_T_SIZE_SHORT,									// 0x41 ZCL_TYPE_STRING_OCTET
	ZCL_T_SIZE_SHORT,									// 0x42 ZCL_TYPE_STRING_CHAR
	ZCL_T_SIZE_LONG,									// 0x43 ZCL_TYPE_STRING_LONG_OCTET
	ZCL_T_SIZE_LONG,									// 0x44 ZCL_TYPE_STRING_LONG_CHAR
	ZCL_T_INVALID,										// 0x45
	ZCL_T_INVALID,										// 0x46
	ZCL_T_INVALID,										// 0x47

// Note that the driver currently does not support Array, Struct, Set or Bag,
// so these entries are all set to INVALID.
	ZCL_T_SIZE_VARIABLE,								// 0x48 ZCL_TYPE_ARRAY
	ZCL_T_INVALID,										// 0x49
	ZCL_T_INVALID,										// 0x4A
	ZCL_T_INVALID,										// 0x4B
	ZCL_T_SIZE_VARIABLE,								// 0x4C ZCL_TYPE_STRUCT
	ZCL_T_INVALID,										// 0x4D
	ZCL_T_INVALID,										// 0x4E
	ZCL_T_INVALID,										// 0x4F

	ZCL_T_SIZE_VARIABLE,								// 0x50 ZCL_TYPE_SET
	ZCL_T_SIZE_VARIABLE,								// 0x51 ZCL_TYPE_BAG
	ZCL_T_INVALID,										// 0x52
	ZCL_T_INVALID,										// 0x53
	ZCL_T_INVALID,										// 0x54
	ZCL_T_INVALID,										// 0x55
	ZCL_T_INVALID,										// 0x56
	ZCL_T_INVALID,										// 0x57

	ZCL_T_INVALID,										// 0x58
	ZCL_T_INVALID,										// 0x59
	ZCL_T_INVALID,										// 0x5A
	ZCL_T_INVALID,										// 0x5B
	ZCL_T_INVALID,										// 0x5C
	ZCL_T_INVALID,										// 0x5D
	ZCL_T_INVALID,										// 0x5E
	ZCL_T_INVALID,										// 0x5F

	// 0x60 - 0x6F
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0x70 - 0x7F
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0x80 - 0x8F
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0x90 - 0x9F
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0xA0 - 0xAF
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0xB0 - 0xBF
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0xC0 - 0xCF
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	// 0xD0 - 0xDF
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,
	ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID, ZCL_T_INVALID,

	4 | ZCL_T_ANALOG,									// 0xE0 ZCL_TYPE_TIME_TIMEOFDAY
	4 | ZCL_T_ANALOG,									// 0xE1 ZCL_TYPE_TIME_DATE
	4 | ZCL_T_ANALOG,									// 0xE2 ZCL_TYPE_TIME_UTCTIME
	ZCL_T_INVALID,										// 0xE3
	ZCL_T_INVALID,										// 0xE4
	ZCL_T_INVALID,										// 0xE5
	ZCL_T_INVALID,										// 0xE6
	ZCL_T_INVALID,										// 0xE7

	2 | ZCL_T_DISCRETE,								// 0xE8 ZCL_TYPE_ID_CLUSTER
	2 | ZCL_T_DISCRETE,								// 0xE9 ZCL_TYPE_ID_ATTRIB
	4 | ZCL_T_DISCRETE,								// 0xEA ZCL_TYPE_ID_BACNET_OID
	ZCL_T_INVALID,										// 0xEB
	ZCL_T_INVALID,										// 0xEC
	ZCL_T_INVALID,										// 0xED
	ZCL_T_INVALID,										// 0xEE
	ZCL_T_INVALID,										// 0xEF

	8 | ZCL_T_DISCRETE,								// 0xF0 ZCL_TYPE_IEEE_ADDR
	ZCL_T_SIZE_128BIT | ZCL_T_DISCRETE,			// 0xF1 ZCL_TYPE_SECURITY_KEY
	ZCL_T_INVALID,										// 0xF2
	ZCL_T_INVALID,										// 0xF3
	ZCL_T_INVALID,										// 0xF4
	ZCL_T_INVALID,										// 0xF5
	ZCL_T_INVALID,										// 0xF6
	ZCL_T_INVALID,										// 0xF7

	ZCL_T_INVALID,										// 0xF8
	ZCL_T_INVALID,										// 0xF9
	ZCL_T_INVALID,										// 0xFA
	ZCL_T_INVALID,										// 0xFB
	ZCL_T_INVALID,										// 0xFC
	ZCL_T_INVALID,										// 0xFD
	ZCL_T_INVALID,										// 0xFE
	0														// 0xFF ZCL_TYPE_UNKNOWN
};


/*** BeginHeader zcl_sizeof_type */
/*** EndHeader */
/**
	@brief
	Return the number of octets used by a given ZCL datatype.

	@param[in]	type	Type to look up.  Typically one of the ZCL_TYPE_*
							macros, or the \c type element of an attribute record.

	@retval ZCL_SIZE_INVALID		unknown or invalid type
	@retval ZCL_SIZE_SHORT			1-octet size prefix
	@retval ZCL_SIZE_LONG			2-octet size prefix
	@retval ZCL_SIZE_VARIABLE		variable-length type (array, struct, set, bag)
	@retval >=0							number of octets used by type (0 to 8, 16)

	@sa zigbee/zcl_types.h
*/
zcl_types_debug
int zcl_sizeof_type( uint8_t type)
{
	uint8_t size;

	size = zcl_type_info[type] & ZCL_T_SIZE_MASK;
	switch (size)
	{
		default:
			return size;

		case ZCL_T_SIZE_SHORT:
			return ZCL_SIZE_SHORT;			// variable length, 1-octet size

		case ZCL_T_SIZE_LONG:
			return ZCL_SIZE_LONG;			// variable length, 2-octet size

		case ZCL_T_SIZE_VARIABLE:
			return ZCL_SIZE_VARIABLE;

		case ZCL_T_SIZE_INVALID:
			return ZCL_SIZE_INVALID;

		case ZCL_T_SIZE_128BIT:
			return 16;
	}
}


/*** BeginHeader zcl_type_name */
/*** EndHeader */
#include <stdio.h>
/**
	@brief
	Return a descriptive string for a given ZCL attribute type.

	@param[in]	type	ZCL attribute type

	@return	Pointer to a string description of the type, or
				"INVALID_0xHH" on unrecognized types (where HH is the hex
				representation of the type).  Never returns NULL.
*/
zcl_types_debug
const char *zcl_type_name( uint8_t type)
{
	static char name_buf[sizeof "UNSIGNED_64BIT"];
	char *prefix;

#define _IN_RANGE_ASSIGN_PREFIX( group, low, high) \
	(ZCL_TYPE_ ## group ## _ ## low ## BIT <= type \
		&& type <= ZCL_TYPE_ ## group ## _ ## high ## BIT) prefix = # group

	if _IN_RANGE_ASSIGN_PREFIX( GENERAL, 8, 64);
	else if _IN_RANGE_ASSIGN_PREFIX( BITMAP, 8, 64);
	else if _IN_RANGE_ASSIGN_PREFIX( UNSIGNED, 8, 64);
	else if _IN_RANGE_ASSIGN_PREFIX( SIGNED, 8, 64);
	else if _IN_RANGE_ASSIGN_PREFIX( ENUM, 8, 16);
	else
	{
		// Not a GENERAL, BITMAP, UNSIGNED, SIGNED or ENUM type
		#define _CASE_WITH_RETURN(n)		case ZCL_TYPE_ ## n:	return # n;
		switch (type)
		{
			_CASE_WITH_RETURN( NO_DATA)
			_CASE_WITH_RETURN( LOGICAL_BOOLEAN)
			_CASE_WITH_RETURN( FLOAT_SEMI)
			_CASE_WITH_RETURN( FLOAT_SINGLE)
			_CASE_WITH_RETURN( FLOAT_DOUBLE)
			_CASE_WITH_RETURN( STRING_OCTET)
			_CASE_WITH_RETURN( STRING_CHAR)
			_CASE_WITH_RETURN( STRING_LONG_OCTET)
			_CASE_WITH_RETURN( STRING_LONG_CHAR)
			_CASE_WITH_RETURN( ARRAY)
			_CASE_WITH_RETURN( STRUCT)
			_CASE_WITH_RETURN( SET)
			_CASE_WITH_RETURN( BAG)
			_CASE_WITH_RETURN( TIME_TIMEOFDAY)
			_CASE_WITH_RETURN( TIME_DATE)
			_CASE_WITH_RETURN( TIME_UTCTIME)
			_CASE_WITH_RETURN( ID_CLUSTER)
			_CASE_WITH_RETURN( ID_ATTRIB)
			_CASE_WITH_RETURN( ID_BACNET_OID)
			_CASE_WITH_RETURN( IEEE_ADDR)
			_CASE_WITH_RETURN( SECURITY_KEY)
			_CASE_WITH_RETURN( UNKNOWN)
			default:
				// catchall for types the library doesn't know
				sprintf( name_buf, "INVALID_0x%02X", type);
				return name_buf;
		}
		#undef _CASE_WITH_RETURN
	}

#undef _IN_RANGE_ASSIGN_PREFIX

	// Only reaches this code if <prefix> was set.
	// low three bits of type indicate bits; 8, 16, 24, 32, ... 64
	sprintf( name_buf, "%s_%uBIT", prefix, ((type & 0x07) + 1) << 3);
	return name_buf;
}

