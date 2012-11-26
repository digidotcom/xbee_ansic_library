/*
 * Copyright (c) 2011-2012 Digi International Inc.,
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
	@addtogroup zcl_64
	@{
	@file zigbee/zcl64.h
	Macros for working with 64-bit integers via the zcl64_t datatype.  Makes
	use of the JSInt64 type and support functions included in the platform
	support code (jslong.h, jslong.c).

	On some platforms (Win32), zcl64_t is simply a uint64_t.  Most embedded
	platforms represent a zcl64_t value as a structure.  Because of that
	difference, you can't just write "c = a + b" or
	"a.lo = ~a.lo, a.hi = ~a.hi" -- neither statement is portable to the
	other platform type.

	Therefore, it is necessary to use the macro functions in this file to
	manipulate zcl64_t variables.

	@todo add a ZCL64_SPLIT macro to split zcl64_t into high and low halves
*/

#ifndef ZIGBEE_ZCL64_H
#define ZIGBEE_ZCL64_H

#include "xbee/platform.h"

/// 64-bit integer in host-byte-order
/// Use for 56-bit values as well -- ZCL layer will make sure top byte is
/// sign-extended.
/// @todo Actually code up support for 56-bit values like we did for 24 bit.
typedef JSUint64 zcl64_t;

/**
	@brief Initialize a zcl64_t variable with two literal 32-bit values.

	Note that this macro is only valid as an initializer in a variable
	declaration.  Use ZCL64_LOAD in general program statements.

	@param[in]	hi		upper 32 bits
	@param[in]	lo		lower 32 bits

	@return	an initializer for a zcl64_t variable

	@sa ZCL64_LOAD
*/
#define ZCL64_INIT(hi, lo)				JSLL_INIT(hi, lo)

/** @brief Load a zcl64_t variable with two 32-bit values (high and low).

	@param[out]	r		zcl64_t variable to assign (hi << 32 + lo) to
	@param[in]	hi32	upper 32 bits to load into \param r
	@param[in]	lo32	lower 32 bits to load into \param r

	@sa ZCL64_INIT
*/
#ifdef XBEE_NATIVE_64BIT
	#define ZCL64_LOAD(r, hi32, lo32)	r = (((uint64_t)(hi32) << 32) + lo32)
#else
	#define ZCL64_LOAD(r, hi32, lo32)	((r).hi = (hi32), (r).lo = (lo32))
#endif

/** @brief Compare a zcl64_t variable to zero.

	@param[in]	a		zcl64_t variable

	@return		(a == 0)

	@sa ZCL64_EQ, ZCL64_NE, ZCL64_GE_ZERO, ZCL64_CMP, ZCL64_UCMP
*/
#define ZCL64_IS_ZERO(a)				JSLL_IS_ZERO(a)

/** @brief Compare two zcl64_t variables for equality.

	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@return (a == b)

	@sa ZCL64_IS_ZERO, ZCL64_NE, ZCL64_GE_ZERO, ZCL64_CMP, ZCL64_UCMP
*/
#define ZCL64_EQ(a, b)					JSLL_EQ(a, b)

/** @brief Compare two zcl64_t variables for inequality.

	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@return (a != b)

	@sa ZCL64_IS_ZERO, ZCL64_EQ, ZCL64_GE_ZERO, ZCL64_CMP, ZCL64_UCMP
*/
#define ZCL64_NE(a, b)					JSLL_NE(a, b)

/** @brief Compare a signed zcl64_t variable to 0.

	@param[in]	a		signed zcl64_t variable

	@return (a >= 0)

	@sa ZCL64_IS_ZERO, ZCL64_EQ, ZCL64_NE, ZCL64_CMP, ZCL64_UCMP
*/
#define ZCL64_GE_ZERO(a)				JSLL_GE_ZERO(a)

/** @brief Compare two zcl64_t variables (signed less-than comparison)
	@param[in]	a		signed zcl64_t variable
	@param[in]	b		signed zcl64_t variable

	@return (a < b)

	@sa ZCL64_IS_ZERO, ZCL64_EQ, ZCL64_NE, ZCL64_GE_ZERO, ZCL64_LT
*/
#define ZCL64_LT(a, b)					JSLL_REAL_CMP(a, <, b)

/** @brief Compare two zcl64_t variables (unsigned less-than comparison)
	@param[in]	a		unsigned zcl64_t variable
	@param[in]	b		unsigned zcl64_t variable

	@return (a < b)

	@sa ZCL64_IS_ZERO, ZCL64_EQ, ZCL64_NE, ZCL64_GE_ZERO, ZCL64_LTU
*/
#define ZCL64_LTU(a, b)					JSLL_REAL_UCMP(a, <, b)

/** @brief Perform a bitwise AND of two zcl64_t variables.
	@param[out]	r		zcl64_t variable to assign (a & b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_OR, ZCL64_XOR, ZCL64_NOT, ZCL64_NEG
*/
#define ZCL64_AND(r, a, b)				JSLL_AND(r, a, b)

/** @brief Perform a bitwise OR of two zcl64_t variables.
	@param[out]	r		zcl64_t variable to assign (a | b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_AND, ZCL64_XOR, ZCL64_NOT, ZCL64_NEG
*/
#define ZCL64_OR(r, a, b)				JSLL_OR(r, a, b)

/** @brief Perform a bitwise XOR of two zcl64_t variables.
	@param[out]	r		zcl64_t variable to assign (a ^ b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_AND, ZCL64_OR, ZCL64_NOT, ZCL64_NEG
*/
#define ZCL64_XOR(r, a, b)				JSLL_XOR(r, a, b)

/** @brief Perform a bitwise NOT of a zcl64_t variable.
	@param[out]	r		zcl64_t variable to assign (~a) to
	@param[in]	a		zcl64_t variable

	@sa	ZCL64_AND, ZCL64_OR, ZCL64_XOR, ZCL64_NEG
*/
#define ZCL64_NOT(r, a)					JSLL_NOT(r, a)

/** @brief Negate a zcl64_t variable.
	@param[out]	r		zcl64_t variable to assign (-a) to
	@param[in]	a		zcl64_t variable

	@sa	ZCL64_AND, ZCL64_OR, ZCL64_XOR, ZCL64_NOT
*/
#define ZCL64_NEG(r, a)					JSLL_NEG(r, a)

/** @brief Add two zcl64_t variables.
	@param[out]	r		zcl64_t variable to assign (a + b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_SUB, ZCL64_MUL, ZCL64_MUL32, ZCL64_UDIVMOD, ZCL64_DIV, ZCL64_MOD
*/
#define ZCL64_ADD(r, a, b)				JSLL_ADD(r, a, b)

/** @brief Subtract two zcl64_t variables.
	@param[out]	r		zcl64_t variable to assign (a - b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_ADD, ZCL64_MUL, ZCL64_MUL32, ZCL64_UDIVMOD, ZCL64_DIV, ZCL64_MOD
*/
#define ZCL64_SUB(r, a, b)				JSLL_SUB(r, a, b)

/** @brief Multiply two zcl64_t variables.
	@param[out]	r		zcl64_t variable to assign (a * b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_ADD, ZCL64_SUB, ZCL64_MUL32, ZCL64_UDIVMOD, ZCL64_DIV, ZCL64_MOD
*/
#define ZCL64_MUL(r, a, b)				JSLL_MUL(r, a, b)

/** @brief
		Multiply two 32-bit variables (int32_t or uint32_t) and store the result
		in a zcl64_t variable.
	@param[out]	r		zcl64_t variable to assign (a * b) to
	@param[in]	a		uint32_t variable
	@param[in]	b		uint32_t variable

	@sa	ZCL64_ADD, ZCL64_SUB, ZCL64_MUL, ZCL64_UDIVMOD, ZCL64_DIV, ZCL64_MOD
*/
#define ZCL64_MUL32(r, a, b)			JSLL_MUL32(r, a, b)

/** @brief
		Divide an unsigned zcl64_t variable by another unsigned zcl64_t variable
		and store the 64-bit quotient and remainder.
	@param[out]	qp		NULL to ignore the quotient, or address of a zcl64_t
							variable to assign (a / b) to
	@param[out]	rp		NULL to ignore the remainder, or address of a zcl64_t
							variable to assign (a % b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_ADD, ZCL64_SUB, ZCL64_MUL, ZCL64_MUL32, ZCL64_DIV, ZCL64_MOD
*/
#define ZCL64_UDIVMOD(qp, rp, a, b)	JSLL_UDIVMOD(qp, rp, a, b)

// Is there a need for a signed divmod, based on JSLL_DIV and JSLL_MOD?
// Would need to store two negate flags, one for quotient and one for
// remainder.  Remainder only negative if a is negative, quotient is negative
// if sign of a and b differ

/**	@brief
		Perform signed division of two zcl64_t variables and store the quotient.
	@param[out]	r		signed zcl64_t variable to assign (a / b) to
	@param[in]	a		signed zcl64_t variable
	@param[in]	b		signed zcl64_t variable

	@sa	ZCL64_ADD, ZCL64_SUB, ZCL64_MUL, ZCL64_MUL32, ZCL64_UDIVMOD, ZCL64_MOD
*/
#define ZCL64_DIV(r, a, b)				JSLL_DIV(r, a, b)

/**	@brief
		Perform signed division of two zcl64_t variables and store the remainder.
	@param[out]	r		zcl64_t variable to assign (a + b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		zcl64_t variable

	@sa	ZCL64_ADD, ZCL64_SUB, ZCL64_MUL, ZCL64_MUL32, ZCL64_UDIVMOD, ZCL64_DIV
*/
#define ZCL64_MOD(r, a, b)				JSLL_MOD(r, a, b)

/**	@brief
		Arithmetic Shift Left of a zcl64_t variable.  Shifts bits of \c a left
		by \b positions, inserting zeros on the right.  Equivalent to ZCL64_LSL.
	@param[out]	r		zcl64_t variable to assign (a << b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		integral value from 0 to 63

	@sa	ZCL64_LSL, ZCL64_ASR, ZCL64_LSR
*/
#define ZCL64_ASL(r, a, b)					JSLL_SHL(r, a, b)

/**	@brief
		Logical Shift Left of a zcl64_t variable.  Shifts bits of \c a left
		by \b positions, inserting zeros on the right.  Equivalent to ZCL64_ASL.
	@param[out]	r		zcl64_t variable to assign (a << b) to
	@param[in]	a		zcl64_t variable
	@param[in]	b		integral value from 0 to 63

	@sa	ZCL64_ASL, ZCL64_ASR, ZCL64_LSR
*/
#define ZCL64_LSL(r, a, b)					ZCL64_SLA(r, a, b)

/**	@brief
		Arithmetic Shift Right of a signed zcl64_t variable.  Shifts bits of
		\c a right by \b positions, extending the sign bit on the left.
	@param[out]	r		signed zcl64_t variable to assign (a >> b) to
	@param[in]	a		signed zcl64_t variable
	@param[in]	b		integral value from 0 to 63

	@sa	ZCL64_ASL, ZCL64_LSL, ZCL64_LSR
*/
#define ZCL64_ASR(r, a, b)					JSLL_SHR(r, a, b)

/**	@brief
		Logical Shift Right of an unsigned zcl64_t variable.  Shifts bits of
		\c a right by \b positions, inserting zeros on the left.
	@param[out]	r		unsigned zcl64_t variable to assign (a >> b) to
	@param[in]	a		unsigned zcl64_t variable
	@param[in]	b		integral value from 0 to 63

	@sa	ZCL64_ASL, ZCL64_LSL, ZCL64_ASR
*/
#define ZCL64_LSR(r, a, b)					JSLL_USHR(r, a, b)

/* a is an JSInt32, b is JSInt32, r is JSInt64 */
//#define JSLL_ISHL(r, a, b)

/** @brief Cast a signed zcl64_t variable down to a signed 32-bit integer.
	@param[out]	i32	int32_t variable to cast \c i64 into
	@param[in]	i64	signed zcl64_t variable

	@sa	ZCL64_TO_UINT32, ZCL64_TO_FLOAT, ZCL64_TO_DOUBLE, ZCL64_FROM_INT32,
			ZCL64_FROM_UINT32, ZCL64_FROM_FLOAT, ZCL64_FROM_DOUBLE
*/
#define ZCL64_TO_INT32(i32, i64)			JSLL_L2I(i32, i64)

/** @brief Cast an unsigned zcl64_t variable down to an unsigned 32-bit integer.
	@param[out]	u32	uint32_t variable to cast \c u64 into
	@param[in]	u64	unsigned zcl64_t variable

	@sa	ZCL64_TO_INT32, ZCL64_TO_FLOAT, ZCL64_TO_DOUBLE, ZCL64_FROM_INT32,
			ZCL64_FROM_UINT32, ZCL64_FROM_FLOAT, ZCL64_FROM_DOUBLE
*/
#define ZCL64_TO_UINT32(u32, u64)		JSLL_L2UI(u32, u64)

/** @brief The lower-32 bits of a ZCL64 value.

	@param[in]	u64		unsigned zcl64_t variable
*/
#ifdef XBEE_NATIVE_64BIT
	#define ZCL64_LOW32(u64)				((uint32_t)(u64))
#else
	#define ZCL64_LOW32(u64)				((u64).lo)
#endif

/** @brief The upper-32 bits of a ZCL64 value.

	@param[in]	u64		unsigned zcl64_t variable
*/
#ifdef XBEE_NATIVE_64BIT
	#define ZCL64_HIGH32(u64)				((uint32_t)(u64 >> 32))
#else
	#define ZCL64_HIGH32(u64)				((u64).hi)
#endif

/** @brief Cast a signed zcl64_t variable to a \c float.
	@param[out]	f		float variable to cast \c i64 into
	@param[in]	i64	signed zcl64_t variable

	@sa	ZCL64_TO_INT32, ZCL64_TO_UINT32, ZCL64_TO_DOUBLE, ZCL64_FROM_INT32,
			ZCL64_FROM_UINT32, ZCL64_FROM_FLOAT, ZCL64_FROM_DOUBLE,
*/
#define ZCL64_TO_FLOAT(f, i64)			JSLL_L2F(f, i64)

/** @brief Cast a signed zcl64_t variable to a \c double.
	@param[out]	d		double variable to cast \c i64 into
	@param[in]	i64	signed zcl64_t variable

	@sa	ZCL64_TO_INT32, ZCL64_TO_UINT32, ZCL64_TO_FLOAT, ZCL64_FROM_INT32,
			ZCL64_FROM_UINT32, ZCL64_FROM_FLOAT, ZCL64_FROM_DOUBLE
*/
#define ZCL64_TO_DOUBLE(d, i64)			JSLL_L2D(d, i64)


/** @brief Cast a signed 32-bit integer up to a zcl64_t variable.
	@param[out]	i64	signed zcl64_t variable to cast \c i32 into
	@param[in]	i32	int32_t variable

	@sa	ZCL64_TO_INT32, ZCL64_TO_UINT32, ZCL64_TO_FLOAT, ZCL64_TO_DOUBLE,
			ZCL64_FROM_UINT32, ZCL64_FROM_FLOAT, ZCL64_FROM_DOUBLE
*/
#define ZCL64_FROM_INT32(i64, i32)		JSLL_I2L(i64, i32)

/** @brief Cast an unsigned 32-bit integer up to a zcl64_t variable.
	@param[out]	u64	unsigned zcl64_t variable to cast \c u32 into
	@param[in]	u32	uint32_t variable

	@sa	ZCL64_TO_INT32, ZCL64_TO_UINT32, ZCL64_TO_FLOAT, ZCL64_TO_DOUBLE,
			ZCL64_FROM_INT32, ZCL64_FROM_FLOAT, ZCL64_FROM_DOUBLE
*/
#define ZCL64_FROM_UINT32(u64, u32)		JSLL_UI2L(u64, u32)

/** @brief Cast a \c double into a signed zcl64_t variable.
	@param[out]	i64	signed zcl64_t variable to cast \c f into
	@param[in]	f		float variable/value

	@sa	ZCL64_TO_INT32, ZCL64_TO_UINT32, ZCL64_TO_FLOAT, ZCL64_TO_DOUBLE,
			ZCL64_FROM_INT32, ZCL64_FROM_UINT32, ZCL64_FROM_DOUBLE
*/
#define ZCL64_FROM_FLOAT(i64, f)			JSLL_F2L(i64, f)

/** @brief Cast a \c float into a signed zcl64_t variable.
	@param[out]	i64	signed zcl64_t variable to cast \c d into
	@param[in]	d		double variable/value

	@sa	ZCL64_TO_INT32, ZCL64_TO_UINT32, ZCL64_TO_FLOAT, ZCL64_TO_DOUBLE,
			ZCL64_FROM_INT32, ZCL64_FROM_UINT32, ZCL64_FROM_FLOAT
*/
#define ZCL64_FROM_DOUBLE(i64, d)		JSLL_D2L(i64, d)


/**	@brief
		Convert a zcl64_t variable to a 16-character printable hexadecimal
		string.
	@param[out]	buffer	17-character buffer to hold hexadecimal string
	@param[in]	var		zcl64_t variable to stringify

	@retval	16		this function always returns 16, the number of
						characters written to \c buffer (in addition to
						the null terminator)

	@sa ZCL64_TO_DECSTR, ZCL64_TO_UDECSTR
*/
#define ZCL64_TO_HEXSTR(buffer, var)	JSLL_HEXSTR(buffer, var)

/**	@brief
		Convert a signed zcl64_t variable to a 20-character printable decimal
		string.
	@param[out]	buffer	21-character buffer to hold hexadecimal string
	@param[in]	var		signed zcl64_t variable to stringify

	@return	number of characters written to \c buffer (1 to 20), in addition
				to the null terminator

	@sa ZCL64_TO_HEXSTR, ZCL64_TO_UDECSTR
*/
#define ZCL64_TO_DECSTR(buffer, var)	JSLL_DECSTR(buffer, var)

/**	@brief
		Convert an unsigned zcl64_t variable to a 20-character printable decimal
		string.
	@param[out]	buffer	21-character buffer to hold hexadecimal string
	@param[in]	var		unsigned zcl64_t variable to stringify

	@return	number of characters written to \c buffer (1 to 20), in addition
				to the null terminator

	@sa ZCL64_TO_HEXSTR, ZCL64_TO_DECSTR
*/
#define ZCL64_TO_UDECSTR(buffer, var)	JSLL_UDECSTR(buffer, var)

#endif
