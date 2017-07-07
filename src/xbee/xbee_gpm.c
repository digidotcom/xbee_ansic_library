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
	@file xbee_gpm.c
	General Purpose Memory interface, used on multiple module types (900HP,
	868LP, Wi-Fi).
*/

#include "xbee/byteorder.h"
#include "xbee/gpm.h"

int xbee_gpm_envelope_create( wpan_envelope_t FAR *envelope, wpan_dev_t *dev,
	const addr64 FAR *ieee)
{
	if (envelope == NULL || dev == NULL || ieee == NULL)
	{
		return -EINVAL;
	}
	wpan_envelope_create( envelope, dev, ieee, WPAN_NET_ADDR_UNDEFINED);
	envelope->profile_id = WPAN_PROFILE_DIGI;
	envelope->cluster_id = DIGI_CLUST_MEMORY_ACCESS;
	envelope->source_endpoint =
			envelope->dest_endpoint = WPAN_ENDPOINT_DIGI_DEVICE;

	return 0;
}

int xbee_gpm_envelope_local( wpan_envelope_t *envelope, wpan_dev_t *dev)
{
	return xbee_gpm_envelope_create( envelope, dev, &dev->address.ieee);
}

int _xbee_gpm_parameterless_command( const wpan_envelope_t FAR *envelope,
		uint8_t command)
{
	wpan_envelope_t				gpm_envelope;
	xbee_gpm_request_header_t	header;

	if (envelope == NULL)
	{
		return -EINVAL;
	}
	memset( &header, 0, sizeof header);
	header.cmd_id = command;

	gpm_envelope = *envelope;
	gpm_envelope.payload = &header;
	gpm_envelope.length = sizeof header;

	return wpan_envelope_send( &gpm_envelope);
}

int xbee_gpm_get_flash_info(  const wpan_envelope_t FAR *envelope)
{
	return _xbee_gpm_parameterless_command( envelope,
			XBEE_GPM_CMD_PLATFORM_INFO_REQ);
}

int xbee_gpm_erase_block( const wpan_envelope_t *envelope,
		uint16_t block_num, uint16_t block_size)
{
	wpan_envelope_t				gpm_envelope;
	xbee_gpm_request_header_t	header;

	if (envelope == NULL)
	{
		return -EINVAL;
	}
	memset( &header, 0, sizeof header);
	header.cmd_id = XBEE_GPM_CMD_ERASE_REQ;
	header.block_num_be = htobe16( block_num);
	header.byte_count_be = htobe16( block_size);

	gpm_envelope = *envelope;
	gpm_envelope.payload = &header;
	gpm_envelope.length = sizeof header;

	return wpan_envelope_send( &gpm_envelope);
}

uint16_t xbee_gpm_max_write( const wpan_dev_t *dev)
{
	uint16_t max_bytes;

	if (dev == NULL)
	{
		return 0;
	}

	max_bytes = dev->payload;
	if (max_bytes > XBEE_MAX_RFPAYLOAD)
	{
		max_bytes = XBEE_MAX_RFPAYLOAD;
	}

	return max_bytes - sizeof(xbee_gpm_request_header_t);
}

int _xbee_gpm_write( const wpan_envelope_t *envelope, uint16_t block_num,
		uint16_t byte_offset, uint16_t byte_count, const void FAR *data,
		bool_t erase_first)
{
	wpan_envelope_t				gpm_envelope;
	PACKED_STRUCT {
		xbee_gpm_request_header_t	header;
		uint8_t							data[XBEE_MAX_RFPAYLOAD];
	} frame;

	if (envelope == NULL || data == NULL)
	{
		return -EINVAL;
	}
	if (byte_count > xbee_gpm_max_write( envelope->dev))
	{
		return -EMSGSIZE;
	}

	memset( &frame.header, 0, sizeof frame.header);
	frame.header.cmd_id = erase_first ? XBEE_GPM_CMD_ERASE_THEN_WRITE_REQ
										: XBEE_GPM_CMD_WRITE_REQ;
	frame.header.block_num_be = htobe16( block_num);
	frame.header.start_index_be = htobe16( byte_offset);
	frame.header.byte_count_be = htobe16( byte_count);

	memcpy( frame.data, data, byte_count);

	gpm_envelope = *envelope;
	gpm_envelope.payload = &frame;
	gpm_envelope.length = sizeof frame.header + byte_count;

	return wpan_envelope_send( &gpm_envelope);
}

int xbee_gpm_write( const wpan_envelope_t *envelope, uint16_t block_num,
		uint16_t byte_offset, uint16_t byte_count, const void FAR *data)
{
	return _xbee_gpm_write( envelope, block_num, byte_offset, byte_count,
			data, FALSE);
}

int xbee_gpm_erase_then_write( const wpan_envelope_t *envelope,
		uint16_t block_num, uint16_t byte_offset, uint16_t byte_count,
		const void FAR *data)
{
	return _xbee_gpm_write( envelope, block_num, byte_offset, byte_count,
			data, TRUE);
}

int xbee_gpm_read( const wpan_envelope_t *envelope, uint16_t block_num,
		uint16_t byte_offset, uint16_t byte_count)
{
	wpan_envelope_t				gpm_envelope;
	xbee_gpm_request_header_t	header;

	if (envelope == NULL)
	{
		return -EINVAL;
	}
	if (byte_count > envelope->dev->payload || byte_count > XBEE_MAX_RFPAYLOAD)
	{
		return -EMSGSIZE;
	}
	memset( &header, 0, sizeof header);
	header.cmd_id = XBEE_GPM_CMD_READ_REQ;
	header.block_num_be = htobe16( block_num);
	header.start_index_be = htobe16( byte_offset);
	header.byte_count_be = htobe16( byte_count);

	gpm_envelope = *envelope;
	gpm_envelope.payload = &header;
	gpm_envelope.length = sizeof header;

	return wpan_envelope_send( &gpm_envelope);
}

// TODO: possibly move the firmware commands to a gpm_firmware layer, in case
// a device just wants GPM access and doesn't need the extra functionality of
// doing firmware updates.  Base the API for gpm_firmware on the existing
// xbee_firmware API, with callbacks to read the firmware image.

int xbee_gpm_firmware_verify( const wpan_envelope_t *envelope)
{
	return _xbee_gpm_parameterless_command( envelope,
			XBEE_GPM_CMD_FIRMWARE_VERIFY_REQ);
}

int xbee_gpm_firmware_install( const wpan_envelope_t *envelope)
{
	return _xbee_gpm_parameterless_command( envelope,
			XBEE_GPM_CMD_FIRMWARE_INSTALL_REQ);
}


