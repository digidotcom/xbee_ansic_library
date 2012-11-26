/*
 * Copyright (c) 2008-2012 Digi International Inc.,
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
	@addtogroup wpan_types
	@{
	@file wpan/types.h
	WPAN datatypes and support functions, valid for ZigBee and DigiMesh.
*/

#ifndef __WPAN_TYPES_H
#define __WPAN_TYPES_H

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

/// Typedef used to hold a 64-bit IEEE address, represented as 8 bytes,
/// 4 16-bit values or 2 32-bit values.
/// Note that (for now) all addr64 elements are stored MSB-first (the order
/// used in XBee frames).
/// @todo update all addr64 variables and structure elements to end in _be
/// (big-endian) or _le (little-endian) where appropriate.  Add functions
/// to convert 64-bit values between host byte order and big/little endian.
typedef union {
	uint8_t			b[8];
	uint16_t			u[4];
	uint32_t			l[2];
} addr64;


#ifndef ADDR64_FORMAT_SEPARATOR
	/// Separator used by addr64_format(), defaults to '-' unless specified
	/// at compile time.
	#define ADDR64_FORMAT_SEPARATOR '-'
#endif

/// Size of character buffer to pass to addr64_format()
/// (8 2-character bytes, 7 separators and 1 null).
#define ADDR64_STRING_LENGTH (8 * 2 + 7 + 1)

// These functions are documented with their implementation in wpan_types.c
char FAR *addr64_format( char FAR *buffer, const addr64 FAR *address);
bool_t addr64_equal( const addr64 FAR *addr1, const addr64 FAR *addr2);
bool_t addr64_is_zero( const addr64 FAR *addr);
int addr64_parse( addr64 *address, const char FAR *str);

/** @name Reserved/Special WPAN network (16-bit) addresses
	@{
*/
/// network broadcast address for all nodes
#define WPAN_NET_ADDR_BCAST_ALL_NODES	0xFFFF
/// network broadcast address for non-sleeping devices
#define WPAN_NET_ADDR_BCAST_NOT_ASLEEP	0xFFFD
/// network broadcast address for all routers (and coordinators)
#define WPAN_NET_ADDR_BCAST_ROUTERS		0xFFFC

/// used to indicate 64-bit addressing (16-bit address is ignored)
#define WPAN_NET_ADDR_UNDEFINED			0xFFFE

/// network coordinator always uses network address 0x0000
#define WPAN_NET_ADDR_COORDINATOR		0x0000
//@}

/** @name Reserved/Special WPAN MAC (64-bit) addresses
	@{
*/
/// Pointer to \c addr64 representing an undefined IEEE address
/// (all ones).
#define WPAN_IEEE_ADDR_UNDEFINED (&_WPAN_IEEE_ADDR_UNDEFINED)
extern const addr64 _WPAN_IEEE_ADDR_UNDEFINED;

/// Pointer to \c addr64 representing the broadcast IEEE address.
#define WPAN_IEEE_ADDR_BROADCAST (&_WPAN_IEEE_ADDR_BROADCAST)
extern const addr64 _WPAN_IEEE_ADDR_BROADCAST;

/// Pointer to \c addr64 representing the coordinator's IEEE adddress
/// (all zeros).
#define WPAN_IEEE_ADDR_COORDINATOR (&_WPAN_IEEE_ADDR_COORDINATOR)
extern const addr64 _WPAN_IEEE_ADDR_COORDINATOR;

/// Pointer to \c addr64 of all zeros.
/// @see addr64_is_zero
#define WPAN_IEEE_ADDR_ALL_ZEROS (&_WPAN_IEEE_ADDR_COORDINATOR)
//@}

/// Single structure to hold an 802.15.4 device's 64-bit IEEE/MAC address
/// and 16-bit network address.
typedef struct _wpan_address_t {
	addr64		ieee;
	uint16_t		network;
} wpan_address_t;

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
   #use "wpan_types.c"
#endif

#endif // #ifdef __WPAN_TYPES_H
