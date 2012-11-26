/*
 * Copyright (c) 2009-2012 Digi International Inc.,
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
	@file zigbee/zcl_types.h
	Macros and structures related to the ZCL datatypes.

	All multi-byte types are little-endian.
*/

#ifndef __ZIGBEE_ZCL_TYPES_H
#define __ZIGBEE_ZCL_TYPES_H

#include "xbee/platform.h"
#include "xbee/byteorder.h"

XBEE_BEGIN_DECLS

/** @name ZCL Null Types
	@{
*/
#define ZCL_TYPE_NO_DATA				0x00		// 0 octets
//@}


/** @name ZCL General Data (discrete) Types
	@{
*/
#define ZCL_TYPE_GENERAL_8BIT			0x08		// 1 octet
#define ZCL_TYPE_GENERAL_16BIT		0x09		// 2 octets
#define ZCL_TYPE_GENERAL_24BIT		0x0a		// 3 octets
#define ZCL_TYPE_GENERAL_32BIT		0x0b		// 4 octets
#define ZCL_TYPE_GENERAL_40BIT		0x0c		// 5 octets
#define ZCL_TYPE_GENERAL_48BIT		0x0d		// 6 octets
#define ZCL_TYPE_GENERAL_56BIT		0x0e		// 7 octets
#define ZCL_TYPE_GENERAL_64BIT		0x0f		// 8 octets
//@}

/** @name ZCL Logical (discrete) Types
	@{
*/
#define ZCL_TYPE_LOGICAL_BOOLEAN		0x10		// 1 octet, invalid=0xff
//@}

/** @name Values for #ZCL_TYPE_LOGICAL_BOOLEAN
	@{
*/
#define ZCL_BOOL_FALSE		0x00
#define ZCL_BOOL_TRUE		0x01
#define ZCL_BOOL_INVALID	0xff
//@}

/** @name ZCL Bitmap (discrete) Types
	@{
*/
#define ZCL_TYPE_BITMAP_8BIT			0x18		// 1 octet
#define ZCL_TYPE_BITMAP_16BIT			0x19		// 2 octets
#define ZCL_TYPE_BITMAP_24BIT			0x1a		// 3 octets
#define ZCL_TYPE_BITMAP_32BIT			0x1b		// 4 octets
#define ZCL_TYPE_BITMAP_40BIT			0x1c		// 5 octets
#define ZCL_TYPE_BITMAP_48BIT			0x1d		// 6 octets
#define ZCL_TYPE_BITMAP_56BIT			0x1e		// 7 octets
#define ZCL_TYPE_BITMAP_64BIT			0x1f		// 8 octets
//@}

/** @name ZCL Unsigned integer (analog) Types
	@{
*/
#define ZCL_TYPE_UNSIGNED_8BIT		0x20		// 1 octet, invalid=0xff
#define ZCL_TYPE_UNSIGNED_16BIT		0x21		// 2 octets, invalid = 0xffff
#define ZCL_TYPE_UNSIGNED_24BIT		0x22		// 3 octets, invalid = 0xffffff
#define ZCL_TYPE_UNSIGNED_32BIT		0x23		// 4 octets, invalid = 0xffffffff
#define ZCL_TYPE_UNSIGNED_40BIT		0x24		// 5 octets, invalid = all ff's
#define ZCL_TYPE_UNSIGNED_48BIT		0x25		// 6 octets, invalid = all ff's
#define ZCL_TYPE_UNSIGNED_56BIT		0x26		// 7 octets, invalid = all ff's
#define ZCL_TYPE_UNSIGNED_64BIT		0x27		// 8 octets, invalid = all ff's
//@}

/** @name ZCL Signed integer (analog) Types
	@{
*/
#define ZCL_TYPE_SIGNED_8BIT			0x28		// 1 octet, invalid=0x80
#define ZCL_TYPE_SIGNED_16BIT			0x29		// 2 octets, invalid=0x8000
#define ZCL_TYPE_SIGNED_24BIT			0x2a		// 3 octets, invalid=0x800000
#define ZCL_TYPE_SIGNED_32BIT			0x2b		// 4 octets, invalid=0x80000000
#define ZCL_TYPE_SIGNED_40BIT			0x2c		// 5 octets, invalid=0x8000000000
#define ZCL_TYPE_SIGNED_48BIT			0x2d		// 6 octets, invalid=0x80...0
#define ZCL_TYPE_SIGNED_56BIT			0x2e		// 7 octets, invalid=0x80...0
#define ZCL_TYPE_SIGNED_64BIT			0x2f		// 8 octets, invalid=0x80...0
//@}

/** @name ZCL Enumeration (discrete) Types
	@{
*/
#define ZCL_TYPE_ENUM_8BIT				0x30		// 1 octet, invalid=0xff
#define ZCL_TYPE_ENUM_16BIT			0x31		// 2 octets, invalid=0xffff
//@}

/** @name ZCL Floating point (analog) Types
	@{
*/
/// semi-precision (16-bit) floating point (unsupported)
#define ZCL_TYPE_FLOAT_SEMI			0x38		// 2 octets, invalid=NaN

/// single-precision (32-bit) IEEE 754 floating point
#define ZCL_TYPE_FLOAT_SINGLE			0x39		// 4 octets, invalid=NaN

/// double-precision (64-bit) IEEE 754 floating point
#define ZCL_TYPE_FLOAT_DOUBLE			0x3a		// 8 octets, invalid=NaN
//@}

/*	For now, comment out all floating point macros.
	Needs significant work if we're going to use it.
	Decide on whether the _EXPONENT macro should be the actual signed exponent,
	or the value of the bits.

	Single precision and double-precision should map just fine to 'float' and
	'double' on platforms that properly support IEEE 754 floating point.

/ ** @name Semi-Precision Floating Point
	Macros for working with host-byte-order semi-precision floating-point values,
	stored in a uint16_t.

	@internal

	@WARNING The current driver does not implement floating point values,
				these macros are completely untested.
	@{
* /
/// True if semi-precision float \a f is negative.
#define ZCL_FLOAT_SEMI_IS_NEGATIVE(f)	((uint16_t)(f) & UINT16_C(0x8000))
/// The 5-bit, sign-extended exponent of semi-precision float \a f.
#define ZCL_FLOAT_SEMI_EXPONENT(f)		\
								(((int16_t)(f) >> 10 & UINT16_C(0x001F)) - 15)
/// The 10-bit mantissa of semi-precision float \a f.
#define ZCL_FLOAT_SEMI_MANTISSA(f)		((int16_t)(f) & UINT16_C(0x03FF))

/// Not a Number (NaN) used for undefined values and is indicated by an
/// exponent of all-ones with a non-zero mantissa.
#define ZCL_FLOAT_SEMI_IS_NAN(f)			(ZCL_FLOAT_SEMI_EXPONENT(f) == 16) && \
													(ZCL_FLOAT_SEMI_MANTISSA(f))

/// Un-normalised numbers: numbers < 2^-14 are indicated by a value of 0 for the
/// exponent.  The hidden bit is set to 0
#define ZCL_FLOAT_SEMI_NORMALISED(f)	(ZCL_FLOAT_SEMI_EXPONENT(f) == -15)

/// Semi-Precision, Positive Infinity:
///	sign of 0, exponent of 31 and zero mantissa
#define ZCL_FLOAT_SEMI_INF_POS			UINT16_C(0x7C00)
/// Semi-Precision, Negative Infinity:
///	sign of 1, exponent of 31 and zero mantissa
#define ZCL_FLOAT_SEMI_INF_NEG			UINT16_C(0xFC00)

/// Semi-Precision, Positive Zero:
///	sign of 0, exponent of 0 and mantissa of 0
#define ZCL_FLOAT_SEMI_ZERO_POS			UINT16_C(0x0000)
/// Semi-Precision, Negative Zero:
///	sign of 1, exponent of 0 and mantissa of 0
#define ZCL_FLOAT_SEMI_ZERO_NEG			UINT16_C(0x8000)
//@}

/ ** @name Semi-Precision Floating Point
	Macros for working with single-precision IEEE 754 floating-point values, stored
	in a uint32_t.

	@internal

	@WARNING The current driver does not implement floating point values,
				these macros are completely untested.
	@{
* /
/// True if single-precision float \a f is negative.
#define ZCL_FLOAT_IS_NEGATIVE(f)		((uint32_t)(f) & UINT32_C(0x80000000))
/// The 8-bit exponent of single-precision float \a f.
#define ZCL_FLOAT_EXPONENT(f)			\
									(((int32_t)(f) >> 23 & UINT32_C(0x00FF)) - 127)
/// The 23-bit mantissa of single-precision float \a f.
#define ZCL_FLOAT_MANTISSA(f)			((uint32_t)(f) & UINT32_C(0x007FFFFF))

// Not a Number (NaN) used for undefined values and is indicated by an exponent
// of all-ones with a non-zero mantissa.
#define ZCL_FLOAT_IS_NAN(f)			(ZCL_FLOAT_EXPONENT(f) == 128) && \
												(ZCL_FLOAT_MANTISSA(f))

// Un-normalised numbers: numbers < 2^-14 are indicated by a value of 0 for the
// exponent.
#define ZCL_FLOAT_NORMALISED(f)		(ZCL_FLOAT_EXPONENT(f) == -127)

// Infinity: indicated by an exponent of 31 and a zero mantissa
#define ZCL_FLOAT_INF_POS				(float) 0x7F800000ul
#define ZCL_FLOAT_INF_NEG				(float) 0xFF800000ul

// Zero: indicated by a zero exponent and zero mantissa
#define ZCL_FLOAT_ZERO_POS				(float) 0x00000000ul
#define ZCL_FLOAT_ZERO_NEG				(float) 0x80000000ul
//@}

*/


/** @name ZCL String (discrete) Types
	@{
*/
//
/// First octet is number of bytes in string or 0xff for invalid.
#define ZCL_TYPE_STRING_OCTET			0x41

/// First octet is number of characters in string or 0xff for invalid.
/// Note that each character may be more than one byte depending on the
/// "language and character set field of the complex descriptor contained
/// in the character data [following the first octet]".  See ZCL for details.
#define ZCL_TYPE_STRING_CHAR			0x42

/// First two octets are number of bytes in string or 0xffff for invalid.
/// This will come in so handy for a protcol with frames of 256 bytes max
/// (he said sarcastically).
#define ZCL_TYPE_STRING_LONG_OCTET	0x43

/// First two octets are number of characters in string or 0xffff for invalid.
/// Note that each character may be more than one byte depending on the
/// "language and character set field of the complex descriptor contained
/// in the character data [following the first octet]".  See ZCL for details.
#define ZCL_TYPE_STRING_LONG_CHAR	0x44
//@}

// Array, Structure, Set, Bag
// These datatypes are currently unsupported.

/** @name ZCL Ordered Sequence (discrete) Types
	@{
*/
/// ZCL Array (unsupported)
#define ZCL_TYPE_ARRAY					0x48		// invalid=0xffff in first 2 octets
/// ZCL Struct (unsupported)
#define ZCL_TYPE_STRUCT					0x4C		// invalid=0xffff in first 2 octets
//@}

/** @name ZCL Collection (discrete) Types
	@{
*/
/// ZCL Set (unsupported)
#define ZCL_TYPE_SET						0x50		// number of elements = 0xffff
/// ZCL Bag (unsupported)
#define ZCL_TYPE_BAG						0x51		// number of elements = 0xffff
//@}

/** @name ZCL Time (analog) Types
	@{
*/
/// see zcl_timeofday_t
#define ZCL_TYPE_TIME_TIMEOFDAY		0xE0		// 4 octets, invalid=0xffffffff

/// see zcl_date_t
#define ZCL_TYPE_TIME_DATE				0xE1		// 4 octets, invalid=0xffffffff

/// number of seconds (stored in uint32_t) since Midnight on 1/1/2000 UTC
#define ZCL_TYPE_TIME_UTCTIME			0xE2		// 4 octets, invalid=0xffffffff
//@}

typedef uint32_t zcl_utctime_t;
#define ZCL_UTCTIME_INVALID		0xFFFFFFFF

/// Time of Day (for #ZCL_TYPE_TIME_TOD Data Type)
typedef PACKED_STRUCT zcl_timeofday_t {
	uint8_t	hours;		///< 0-23 or 0xff for unused
	uint8_t	minutes;		///< 0-59 or 0xff for unused
	uint8_t	seconds;		///< 0-59 or 0xff for unused
	uint8_t	hundredths;	///< 0-99 or 0xff for unused
} zcl_timeofday_t;

/// Date (for #ZCL_TYPE_TIME_DATE Data Type)
typedef PACKED_STRUCT zcl_date_t {
	uint8_t	year;			///< year - 1900 or 0xff for unused
	uint8_t	month;		///< 1-12 or 0xff for unused
	uint8_t	day;			///< 1-31 or 0xff for unused
	uint8_t	dayofweek;	///< 1-7 (1 = Monday, 7 = Sunday) or 0xff for unused
} zcl_date_t;

/** @name ZCL Identifier (discrete) Types
	@{
*/
/// 16-bit cluster ID
#define ZCL_TYPE_ID_CLUSTER		0xE8			// 2 octets, invalid=0xffff

/// 16-bit attribute ID
#define ZCL_TYPE_ID_ATTRIB			0xE9			// 2 octets, invalid=0xffff

/// 32-bit BACnet OID
#define ZCL_TYPE_ID_BACNET_OID	0xEA			// 4 octets, invalid=0xffffffff
//@}

/** @name ZCL Miscellaneous Types
	@{
*/
/// 64-bit IEEE address
#define ZCL_TYPE_IEEE_ADDR			0xF0			// 8 octets, invalid=all ffs

/// 128-bit security key
#define ZCL_TYPE_SECURITY_KEY		0xF1			// 16 octets (128-bit security key)
//@}

/** @name ZCL Unknown Type
	@{
*/
/// Unknown ZCL Type
#define ZCL_TYPE_UNKNOWN			0xFF			// 0 octets
//@}

// DEVNOTE: May be necessary to add pragmas to pack these structures on
//				non-embedded platforms.

/// 40-bit unsigned in host-byte-order
typedef union zcl40_t {
	uint8_t		u8[5];
	PACKED_STRUCT {
		#if BYTE_ORDER == LITTLE_ENDIAN
			uint32_t	low32;
			uint8_t	high8;
		#else
			uint8_t	high8;
			uint32_t	low32;
		#endif
	} mixed;
} zcl40_t;

/// 48-bit unsigned in host-byte-order
typedef union zcl48_t {
	uint8_t		u8[6];
	uint16_t		u16[3];
	PACKED_STRUCT {
		#if BYTE_ORDER == LITTLE_ENDIAN
			uint32_t	low32;
			uint16_t	high16;
		#else
			uint16_t	high16;
			uint32_t	low32;
		#endif
	} mixed;
} zcl48_t;

// load separate header containing definition of zcl64_t and macros
// to work with 64-bit integers
#include "zigbee/zcl64.h"

/** @name Settings for zcl_type_info[]
	These bitfields should only be used internally by the library, and should
	be exposed to user code via public APIs.
	@{
*/
/// Not a valid ZCL type.
#define ZCL_T_INVALID		0x0F
/// Discrete values (bitmap, enum, etc.) that can't be added/subtracted or
/// (if reportable) have a reportable change.  Default is analog.
#define ZCL_T_DISCRETE		0x80
/// If an attribute isn't discrete, it's analog (signed, unsigned, float,
/// etc.)
#define ZCL_T_ANALOG			0x20
/// Type is signed (default is unsigned).  Only analog types can be signed.
#define ZCL_T_SIGNED			0x40
/// Type is floating point (single, double or semi-precision).
#define ZCL_T_FLOAT			0x10
/// Type is reportable.  Both the type AND attribute must have reportable flags
/// set if they are reportable.
#define ZCL_T_REPORTABLE	(ZCL_T_ANALOG | ZCL_T_DISCRETE)

/// 0x00 to 0x08 represent 0 to 8 bytes; 0x0C is 16 bytes; 0x0D is two-octet
/// size prefix, 0x0E is one-octet size prefix; 0x0F is invalid;
/// 0x09, 0x0A and 0x0B are still available to encode special size information.
/// Be sure to update zcl_sizeof_type when adding to or changing these macros.

/// Low nibble of each zcl_type_info byte stores the type's size.
#define ZCL_T_SIZE_MASK		0x0F
/// Macro used when type is invalid or doesn't have a known size.
#define ZCL_T_SIZE_INVALID	(ZCL_T_INVALID & ZCL_T_SIZE_MASK)
/// First octet of attribute's value is its size.
#define ZCL_T_SIZE_SHORT	0x0E
/// First two octets of attribute's value are its size.
#define ZCL_T_SIZE_LONG		0x0D
/// 128-bit (16-byte) value
#define ZCL_T_SIZE_128BIT	0x0C
/// variable-length value (array, set, bag, struct)
#define ZCL_T_SIZE_VARIABLE	0x0B
//@}

/// ZCL data type \a t is an analog type
#define ZCL_TYPE_IS_ANALOG(t) 		(zcl_type_info[t] & ZCL_T_ANALOG)
/// ZCL data type \a t is a discrete type
#define ZCL_TYPE_IS_DISCRETE(t) 		(zcl_type_info[t] & ZCL_T_DISCRETE)
/// ZCL data type \a t is a signed type
#define ZCL_TYPE_IS_SIGNED(t) 		(zcl_type_info[t] & ZCL_T_SIGNED)
/// ZCL data type \a t is a reportable type
#define ZCL_TYPE_IS_REPORTABLE(t)	(zcl_type_info[t] & ZCL_T_REPORTABLE)
/// ZCL data type \a t is an invalid (unsupported/unrecognized) type
#define ZCL_TYPE_IS_INVALID(t)		(zcl_type_info[t] == ZCL_T_INVALID)

// documented in zcl_types.c
extern const uint8_t zcl_type_info[256];
#define ZCL_SIZE_SHORT		-1
#define ZCL_SIZE_LONG		-2
#define ZCL_SIZE_VARIABLE	-3
#define ZCL_SIZE_INVALID	-127
int zcl_sizeof_type( uint8_t type);
const char *zcl_type_name( uint8_t type);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "zcl_types.c"
#endif

#endif		// __ZIGBEE_ZCL_TYPES_H
