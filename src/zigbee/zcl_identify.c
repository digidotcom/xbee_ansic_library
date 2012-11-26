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
	@addtogroup zcl_identify
	@{
	@file zcl_identify.c
	Server code to implement the ZigBee Identify Cluster (ZCL Spec section 3.5).

	Main program should call zcl_identify_isactive() in its main loop and
	perform some sort of action (flash an LED or LCD backlight) to help an
	installer identify the device during network commissioning.

	Use the macro #ZCL_CLUST_ENTRY_IDENTIFY_SERVER to add an Identify Cluster
	Server to the cluster list of an endpoint.

	Call zcl_identify_isactive() in the main loop of your program to see how
	many seconds are left in "Identification mode".

	@note Maybe generalize the attribute read/write functions to use
				attribute->value (passed in) instead of referencing
				zcl_identify_time directly?  Maybe not, makes it harder for
				other parts of the system to call _zcl_identify_time_set() and
				_zcl_identify_time_get().
*/

/*** BeginHeader */

#include "xbee/platform.h"
#ifdef XBEE_ZCL_VERBOSE
	#include <stdio.h>
#endif

#include "xbee/byteorder.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_identify.h"
/*** EndHeader */

/*** BeginHeader zcl_identify_time, zcl_identify_end, zcl_identify_isactive */
extern uint16_t zcl_identify_time;
extern uint32_t zcl_identify_end;
/*** EndHeader */
/// Private variable for tracking amount of "Identification mode" time left.
uint16_t		zcl_identify_time = 0;
/// Private variable for tracking value of xbee_seconds_timer() that
/// "Idenfication mode" should end.
uint32_t		zcl_identify_end = 0;

/**
	@brief
	Used by main program to see if a device is in "Identification mode".

	@retval	>0	Number of seconds of "Identification mode" left.
	@retval	0	Device is not in "Identification mode".
*/
uint16_t zcl_identify_isactive( void)
{
	int32_t delta;

	if (zcl_identify_time)
	{
		delta = (int32_t)(zcl_identify_end - xbee_seconds_timer());
		if (delta <= 0)
		{
			zcl_identify_time = 0;
		}
		else
		{
			zcl_identify_time = (uint16_t) delta;
		}
	}

	return zcl_identify_time;
}

/*** BeginHeader _zcl_identify_time_set */
int _zcl_identify_time_set( const zcl_attribute_full_t FAR *attribute,
	zcl_attribute_write_rec_t *rec);
/*** EndHeader */
/**
	@brief
	Function registered to #ZCL_IDENTIFY_ATTR_IDENTIFY_TIME attribute of
	Identify Cluster.

	Responsible for updating the IdentifyTime attribute, and also for
	determining when identify mode should end.

	@param[in]		attribute	ignored; assumed to point to zcl_identify_time
	@param[in,out]	rec			value from write request or NULL to update the
										time when identify mode should end (used by the
										Identify cluster command).

	See zcl_attribute_write_fn() for calling convention.
*/
int _zcl_identify_time_set( const zcl_attribute_full_t FAR *attribute,
	zcl_attribute_write_rec_t *rec)
{
	int16_t bytes_read = 0;

	if (rec)			// this is a Write Attributes Request
	{
		bytes_read = zcl_decode_attribute( &attribute->base, rec);
		if (! (rec->flags & ZCL_ATTR_WRITE_FLAG_ASSIGN))
		{
			return bytes_read;
		}
	}

	zcl_identify_end = xbee_seconds_timer() + zcl_identify_time;

	return bytes_read;
}

/*** BeginHeader _zcl_identify_time_get */
uint_fast8_t _zcl_identify_time_get( const zcl_attribute_full_t FAR *attribute);
/*** EndHeader */
/**
	@internal
	@brief
	Function registered to #ZCL_IDENTIFY_ATTR_IDENTIFY_TIME attribute of
	Identify Cluster and called to refresh the IdentifyTime attribute.

	See zcl_attribute_update_fn() for calling convention.
*/
uint_fast8_t _zcl_identify_time_get( const zcl_attribute_full_t FAR *attribute)
{
	// zcl_attribute_update_fn API, but does not use 'attribute' parameter
	// zcl_identify_time variable is global for all Identify server clusters
	XBEE_UNUSED_PARAMETER( attribute);

	zcl_identify_isactive();		// updates zcl_identify_time
	return ZCL_STATUS_SUCCESS;
}

/*** BeginHeader zcl_identify_attribute_tree */
/*** EndHeader */
/// Standard attribute list for the Identify Server Cluster.
const struct {
	zcl_attribute_full_t			identify_time;
	uint16_t							end_of_list;
} zcl_identify_attr =
{
//	  ID, Flags, Type, Address to data, min, max, read, write
	{ { ZCL_IDENTIFY_ATTR_IDENTIFY_TIME,
		ZCL_ATTRIB_FLAG_FULL,
		ZCL_TYPE_UNSIGNED_16BIT,
		&zcl_identify_time },
		{ 0 }, { 0 },
		&_zcl_identify_time_get, &_zcl_identify_time_set },
	ZCL_ATTRIBUTE_END_OF_LIST
};

const zcl_attribute_tree_t zcl_identify_attribute_tree[] =
		{ { ZCL_MFG_NONE, &zcl_identify_attr.identify_time.base, NULL } };

/*** BeginHeader zcl_identify_command */
/*** EndHeader */
/**
	@brief
	Handler for ZCL Identify Server Commands (Identify, IdentifyQuery).

	Used in the #ZCL_CLUST_IDENTIFY cluster entry for an endpoint.

	@param[in]	envelope	envelope from received message
	@param[in]	context	pointer to attribute list for cluster
								(typically passed in via endpoint dispatcher)

	@retval	0	command was processed, including sending a possible
					Identify Query Response
	@retval	!0	error sending response or processing command
*/
int zcl_identify_command( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	zcl_command_t							zcl;

	// Make sure frame is client-to-server, not manufacturer-specific and
	// a cluster command (not "profile-wide").
	if (zcl_command_build( &zcl, envelope, context) == 0 &&
		ZCL_CMD_MATCH( &zcl.frame_control, GENERAL, CLIENT_TO_SERVER, CLUSTER))
	{
		// commands sent to the Identify Server Cluster
		// only use the stack if this isn't a general command
		PACKED_STRUCT {
			zcl_header_response_t			header;
			uint16_t								timeout;
		} response;
		int offset;

		#ifdef XBEE_ZCL_VERBOSE
			printf( "%s: processing %u-byte IDENTIFY command\n", __FUNCTION__,
				envelope->length);
			hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
		#endif


		switch (zcl.command)
		{
			case ZCL_IDENTIFY_CMD_IDENTIFY:
				if (zcl.length >= 2)
				{
					zcl_identify_time = le16toh(
											*(uint16_t FAR *) zcl.zcl_payload);
					_zcl_identify_time_set( NULL, NULL);

					return zcl_default_response( &zcl, ZCL_STATUS_SUCCESS);
				}

				#ifdef XBEE_ZCL_VERBOSE
					printf( "%s: bad IDENTIFY packet?  length < 2\n",
						__FUNCTION__);
				#endif
				return zcl_default_response( &zcl, ZCL_STATUS_MALFORMED_COMMAND);

			case ZCL_IDENTIFY_CMD_QUERY:
				// Ignore payload -- future versions of this command may have
				// parameters.
				_zcl_identify_time_get( NULL);
				if (zcl_identify_time)
				{
					response.header.command = ZCL_IDENTIFY_CMD_QUERY_RESPONSE;
					offset = zcl_build_header( &response.header, &zcl);
					response.timeout = htole16( zcl_identify_time);
					return zcl_send_response( &zcl, (uint8_t *)&response + offset,
																sizeof(response) - offset);
				}

				return zcl_default_response( &zcl, ZCL_STATUS_SUCCESS);
		}
	}

	// Command not handled by this function, pass to General Command Handler.
	return zcl_general_command( envelope, context);
}
