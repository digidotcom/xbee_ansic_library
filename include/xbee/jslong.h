/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This is a modified version of jslong.h Rev 3.13 from mozilla.org, with as
   few changes as possible.  The original file is available here:
      http://mxr.mozilla.org/mozilla/source/js/src/jslong.h

   Do not include it directly, as it requires glue macros and typedefs defined
	in xbee/platform.h (which includes it automatically).
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

/*
** File:                jslong.h
** Description: Portable access to 64 bit numerics
**
** Long-long (64-bit signed integer type) support. Some C compilers
** don't support 64 bit integers yet, so we use these macros to
** support both machines that do and don't.
**/
#ifndef jslong_h___
#define jslong_h___

#ifdef JS_HAVE_LONG_LONG

#define JSLL_INIT(hi, lo)  ((UINT64_C(hi) << 32) + UINT64_C(lo))

/***********************************************************************
** MACROS:      JSLL_*
** DESCRIPTION:
**      The following macros define portable access to the 64 bit
**      math facilities.
**
***********************************************************************/

/***********************************************************************
** MACROS:      JSLL_<relational operators>
**
**  JSLL_IS_ZERO        Test for zero
**  JSLL_EQ             Test for equality
**  JSLL_NE             Test for inequality
**  JSLL_GE_ZERO        Test for zero or positive
**  JSLL_CMP            Compare two values
***********************************************************************/
#define JSLL_IS_ZERO(a)       ((a) == 0)
#define JSLL_EQ(a, b)         ((a) == (b))
#define JSLL_NE(a, b)         ((a) != (b))
#define JSLL_GE_ZERO(a)       ((a) >= 0)
#define JSLL_CMP(a, op, b)    ((JSInt64)(a) op (JSInt64)(b))
#define JSLL_UCMP(a, op, b)   ((JSUint64)(a) op (JSUint64)(b))

/***********************************************************************
** MACROS:      JSLL_<logical operators>
**
**  JSLL_AND            Logical and
**  JSLL_OR             Logical or
**  JSLL_XOR            Logical exclusion
**  JSLL_OR2            A disgusting deviation
**  JSLL_NOT            Negation (one's compliment)
***********************************************************************/
#define JSLL_AND(r, a, b)       ((r) = (a) & (b))
#define JSLL_OR(r, a, b)        ((r) = (a) | (b))
#define JSLL_XOR(r, a, b)       ((r) = (a) ^ (b))
#define JSLL_OR2(r, a)          ((r) = (r) | (a))
#define JSLL_NOT(r, a)          ((r) = ~(a))

/***********************************************************************
** MACROS:      JSLL_<mathematical operators>
**
**  JSLL_NEG            Negation (two's compliment)
**  JSLL_ADD            Summation (two's compliment)
**  JSLL_SUB            Difference (two's compliment)
***********************************************************************/
#define JSLL_NEG(r, a)        ((r) = -(a))
#define JSLL_ADD(r, a, b)     ((r) = (a) + (b))
#define JSLL_SUB(r, a, b)     ((r) = (a) - (b))

/***********************************************************************
** MACROS:      JSLL_<mathematical operators>
**
**  JSLL_MUL            Product (two's compliment)
**  JSLL_MUL32          64-bit product of two unsigned 32-bit integers
**  JSLL_DIV            Quotient (two's compliment)
**  JSLL_MOD            Modulus (two's compliment)
***********************************************************************/
#define JSLL_MUL(r, a, b)        ((r) = (a) * (b))
#define JSLL_MUL32(r, a, b)		((r) = (JSUint64)(a) * (JSUint32)(b))
#define JSLL_DIV(r, a, b)        ((r) = (a) / (b))
#define JSLL_MOD(r, a, b)        ((r) = (a) % (b))

/***********************************************************************
** MACROS:      JSLL_<shifting operators>
**
**  JSLL_SHL            Shift left [0..64] bits
**  JSLL_SHR            Shift right [0..64] bits with sign extension
**  JSLL_USHR           Unsigned shift right [0..64] bits
**  (consider renaming to LSR and ASR for logical and arithmetic shift right)
**  JSLL_ISHL           Integer (32-bit) shift left [0..64] bits
***********************************************************************/
#define JSLL_SHL(r, a, b)     ((r) = (JSInt64)(a) << (b))
#define JSLL_SHR(r, a, b)     ((r) = (JSInt64)(a) >> (b))
#define JSLL_USHR(r, a, b)    ((r) = (JSUint64)(a) >> (b))
#define JSLL_ISHL(r, a, b)    ((r) = (JSInt64)(a) << (b))

/***********************************************************************
** MACROS:      JSLL_<conversion operators>
**
**  JSLL_L2I            Convert 64-bit to signed 32-bit
**  JSLL_L2UI           Convert 64-bit to unsigned 32-bit
**  JSLL_L2F            Convert 64-bit to float
**  JSLL_L2D            Convert 64-bit to double
**  JSLL_I2L            Convert signed 32-bit to 64-bit
**  JSLL_UI2L           Convert unsigned 32-bit to 64-bit
**  JSLL_F2L            Convert float to 64-bit
**  JSLL_D2L            Convert double to 64-bit
***********************************************************************/
#define JSLL_L2I(i, l)        ((i) = (JSInt32)(l))
#define JSLL_L2UI(ui, l)        ((ui) = (JSUint32)(l))
#define JSLL_L2F(f, l)        ((f) = (JSFloat64)(l))
#define JSLL_L2D(d, l)        ((d) = (JSFloat64)(l))

#define JSLL_I2L(l, i)        ((l) = (JSInt64)(i))
#define JSLL_UI2L(l, ui)        ((l) = (JSInt64)(ui))
#define JSLL_F2L(l, f)        ((l) = (JSInt64)(f))
#define JSLL_D2L(l, d)        ((l) = (JSInt64)(d))

/***********************************************************************
** MACROS:      JSLL_UDIVMOD
** DESCRIPTION:
**  Produce both a quotient and a remainder given an unsigned
** INPUTS:      JSUint64 a: The dividend of the operation
**              JSUint64 b: The quotient of the operation
** OUTPUTS:     JSUint64 *qp: pointer to quotient
**              JSUint64 *rp: pointer to remainder
***********************************************************************/
#define JSLL_UDIVMOD(qp, rp, a, b) \
    (*(qp) = ((JSUint64)(a) / (b)), \
     *(rp) = ((JSUint64)(a) % (b)))

/***********************************************************************
** MACROS:      JSLL_HEXSTR, JSLL_DECSTR
** DESCRIPTION:
**  Convert 64-bit value to hexadecimal or decimal string.
**  JSLL_HEXSTR         Convert to (lowercase) hexadecimal string, padded
**                      with zeros to 16 characters
**  JSLL_DECSTR         Convert signed value to decimal string
**  JSLL_UDECSTR        Convert unsigned value to decimal string
**
**  Returns number of characters written to buffer.
***********************************************************************/
#define JSLL_HEXSTR(s, a)        sprintf( s, "%016" PRIx64, a)
#define JSLL_DECSTR(s, a)        sprintf( s, "%" PRId64, a)
#define JSLL_UDECSTR(s, a)       sprintf( s, "%" PRIu64, a)

#else  /* !JS_HAVE_LONG_LONG */

#ifdef IS_LITTLE_ENDIAN
#define JSLL_INIT(hi, lo) { lo, hi }
#else
#define JSLL_INIT(hi, lo) { hi, lo }
#endif

#define JSLL_IS_ZERO(a)         (((a).hi == 0) && ((a).lo == 0))
#define JSLL_EQ(a, b)           (((a).hi == (b).hi) && ((a).lo == (b).lo))
#define JSLL_NE(a, b)           (((a).hi != (b).hi) || ((a).lo != (b).lo))
#define JSLL_GE_ZERO(a)         (((a).hi >> 31) == 0)

#ifdef DEBUG
#define JSLL_CMP(a, op, b)      (JS_ASSERT((#op)[1] != '='), JSLL_REAL_CMP(a, op, b))
#define JSLL_UCMP(a, op, b)     (JS_ASSERT((#op)[1] != '='), JSLL_REAL_UCMP(a, op, b))
#else
#define JSLL_CMP(a, op, b)      JSLL_REAL_CMP(a, op, b)
#define JSLL_UCMP(a, op, b)     JSLL_REAL_UCMP(a, op, b)
#endif

#define JSLL_REAL_CMP(a,op,b)   (((JSInt32)(a).hi op (JSInt32)(b).hi) || \
                                 (((a).hi == (b).hi) && ((a).lo op (b).lo)))
#define JSLL_REAL_UCMP(a,op,b)  (((a).hi op (b).hi) || \
                                 (((a).hi == (b).hi) && ((a).lo op (b).lo)))

#define JSLL_AND(r, a, b)       ((r).lo = (a).lo & (b).lo, \
                                 (r).hi = (a).hi & (b).hi)
#define JSLL_OR(r, a, b)        ((r).lo = (a).lo | (b).lo, \
                                 (r).hi = (a).hi | (b).hi)
#define JSLL_XOR(r, a, b)       ((r).lo = (a).lo ^ (b).lo, \
                                 (r).hi = (a).hi ^ (b).hi)
#define JSLL_OR2(r, a)          ((r).lo = (r).lo | (a).lo, \
                                 (r).hi = (r).hi | (a).hi)
#define JSLL_NOT(r, a)          ((r).lo = ~(a).lo, \
                                 (r).hi = ~(a).hi)

#define JSLL_NEG(r, a)          ((r).lo = -(JSInt32)(a).lo, \
                                 (r).hi = -(JSInt32)(a).hi - ((r).lo != 0))
#define JSLL_ADD(r, a, b) { \
    JSUint32 t; \
    t = (a).lo + (b).lo; \
    (r).hi = (a).hi + (b).hi + (t < (b).lo); \
	 (r).lo = t; \
}

#define JSLL_SUB(r, a, b) { \
    (r).hi = (a).hi - (b).hi - ((a).lo < (b).lo); \
    (r).lo = (a).lo - (b).lo; \
}

#define JSLL_MUL(r, a, b)             jsll_mul( &(r), a, b)
void jsll_mul(JSUint64 *rp, JSUint64 a, JSUint64 b);

#define JSLL_MUL32(r, a, b)           jsll_mul32( &(r), a, b)
void jsll_mul32(JSUint64 *rp, JSUint32 a, JSUint32 b);

#define JSLL_UDIVMOD(qp, rp, a, b)    jsll_udivmod(qp, rp, a, b)
void jsll_udivmod(JSUint64 *qp, JSUint64 *rp, JSUint64 a, JSUint64 b);

#define JSLL_DIV(r, a, b)             jsll_div( &(r), &(a), &(b))
void jsll_div( JSUint64 *r, const JSUint64 *a, const JSUint64 *b);

#define JSLL_MOD(r, a, b)             jsll_mod( &(r), &(a), &(b))
void jsll_mod( JSUint64 *r, const JSUint64 *a, const JSUint64 *b);

/* a is an JSInt32, b is JSInt32, r is JSInt64 */
#define JSLL_ISHL(r, a, b) { \
    if (b) { \
        if ((b) < 32) { \
            (r).lo = (a) << ((b) & 31); \
            (r).hi = ((a) >> (32 - (b))); \
        } else { \
            (r).lo = 0; \
            (r).hi = (a) << ((b) & 31); \
        } \
    } else { \
        (r).lo = (a); \
        (r).hi = 0; \
    } \
}

#define JSLL_SHL(r, a, b)             jsll_shl( &(r), a, b)
#define JSLL_SHR(r, a, b)             jsll_shr( &(r), a, b)
#define JSLL_USHR(r, a, b)            jsll_ushr( &(r), a, b)

void jsll_shl( JSUint64 *r, JSUint64 a, uint_fast8_t b);
void jsll_shr( JSInt64 *r, JSInt64 a, uint_fast8_t b);
void jsll_ushr( JSUint64 *r, JSUint64 a, uint_fast8_t b);

#define JSLL_L2I(i, l)        ((i) = (l).lo)
#define JSLL_L2UI(ui, l)      ((ui) = (l).lo)
#define JSLL_L2F(f, l)        { double _d; JSLL_L2D(_d, l); (f) = (JSFloat64)_d; }

#define JSLL_L2D(d, l) { \
    bool_t _negative; \
    JSInt64 _absval; \
 \
    _negative = (JSInt32)(l).hi < 0; \
    if (_negative) { \
        JSLL_NEG(_absval, l); \
    } else { \
        _absval = l; \
    } \
    (d) = (double)_absval.hi * 4.294967296e9 + _absval.lo; \
    if (_negative) \
        (d) = -(d); \
}

#define JSLL_I2L(l, i)        {(l).lo = (i); (l).hi = (JSInt32)(i) >> 31; }
#define JSLL_UI2L(l, ui)      ((l).lo = (ui), (l).hi = 0)
#define JSLL_F2L(l, f)        { double _d = (double)f; JSLL_D2L(l, _d); }

#define JSLL_D2L(l, d) { \
    int _negative; \
    double _absval, _d_hi; \
    JSInt64 _lo_d; \
 \
    _negative = ((d) < 0); \
    _absval = _negative ? -(d) : (d); \
 \
    (l).hi = _absval / 4.294967296e9; \
    (l).lo = 0; \
    JSLL_L2D(_d_hi, l); \
    _absval -= _d_hi; \
    _lo_d.hi = 0; \
    if (_absval < 0) { \
    _lo_d.lo = -_absval; \
    JSLL_SUB(l, l, _lo_d); \
    } else { \
    _lo_d.lo = _absval; \
    JSLL_ADD(l, l, _lo_d); \
    } \
 \
    if (_negative) \
    JSLL_NEG(l, l); \
}

#define JSLL_HEXSTR(s, a) \
    sprintf( s, "%08" PRIx32 "%08" PRIx32, (a).hi, (a).lo)
#define JSLL_DECSTR(s, a)        jsll_decstr( s, &(a))
#define JSLL_UDECSTR(s, a)       jsll_udecstr( s, &(a))

int jsll_decstr( char *buffer, const JSInt64 *v);
int jsll_udecstr( char *buffer, const JSUint64 *v);

#endif /* !JS_HAVE_LONG_LONG */

#endif /* jslong_h___ */
