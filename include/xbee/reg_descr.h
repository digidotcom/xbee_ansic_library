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
	@addtogroup xbee_atcmd
	@{
	@file xbee/reg_descr.h
	Header for XBee register descriptors

   @note This header is sensitive to the value of the macro IDIGI_USE_XBEE.
   If this macro is defined, some additional fields in the register descriptor
   structure are defined (and initialized).  This data is required for
   iDigi support to be included.

*/
#ifndef __XBEE_REG_DESCR
#define __XBEE_REG_DESCR

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

/// iDigi RCI type names used by do_command type=zigbee.  This table is used
/// so as to avoid having XBEE_RCI_TYPE_STRING pointers in the register
/// descriptor table.
typedef enum
{
	XBEE_RCI_TYPE_UINT32,
	XBEE_RCI_TYPE_HEX32,
	XBEE_RCI_TYPE_STRING,
	XBEE_RCI_TYPE_BIN,	// Bin turns into XBEE_RCI_TYPE_STRING for RCI, but formatted as 0x
   							// followed by hex digits.
	XBEE_RCI_TYPE_ADDR64,
} _xbee_rci_type_t;

/// Table to map _xbee_rci_type_t to RCI type string
extern const char *_xbee_rci_types[];



typedef struct _xbee_reg_descr_t
{
	/// 1 if a "state", 0 if a "setting", or other values for special purposes.
   /// A state is read-only, and
   /// reflects dynamic state such as error counts.  A setting is read/write
   /// or write-only, which can (in principle) be saved to non-volatile
   /// storage on the device.
	int	is_state;
	/// Two-character AT command string for this register (except entry for
   /// <dest_addr> is "DH/DL").
	const char *alias;
	/** Custom xbee_type value (bitfield) for query descriptor.

       This field is a bitmask, which determines whether this entry is
       applicable to a given XBee module type.  A mask (M) for the module is
       obtained by (e.g.) M = sxa_module_type_mask(sxa).  Then, only if
       (M & cxval) == M is the entry applicable.

       One of the bits in cxval is for a "gateway" i.e. a local module.
       If this bit is NOT set in cxval, then such commands are not usually
       performed on local devices, usually because changing that register
       might disrupt the network and make it inaccessible.  In cases where
       it really is necessary to change one of these registers, the gateway
       bit in M can be reset before applying the above bitmask test.

       @note This mask behavior is derived from NDS and is used by iDigi.
   */
   uint16_t		cxval;
	/// RCI type
   _xbee_rci_type_t		rci_type;
#ifdef IDIGI_USE_XBEE
	/// If iDigi support: RCI element name for this register
	const char *rci_element;
	/// If iDigi support: register description string
   const char *desc;
	/// If iDigi support: "units" element value for query descriptor
   const char *units;
	// Note: these are stored as strings since it results in less storage
   // use compared with 32-bit numbers, plus the format is correct for RCI.
	/// If iDigi support: "min" element value for query descriptor
   const char *min;
	/// If iDigi support: "max" element value for query descriptor
   const char *max;
#endif
	/// Offset in SXA node structure of cached field value
   uint16_t 	sxa_offs;
	/// Length of cached field value
   uint16_t 	sxa_len;
	/// SXA cache group (bitfield), or SXA_CACHED_MISC if not grouped.
   uint16_t 	sxa_cache_group;
} _xbee_reg_descr_t;



extern const FAR _xbee_reg_descr_t _xbee_reg_table[];

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_reg_descr.c"
#endif

#endif		// __XBEE_REG_DESCR

//@}
