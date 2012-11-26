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
	@addtogroup zcl
	@{
	@file zigbee_zcl.c
	Process General Commands for ZigBee Cluster Library and provide support
	functions to other layers of the stack.

	@note For performance, on a low-resource, single-threaded system, the
		zcl_command_t object should be a global instead of building it on
		the stack of zcl_general_command and then passing a pointer around to the
		other functions.
*/

/*** BeginHeader */
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "wpan/aps.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_types.h"

#ifndef __DC__
	#define zigbee_zcl_debug
#elif defined ZIGBEE_ZCL_DEBUG
	#define zigbee_zcl_debug		__debug
#else
	#define zigbee_zcl_debug		__nodebug
#endif
/*** EndHeader */

/*** BeginHeader zcl_attributes_none */
/*** EndHeader */
// documented in zigbee/zcl.h
const zcl_attribute_tree_t FAR zcl_attributes_none[1] =
	{ { ZCL_MFG_NONE, NULL, NULL } };

/*** BeginHeader zcl_send_response */
/*** EndHeader */
/**
	@brief
	Send a response to a ZCL command.

	Uses information from the command received to populate fields in the
	response.

	@param[in]	request	command to respond to
	@param[in]	response	payload to send as response
	@param[in]	length	number of bytes in \a response

	@retval	0			successfully sent response
	@retval	!0			error on send
*/
zigbee_zcl_debug
int zcl_send_response( zcl_command_t *request, const void FAR *response,
	uint16_t length)
{
	int									retval;
	wpan_envelope_t					reply_envelope;

	if (request == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	// address envelope of reply
	retval = wpan_envelope_reply( &reply_envelope, request->envelope);
	if (retval == 0)
	{
		reply_envelope.payload = response;
		reply_envelope.length = length;
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: sending %d-byte response\n", __FUNCTION__,
				reply_envelope.length);
			wpan_envelope_dump( &reply_envelope);
		#endif

		retval = wpan_envelope_send( &reply_envelope);
	}

	return retval;
}

/*** BeginHeader zcl_attribute_get_next */
/*** EndHeader */
/**
	@brief
	Return a pointer to the next attribute entry from an attribute table.

	@param[in]	entry		current entry from the table

	@retval		NULL		entry was NULL
	@retval		!NULL		pointer to next attribute record

	@note If \a entry points to the last entry in the table, this function
	will return the same address instead of advancing off the end of the table.

	@sa zcl_find_attribute
*/
zigbee_zcl_debug
const zcl_attribute_base_t FAR *zcl_attribute_get_next(
	const zcl_attribute_base_t FAR *entry)
{
	if (! entry)
	{
		return NULL;
	}

	if (entry->id == ZCL_ATTRIBUTE_END_OF_LIST)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: already at end of list\n", __FUNCTION__);
		#endif
		// don't advance past the end of the list
		return entry;
	}

	if (entry->flags & ZCL_ATTRIB_FLAG_FULL)
	{
		// advance pointer over a zcl_attribute_full_t record
		return (const zcl_attribute_base_t FAR *)
			((const zcl_attribute_full_t FAR *)entry + 1);
	}

	return (entry + 1);
}

/*** BeginHeader zcl_find_attribute */
/*** EndHeader */
/**
	@brief
	Search the attribute table starting at \p entry, for attribute ID
	\p search_id.

	@param[in]	entry			starting entry for search
	@param[in]	search_id	attribute ID to look for

	@retval NULL		attribute id \p search_id not in list
	@retval !NULL		pointer to attribute record
*/
zigbee_zcl_debug
const zcl_attribute_base_t FAR *zcl_find_attribute(
	const zcl_attribute_base_t FAR *entry, uint16_t search_id)
{
	uint16_t		id;

	if (! entry)
	{
		return NULL;
	}

	for (id = entry->id; id < search_id; id = entry->id)
	{
		entry = zcl_attribute_get_next( entry);
	}
	if (id == search_id)
	{
		return entry;
	}

	return NULL;
}

/*** BeginHeader zcl_build_header */
/*** EndHeader */
/**
	@brief
	Support function to fill in a ZCL response header.

	Set the \c frame_control, \c sequence and optional \c mfg_code fields of
	the ZCL response header, and return the offset to the actual start of
	the header (non-mfg specific skips the first two bytes).

	@param[out]		rsp	buffer to store header of response
	@param[in]		cmd	command we're responding to

	@retval	0	responding to manufacturer-specific command
	@retval	2	responding to non-manufacturer-specific command
	@retval	-EINVAL		invalid parameter passed to function
*/
zigbee_zcl_debug
int zcl_build_header( zcl_header_response_t *rsp, zcl_command_t *cmd)
{
	uint8_t frame_control;

	if (rsp == NULL || cmd == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	// Keep direction & type bits from request (old_frame_control & mask),
	// toggle direction (1 ^ old_direction)
	// and set disable default response (1 ^ 0) bit.
	// direction is opposite of command's direction, use xor to toggle
	frame_control = (ZCL_FRAME_DISABLE_DEF_RESP | ZCL_FRAME_DIRECTION)
			^ (cmd->frame_control & (ZCL_FRAME_DIRECTION | ZCL_FRAME_TYPE_MASK));
	rsp->sequence = cmd->sequence;
	if (cmd->frame_control & ZCL_FRAME_MFG_SPECIFIC)
	{
		rsp->u.mfg.mfg_code_le = htole16( cmd->mfg_code);
		rsp->u.mfg.frame_control = frame_control | ZCL_FRAME_MFG_SPECIFIC;
		return 0;
	}

	rsp->u.std.frame_control = frame_control;
	return 2;
}


/*** BeginHeader zcl_default_response */
/*** EndHeader */
/**
	@brief
	Send a Default Response (#ZCL_CMD_DEFAULT_RESP) to a given command.

	@param[in]		request	command we're responding to
	@param[in]		status	status to use in the response
			(see ZCL_STATUS_* macros under "ZCL Status Enumerations"
			in zigbee/zcl.h)

	@retval	0			successfully sent response, or response not required
							(message was broadcast, sent to the broadcast endpoint,
							or sender set the disable default response bit)
	@retval	!0			error on send
	@retval	-EINVAL	invalid parameter passed to function
*/
zigbee_zcl_debug
int zcl_default_response( zcl_command_t *request, uint8_t status)
{
	PACKED_STRUCT {
		zcl_header_response_t		header;
		uint8_t							command;
		uint8_t							status;
	} response;
	uint8_t								*start;

	if (request == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	if (status == ZCL_STATUS_SUCCESS &&
		(request->frame_control & ZCL_FRAME_DISABLE_DEF_RESP))
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: don't send SUCCESS; sender disabled default response\n",
				__FUNCTION__);
		#endif
		return 0;
	}

	if (request->envelope->options
		& (WPAN_ENVELOPE_BROADCAST_ADDR | WPAN_ENVELOPE_BROADCAST_EP))
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: not responding to %s\n", __FUNCTION__,
				"broadcast message");
		#endif
		return 0;
	}

	if ((request->frame_control & ZCL_FRAME_TYPE_MASK) == ZCL_FRAME_TYPE_PROFILE
		&& request->command == ZCL_CMD_DEFAULT_RESP)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: not responding to %s\n", __FUNCTION__,
				"default response");
		#endif
		return 0;
	}

	response.header.command = ZCL_CMD_DEFAULT_RESP;
	start = (uint8_t *)&response + zcl_build_header( &response.header, request);

	// clear the frame type bits -- default response is not a cluster command
	// (frame control is at address <start> -- different offset into
	// response structure depending on whether command is manufacturer-specific
	*start &= ~ZCL_FRAME_TYPE_MASK;

	response.command = request->command;
	response.status = status;

	return zcl_send_response( request, start,
														(uint8_t *)(&response + 1) - start);
}

/*** BeginHeader zcl_check_minmax */
/*** EndHeader */
/**
	@brief
	Checks whether a new value from a Write Attributes command is within the
	limits specified for a given attribute.

	@param[in]	entry		entry to use for min/max check
	@param[in]	buffer_le	buffer with new value (in little-endian byte order),
									from write attributes command

	@retval	0	new value is within range (or attribute doesn't have a min/max)
	@retval	-EINVAL	invalid parameter passed to function
	@retval	!0	value is out of range
*/
zigbee_zcl_debug
int zcl_check_minmax( const zcl_attribute_base_t FAR *entry,
	const uint8_t FAR *buffer_le)
{
	#ifdef ZIGBEE_ZCL_VERBOSE
		static char *zcl_result[] = { "within range", "below min", "above max" };
	#endif
	zcl_attribute_minmax_t value;
	int sizeof_type;
	uint8_t typeinfo;
	const zcl_attribute_full_t FAR *full;
	int retval = 0;

	if (entry == NULL || buffer_le == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	if (entry->flags & (ZCL_ATTRIB_FLAG_HAS_MIN | ZCL_ATTRIB_FLAG_HAS_MAX))
	{
		sizeof_type = zcl_sizeof_type( entry->type);
		if (sizeof_type < 1 || sizeof_type > 4)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: no min/max checking for type 0x%02x\n",
					__FUNCTION__, entry->type);
			#endif
			return 0;	// min/max checking is only valid for 1 to 4 byte values
		}
		typeinfo = zcl_type_info[entry->type];
		if (typeinfo & ZCL_T_FLOAT)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: no min/max checking for floating point\n",
					__FUNCTION__);
			#endif
			return 0;
		}

		value._unsigned = 0;
		#if BYTE_ORDER == LITTLE_ENDIAN
			_f_memcpy( &value, buffer_le, sizeof_type);
		#else
			// for big-endian, make sure we fill in the low bytes
			memcpy_letoh( (uint8_t *)&value + (4 - sizeof_type), buffer_le,
				(uint8_t) sizeof_type);
		#endif

		full = (const zcl_attribute_full_t FAR *) entry;
		if (typeinfo & ZCL_T_SIGNED)
		{
			// if sign bit was set, sign-extend to 32 bits
			if ((sizeof_type < 4) && (buffer_le[sizeof_type - 1] & 0x80) )
			{
				value._unsigned |= (0xFFFFFFFF << (uint8_t) sizeof_type);
			}

			if ((entry->flags & ZCL_ATTRIB_FLAG_HAS_MIN) &&
				(full->min._signed > value._signed))
			{
				retval = 1;
			}
			else if ((entry->flags & ZCL_ATTRIB_FLAG_HAS_MAX) &&
				(full->max._signed < value._signed))
			{
				retval = 2;
			}
		}
		else
		{
			if ((entry->flags & ZCL_ATTRIB_FLAG_HAS_MIN) &&
				(full->min._unsigned > value._unsigned))
			{
				retval = 1;
			}
			else if ((entry->flags & ZCL_ATTRIB_FLAG_HAS_MAX) &&
				(full->max._unsigned < value._unsigned))
			{
				retval = 2;
			}
		}
	}
	#ifdef ZIGBEE_ZCL_VERBOSE
		printf( "%s: new value %s\n", __FUNCTION__, zcl_result[retval]);
	#endif

	return retval;
}


/*** BeginHeader zcl_parse_attribute_record */
/*** EndHeader */
/**
	@brief
	Parse an attribute record from a Write Attributes Request or a Read
	Attributes Response and possibly store the new attribute value.

	If #ZCL_ATTR_WRITE_FLAG_READ_RESP is set in \c write_rec->flags,
	\c write_rec->buffer will be parsed as a Read Attributes Response record.
	Otherwise, it's parsed as a Write Attributes Request record.

	@param[in]		entry			attribute table to search or NULL if there aren't
										any attributes on this cluster
	@param[in,out]	write_rec	state information for parsing write request

	@retval	>=0	number of bytes consumed from buffer
	@retval	-EINVAL	invalid parameter passed to function
*/
zigbee_zcl_debug
int zcl_parse_attribute_record( const zcl_attribute_base_t FAR *entry,
	zcl_attribute_write_rec_t *write_rec)
{
	uint16_t									attribute;
	uint8_t									type;
	uint8_t									value_offset;	// where is value in buffer?
	int										sizeof_type;
	zcl_attribute_base_t					fake_attribute;
	const zcl_attribute_full_t	FAR	*full;
	int16_t									start_buflen;
	int										bytes_read;

	if (write_rec == NULL)
	{
		// Note that a NULL entry parameter is OK and represents a cluster
		// without any attributes.
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	attribute = le16toh( *(uint16_t FAR *)write_rec->buffer);
	if (write_rec->flags & ZCL_ATTR_WRITE_FLAG_READ_RESP)
	{
		// If this is a read attribute response and the status was not success,
		// there won't by a type or a value.  Just skip over the attribute ID
		// and status to get to the next record.
		if (write_rec->buffer[2] != ZCL_STATUS_SUCCESS)
		{
			///@todo should we set a flag in write_rec to indicate that the
			///		status was not success?
			write_rec->buffer += 3;
			return 3;
		}
		value_offset = 4;
	}
	else
	{
		value_offset = 3;
	}

	start_buflen = write_rec->buflen;
	if (start_buflen < value_offset)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: buffer too small (can't hold id and type)\n",
				__FUNCTION__);
		#endif
		write_rec->status = ZCL_STATUS_MALFORMED_COMMAND;
	}
	else
	{
		type = write_rec->buffer[value_offset - 1];
		write_rec->buffer += value_offset;
		write_rec->buflen -= value_offset;

		// look up attribute
		entry = zcl_find_attribute( entry, attribute);
		if (! entry)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: ERROR couldn't find attribute 0x%04x\n",
					__FUNCTION__, attribute);
			#endif
			write_rec->status = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
		}
		else if (entry->type != type)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: ERROR type mismatch (0x%02x != 0x%02x) on 0x%04x\n",
					__FUNCTION__, type, entry->type, attribute);
			#endif
			if (ZCL_TYPE_IS_INVALID( type))
			{
				// Not only is the type wrong, we can't parse it so error out as a
				// malformed command.
				write_rec->status = ZCL_STATUS_MALFORMED_COMMAND;
			}
			else
			{
				write_rec->status = ZCL_STATUS_INVALID_DATA_TYPE;
			}
		}
		else if (entry->flags & ZCL_ATTRIB_FLAG_READONLY)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: ERROR attribute 0x%04x is read-only\n",
					__FUNCTION__, attribute);
			#endif
			write_rec->status = ZCL_STATUS_READ_ONLY;
		}
		else
		{
			sizeof_type = zcl_sizeof_type( type);
			if (sizeof_type >= 0 && write_rec->buflen < sizeof_type)
			{
				#ifdef ZIGBEE_ZCL_VERBOSE
					printf( "%s: buffer too small for type 0x%02x (have %d, need %d)\n",
						__FUNCTION__, type, write_rec->buflen, sizeof_type);
				#endif
				write_rec->status = ZCL_STATUS_MALFORMED_COMMAND;
			}
		}
		// if successful up until now, verify min/max values
		if (write_rec->status == ZCL_STATUS_SUCCESS
			&& zcl_check_minmax( entry, write_rec->buffer))
		{
			write_rec->status = ZCL_STATUS_INVALID_VALUE;
		}
	}

	if (write_rec->status == ZCL_STATUS_MALFORMED_COMMAND)
	{
		// can't continue parsing a malformed command, so mark buffer as empty
		write_rec->buflen = 0;
	}
	else
	{
		if (write_rec->status != ZCL_STATUS_SUCCESS)
		{
			// don't perform assignment, just parse the request
			write_rec->flags &= ~ZCL_ATTR_WRITE_FLAG_ASSIGN;

			// make a fake attribute entry so zcl_decode_attribute can parse
			// attribute value based on type in request
			memset( &fake_attribute, 0, sizeof fake_attribute);
			fake_attribute.id = attribute;
			fake_attribute.type = type;

			entry = &fake_attribute;
		}

		// Call associated write function, if present.  Note that if using the
		// fake attribute, none of the flags are set (flags == 0).
		full = (const zcl_attribute_full_t FAR *) entry;
		if ((entry->flags & ZCL_ATTRIB_FLAG_FULL) && full->write)
		{
			bytes_read = full->write( full, write_rec);
		}
		else
		{
			bytes_read = zcl_decode_attribute( entry, write_rec);
		}
		// Update buffer position and number of bytes left, based on number
		// of bytes consumed.
		write_rec->buffer += bytes_read;
		write_rec->buflen -= bytes_read;
	}

	#ifdef ZIGBEE_ZCL_VERBOSE
		if (write_rec->status != ZCL_STATUS_SUCCESS)
		{
			printf( "%s: failure (0x%02x) trying to write attribute 0x%04x\n",
				__FUNCTION__, write_rec->status, entry->id);
		}
	#endif

	return start_buflen - write_rec->buflen;
}

/*** BeginHeader zcl_convert_24bit */
/*** EndHeader */
/**
	@brief
	Convert a 24-bit (3-byte) little-endian value to a 32-bit value in
	host byte order.

	@param[in]	value_le		pointer to 3 bytes to convert
	@param[in]	extend_sign	if TRUE, set high byte of result to 0xFF if top
									bit of 24-bit value is set

	@return A 32-bit, host-byte-order version of the 24-bit, little-endian
				value passed in.
*/
zigbee_zcl_debug
uint32_t zcl_convert_24bit( const void FAR *value_le, bool_t extend_sign)
{
	uint8_t result[4];
	const char FAR *bytes = value_le;

	if (value_le == NULL)
	{
		return 0;
	}

	#if BYTE_ORDER == BIG_ENDIAN
		result[3] = bytes[0];
		result[2] = bytes[1];
		result[1] = bytes[2];
		result[0] = ((result[1] & 0x80) && extend_sign) ? 0xFF : 0x00;
	#else
		result[0] = bytes[0];
		result[1] = bytes[1];
		result[2] = bytes[2];
		result[3] = ((result[2] & 0x80) && extend_sign) ? 0xFF : 0x00;
	#endif

	return *(uint32_t *) result;
}

/*** BeginHeader zcl_decode_attribute */
/*** EndHeader */
/**
	@brief
	Decode attribute value from a Write Attribute Request or Read Attribute
	Response record and optionally write to \p entry.

	@param[in]		entry			entry from attribute table
	@param[in,out]	rec			state information for parsing write request

	@retval	>=0	number of bytes consumed from \p rec->buffer
	@retval	-EINVAL	invalid parameter passed to function
*/
zigbee_zcl_debug
int zcl_decode_attribute( const zcl_attribute_base_t FAR *entry,
	zcl_attribute_write_rec_t *rec)
{
	int										retval = 0;
	int										sizeof_type;
	uint8_t							FAR	*p;
	int										length, maxlength;
	uint32_t									ulong;
	uint8_t									newstatus;
	const zcl_attribute_full_t	FAR	*full;
	bool_t									assign;

	if (entry == NULL || rec == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	newstatus = rec->status;
	assign = (rec->flags & ZCL_ATTR_WRITE_FLAG_ASSIGN) != 0;
	if (assign && entry->value == NULL)
	{
		assign = FALSE;
		newstatus = ZCL_STATUS_DEFINED_OUT_OF_BAND;
	}
	// Note that zcl_parse_attribute_record() already verified that there
	// was enough bytes in the record for the given types.
	sizeof_type = zcl_sizeof_type( entry->type);
	switch (entry->type)
	{
		case ZCL_TYPE_GENERAL_24BIT:
		case ZCL_TYPE_BITMAP_24BIT:
		case ZCL_TYPE_UNSIGNED_24BIT:
		case ZCL_TYPE_SIGNED_24BIT:
			if (assign)
			{
				if (entry->flags & ZCL_ATTRIB_FLAG_RAW)
				{
					goto default_assign;
				}

				// special case for 24-bit values, promote to 32-bits on host
				ulong = zcl_convert_24bit( rec->buffer,
													entry->type == ZCL_TYPE_SIGNED_24BIT);
				#ifdef ZIGBEE_ZCL_VERBOSE
					printf( "%s: assigning 0x%06" PRIx32 " to attribute 0x%04x\n",
						__FUNCTION__, ulong & 0x00FFFFFF, entry->id);
				#endif
				*(uint32_t FAR *)entry->value = ulong;
			}
			retval = 3;
			break;

		case ZCL_TYPE_LOGICAL_BOOLEAN:
			if (*rec->buffer & 0xFE)
			{
				#ifdef ZIGBEE_ZCL_VERBOSE
					printf( "%s: value 0x%02x is invalid for LOGICAL_BOOLEAN\n",
						__FUNCTION__, *rec->buffer);
				#endif
				newstatus = ZCL_STATUS_INVALID_VALUE;
				assign = FALSE;
			}
			// fall through to other 8-bit types
		default:								// covers most types
			if (ZCL_TYPE_IS_INVALID( entry->type))
			{
				// this is a failure on our part -- type used in OUR attribute list
				// that this function doesn't support
				#ifdef ZIGBEE_ZCL_VERBOSE
					printf( "%s: unsupported type 0x%02x\n", __FUNCTION__,
						entry->type);
				#endif
				newstatus = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
			}
			else if (sizeof_type > 0)
			{
				if (assign)
				{
					#ifdef ZIGBEE_ZCL_VERBOSE
						uint_fast8_t i;
					#endif
default_assign:
					#ifdef ZIGBEE_ZCL_VERBOSE
						printf( "%s: assigning 0x", __FUNCTION__);
						for (i = 0; i < sizeof_type; ++i)
						{
							printf( "%02x", rec->buffer[i]);
						}
						printf( " to attribute 0x%04x\n", entry->id);
					#endif

					// Note that we cast away the const of entry->value since it's
					// only treated as const if ZCL_ATTRIB_FLAG_READONLY is set.

					// If this is a big-endian device and it's not a raw attribute,
					// swap the byte order while copying bytes.
					#if BYTE_ORDER == BIG_ENDIAN
						if (! (entry->flags & ZCL_ATTRIB_FLAG_RAW))
						{
							memcpy_letoh( (void FAR *)entry->value, rec->buffer,
								(uint8_t) sizeof_type);
						}
						else
					#endif
						{
							// simple copy for raw attribute or little-endian device
							_f_memcpy( (void FAR *)entry->value, rec->buffer,
								sizeof_type);
						}
				}
				retval = sizeof_type;
			}
			break;

		case ZCL_TYPE_STRING_CHAR:
			// does packet contain number of bytes from length field?
			length = *rec->buffer;
			if (length + 1 > rec->buflen)		// +1 for length byte
			{
				newstatus = ZCL_STATUS_MALFORMED_COMMAND;
				retval = rec->buflen;	// eat all remaining bytes
			}
			else
			{
				// If this is a full attribute with a value for "max", that value
				// represents the maximum string length.  Make sure we're not
				// exceeding it (if present).
				full = (const zcl_attribute_full_t FAR *) entry;
				maxlength = (int) full->max._signed;
				if ((entry->flags & ZCL_ATTRIB_FLAG_FULL)
					&& maxlength && length > maxlength)
				{
					newstatus = ZCL_STATUS_INVALID_VALUE;
				}
				else if (assign)
				{
					p = (uint8_t FAR *)(entry->value);
					_f_memcpy( p, rec->buffer + 1, length);
					p[length] = '\0';		// add null terminator
					#ifdef ZIGBEE_ZCL_VERBOSE
						printf( "%s: assigned '%" PRIsFAR "' to attribute 0x%04x\n",
							__FUNCTION__, p, entry->id);
					#endif
				}
				retval = length + 1;
			}
			break;

		case ZCL_TYPE_NO_DATA:
		case ZCL_TYPE_UNKNOWN:
			newstatus = ZCL_STATUS_INVALID_DATA_TYPE;
			break;
	}

	// update status if it was set to SUCCESS at start of function
	if (rec->status == ZCL_STATUS_SUCCESS)
	{
		rec->status = newstatus;
	}

	return retval;		// number of bytes consumed
}

/*** BeginHeader _zcl_write_attributes */
int _zcl_write_attributes( zcl_command_t *cmd);
/*** EndHeader */
/**
	@internal
	@brief
	Process the three types of Write Attributes requests.

	Handles Write Attributes (#ZCL_CMD_WRITE_ATTRIB), Write Attributes
	Undivided (#ZCL_CMD_WRITE_ATTRIB_UNDIV) and
	Write Attributes No Response (#ZCL_CMD_WRITE_ATTRIB_NORESP).

	@param[in]	cmd	command to respond to

	@retval	0			successfully sent response or "No Response" requested
	@retval	!0			error on send

	@todo Generate error if command is not one of the three expected commands.

	@todo Update to limit number of status records in response.
*/
zigbee_zcl_debug
int _zcl_write_attributes( zcl_command_t *cmd)
{
	int16_t									length;
	uint_fast8_t							pass;
	PACKED_STRUCT {
		zcl_header_response_t			header;
		zcl_rec_write_attrib_status_t	status[25];		// limit to how many recs?
	} response;
	uint8_t									*start_response;
	zcl_rec_write_attrib_status_t		*status_rec;
	zcl_attribute_write_rec_t			parse_record;

	#ifdef ZIGBEE_ZCL_VERBOSE
		printf( "%s: Write Attributes (cmd=0x%02x)\n", __FUNCTION__,
			cmd->command);
		hex_dump( cmd->zcl_payload, cmd->length, HEX_DUMP_FLAG_TAB);
	#endif

	response.header.command = ZCL_CMD_WRITE_ATTRIB_RESP;
	start_response = (uint8_t *)&response
		+ zcl_build_header( &response.header, cmd);
	status_rec = response.status;

	/*
		This outer loop handles the Write Attributes Undivided command.  The
		other two write commands (standard and No Response) set pass to 1,
		so the loop only executes once.

		For Undivided, the loop runs once with assignment disabled (just checks
		for errors in the request) and if there aren't any errors (status_rec
		still set to response.status) runs a second time with assignment
		enabled.

		WARNING: There is an assumption that if there aren't any errors on the
		first pass, there won't be any on the second pass.
	*/

	pass = (cmd->command == ZCL_CMD_WRITE_ATTRIB_UNDIV) ? 0 : 1;
	while (pass <= 1 && status_rec == response.status)
	{
		// reset the parse record to the start of the buffer
		parse_record.buffer = cmd->zcl_payload;
		parse_record.buflen = cmd->length;

		while (parse_record.buflen > 0)
		{
			// copy attribute ID to status record before advancing buffer
			status_rec->id_le = *(uint16_t FAR *)(parse_record.buffer);

			// reset flag to do assignment
			parse_record.flags =
				pass ? ZCL_ATTR_WRITE_FLAG_ASSIGN : ZCL_ATTR_WRITE_FLAG_NONE;
			// assume success; zcl_parse_attribute_record() will modify
			parse_record.status = ZCL_STATUS_SUCCESS;
			zcl_parse_attribute_record( cmd->attributes, &parse_record);

			// .buffer, .buflen and .status are modified by
			// zcl_parse_attribute_record()

			if (parse_record.status != ZCL_STATUS_SUCCESS)
			{
				status_rec->status = parse_record.status;
				++status_rec;
			}
		}
		++pass;
	}

	if (cmd->command == ZCL_CMD_WRITE_ATTRIB_NORESP)
	{
		// Client doesn't want a response, so just return.
		return 0;
	}

	if (status_rec == response.status)
	{
		// Successfully wrote all attributes -- response is a single SUCCESS
		// status with the attribute ID omitted.
		response.status[0].status = ZCL_STATUS_SUCCESS;
		length = &response.status[0].status + 1 - start_response;
	}
	else
	{
		length = (uint8_t *)status_rec - start_response;
	}
	return zcl_send_response( cmd, start_response, length);
}

/*** BeginHeader zcl_encode_attribute_value */
/*** EndHeader */
/** @internal
	@brief
	Format a ZCL struct for a Read Attributes Response or a Write Attributes
	Request.

	@param[out]	buffer		buffer to store encoded attribute
	@param[in]	bufsize		number of bytes available in \p buffer
	@param[in]	zcl_struct	struct to encode into \p buffer

	Copies the value of attribute \p entry to \p buffer in little-endian
	byte order.  Will not write more than \p bufsize bytes.  Used to build
	ZCL frames.

	@retval	-ZCL_STATUS_INSUFFICIENT_SPACE	buffer too small to encode value
	@retval	-ZCL_STATUS_FAILURE					unknown/unsupported attribute type
	@retval	-ZCL_STATUS_HARDWARE_FAILURE		failure updating attribute value
	@retval	-ZCL_STATUS_SOFTWARE_FAILURE		failure updating attribute value
	@retval	>=0	number of bytes written

	@sa zcl_encode_attribute_value
*/
zigbee_zcl_debug
int _zcl_encode_struct_value( uint8_t FAR *buffer, int16_t bufsize,
	const zcl_struct_t FAR *zcl_struct)
{
	const zcl_struct_element_t FAR *elem;
	uint16_t		element_count;
	zcl_attribute_base_t FAR fake_attr = { 0 };
	int			written = 0;
	int			encode;

	// even an struct of 0 elements requires 2 bytes to encode
	if (bufsize < 2)
	{
		return -ZCL_STATUS_INSUFFICIENT_SPACE;
	}

	element_count = zcl_struct->element_count;
	buffer[0] = element_count & 0xFF;
	buffer[1] = element_count >> 8;
	written = 2;

	elem = zcl_struct->element;
	// Note that we only set the .type and .value fields of fake_attr; the .id
	// and .flags fields remain initialized to 0.
	while (element_count--)
	{
		if (written == bufsize)
		{
			return -ZCL_STATUS_INSUFFICIENT_SPACE;		// no room for type byte
		}

		buffer[written++] = fake_attr.type = elem->zcl_type;
		fake_attr.value = (const uint8_t FAR *)zcl_struct->base_address
																		+ elem->offset;
		encode = zcl_encode_attribute_value( &buffer[written], bufsize - written,
			&fake_attr);
		if (encode < 0)
		{
			// error encoding attribute (typ. INSUFFICIENT_SPACE)
			return encode;
		}
		written += encode;		// add to count of bytes encoded into buffer
		++elem;						// point to next element to encode
	}

	return written;
}

/** @internal
	@brief
	Format a ZCL array for a Read Attributes Response or a Write Attributes
	Request.

	@param[out]	buffer		buffer to store encoded attribute
	@param[in]	bufsize		number of bytes available in \p buffer
	@param[in]	array			array to encode into \p buffer

	Copies the value of attribute \p entry to \p buffer in little-endian
	byte order.  Will not write more than \p bufsize bytes.  Used to build
	ZCL frames.

	@retval	-ZCL_STATUS_INSUFFICIENT_SPACE	buffer too small to encode value
	@retval	-ZCL_STATUS_FAILURE					unknown/unsupported attribute type
	@retval	-ZCL_STATUS_HARDWARE_FAILURE		failure updating attribute value
	@retval	-ZCL_STATUS_SOFTWARE_FAILURE		failure updating attribute value
	@retval	>=0	number of bytes written

	@sa zcl_encode_attribute_value
*/
zigbee_zcl_debug
int _zcl_encode_array_value( uint8_t FAR *buffer, int16_t bufsize,
	const zcl_array_t FAR *array)
{
	uint16_t		element_count;
	zcl_attribute_base_t FAR fake_attr = { 0 };
	int			written = 0;
	int			encode;

	// even an array of 0 elements requires 3 bytes to encode
	if (bufsize < 3)
	{
		return -ZCL_STATUS_INSUFFICIENT_SPACE;
	}

	buffer[0] = fake_attr.type = array->type;

	element_count = array->current_count;
	buffer[1] = element_count & 0xFF;
	buffer[2] = element_count >> 8;
	written = 3;

	fake_attr.value = array->value;
	// Note that fake_attr.type remains the same, but fake_attr.value changes
	// as we walk the elements of the array to encode.  The other fields of
	// fake_attr remain initialized to 0.
	while (element_count--)
	{
		encode = zcl_encode_attribute_value( &buffer[written], bufsize - written,
			&fake_attr);
		if (encode < 0)
		{
			// error encoding attribute (typ. INSUFFICIENT_SPACE)
			return encode;
		}
		written += encode;		// add to count of bytes encoded into buffer

		// point to next attribute value
		fake_attr.value = (const uint8_t FAR *)fake_attr.value + array->element_size;
	}

	return written;
}

/**
	@brief
	Format a ZCL attribute's value for a Read Attributes Response or a Write
	Attributes Request.

	Copies the value of attribute \p entry to \p buffer in little-endian
	byte order.  Will not write more than \p bufsize bytes.  Used to build
	ZCL frames.

	@param[out]	buffer	buffer to store encoded attribute
	@param[in]	bufsize	number of bytes available in \p buffer
	@param[in]	entry		attribute to encode into \p buffer

	@retval	-ZCL_STATUS_SOFTWARE_FAILURE		invalid parameter passed in
	@retval	-ZCL_STATUS_DEFINED_OUT_OF_BAND	ghost attribute with NULL \c value
	@retval	-ZCL_STATUS_INSUFFICIENT_SPACE	buffer too small to encode value
	@retval	-ZCL_STATUS_FAILURE					unknown/unsupported attribute type
	@retval	-ZCL_STATUS_HARDWARE_FAILURE		failure updating attribute value
	@retval	-ZCL_STATUS_SOFTWARE_FAILURE		failure updating attribute value
	@retval	>=0	number of bytes written
*/
zigbee_zcl_debug
int zcl_encode_attribute_value( uint8_t FAR *buffer,
	int16_t bufsize, const zcl_attribute_base_t FAR *entry)
{
	uint8_t	FAR *end, *start;
	uint8_t	FAR *p;
	uint8_t	type;
	uint8_t	status = ZCL_STATUS_SUCCESS;
	int		sizeof_type;
	const zcl_attribute_full_t FAR *full;

	if (entry == NULL || buffer == NULL)
	{
		return -ZCL_STATUS_SOFTWARE_FAILURE;
	}

	type = entry->type;
	sizeof_type = zcl_sizeof_type( type);
	if (sizeof_type == 0)
	{
		return 0;			// nothing to encode
	}

	if (entry->value == NULL)
	{
		return -ZCL_STATUS_DEFINED_OUT_OF_BAND;
	}

	// Call associated .read function if present so it can update the
	// underlying variable if necessary (e.g., read status of digital input).
	full = (const zcl_attribute_full_t FAR *) entry;
	if ((entry->flags & ZCL_ATTRIB_FLAG_FULL) && full->read)
	{
		status = full->read( full);
		if (status != ZCL_STATUS_SUCCESS)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: attribute's read_fn failed (0x%02x)\n",
					__FUNCTION__, status);
			#endif

			return -(int)status;
		}
	}

	if (type == ZCL_TYPE_ARRAY || type == ZCL_TYPE_SET || type == ZCL_TYPE_BAG)
	{
		return _zcl_encode_array_value( buffer, bufsize, entry->value);
	}
	else if (type == ZCL_TYPE_STRUCT)
	{
		return _zcl_encode_struct_value( buffer, bufsize, entry->value);
	}

	if (sizeof_type >= 0)
	{
		if (bufsize < sizeof_type)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: out of space (need %d, have %d)\n",
					__FUNCTION__, sizeof_type, bufsize);
			#endif
			return -ZCL_STATUS_INSUFFICIENT_SPACE;
		}
		if (entry->flags & ZCL_ATTRIB_FLAG_RAW)
		{
			_f_memcpy( buffer, entry->value, sizeof_type);
			return sizeof_type;
		}
	}

	start = buffer;
	end = start + bufsize;
	switch (type)
	{
		case ZCL_TYPE_LOGICAL_BOOLEAN:
			// special case -- convert non-zero to ZCL_BOOL_TRUE and zero to
			// ZCL_BOOL_FALSE
			*buffer++ = *(uint8_t FAR *) entry->value
															? ZCL_BOOL_TRUE : ZCL_BOOL_FALSE;
			break;

		case ZCL_TYPE_STRING_CHAR:
			// copy the string if there's space, keep track of length and
			// write that out to the first byte.
			p = (uint8_t FAR *) entry->value;
			while (++buffer < end && *p)
			{
				*buffer = *p;
				++p;
			}
			if (*p)
			{
				#ifdef ZIGBEE_ZCL_VERBOSE
					printf( "%s: string too large for buffer\n", __FUNCTION__);
				#endif
				return -ZCL_STATUS_INSUFFICIENT_SPACE;
			}

			// fill in length byte
			*start = (uint8_t) (buffer - start - 1);
			break;

		// special case for 24-bit values on big-endian
		// need to demote from 32-bits on host
		// default case works for 24-bit values on little-endian host
	#if BYTE_ORDER == BIG_ENDIAN
		case ZCL_TYPE_GENERAL_24BIT:
		case ZCL_TYPE_BITMAP_24BIT:
		case ZCL_TYPE_UNSIGNED_24BIT:
		case ZCL_TYPE_SIGNED_24BIT:
			// copy 3 least-significant bytes (starts at second byte of value)
			// while reversing byte order (big-endian to little-endian).
			memcpy_htole( buffer, (uint8_t FAR *) entry->value + 1, 3);
			buffer += 3;
			break;
	#endif

		default:								// covers most types
			if (ZCL_TYPE_IS_INVALID( type))
			{
				// this is a failure on our part -- type used in OUR attribute list
				// that this function doesn't support
				#ifdef ZIGBEE_ZCL_VERBOSE
					printf( "%s: unsupported type 0x%02x\n", __FUNCTION__, type);
				#endif
				return -ZCL_STATUS_FAILURE;
			}
			else if (sizeof_type > 0)
			{
				memcpy_htole( buffer, entry->value, (uint8_t) sizeof_type);
				buffer += sizeof_type;
			}
			break;
	}

	return (int)(buffer - start);
}

/*** BeginHeader _zcl_read_attributes */
int _zcl_read_attributes( zcl_command_t *cmd);
/*** EndHeader */
/**
	@internal
	@brief
	Process the Read Attributes Command (#ZCL_CMD_READ_ATTRIB).

	@param[in]	cmd	command to respond to

	@retval	0			successfully sent response
	@retval	!0			error on send
	@retval	-EINVAL	NULL \p cmd parameter
*/
zigbee_zcl_debug
int _zcl_read_attributes( zcl_command_t *cmd)
{
	uint16_t									requests;
	uint16_t									payload_length;
	uint16_t									attribute;
	uint16_t							FAR	*id_le;
	const zcl_attribute_base_t	FAR	*entry;
	PACKED_STRUCT {
		zcl_header_response_t			header;
		uint8_t								buffer[80];		// limit to how many bytes?
	} response;
	uint8_t									*start_response;
	uint8_t									*end_response;
	int										bytesleft;
	int										byteswritten;

	if (cmd == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	// DEVNOTE: This could be an assert, or only compiled in for debug mode.
	//				If request has odd byte, we'll ignore it.
	payload_length = cmd->length;
	if (payload_length & 0x0001)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: ERROR -- length (%u) is not even\n", __FUNCTION__,
				payload_length);
		#endif
		return zcl_default_response( cmd, ZCL_STATUS_MALFORMED_COMMAND);
	}

	response.header.command = ZCL_CMD_READ_ATTRIB_RESP;
	start_response = (uint8_t *)&response
		+ zcl_build_header( &response.header, cmd);
	end_response = response.buffer;
	// can't go beyond (sizeof response + (uint8_t *) &response)

	requests = payload_length >> 1;			// 2 bytes per request
	for (id_le = (uint16_t FAR *) cmd->zcl_payload; requests;
		--requests, ++id_le)
	{
		bytesleft = (uint8_t *)(&response + 1) - end_response;

		if (bytesleft < 3)
		{
			// not enough room for attribute id and status
			break;
		}

		// copy ID to response (still little-endian) and convert to host-order
		attribute = le16toh( *(uint16_t *)end_response = *id_le );
		end_response += 2;
		bytesleft -= 2;
		entry = zcl_find_attribute( cmd->attributes, attribute);
		if (entry == NULL)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: couldn't find attribute 0x%04x\n", __FUNCTION__,
					attribute);
			#endif
			*end_response++ = ZCL_STATUS_UNSUPPORTED_ATTRIBUTE;
		}
		else if (entry->flags & ZCL_ATTRIB_FLAG_WRITEONLY)
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: attribute 0x%04x is write-only\n", __FUNCTION__,
					attribute);
			#endif
			*end_response++ = ZCL_STATUS_WRITE_ONLY;
		}
		else
		{
			#ifdef ZIGBEE_ZCL_VERBOSE
				printf( "%s: found attribute 0x%04x @ index %u\n", __FUNCTION__,
					attribute, (unsigned) (entry - cmd->attributes));
			#endif

			// write the attribute value after the status and type fields of
			// the response -- +2 to skip fields, -2 to reduce space available
			byteswritten = zcl_encode_attribute_value( end_response + 2,
																		bytesleft - 2, entry);
			if (byteswritten < 0)
			{
				// byteswritten is a negative ZCL_STATUS_* macro
				*end_response++ = (uint8_t) -byteswritten;
				break;
			}

			// fill in the status and attribute type
			*end_response++ = ZCL_STATUS_SUCCESS;
			*end_response++ = entry->type;
			end_response += byteswritten;
		}
	}

	return zcl_send_response( cmd, start_response,
																end_response - start_response);
}


/*** BeginHeader _zcl_discover_attributes */
int _zcl_discover_attributes( zcl_command_t *cmd);
/*** EndHeader */
#define ZCL_DISCOVER_ATTRIB_MAX		20
/**
	@internal @brief
	Process the Discover Attributes Command (#ZCL_CMD_DISCOVER_ATTRIB).

	@param[in]	cmd	command to respond to

	@retval	0			successfully sent response
	@retval	!0			error on send
*/
zigbee_zcl_debug
int _zcl_discover_attributes( zcl_command_t *cmd)
{
	const zcl_discover_attrib_t	FAR	*discover;
	PACKED_STRUCT {
		zcl_header_response_t				header;
		uint8_t									complete;
		zcl_rec_attrib_report_t				attribs[ZCL_DISCOVER_ATTRIB_MAX];
	} response;
	uint8_t										*start_response;
	zcl_rec_attrib_report_t 				*write_attrib;
	const zcl_attribute_base_t		FAR	*attribute;

	uint16_t start_attrib_id;
	uint_fast8_t remaining;

	if (cmd == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	discover = cmd->zcl_payload;
	#ifdef ZIGBEE_ZCL_VERBOSE
		printf( "%s: --- discover attributes, start @ 0x%04x, return %u max\n",
			__FUNCTION__, le16toh( discover->start_attrib_id_le),
			discover->max_return_count);
	#endif

	start_response = (uint8_t *)&response
		+ zcl_build_header( &response.header, cmd);
	response.header.command = ZCL_CMD_DISCOVER_ATTRIB_RESP;
	write_attrib = response.attribs;
   attribute = cmd->attributes;
	if (! attribute)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: cluster has no attributes\n", __FUNCTION__);
		#endif
		response.complete = ZCL_BOOL_TRUE;			// no attributes to send
	}
	else
	{
		// since end of list ID 0xFFFF, we'll always stop when we hit it
		start_attrib_id = le16toh( discover->start_attrib_id_le);
		while (attribute->id < start_attrib_id)
		{
			attribute = zcl_attribute_get_next( attribute);
		}

		// limit response to minimum of requested amount or our buffer size
		remaining = discover->max_return_count;
		if (remaining > ZCL_DISCOVER_ATTRIB_MAX)
		{
			remaining = ZCL_DISCOVER_ATTRIB_MAX;
		}

		// Odd loop construct (with two breaks) allows for setting "complete"
		// field to TRUE, even if we've run out of room or requestor doesn't want
		// more attributes, or our attribute list was empty.
		response.complete = ZCL_BOOL_FALSE;			// default to incomplete
		for (;;)
		{
			// see if there are any more attributes to send
			if (attribute->id == ZCL_ATTRIBUTE_END_OF_LIST)
			{
				response.complete = ZCL_BOOL_TRUE;
				break;
			}
			// Break out of loop if we're out of space, or filled the request.
			// Leave response.complete set to FALSE since there's more
			// attributes left.
			if (! remaining--)
			{
				break;
			}
			write_attrib->id_le = htole16( attribute->id);
			write_attrib->type = attribute->type;
			++write_attrib;
			attribute = zcl_attribute_get_next( attribute);
		}
	}

	return zcl_send_response( cmd, start_response,
										(uint8_t *)write_attrib - start_response);
}

/*** BeginHeader zcl_command_build */
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C4000		// Condition always TRUE is OK
#endif
/**
	@brief
	Parse a ZCL request and store in a zcl_command_t structure with fields
	in fixed locations (therefore easier to use than the variable-length
	frame header).

	Also uses the direction bit and manufacturer ID to search the attribute
	tree for the correct list of attributes.

	@param[out]	cmd		buffer to store parsed ZCL command
	@param[in]	envelope	envelope from received message
	@param[in]	tree		pointer to attribute tree for cluster or NULL if
								the cluster doesn't have any attributes
								(typically passed in via endpoint dispatcher)

	@retval	0			parsed payload from \p envelope into \p cmd
	@retval	-EINVAL	invalid parameter passed to function
	@retval	-EBADMSG	frame is too small for ZCL header's frame_control value
*/
zigbee_zcl_debug
int zcl_command_build( zcl_command_t *cmd,
	const wpan_envelope_t FAR *envelope, zcl_attribute_tree_t FAR *tree)
{
	const zcl_header_t			FAR	*header;
	const zcl_header_common_t	FAR	*common;
	zcl_command_t							command;

	if (cmd == NULL || envelope == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	header = envelope->payload;
	command.envelope = envelope;
	command.frame_control = header->frame_control;
	if (command.frame_control & ZCL_FRAME_MFG_SPECIFIC)
	{
		common = &header->type.mfg.common;
		command.mfg_code = le16toh( header->type.mfg.mfg_code_le);
		command.length = (int16_t) envelope->length - 5;
	}
	else
	{
		common = &header->type.std.common;
		command.mfg_code = ZCL_MFG_NONE;
		command.length = (int16_t) envelope->length - 3;
	}

	if (command.length < 0)
	{
		memset( cmd, 0, sizeof *cmd);
		return -EBADMSG;
	}

	command.sequence = common->sequence;
	command.command = common->command;
	command.zcl_payload = &common->payload;

	command.attributes = NULL;
	if (tree)
	{
		for (;;)
		{
			if (tree->manufacturer_id == command.mfg_code)
			{
				// found the attributes for the manufacturer ID
				if ((command.frame_control & ZCL_FRAME_DIRECTION)
															== ZCL_FRAME_CLIENT_TO_SERVER)
				{
					command.attributes = tree->server;
				}
				else
				{
					command.attributes = tree->client;
				}
				break;
			}
			else if (tree->manufacturer_id == ZCL_MFG_NONE)
			{
				// this is the last branch of the tree
				break;
			}
			++tree;
		}
	}

	*cmd = command;
	return 0;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C4000		// restore C4000 (Condition always TRUE)
#endif


/*** BeginHeader zcl_command_dump */
/*** EndHeader */
/**
	@brief Debugging routine that dumps contents of a parsed ZCL command
			structure to STDOUT.

	@param[in]	cmd	zcl_command_t structure populated by zcl_command_build()
*/
zigbee_zcl_debug
void zcl_command_dump( const zcl_command_t *cmd)
{
	uint8_t frame_control;
	const uint8_t FAR *b;
	int length;
	const char *zcl_frame_type[] =
		{ "profile", "cluster", "reserved1", "reserved2" };
	const char *zcl_general_cmd[] =
		{	"read_attrib", "read_attrib_resp", "write_attrib",
			"write_attrib_undiv", "write_attrib_resp", "write_attrib_noresp",
			"configure_report", "configure_report_resp", "read_report_cfg",
			"read_report_cfg_resp", "report_attrib", "default_resp",
			"discover_attrib", "discover_attrib_resp", "read_struct_attrib",
			"write_struct_attrib", "write_struct_attrib_resp" };

	if (cmd == NULL)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			puts( "can't dump NULL");
		#endif
		return;
	}

	frame_control = cmd->frame_control;
	printf( "%s command, %s, ",
		zcl_frame_type[frame_control & ZCL_FRAME_TYPE_MASK],
		(frame_control & ZCL_FRAME_DIRECTION) == ZCL_FRAME_CLIENT_TO_SERVER ?
		"client-to-server" : "server-to-client");
	if (frame_control & ZCL_FRAME_DISABLE_DEF_RESP)
	{
		printf( "disable default response, ");
	}

	if (cmd->mfg_code)
	{
		printf( "MFG 0x%04X", cmd->mfg_code);
	}

	printf( "\nsequence = 0x%02X, command = 0x%02X",
		cmd->sequence, cmd->command);

	if ((frame_control & ZCL_FRAME_TYPE_MASK) == ZCL_FRAME_TYPE_PROFILE
		&& cmd->command < _TABLE_ENTRIES( zcl_general_cmd))
	{
		printf( " (%s)", zcl_general_cmd[cmd->command]);
		b = cmd->zcl_payload;
		length = cmd->length;

		// parse certain general commands
		switch (cmd->command)
		{
			case ZCL_CMD_DEFAULT_RESP:
				if (length == 2)
				{
					puts( "");
					printf( "\tCommand 0x%02X: %s\n", b[0], zcl_status_text( b[1]));
					return;
				}
				break;

			case ZCL_CMD_WRITE_ATTRIB_RESP:
				if (length == 1 && *b == ZCL_STATUS_SUCCESS)
				{
					puts( " SUCCESS");
				}
				else
				{
					puts( " with errors:");
					while (length >= 3)
					{
						printf( "\tattribute 0x%02X%02X: %s\n", b[2], b[1],
							zcl_status_text( b[0]));
						length -= 3;
						b += 3;
					}
					if (length == 2)
					{
						printf( "\tunmatched bytes 0x%02X 0x%02X\n", b[0], b[1]);
					}
					else if (length == 1)
					{
						printf( "\tunmatched byte 0x%02X\n", b[0]);
					}
				}
				return;
		}
	}

	printf( " with %u-byte payload:\n", cmd->length);
	hex_dump( cmd->zcl_payload, cmd->length, HEX_DUMP_FLAG_TAB);
}


/*** BeginHeader zcl_envelope_dump */
/*** EndHeader */
/**
	@brief Debugging routine that parses an envelope payload as a ZCL command
			and dumps a parsed copy to STDOUT.

	@param[in]	envelope	wpan_envelope_t structure with a ZCL command payload
*/
void zcl_envelope_payload_dump( const wpan_envelope_t *envelope)
{
	zcl_command_t zcl;
	int err;

	if (envelope == NULL)
	{
		printf( "%s: NULL envelope passed\n", __FUNCTION__);
		return;
	}

	err = zcl_command_build( &zcl, envelope, NULL);
	if (err)
	{
		printf( "%s: zcl_command_dump() returned %d for %u-byte payload\n",
			__FUNCTION__, err, envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_OFFSET);
		return;
	}

	zcl_command_dump( &zcl);
}


/*** BeginHeader zcl_invalid_cluster */
/*** EndHeader */
/**
	@brief
	Called if a request comes in for an invalid endpoint/cluster combination.

	Sends a Default Response (#ZCL_CMD_DEFAULT_RESP) of #ZCL_STATUS_FAILURE
	unless the received command was a Default Response or was broadcast.

	@param[in]	envelope	envelope from received message
	@param[in]	ep_state	pointer to endpoint's wpan_ep_state_t

	@retval	0	command was a default response (and therefore ignored),
					or a default response was successfully sent
	@retval	!0	error sending response
*/
zigbee_zcl_debug
int zcl_invalid_cluster( const wpan_envelope_t FAR *envelope,
	wpan_ep_state_t FAR *ep_state)
{
	zcl_command_t							zcl;
	int										error;

	// endpoint handlers follow specific API, this one doesn't use ep_state
	XBEE_UNUSED_PARAMETER( ep_state);

	// note that zcl_command_build will check for NULL envelope

	error = zcl_command_build( &zcl, envelope, NULL);
	if (error == 0)
	{
		error = zcl_default_response( &zcl, ZCL_STATUS_FAILURE);
	}

	#ifdef ZIGBEE_ZCL_VERBOSE
		printf( "%s: returning %d\n", __FUNCTION__, error);
	#endif
	return error;
}

/*** BeginHeader zcl_invalid_command */
/*** EndHeader */
/**
	@brief
	Called if a request comes in for an invalid command on a valid
	endpoint/cluster combination.

	Sends a Default Response (#ZCL_CMD_DEFAULT_RESP) unless the received
	command was a Default Response or was broadcast.  Depending on the
	command received, the response will be one of:
		-	ZCL_STATUS_UNSUP_CLUSTER_COMMAND
		-	ZCL_STATUS_UNSUP_GENERAL_COMMAND
		-	ZCL_STATUS_UNSUP_MANUF_CLUSTER_COMMAND
		-	ZCL_STATUS_UNSUP_MANUF_GENERAL_COMMAND

	@param[in]	envelope	envelope from received message

	@retval	0	default response was successfully sent, or command did not
					require a default response
	@retval	!0	error building or sending response
*/
zigbee_zcl_debug
int zcl_invalid_command( const wpan_envelope_t FAR *envelope)
{
	uint8_t									status;
	zcl_command_t							zcl;
	int										error;

	// note that zcl_command_build checks for NULL envelope
	error = zcl_command_build( &zcl, envelope, NULL);
	if (error == 0)
	{
		status = (zcl.frame_control & ZCL_FRAME_MFG_SPECIFIC)
			? ZCL_STATUS_UNSUP_MANUF_GENERAL_COMMAND
			: ZCL_STATUS_UNSUP_GENERAL_COMMAND;
		if ((zcl.frame_control & ZCL_FRAME_TYPE_MASK) == ZCL_FRAME_TYPE_CLUSTER)
		{
			// cluster-specific error codes are one less than general error codes
			--status;
		}

		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: status 0x%02x for invalid cluster 0x%04x/cmd 0x%02x\n",
				__FUNCTION__, status, envelope->cluster_id, zcl.command);
		#endif

		error = zcl_default_response( &zcl, status);
	}

	#ifdef ZIGBEE_ZCL_VERBOSE
		printf( "%s: returning %d\n", __FUNCTION__, error);
	#endif
	return error;
}

/*** BeginHeader zcl_general_command */
/*** EndHeader */
/**
	@brief
	Handler for ZCL General Commands.

	Used as the handler for attribute-only
	clusters (e.g., Basic Cluster), or called from a cluster's handler
	for commands it doesn't handle.

	Will send a Default Response for commands it can't handle.

	Currently does not support attribute reporting.

	@param[in]	envelope	envelope from received message
	@param[in]	context	pointer to attribute tree for cluster or NULL if
								the cluster doesn't have any attributes
								(typically passed in via endpoint dispatcher)

	@retval	0	command was processed, including sending a possible response
	@retval	!0	error sending response or processing command
*/
zigbee_zcl_debug
int zcl_general_command( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	zcl_command_t							zcl;
	int										conversation;
	int										error;

	error = zcl_command_build( &zcl, envelope, context);
	if (error != 0)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: returning %d\n", __FUNCTION__, error);
		#endif
		return error;
	}

	if ((zcl.frame_control & ZCL_FRAME_TYPE_MASK) != ZCL_FRAME_TYPE_PROFILE)
	{
		#ifdef ZIGBEE_ZCL_VERBOSE
			printf( "%s: ERROR, non-profile frame type on %u-byte command\n",
				__FUNCTION__, envelope->length);
			hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
		#endif

		return zcl_invalid_command( envelope);
	}

	#ifdef ZIGBEE_ZCL_VERBOSE
		printf( "%s: processing %u-byte GENERAL command\n",
			__FUNCTION__, envelope->length);
		hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	#endif

	/*
		Note regarding manufacturer-specific flag.  On general commands
		(where the frame type is PROFILE), the flag indicates manufacturer-
		specific ATTRIBUTES, not COMMANDS.

		The zcl_command_build() function will select the correct attributes
		branch of the attribute tree, based on the manufacturer ID of the
		request.
	*/

	switch (zcl.command)
	{
		case ZCL_CMD_DISCOVER_ATTRIB:
			return _zcl_discover_attributes( &zcl);

		case ZCL_CMD_READ_ATTRIB:
			return _zcl_read_attributes( &zcl);

		case ZCL_CMD_WRITE_ATTRIB:
		case ZCL_CMD_WRITE_ATTRIB_UNDIV:
		case ZCL_CMD_WRITE_ATTRIB_NORESP:
			return _zcl_write_attributes( &zcl);

		case ZCL_CMD_DEFAULT_RESP:
			#ifdef ZIGBEE_ZCL_VERBOSE
				{
					const zcl_default_response_t FAR *resp = zcl.zcl_payload;
					printf( "%s: got default response (%s) for command 0x%02x\n",
						__FUNCTION__, zcl_status_text( resp->status), resp->command);
				}
			#endif
			// need to match transaction ID in conversation list
			// fall through to other response handlers
		case ZCL_CMD_READ_ATTRIB_RESP:
		case ZCL_CMD_WRITE_ATTRIB_RESP:
		case ZCL_CMD_DISCOVER_ATTRIB_RESP:
		case ZCL_CMD_WRITE_STRUCT_ATTRIB_RESP:
			conversation = wpan_conversation_response( NULL, zcl.sequence,
																envelope);
			if (conversation >= 0)
			{
				// successfully dispatched to an existing conversation
				return conversation;
			}
			break;			// exit to default response

//		case ZCL_CMD_READ_STRUCT_ATTRIB:
//		case ZCL_CMD_WRITE_STRUCT_ATTRIB:
			// reading/writing of structured attributes (arrays, structures,
			// sets, bags) not implemented
	}

	return zcl_invalid_command( envelope);
}

/*** BeginHeader zcl_status_text */
/*** EndHeader */
/**
	@brief
	Converts a ZCL status byte (one of the ZCL_STATUS_* macros) into a string.

	@param[in]	status	status byte to convert

	@return pointer to an unmodifiable string with a description of the status
*/
zigbee_zcl_debug
const char *zcl_status_text( uint_fast8_t status)
{
	static char unknown[15];

	switch (status)
	{
		case ZCL_STATUS_SUCCESS:				return "Success";
		case ZCL_STATUS_FAILURE:				return "Failure";
		case ZCL_STATUS_NOT_AUTHORIZED:		return "Not Authorized";
		case ZCL_STATUS_RESERVED_FIELD_NOT_ZERO:
														return "Rerserved Field Not Zero";
		case ZCL_STATUS_MALFORMED_COMMAND:	return "Malformed Command";
		case ZCL_STATUS_UNSUP_CLUSTER_COMMAND:
														return "Unsupported Cluster Command";
		case ZCL_STATUS_UNSUP_GENERAL_COMMAND:
														return "Unsupported General Command";
		case ZCL_STATUS_UNSUP_MANUF_CLUSTER_COMMAND:
														return "Unsup. Mfg Cluster Command";
		case ZCL_STATUS_UNSUP_MANUF_GENERAL_COMMAND:
														return "Unsup. Mfg General Command";
		case ZCL_STATUS_INVALID_FIELD:		return "Invalid Field";
		case ZCL_STATUS_UNSUPPORTED_ATTRIBUTE:
														return "Unsupported Attribute";
		case ZCL_STATUS_INVALID_VALUE:		return "Invalid Value";
		case ZCL_STATUS_READ_ONLY:				return "Read-only";
		case ZCL_STATUS_INSUFFICIENT_SPACE:	return "Insufficient Space";
		case ZCL_STATUS_DUPLICATE_EXISTS:	return "Duplicate Exists";
		case ZCL_STATUS_NOT_FOUND:				return "Not Found";
		case ZCL_STATUS_UNREPORTABLE_ATTRIBUTE:
														return "Unreportable Attribute";
		case ZCL_STATUS_INVALID_DATA_TYPE:	return "Invalid Data Type";
		case ZCL_STATUS_INVALID_SELECTOR:	return "Invalid Selector";
		case ZCL_STATUS_WRITE_ONLY:			return "Write-only";
		case ZCL_STATUS_INCONSISTENT_STARTUP_STATE:
														return "Inconsistent Startup State";
		case ZCL_STATUS_DEFINED_OUT_OF_BAND:
														return "Defined Out Of Band";
		case ZCL_STATUS_HARDWARE_FAILURE:	return "Hardware Failure";
		case ZCL_STATUS_SOFTWARE_FAILURE:	return "Software Failure";
		case ZCL_STATUS_CALIBRATION_ERROR:	return "Calibration Error";

		default:
			sprintf( unknown, "Unknown (0x%02x)", status);
			return unknown;
	}
}
