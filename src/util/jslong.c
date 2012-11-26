/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This is a modified version of jslong.c Rev 3.11 from mozilla.org, with as
   few changes as possible.  The original file is available here:
      http://mxr.mozilla.org/mozilla/source/js/src/jslong.c

   Do not link it with your project, it is included automatically by the
	platform_xxx.c file on platforms that don't have native 64-bit integers.
*/
/**
	@addtogroup zcl_64
	@{
	@file util/jslong.c
*/
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*** BeginHeader */
#include <stdio.h>
#include <string.h>
#include "xbee/platform.h"

#ifndef JS_HAVE_LONG_LONG

#define jslo16(a)        ((a) & 0x0000FFFF)
#define jshi16(a)        ((uint32_t)(a) >> 16)
/*** EndHeader */

/*** BeginHeader jsll_mul */
/*** EndHeader */
void jsll_mul(JSUint64 *rp, JSUint64 a, JSUint64 b)
{
    jsll_mul32(rp, a.lo, b.lo);
    rp->hi += a.hi * b.lo + a.lo * b.hi;
}

/*** BeginHeader jsll_mul32 */
/*** EndHeader */
void jsll_mul32(JSUint64 *rp, JSUint32 a, JSUint32 b)
{
    JSUint32 _a1, _a0, _b1, _b0, _y0, _y1, _y2, _y3;

    _a1 = jshi16(a), _a0 = jslo16(a);
    _b1 = jshi16(b), _b0 = jslo16(b);
    _y0 = _a0 * _b0;
    _y1 = _a0 * _b1;
    _y2 = _a1 * _b0;
    _y3 = _a1 * _b1;
    _y1 += jshi16(_y0);                         /* can't carry */
    _y1 += _y2;                                /* might carry */
    if (_y1 < _y2)
	 {
        _y3 += (JSUint32)(0x00010000);  /* propagate */
    }
    rp->lo = (jslo16(_y1) << 16) + jslo16(_y0);
    rp->hi = _y3 + jshi16(_y1);
}

/*** BeginHeader jsll_udivmod */
/*** EndHeader */
/*
** Divide 64-bit a by 32-bit b, which must be normalized so its high bit is 1.
*/
static void norm_udivmod32(JSUint32 *qp, JSUint32 *rp, JSUint64 a, JSUint32 b)
{
    JSUint32 d1, d0, q1, q0;
    JSUint32 r1, r0, m;

    d1 = jshi16(b);
    d0 = jslo16(b);
    r1 = a.hi % d1;
    q1 = a.hi / d1;
    m = q1 * d0;
    r1 = (r1 << 16) | jshi16(a.lo);
    if (r1 < m) {
        q1--, r1 += b;
        if (r1 >= b     /* i.e., we didn't get a carry when adding to r1 */
            && r1 < m) {
            q1--, r1 += b;
        }
    }
    r1 -= m;
    r0 = r1 % d1;
    q0 = r1 / d1;
    m = q0 * d0;
    r0 = (r0 << 16) | jslo16(a.lo);
    if (r0 < m) {
        q0--, r0 += b;
        if (r0 >= b
            && r0 < m) {
            q0--, r0 += b;
        }
    }
    *qp = (q1 << 16) | q0;
    *rp = r0 - m;
}

#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
static uint_fast8_t CountLeadingZeros(JSUint32 a)
{
    JSUint32 t;
    uint_fast8_t r = 32;

    if ((t = a >> 16) != 0)
        r -= 16, a = t;
    if ((t = a >> 8) != 0)
        r -= 8, a = t;
    if ((t = a >> 4) != 0)
        r -= 4, a = t;
    if ((t = a >> 2) != 0)
        r -= 2, a = t;
    if ((t = a >> 1) != 0)
        r -= 1, a = t;
    if (a & 1)
        r--;
    return r;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

void jsll_udivmod(JSUint64 *qp, JSUint64 *rp, JSUint64 a, JSUint64 b)
{
    JSUint32 n0, n1, n2;
    JSUint32 q0, q1;
    uint_fast8_t rsh, lsh;

    n0 = a.lo;
    n1 = a.hi;

    if (b.hi == 0) {
        if (b.lo > n1) {
            /* (0 q0) = (n1 n0) / (0 D0) */

            lsh = CountLeadingZeros(b.lo);

            if (lsh) {
                /*
                 * Normalize, i.e. make the most significant bit of the
                 * denominator be set.
                 */
                b.lo = b.lo << lsh;
                n1 = (n1 << lsh) | (n0 >> (uint_fast8_t)(32 - lsh));
                n0 = n0 << lsh;
            }

            a.lo = n0, a.hi = n1;
            norm_udivmod32(&q0, &n0, a, b.lo);
            q1 = 0;

            /* remainder is in n0 >> lsh */
        } else {
            /* (q1 q0) = (n1 n0) / (0 d0) */

            if (b.lo == 0)              /* user wants to divide by zero! */
                b.lo = 1 / b.lo;        /* so go ahead and crash */

            lsh = CountLeadingZeros(b.lo);

            if (lsh == 0) {
                /*
                 * From (n1 >= b.lo)
                 *   && (the most significant bit of b.lo is set),
                 * conclude that
                 *      (the most significant bit of n1 is set)
                 *   && (the leading quotient digit q1 = 1).
                 *
                 * This special case is necessary, not an optimization
                 * (Shifts counts of 32 are undefined).
                 */
                n1 -= b.lo;
                q1 = 1;
            } else {
                /*
                 * Normalize.
                 */
                rsh = 32 - lsh;

                b.lo = b.lo << lsh;
                n2 = n1 >> rsh;
                n1 = (n1 << lsh) | (n0 >> rsh);
                n0 = n0 << lsh;

                a.lo = n1, a.hi = n2;
                norm_udivmod32(&q1, &n1, a, b.lo);
            }

            /* n1 != b.lo... */

            a.lo = n0, a.hi = n1;
            norm_udivmod32(&q0, &n0, a, b.lo);

            /* remainder in n0 >> lsh */
        }

        if (rp) {
            rp->lo = n0 >> lsh;
            rp->hi = 0;
        }
    } else {
        if (b.hi > n1) {
            /* (0 0) = (n1 n0) / (D1 d0) */

            q0 = 0;
            q1 = 0;

            /* remainder in (n1 n0) */
            if (rp) {
                rp->lo = n0;
                rp->hi = n1;
            }
        } else {
            /* (0 q0) = (n1 n0) / (d1 d0) */

            lsh = CountLeadingZeros(b.hi);
            if (lsh == 0) {
                /*
                 * From (n1 >= b.hi)
                 *   && (the most significant bit of b.hi is set),
                 * conclude that
                 *      (the most significant bit of n1 is set)
                 *   && (the quotient digit q0 = 0 or 1).
                 *
                 * This special case is necessary, not an optimization.
                 */

                /*
                 * The condition on the next line takes advantage of that
                 * n1 >= b.hi (true due to control flow).
                 */
                if (n1 > b.hi || n0 >= b.lo) {
                    q0 = 1;
                    a.lo = n0, a.hi = n1;
                    JSLL_SUB(a, a, b);
                } else {
                    q0 = 0;
                }
                q1 = 0;

                if (rp) {
                    rp->lo = n0;
                    rp->hi = n1;
                }
            } else {
                JSInt64 m;

                /*
                 * Normalize.
                 */
                rsh = 32 - lsh;

                b.hi = (b.hi << lsh) | (b.lo >> rsh);
                b.lo = b.lo << lsh;
                n2 = n1 >> rsh;
                n1 = (n1 << lsh) | (n0 >> rsh);
                n0 = n0 << lsh;

                a.lo = n1, a.hi = n2;
                norm_udivmod32(&q0, &n1, a, b.hi);
                jsll_mul32(&m, q0, b.lo);

                if ((m.hi > n1) || ((m.hi == n1) && (m.lo > n0))) {
                    q0--;
                    JSLL_SUB(m, m, b);
                }

                q1 = 0;

                /* Remainder is ((n1 n0) - (m1 m0)) >> lsh */
                if (rp) {
                    a.lo = n0, a.hi = n1;
                    JSLL_SUB(a, a, m);
                    rp->lo = (a.hi << rsh) | (a.lo >> lsh);
                    rp->hi = a.hi >> lsh;
                }
            }
        }
    }

    if (qp) {
        qp->lo = q0;
        qp->hi = q1;
    }
}

/*** BeginHeader jsll_div */
/*** EndHeader */
void jsll_div( JSUint64 *r, const JSUint64 *a, const JSUint64 *b)
{
    // since we may need to copy a and b (to negate them), pass them
    // as pointers
    JSInt64 _a, _b;
    bool_t _negative = (JSInt32)a->hi < 0;

    if (_negative)
    {
        JSLL_NEG(_a, *a);
    } else {
        _a = *a;
    }
    if ((JSInt32)b->hi < 0)
    {
        _negative ^= 1;
        JSLL_NEG(_b, *b);
    } else {
        _b = *b;
    }
    jsll_udivmod(r, NULL, _a, _b);
    if (_negative)
    {
        JSLL_NEG(*r, *r);
    }
}

/*** BeginHeader jsll_mod */
/*** EndHeader */
void jsll_mod( JSUint64 *r, const JSUint64 *a, const JSUint64 *b)
{
    // since we may need to copy a and b (to negate them), pass them
    // as pointers
    JSInt64 _a, _b;
    bool_t _negative = (JSInt32)a->hi < 0;

    if (_negative) {
        JSLL_NEG(_a, *a);
    } else {
        _a = *a;
    }
    if ((JSInt32)b->hi < 0) {
        JSLL_NEG(_b, *b);
    } else {
        _b = *b;
    }
    jsll_udivmod(NULL, r, _a, _b);
    if (_negative)
    {
        JSLL_NEG(*r, *r);
    }
}

/*** BeginHeader jsll_shl */
/*** EndHeader */
void jsll_shl( JSUint64 *r, JSUint64 a, uint_fast8_t b)
{
    // pass a by value so we don't have to make a copy of it
    uint_fast8_t lowb;

    if (b) {
        lowb = b & 31;
        if (b < 32) {
            r->lo = a.lo << lowb;
            r->hi = (a.hi << lowb) | (a.lo >> (uint8_t)(32 - b));
        } else {
            r->lo = 0;
            r->hi = a.lo << lowb;
        }
    } else {
        *r = a;
    }
}

/*** BeginHeader jsll_shr */
/*** EndHeader */
void jsll_shr( JSInt64 *r, JSInt64 a, uint_fast8_t b)
{
    // pass a by value so we don't have to make a copy of it
    uint_fast8_t lowb;

    if (b) {
        lowb = b & 31;
        if (b < 32) {
            r->lo = (a.hi << (uint8_t)(32 - b)) | (a.lo >> lowb);
            r->hi = (JSInt32)a.hi >> lowb;
        } else {
            r->lo = (JSInt32)a.hi >> lowb;
            r->hi = (JSInt32)a.hi >> 31;
        }
    } else {
        *r = a;
    }
}

/*** BeginHeader jsll_ushr */
/*** EndHeader */
void jsll_ushr( JSInt64 *r, JSInt64 a, uint_fast8_t b)
{
    // pass a by value so we don't have to make a copy of it
    uint_fast8_t lowb;

    if (b) {
        lowb = b & 31;
        if (b < 32) {
            r->lo = (a.hi << (uint8_t)(32 - b)) | (a.lo >> lowb);
            r->hi = a.hi >> lowb;
        } else {
            r->lo = a.hi >> lowb;
            r->hi = 0;
        }
    } else {
        *r = a;
    }
}

/*** BeginHeader jsll_decstr */
/*** EndHeader */
int jsll_decstr( char *buffer, const JSInt64 *v)
{
	if ((JSInt32)v->hi < 0)
	{
		JSUint64 _abs;

		JSLL_NEG( _abs, *v);
		*buffer = '-';
		return jsll_udecstr( buffer + 1, &_abs);
	}

	return jsll_udecstr( buffer, v);
}

/*** BeginHeader jsll_udecstr */
/*** EndHeader */
// Technique is to convert 4 characters at a time, since 16-bit divide is less
// expensive on embedded platforms.  Limits number of 64-bit divides to 5.
// On platforms with efficient 32-bit divide, this method could be modified
// to divide by 1 billion and convert 9 digits at a time.
int jsll_udecstr( char *buffer, const JSUint64 *v)
{
	char temp[20];		// maximum of 20 digits, don't need pointer for null
	char *p;
	int length;
	uint_fast8_t i;
	bool_t fill;
	JSUint64 quot, rem;
	uint16_t rem16;
	const JSUint64 divisor = JSLL_INIT( 0, 10000);

	quot = *v;
	if (JSLL_IS_ZERO( quot))
	{
		strcpy( buffer, "0");
		return 1;
	}

	p = temp;
	do
	{
		jsll_udivmod( &quot, &rem, quot, divisor);
		rem16 = (uint16_t) rem.lo;
		fill = ! JSLL_IS_ZERO( quot);
		for (i = 4; i && (fill || rem16); ++p, --i)
		{
			*p = '0' + (rem16 % 10);
			rem16 /= 10;
		}
	} while (fill);

	length = p - temp;
	// copy <length> characters from <temp> to <buffer>, in reverse order
	for (i = (uint_fast8_t) length; i; ++buffer, --i)
	{
		*buffer = *--p;
	}
	*buffer = '\0';		// add null terminator

	return length;
}

/*** BeginHeader */
#endif /* !JS_HAVE_LONG_LONG */
/*** EndHeader */

