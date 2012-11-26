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
	@addtogroup xbee_commissioning
	@{
	@file xbee_commissioning.c

	XBee-specific hardware support for ZCL Commissioning Cluster.
*/

/*** BeginHeader */
#include <stdio.h>

#include "xbee/platform.h"
#include "zigbee/zcl_commissioning.h"
#include "xbee/atcmd.h"
#include "xbee/commissioning.h"
/*** EndHeader */

/*** BeginHeader xbee_commissioning_get */
/*** EndHeader */
typedef void (*xbee_comm_fn)(
	zcl_comm_startup_param_t FAR *p,
	const void FAR *value_be
);

typedef struct xbee_comm_reg_t {
	xbee_at_cmd_t	command;				///< command to send to XBee device
	xbee_comm_fn	assign_fn;			///< function to process result
} xbee_comm_reg_t;

/**	@todo
	Depending on the value of StartupControl, the extended_panid attribute
	might be set from ID (extended PAN ID) or OP (operating PAN ID).

	I think.

	Note that spec (3.15.2.2.1.2) says that all Fs is used for "unspecified",
	so we may need to map all 0s to all Fs.
*/

void xbee_comm_SC( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	// Bit 0 of ATSC is channel 11, so shift to correct position in 32-bit mask.
	p->channel_mask = (uint32_t) be16toh( *(uint16_t FAR *)value_be) << 11;
}

void xbee_comm_OP( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	if (memcheck( value_be, 0x00, 8) == 0)
	{
		// per 3.15.2.2.1.2, "unspecified" should be all Fs instead of 0
		_f_memset( &p->extended_panid, 0xFF, 8);
	}
	else
	{
		memcpy_betoh( &p->extended_panid, value_be, 8);
	}
}

void xbee_comm_OI( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	p->panid = be16toh( *(uint16_t FAR *) value_be);
}

#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
void xbee_comm_AR( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	uint8_t ar;

	ar = *(const uint8_t FAR *)value_be;
	// This node is a concentrator if AR != 0xFF.  If it's a concentrator,
	// AR is the discovery time.
	if ( (p->concentrator.flag = (ar != 0xFF)) )
	{
		p->concentrator.discovery_time = ar;
	}
	else
	{
		// discovery time is unknown
		p->concentrator.discovery_time = 0;
	}
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

void xbee_comm_BH( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	uint8_t bh;

	// If BH (broadcast hops) is 0, use maximum radius (0x1E)
	bh = *(const uint8_t FAR *)value_be;
	p->concentrator.radius = (bh == 0) ? 0x1E : bh;
}

// use DL to store misc. parameters
typedef struct xbee_comm_dl_t {
	uint8_t		b0;
	uint8_t		b1;
	uint8_t		b2;
	uint8_t		startup_control;
} xbee_comm_dl_t;
void xbee_comm_DL( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	const xbee_comm_dl_t FAR *dl = value_be;

	p->startup_control = dl->startup_control;
}

void xbee_comm_EO( zcl_comm_startup_param_t FAR *p, const void FAR *value_be)
{
	p->use_insecure_join = *(uint8_t FAR *)value_be & 0x01;
}

const xbee_comm_reg_t xbee_comm_regs[] = {
	{ { { 'D', 'L' } }, xbee_comm_DL },
	{ { { 'S', 'C' } }, xbee_comm_SC },
	{ { { 'A', 'R' } }, xbee_comm_AR },
	{ { { 'B', 'H' } }, xbee_comm_BH },
	{ { { 'E', 'O' } }, xbee_comm_EO },
	// These values change depending on what network we've joined
	{ { { 'O', 'P' } }, xbee_comm_OP },
	{ { { 'O', 'I' } }, xbee_comm_OI },
};

int xbee_comm_reg_callback( const xbee_cmd_response_t FAR *response)
{
	const xbee_comm_reg_t *reg;
	zcl_comm_state_t FAR *comm = response->context;

	uint_fast8_t	i;
	int16_t			request;
	bool_t			found;
	uint16_t			command;

	if (response->flags & XBEE_CMD_RESP_FLAG_TIMEOUT)
	{
		return XBEE_ATCMD_DONE;			// give up
	}

	request = response->handle;
	command = response->command.w;

	reg = xbee_comm_regs;
	found = FALSE;
	for (i = _TABLE_ENTRIES(xbee_comm_regs); i; ++reg, --i)
	{
		if (found)
		{
			// Found a match on last pass through loop, this is the next register
			// to request.
			xbee_cmd_set_command( response->handle, reg->command.str);
			xbee_cmd_send( request);

			return XBEE_ATCMD_REUSE;			// keep this request alive
		}
		if (command == reg->command.w)
		{
			found = TRUE;
			// found it in the list, copy the response if status is success
			if ((response->flags & XBEE_CMD_RESP_MASK_STATUS)
																	== XBEE_AT_RESP_SUCCESS)
			{
				reg->assign_fn( &comm->sas, response->value_bytes);
			}
		}
	}

	// done with lookups, advance state
	if (comm->state == ZCL_COMM_STATE_LOADING)
	{
		comm->state = ZCL_COMM_STATE_STARTUP;
	}
	else if (comm->state == ZCL_COMM_STATE_REFRESH)
	{
		comm->state = ZCL_COMM_STATE_JOINED;
	}

	// Error or last command, tell dispatcher that we're done and it can delete
	// the request entry.
	return XBEE_ATCMD_DONE;
}

int xbee_commissioning_query( zcl_comm_state_t *comm, xbee_dev_t *xbee,
	const char FAR *reg)
{
	int16_t request;
	int error;

	request = xbee_cmd_create( xbee, reg);
	if (request < 0)
	{
		return request;
	}
	error = xbee_cmd_set_callback( request, xbee_comm_reg_callback, comm);

	return error ? error : xbee_cmd_send( request);
}

// populate zcl_comm_startup_param by reading values from XBee
int xbee_commissioning_init( zcl_comm_state_t *comm, xbee_dev_t *xbee)
{
	// Initialize the startup attributes...
	zcl_comm_sas_init( &comm->sas);

	// ...and set the rest of the structure to 0
	memset( &comm->flags, 0,
		sizeof( zcl_comm_state_t) - offsetof( zcl_comm_state_t, flags));

	comm->sas.scan_attempts = XBEE_COMM_SCAN_ATTEMPTS;
	comm->sas.time_between_scans = XBEE_COMM_TIME_BETWEEN_SCANS;
	comm->sas.rejoin_interval = XBEE_COMM_REJOIN_INTERVAL;
	comm->sas.max_rejoin_interval = XBEE_COMM_MAX_REJOIN_INTERVAL;

	switch ((uint16_t) xbee->firmware_version
											& (XBEE_PROTOCOL_MASK | XBEE_NODETYPE_MASK))
	{
		case XBEE_PROTOCOL_ZB_S2C:											// 40xx
		case XBEE_PROTOCOL_SE_S2C | XBEE_NODETYPE_COORD:			// 51xx
			// S2C hardware can be any node type
			comm->flags = ZCL_COMM_FLAG_COORDINATOR_OK
							| ZCL_COMM_FLAG_ROUTER_OK
							| ZCL_COMM_FLAG_ENDDEV_OK;
			break;

		case XBEE_PROTOCOL_ZB | XBEE_NODETYPE_COORD:					// 21xx
		case XBEE_PROTOCOL_SMARTENERGY | XBEE_NODETYPE_COORD:		// 31xx
			comm->flags = ZCL_COMM_FLAG_COORDINATOR_OK;
			break;

		case XBEE_PROTOCOL_ZB | XBEE_NODETYPE_ROUTER:				// 23xx
		case XBEE_PROTOCOL_SMARTENERGY | XBEE_NODETYPE_ROUTER:	// 33xx
			comm->flags = ZCL_COMM_FLAG_ROUTER_OK;
			break;

		case XBEE_PROTOCOL_ZB | XBEE_NODETYPE_ENDDEV:				// 29xx
		case XBEE_PROTOCOL_SMARTENERGY | XBEE_NODETYPE_ENDDEV:	// 39xx
			comm->flags = ZCL_COMM_FLAG_ENDDEV_OK;
			break;
	}

	// load all commissioning registers from the XBee
	return xbee_commissioning_query( comm, xbee, xbee_comm_regs[0].command.str);
}


/*** BeginHeader xbee_commissioning_set */
/*** EndHeader */
// configure XBee with options stored in zcl_comm_startup_param_t
int xbee_commissioning_set( xbee_dev_t *xbee, zcl_comm_startup_param_t *sas)
{
	bool_t encrypt, coord;
	xbee_comm_dl_t dl;
	uint8_t temp_u8;

	// Create macros for swapping byte order to big-endian for setting
	// parameter of AT request, and for dumping data to stdout.
	#if BYTE_ORDER == LITTLE_ENDIAN
		uint8_t	buffer[8];		// used for swapping byte order to big-endian

		#define _set_param_bytes( req, data, len)	\
			(	memcpy_htobe( buffer, data, len),	\
				xbee_cmd_set_param_bytes( req, buffer, len) )
		#define _hex_dump( data, len)	\
					hex_dump( buffer, len, HEX_DUMP_FLAG_NONE)
	#else
		#define _set_param_bytes( req, data, len)	\
				xbee_cmd_set_param_bytes( req, data, len)
		#define _hex_dump( data, len)	\
					hex_dump( data, len, HEX_DUMP_FLAG_NONE)
	#endif

	int16_t request;

	// Since we aren't using a callback for the request, we can re-use it for
	// setting all of the necessary parameters.

	request = xbee_cmd_create( xbee, "ID");
	if (request < 0)
	{
		#ifdef XBEE_COMMISSIONING_VERBOSE
			printf( "%s: can't create command handle (error %d)\n", __FUNCTION__,
				request);
		#endif
		return request;
	}

	// queue all changes until the last one so they're applied in a single batch
	xbee_cmd_set_flags( request,
		XBEE_CMD_FLAG_QUEUE_CHANGE | XBEE_CMD_FLAG_REUSE_HANDLE);

	if (memcheck( &sas->extended_panid, 0xFF, 8) == 0)
	{
		// map Commissioning Cluster's "unspecified" (all F's) to XBee's (0)
		xbee_cmd_set_param_bytes( request, WPAN_IEEE_ADDR_ALL_ZEROS, 8);
			#ifdef XBEE_COMMISSIONING_VERBOSE
				printf( "%s: setting %s=%u\n", __FUNCTION__, "ID", 0);
			#endif
	}
	else
	{
		_set_param_bytes( request, &sas->extended_panid, 8);
		#ifdef XBEE_COMMISSIONING_VERBOSE
			printf( "%s: setting %s=\n", __FUNCTION__, "ID");
			_hex_dump( &sas->extended_panid, 8);
		#endif
	}
	xbee_cmd_send( request);

	xbee_cmd_set_command( request, "II");
	xbee_cmd_set_param( request, sas->panid);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting %s=%04X\n", __FUNCTION__, "II", sas->panid);
	#endif
	xbee_cmd_send( request);

	xbee_cmd_set_command( request, "DL");
	// By storing values in the low bytes of DL, we can get away with setting
	// and sending just part of the structure and having the top bytes get
	// set to 0.
	dl.startup_control = sas->startup_control;
	xbee_cmd_set_param_bytes( request, &dl.startup_control, 1);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting %s=0x%08x\n", __FUNCTION__, "DL",
			dl.startup_control);
	#endif
	xbee_cmd_send( request);

	if (dl.startup_control == ZCL_COMM_STARTUP_COORDINATOR)
	{
		coord = TRUE;
	}
	else if (dl.startup_control == ZCL_COMM_STARTUP_JOINED)
	{
		coord = (sas->short_address == WPAN_NET_ADDR_COORDINATOR);
	}
	else
	{
		coord = FALSE;
	}
	xbee_cmd_set_command( request, "CE");
	xbee_cmd_set_param( request, coord);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting %s=%u\n", __FUNCTION__, "CE", coord);
	#endif
	xbee_cmd_send( request);

	xbee_cmd_set_command( request, "EO");
	// take insecure join setting and set the "use trust center" bit
	xbee_cmd_set_param( request, sas->use_insecure_join | 0x02);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting %s=%u\n", __FUNCTION__, "EO",
			sas->use_insecure_join);
	#endif
	xbee_cmd_send( request);

	// if network key has changed, set it on the XBee
	// Note that we can only set NK on an XBee coordinator
	if (memcheck( sas->network_key_le, 0xFF,
		sizeof(sas->network_key_le)) != 0)
	{
		// On the XBee, a network key of all zeros means "pick a random key".
		// Since we're disabling network encryption, it's OK to erase the
		// current key.
		xbee_cmd_set_command( request, "NK");
		xbee_cmd_set_param_bytes( request, &sas->network_key_le, 16);
		#ifdef XBEE_COMMISSIONING_VERBOSE
			printf( "%s: setting %s=\n", __FUNCTION__, "NK");
			hex_dump( &sas->network_key_le, 16, HEX_DUMP_FLAG_NONE);
		#endif
		xbee_cmd_send( request);

		// network key has been copied to XBee, set it to marker value
		memset( sas->network_key_le, 0xFF, sizeof(sas->network_key_le));
	}

	// if link key has changed, set it on the XBee
	if (memcheck( sas->preconfig_link_key_le, 0xFF,
											sizeof(sas->preconfig_link_key_le)) != 0)
	{
		// queue change to KY (Preconfigured Link Key)
		xbee_cmd_set_command( request, "KY");
		xbee_cmd_set_param_bytes( request, &sas->preconfig_link_key_le, 16);
		#ifdef XBEE_COMMISSIONING_VERBOSE
			printf( "%s: setting %s=\n", __FUNCTION__, "KY");
			hex_dump( &sas->preconfig_link_key_le, 16, HEX_DUMP_FLAG_NONE);
		#endif
		xbee_cmd_send( request);

		// We're interpreting the Commissioning Cluster spec such that a
		// link key value of all zeros means "disable network encryption".
		// The spec says it "indicates that the key is unspecified."
		encrypt = (memcheck( sas->preconfig_link_key_le, 0,
											sizeof(sas->preconfig_link_key_le)) != 0);
		xbee_cmd_set_command( request, "EE");
		xbee_cmd_set_param( request, encrypt);
			#ifdef XBEE_COMMISSIONING_VERBOSE
				printf( "%s: setting %s=%u\n", __FUNCTION__, "EE", encrypt);
			#endif
		xbee_cmd_send( request);

		// link key has been copied to the XBee, set it to the marker value
		memset( sas->preconfig_link_key_le, 0xFF,
			sizeof(sas->preconfig_link_key_le));
	}

	xbee_cmd_set_command( request, "BH");
	xbee_cmd_set_param( request, sas->concentrator.radius);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting %s=%u\n", __FUNCTION__, "BH",
			sas->concentrator.radius);
	#endif
	xbee_cmd_send( request);

	xbee_cmd_set_command( request, "AR");
	temp_u8 = sas->concentrator.flag ? sas->concentrator.discovery_time : 0xFF;
	xbee_cmd_set_param( request, temp_u8);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting %s=0x%02X\n", __FUNCTION__, "AR", temp_u8);
	#endif
	xbee_cmd_send( request);

	// set SC (Scan Channels) and apply all other changes
	xbee_cmd_set_command( request, "SC");
	xbee_cmd_clear_flags( request, XBEE_CMD_FLAG_QUEUE_CHANGE);
	xbee_cmd_set_param( request, sas->channel_mask >> 11);
	#ifdef XBEE_COMMISSIONING_VERBOSE
		printf( "%s: setting SC=0x%04X\n", __FUNCTION__,
			(uint16_t) (sas->channel_mask >> 11));
	#endif
	xbee_cmd_send( request);

	// release the handle so it's available for reuse
	xbee_cmd_release_handle( request);

	return 0;

	#undef _set_param_bytes
	#undef _hex_dump
}

/*** BeginHeader xbee_commissioning_tick */
/*** EndHeader */
/*
	Need to call from main() to update attributes with current values from
	XBee.
*/
void xbee_commissioning_tick( xbee_dev_t *xbee, zcl_comm_state_t *comm)
{
	int err;

	// Check for delayed restart.
	if ((comm->flags & ZCL_COMM_FLAG_DELAYED_RESTART)
		&& (int32_t)(xbee_millisecond_timer() - comm->restart_ms) >= 0)
	{
		if (comm->flags & ZCL_COMM_FLAG_FACTORY_RESET)
		{
			#ifdef XBEE_COMMISSIONING_VERBOSE
				printf( "%s: reset attribute set to factory default\n",
					__FUNCTION__);
			#endif
			comm->sas = zcl_comm_default_sas;
		}
		if (comm->flags & ZCL_COMM_FLAG_INSTALL)
		{
			#ifdef XBEE_COMMISSIONING_VERBOSE
				printf( "%s: installing attribute set\n", __FUNCTION__);
			#endif
			err = xbee_commissioning_set( xbee, &comm->sas);
			if (err != 0)
			{
				#ifdef XBEE_COMMISSIONING_VERBOSE
					printf( "%s: aborting, xbee_commissioning_set returned %d\n",
						__FUNCTION__, err);
				#endif
				return;
			}

			// Instead of doing ATWR here, set a flag and wait until the device
			// is joined to the network.  Then send ATWR and clear the flag.
			comm->flags |= ZCL_COMM_FLAG_SAVE_ON_JOIN;
		}

		#ifdef XBEE_COMMISSIONING_VERBOSE
			printf( "%s: performing network reset\n", __FUNCTION__);
		#endif
		xbee_cmd_simple( xbee, "NR", 0);

		// ** workaround **
		// Coordinator does not notify us when it's leaving the network, so we'll
		// jump the gun and assume we've already left the network so we'll be
		// in the correct state, watching for the COORD_START modem status.
		xbee->wpan_dev.flags &= ~(WPAN_FLAG_JOINED | WPAN_FLAG_AUTHENTICATED);

		// clear delayed restart and related flags
		comm->flags &= ~(ZCL_COMM_FLAG_DELAYED_RESTART
			| ZCL_COMM_FLAG_FACTORY_RESET | ZCL_COMM_FLAG_INSTALL);
	}

	switch (comm->state)
	{
		case ZCL_COMM_STATE_INIT:
			if (xbee_commissioning_init( comm, xbee) == 0)
			{
				#ifdef XBEE_COMMISSIONING_VERBOSE
					printf( "%s: loading SAS from XBee\n", __FUNCTION__);
				#endif
				comm->state = ZCL_COMM_STATE_LOADING;
			}
			break;

		case ZCL_COMM_STATE_LOADING:
			// waiting for AT command layer to finish loading
			break;

		case ZCL_COMM_STATE_STARTUP:
			// depending on startup_control, send possible NR to force rejoin
			if (comm->sas.startup_control >= ZCL_COMM_STARTUP_REJOIN)
			{
				#ifdef XBEE_COMMISSIONING_VERBOSE
					printf( "%s: network reset for startup_control %u\n",
						__FUNCTION__, comm->sas.startup_control);
				#endif
				xbee_cmd_execute( xbee, "NR", NULL, 0);
			}
			comm->state = ZCL_COMM_STATE_UNJOINED;
			break;

		case ZCL_COMM_STATE_UNJOINED:
			if (WPAN_DEV_IS_JOINED( &xbee->wpan_dev))
			{
				if (comm->flags & ZCL_COMM_FLAG_SAVE_ON_JOIN)
				{
					// send an ATWR to commit these changes to the XBee
					#ifdef XBEE_COMMISSIONING_VERBOSE
						printf( "%s: joined with new settings, saving changes\n",
							__FUNCTION__);
					#endif
					xbee_cmd_execute( xbee, "WR", NULL, 0);
					comm->flags &= ~ZCL_COMM_FLAG_SAVE_ON_JOIN;
				}

				// wait for the stack to finish loading ATMY
				if (xbee->wpan_dev.address.network != WPAN_NET_ADDR_UNDEFINED)
				{
					comm->sas.short_address = xbee->wpan_dev.address.network;
					// refresh the settings that change when joined
					if (! xbee_commissioning_query( comm, xbee, "OP"))
					{
						#ifdef XBEE_COMMISSIONING_VERBOSE
							printf( "%s: refreshing network settings\n",
								__FUNCTION__);
						#endif
						comm->state = ZCL_COMM_STATE_REFRESH;
					}
				}
			}
			break;

		case ZCL_COMM_STATE_REFRESH:
			// waiting for AT command layer to finish loading
			break;

		case ZCL_COMM_STATE_JOINED:
			if (! WPAN_DEV_IS_JOINED( &xbee->wpan_dev))
			{
				#ifdef XBEE_COMMISSIONING_VERBOSE
					printf( "%s: switching to UNJOINED state\n", __FUNCTION__);
				#endif
				comm->sas.short_address = WPAN_NET_ADDR_UNDEFINED;
				memset( &comm->sas.extended_panid, 0xFF,
					sizeof(comm->sas.extended_panid));

				comm->state = ZCL_COMM_STATE_UNJOINED;
			}
			break;
	}
}
