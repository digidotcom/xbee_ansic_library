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
	@addtogroup xbee_firmware
	@{
	@file xbee/firmware.h
	Driver layer for performing XBee firmware updates.

	@todo Create typedefs for xbee_fw_source_t read() and seek() handlers,
			write documentation including what the return values are.  We will
			have to check existing functions to confirm the API, but I'm guessing
			<0 for error, >=0 for number of bytes read.  Make sure calls to the
			read() function check for errors!  _xbee_oem_verify() does not!
*/

#ifndef __XBEE_FIRMWARE
#define __XBEE_FIRMWARE

#include "xbee/xmodem.h"

XBEE_BEGIN_DECLS

// Datatypes should be defined in this file as well, possibly with a
// "function help" documentation block to document complex structures.

typedef struct xbee_fw_oem_state_t {
	uint32_t				firmware_length;
	uint32_t				block_offset;
	uint32_t				cur_offset;
	uint16_t				block_length;
} xbee_fw_oem_state_t;

typedef struct xbee_fw_source_t
{
	xbee_dev_t				*xbee;
	int						state;
	int						next_state;
	int						tries;
	char						bootloader_cmd;	// cmd to initiate XMODEM
	uint32_t					timer;
	union {
		xbee_xmodem_state_t	xbxm;			// sub-status of xmodem transfer
		xbee_fw_oem_state_t	oem;			// sub-status of oem transfer
	} u;
	char						buffer[128+5];	// buffer for xmodem packet, must persist
													// for duration of update
	int		(*seek)( void FAR *context, uint32_t offset);
	int		(*read)( void FAR *context, void FAR *buffer, int16_t bytes);
	void				FAR *context;		// spot to hold user data
} xbee_fw_source_t;

#define XBEE_FW_LOAD_TIMEOUT		5000

int xbee_fw_install_init( xbee_dev_t *xbee, const wpan_address_t FAR *target,
	xbee_fw_source_t *source);

int xbee_fw_install_ebl_tick( xbee_fw_source_t *source);
unsigned int xbee_fw_install_ebl_state( xbee_fw_source_t *source);
char FAR *xbee_fw_status_ebl( xbee_fw_source_t *source, char FAR *buffer);

int xbee_fw_install_hcs08_tick( xbee_fw_source_t *source);
/// See documentation for xbee_fw_install_ebl_state().
#define xbee_fw_install_hcs08_state( source) xbee_fw_install_ebl_state( source)
/// See documentation for xbee_fw_status_ebl().
#define xbee_fw_status_hcs08( source, buff) xbee_fw_status_ebl( source, buff)

char FAR *xbee_fw_status_ebl( xbee_fw_source_t *source, char FAR *buffer);

int xbee_fw_install_oem_tick( xbee_fw_source_t *source);
char FAR *xbee_fw_status_oem( xbee_fw_source_t *source, char FAR *buffer);



// Helper function for installing firmware from a .OEM or .EBL file in memory.
typedef struct {
	xbee_fw_source_t source;
	uint32_t			length;
	const char FAR	*address;
	uint32_t			offset;
} xbee_fw_buffer_t;

int xbee_fw_buffer_init( xbee_fw_buffer_t *fw, uint32_t length,
	const char FAR *address);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_firmware.c"
#endif

#endif


