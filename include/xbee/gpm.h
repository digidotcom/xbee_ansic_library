/*
 * Copyright (c) 2009-2013 Digi International Inc.,
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
	@file xbee/gpm.h
	General Purpose Memory interface, used on multiple module types (900HP,
	868LP, Wi-Fi).

	Access is done by sending explicit API frames to the MEMORY_ACCESS cluster
	(0x23) of the DIGI_DEVICE endpoint (0xE6) of the target node, either in a
	self-addressed frame, or a frame addressed to a remote node.
*/

#include "xbee/platform.h"
#include "xbee/device.h"
#include "wpan/aps.h"

#define DIGI_CLUST_MEMORY_ACCESS		0x23

typedef PACKED_STRUCT xbee_gpm_request_header_t {
	uint8_t	cmd_id;				//< one of GPM_CMD_*
	uint8_t	options;				//< varies by command
	uint16_t	block_num_be;		//< block number addressed in the GPM
	uint16_t	start_index_be;	//< byte index within the addressed GPM block
	/// number of bytes in the data field (WRITE) or requested for a READ.
	uint16_t	byte_count_be;
} xbee_gpm_request_header_t;

typedef PACKED_STRUCT xbee_gpm_response_header_t {
	uint8_t	cmd_id;				//< one of GPM_CMD_*, same as cmd_id in request
	uint8_t	status;				//< indication whether command was successful
	uint16_t	block_num_be;		//< block number addressed in the GPM
	uint16_t	start_index_be;	//< byte index within the addressed GPM block
	uint16_t	byte_count_be;		//< number of bytes in the data field
} xbee_gpm_response_header_t;

typedef PACKED_STRUCT xbee_gpm_frame_t {
	union {
		xbee_gpm_request_header_t		request;
		xbee_gpm_response_header_t		response;
	} header;
	uint8_t	data[1];				//< variable-length data field
} xbee_gpm_frame_t;

/// Number of bytes out of the RF payload used by the GPM headers.  When
/// reading or writing to the GPM, note that the byte count for the data bytes
/// field must be <= (xbee.wpan_dev.payload - XBEE_GPM_OVERHEAD).
#define XBEE_GPM_OVERHEAD		(sizeof (xbee_gpm_request_header_t))

enum xbee_gpm_cmd {
	XBEE_GPM_CMD_PLATFORM_INFO_REQ			= 0x00,
	XBEE_GPM_CMD_ERASE_REQ						= 0x01,
	XBEE_GPM_CMD_WRITE_REQ						= 0x02,
	XBEE_GPM_CMD_ERASE_THEN_WRITE_REQ		= 0x03,
	XBEE_GPM_CMD_READ_REQ						= 0x04,
	XBEE_GPM_CMD_FIRMWARE_VERIFY_REQ			= 0x05,
	XBEE_GPM_CMD_FIRMWARE_INSTALL_REQ		= 0x06,

	XBEE_GPM_CMD_PLATFORM_INFO_RESP			= 0x80,
	XBEE_GPM_CMD_ERASE_RESP						= 0x81,
	XBEE_GPM_CMD_WRITE_RESP						= 0x82,
	XBEE_GPM_CMD_ERASE_THEN_WRITE_RESP		= 0x83,
	XBEE_GPM_CMD_READ_RESP						= 0x84,
	XBEE_GPM_CMD_FIRMWARE_VERIFY_RESP		= 0x85,
	XBEE_GPM_CMD_FIRMWARE_INSTALL_RESP		= 0x86,
};

typedef struct xbee_gpm_flash_info_t {
	uint16_t	block_count;
	uint16_t	block_size;
} xbee_gpm_flash_info_t;

enum xbee_gpm_status {
	XBEE_GPM_STATUS_ERROR_FLAG					= (1<<0)
};

int xbee_gpm_envelope_create( wpan_envelope_t *envelope, wpan_dev_t *dev,
	const addr64 FAR *ieee);

int xbee_gpm_envelope_local( wpan_envelope_t *envelope, wpan_dev_t *dev);

int xbee_gpm_request_send( const wpan_envelope_t *envelope,
		const void *request, uint16_t request_length);

int xbee_gpm_get_flash_info( const wpan_envelope_t *envelope);

int xbee_gpm_erase_block( const wpan_envelope_t *envelope,
		uint16_t block_num, uint16_t block_size);

// Maximum number of bytes allowed per write (based on device's ATNP and size
// of GPM header).
uint16_t xbee_gpm_max_write( const wpan_dev_t *dev);

// Command to erase entire flash is to see block num and block size to 0.
#define xbee_gpm_erase_flash( env)	xbee_gpm_erase_block( env, 0, 0)

int xbee_gpm_write( const wpan_envelope_t *envelope, uint16_t block_num,
		uint16_t byte_offset, uint16_t byte_count, const void FAR *data);

int xbee_gpm_erase_then_write( const wpan_envelope_t *envelope,
		uint16_t block_num, uint16_t byte_offset, uint16_t byte_count,
		const void FAR *data);

int xbee_gpm_read( const wpan_envelope_t *envelope, uint16_t block_num,
		uint16_t byte_offset, uint16_t byte_count);

// TODO: possibly move the firmware commands to a gpm_firmware layer, in case
// a device just wants GPM access and doesn't need the extra functionality of
// doing firmware updates.  Base the API for gpm_firmware on the existing
// xbee_firmware API, with callbacks to read the firmware image.

int xbee_gpm_firmware_verify( const wpan_envelope_t *envelope);

int xbee_gpm_firmware_install( const wpan_envelope_t *envelope);

