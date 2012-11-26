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
	@addtogroup zcl_commissioning
	@{
	@file zcl_commissioning.c

	Implementation of ZCL Commissioning Cluster.

	For devices with limited resources (e.g., PXBee), we could have a "minimal"
	zcl_comm_startup_param_t structure and zcl_comm_startup_attributes object,
	similar to how we have "base" and "full" attribute types.

	It would be necessary to have a flag though, so either the minimal or
	full structure can be passed to functions that process it.
*/

/*** BeginHeader */
#include <stdio.h>

#include "zigbee/zcl_commissioning.h"
/*** EndHeader */

/*** BeginHeader zcl_comm_global_epid */
/*** EndHeader */
const zcl64_t zcl_comm_global_epid = ZCL_COMM_GLOBAL_EPID;

/*** BeginHeader zcl_comm_startup_param */
/*** EndHeader */
zcl_comm_state_t zcl_comm_state = { { 0 } };

/*** BeginHeader zcl_comm_startup_attributes, zcl_comm_startup_attribute_tree */
/*** EndHeader */
/**
	@internal
	@brief
	Limit values of startup_control based on firmware type (e.g., some firmwares
	can't be ZCL_COMM_STARTUP_COORDINATOR).

	@param[in]		attribute	should be a pointer to
										zcl_comm_startup_attributes.startup_control
	@param[in,out]	rec			value from write request

	See zcl_attribute_write_fn() for calling convention.
*/
int _zcl_comm_startup_control_set( const zcl_attribute_full_t FAR *attribute,
	zcl_attribute_write_rec_t *rec)
{
	uint8_t value;

	if (attribute != &zcl_comm_startup_attributes.startup_control)
	{
		// Unexpected error, this function is only be called for one attribute.
		rec->status = ZCL_STATUS_SOFTWARE_FAILURE;
	}
	else
	{
		value = *rec->buffer;

		switch (value)
		{
			// Table 3.121 "The AIB attribute apsDesignatedCoordinator shall be
			// set to TRUE in this case."
			case ZCL_COMM_STARTUP_COORDINATOR:
				// confirm that this firmware can operate in coordinator mode
				if (! (zcl_comm_state.flags & ZCL_COMM_FLAG_COORDINATOR_OK))
				{
					rec->status = ZCL_STATUS_INVALID_VALUE;
				}
				break;

			// Table 3.121 "The AIB attribute apsDesignatedCoordinator shall be
			// set to FALSE in [these cases]."
			case ZCL_COMM_STARTUP_REJOIN:
			case ZCL_COMM_STARTUP_FROM_SCRATCH:
				// confirm that this is not a coordiantor-only firmware
				if (! (zcl_comm_state.flags
							& (ZCL_COMM_FLAG_ROUTER_OK | ZCL_COMM_FLAG_ENDDEV_OK)))
				{
					rec->status = ZCL_STATUS_INVALID_VALUE;
				}
				break;
		}

		if ((rec->flags & ZCL_ATTR_WRITE_FLAG_ASSIGN)
												&& rec->status == ZCL_STATUS_SUCCESS)
		{
			zcl_comm_state.sas.startup_control = value;
		}
	}

	// startup_control is a 1-byte value (ENUM8)
	return 1;
}

/*
	Note that we use the ZCL_ATTRIB_FLAG_RAW flag on security keys, in an
	attempt to prevent getting the incorrect byte order.  The keys will be
	stored in network byte order
*/
const zcl_comm_startup_attr_t zcl_comm_startup_attributes =
{
	// id, flags, type, value
	{	ZCL_COMM_ATTR_SHORT_ADDRESS,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_UNSIGNED_16BIT,
		&zcl_comm_state.sas.short_address
	},

	{	ZCL_COMM_ATTR_EXTENDED_PANID,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_IEEE_ADDR,
		&zcl_comm_state.sas.extended_panid
	},

	{	ZCL_COMM_ATTR_PANID,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_UNSIGNED_16BIT,
		&zcl_comm_state.sas.panid
	},

	{	ZCL_COMM_ATTR_CHANNEL_MASK,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_BITMAP_32BIT,
		&zcl_comm_state.sas.channel_mask
	},

	{	{	ZCL_COMM_ATTR_STARTUP_CONTROL,
			ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_ENUM_8BIT,
			&zcl_comm_state.sas.startup_control
		},
		{ ZCL_COMM_STARTUP_JOINED },								// min
		{ ZCL_COMM_STARTUP_FROM_SCRATCH },						// max
		NULL,																// read_fn
		_zcl_comm_startup_control_set								// write_fn
	},

	{	ZCL_COMM_ATTR_TRUST_CTR_ADDR,
		ZCL_ATTRIB_FLAG_READONLY,
		ZCL_TYPE_IEEE_ADDR,
		WPAN_IEEE_ADDR_ALL_ZEROS				// unspecified
	},

	{	ZCL_COMM_ATTR_TRUST_CTR_MASTER_KEY,
		ZCL_ATTRIB_FLAG_WRITEONLY | ZCL_ATTRIB_FLAG_RAW,
		ZCL_TYPE_SECURITY_KEY,
		NULL			// &zcl_comm_state.sas.trust_ctr_master_key
	},

	{	ZCL_COMM_ATTR_NETWORK_KEY,
		ZCL_ATTRIB_FLAG_WRITEONLY | ZCL_ATTRIB_FLAG_RAW,
		ZCL_TYPE_SECURITY_KEY,
		&zcl_comm_state.sas.network_key_le
	},

	{	ZCL_COMM_ATTR_USE_INSECURE_JOIN,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_LOGICAL_BOOLEAN,
		&zcl_comm_state.sas.use_insecure_join
	},

	{	ZCL_COMM_ATTR_PRECONFIG_LINK_KEY,
		ZCL_ATTRIB_FLAG_WRITEONLY | ZCL_ATTRIB_FLAG_RAW,
		ZCL_TYPE_SECURITY_KEY,
		&zcl_comm_state.sas.preconfig_link_key_le
	},

	{	ZCL_COMM_ATTR_NETWORK_KEY_SEQ_NUM,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_UNSIGNED_8BIT,
		&zcl_comm_state.sas.network_key_seq_num
	},

	{	ZCL_COMM_ATTR_NETWORK_KEY_TYPE,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_ENUM_8BIT,
		&zcl_comm_state.sas.network_key_type
	},

	{	ZCL_COMM_ATTR_NETWORK_MGR_ADDR,
		ZCL_ATTRIB_FLAG_NONE,
		ZCL_TYPE_UNSIGNED_16BIT,
		&zcl_comm_state.sas.network_mgr_addr
	},

	{	{	ZCL_COMM_ATTR_SCAN_ATTEMPTS,
			ZCL_ATTRIB_FLAG_READONLY | ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_UNSIGNED_8BIT,
			&zcl_comm_state.sas.scan_attempts
		},
		{ 0x01 }, { 0xFF }, NULL, NULL							// min, max, read, write
	},

	{	{	ZCL_COMM_ATTR_TIME_BTWN_SCANS,
			ZCL_ATTRIB_FLAG_READONLY | ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_UNSIGNED_16BIT,
			&zcl_comm_state.sas.time_between_scans
		},
		{ 0x0001 }, { 0xFFFF }, NULL, NULL						// min, max, read, write
	},

	// If RejoinInterval becomes writable, we need to add a write handler that
	// ensures it does not exceed the max_rejoin_interval.
	// Use *(uint16_t FAR *)((zcl_attribute_full_t FAR *)attr + 1)->base.value,
	// or cheat and reference zcl_comm_state.sas.max_rejoin_interval directly.
	{	{	ZCL_COMM_ATTR_REJOIN_INTERVAL,
			ZCL_ATTRIB_FLAG_READONLY | ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_UNSIGNED_16BIT,
			&zcl_comm_state.sas.rejoin_interval
		},
		{ 0x0001 }, { 0xFFFF }, NULL, NULL						// min, max, read, write
	},

	{	{	ZCL_COMM_ATTR_MAX_REJOIN_INTERVAL,
			ZCL_ATTRIB_FLAG_READONLY | ZCL_ATTRIB_FLAG_FULL,
			ZCL_TYPE_UNSIGNED_16BIT,
			&zcl_comm_state.sas.max_rejoin_interval
		},
		{ 0x0001 }, { 0xFFFF}, NULL, NULL						// min, max, read, write
	},

	{
		{	ZCL_COMM_ATTR_CONCENTRATOR_FLAG,
			ZCL_ATTRIB_FLAG_NONE,
			ZCL_TYPE_LOGICAL_BOOLEAN,
			&zcl_comm_state.sas.concentrator.flag
		},

		{	ZCL_COMM_ATTR_CONCENTRATOR_RADIUS,
			ZCL_ATTRIB_FLAG_NONE,
			ZCL_TYPE_UNSIGNED_8BIT,
			&zcl_comm_state.sas.concentrator.radius
		},

		{	ZCL_COMM_ATTR_CONCENTRATOR_DISC_TIME,
			ZCL_ATTRIB_FLAG_NONE,
			ZCL_TYPE_UNSIGNED_8BIT,
			&zcl_comm_state.sas.concentrator.discovery_time
		}
	},

	ZCL_ATTRIBUTE_END_OF_LIST
};

const zcl_attribute_tree_t zcl_comm_startup_attribute_tree[] =
		{ { ZCL_MFG_NONE, &zcl_comm_startup_attributes.short_address, NULL } };

/*** BeginHeader zcl_comm_response */
/*** EndHeader */
/**
	@brief
	Send response to a Commissioning Server Cluster request.
*/
int zcl_comm_response( const wpan_envelope_t FAR *envelope,
																		uint_fast8_t status)
{
	const zcl_header_nomfg_t FAR *zcl;
	wpan_envelope_t reply_env;
	PACKED_STRUCT {
		zcl_header_nomfg_t		header;
		uint8_t						status;
	} reply;
	int retval;

	// wpan_envelope_reply() will test for envelope == NULL
	retval = wpan_envelope_reply( &reply_env, envelope);
	if (retval != 0)
	{
		return retval;
	}

	zcl = envelope->payload;
	reply.header.frame_control = ZCL_FRAME_SERVER_TO_CLIENT
										| ZCL_FRAME_DISABLE_DEF_RESP
										| ZCL_FRAME_TYPE_CLUSTER;
	reply.header.sequence = zcl->sequence;
	reply.header.command = zcl->command;
	reply.status = (uint8_t) status;

	reply_env.payload = &reply;
	reply_env.length = sizeof reply;

	return wpan_envelope_send( &reply_env);
}

/*** BeginHeader _zcl_comm_command_build */
int _zcl_comm_command_build( wpan_envelope_t FAR *envelope,
	zcl_header_nomfg_t *header);
/*** EndHeader */
/** @internal
	@brief
	Common function used by zcl_comm_restart_device and zcl_comm_reset_parameters

	Fills in frame_control and sequence of header.
	Fills in cluster_id and payload of envelope.
	Fills in source_endpoint and profile_id of envelope if necessary.

	Caller needs to fill in envelope->length and header->command
*/
int _zcl_comm_command_build( wpan_envelope_t FAR *envelope,
	zcl_header_nomfg_t *header)
{
	const wpan_endpoint_table_entry_t *ep;

	if (envelope->source_endpoint == 0)
	{
		#ifdef ZCL_COMMISSIONING_VERBOSE
			printf( "%s: searching for endpoint w/commissioning client cluster\n",
				__FUNCTION__);
		#endif
		ep = wpan_endpoint_of_cluster( envelope->dev, WPAN_APS_PROFILE_ANY,
			ZCL_CLUST_COMMISSIONING, WPAN_CLUST_FLAG_CLIENT);

		if (ep != NULL)
		{
			#ifdef ZCL_COMMISSIONING_VERBOSE
				printf( "%s: commissioning client on ep %u, profile 0x%04x\n",
					__FUNCTION__, ep->endpoint, ep->profile_id);
			#endif
			envelope->source_endpoint = ep->endpoint;
			envelope->profile_id = ep->profile_id;
		}
	}
	else
	{
		ep = wpan_endpoint_of_envelope( envelope);
	}

	if (ep == NULL)
	{
		#ifdef ZCL_COMMISSIONING_VERBOSE
			printf( "%s: couldn't locate source endpoint on this device\n",
				__FUNCTION__);
		#endif
		return -EINVAL;
	}

	envelope->cluster_id = ZCL_CLUST_COMMISSIONING;
	envelope->payload = header;

	header->frame_control = ZCL_FRAME_CLIENT_TO_SERVER
									| ZCL_FRAME_GENERAL
									| ZCL_FRAME_TYPE_CLUSTER
									| ZCL_FRAME_DISABLE_DEF_RESP;
	header->sequence = wpan_endpoint_next_trans( ep);

	return 0;
}

/*** BeginHeader _zcl_comm_command_send */
int _zcl_comm_command_send( wpan_envelope_t FAR *envelope);
/*** EndHeader */
/** @internal
	@brief
	Common function used by zcl_comm_restart_device and zcl_comm_reset_parameters

	sends the envelope and then resets the payload and length before returning
*/
int _zcl_comm_command_send( wpan_envelope_t FAR *envelope)
{
	int retval;

	retval = wpan_envelope_send( envelope);

	// reset payload and length of envelope
	envelope->payload = NULL;
	envelope->length = 0;

	return retval;
}

/*** BeginHeader zcl_comm_restart_device */
/*** EndHeader */
/**
	@brief
	Send a "Restart Device" command to the ZCL Commissioning Cluster.

	@param[in,out]	envelope		Partial envelope used to send the request.
				Caller must set \c dev, \c ieee_address, \c network_address,
				\c profile_id, \c source_endpoint, \c dest_endpoint and
				(optionally) \c options.  On return, \c payload and \c length
				are cleared, and \c cluster_id is set to ZCL_CLUST_COMMISSIONING.

				If \c source_endpoint is 0, function will search the endpoint
				table for a Commissioning Client Cluster and set the envelope's
				\c source_endpoint and \c profile_id.

	@param[in]	parameters	Parameters for the Restart Device command or NULL
				for default settings (save changes and restart without
				delay/jitter).

	@retval	0			request sent
	@retval	-EINVAL	couldn't find source endpoint in endpoint table,
							or some other error in parameter passed to function
	@retval	!0			error trying to send request

*/
int zcl_comm_restart_device( wpan_envelope_t FAR *envelope,
	const zcl_comm_restart_device_cmd_t *parameters)
{
	PACKED_STRUCT {
		zcl_header_nomfg_t					header;
		zcl_comm_restart_device_cmd_t		parameters;
	} request;
	int retval;

	retval = _zcl_comm_command_build( envelope, &request.header);

	if (retval != 0)
	{
		return retval;
	}

	request.header.command = ZCL_COMM_CMD_RESTART_DEVICE;
	if (parameters == NULL)
	{
		memset( &request.parameters, 0, sizeof(request.parameters));
	}
	else
	{
		request.parameters = *parameters;
	}

	envelope->length = sizeof request;
	return _zcl_comm_command_send( envelope);
}

/*** BeginHeader zcl_comm_reset_parameters */
/*** EndHeader */
/**
	@brief
	Send a "Reset Startup Parameters" command to the ZCL Commissioning Cluster.

	@param[in,out]	envelope		Partial envelope used to send the request.
				Caller must set \c dev, \c ieee_address, \c network_address,
				\c profile_id, \c source_endpoint, \c dest_endpoint and
				(optionally) \c options.  On return, \c payload and \c length
				are cleared, and \c cluster_id is set to ZCL_CLUST_COMMISSIONING.

				If \c source_endpoint is 0, function will search the endpoint
				table for a Commissioning Client Cluster and set the envelope's
				\c source_endpoint and \c profile_id.

	@param[in]	parameters	Parameters for the Reset Startup Parameters command
				or NULL for default settings (reset current parameters only).

	@retval	0			request sent
	@retval	-EINVAL	couldn't find source endpoint in endpoint table,
							or some other error in parameter passed to function
	@retval	!0			error trying to send request

*/
int zcl_comm_reset_parameters( wpan_envelope_t FAR *envelope,
	const zcl_comm_reset_startup_param_t *parameters)
{
	PACKED_STRUCT {
		zcl_header_nomfg_t					header;
		zcl_comm_reset_startup_param_t	parameters;
	} request;
	int retval;

	retval = _zcl_comm_command_build( envelope, &request.header);

	if (retval != 0)
	{
		return retval;
	}

	request.header.command = ZCL_COMM_CMD_RESET_STARTUP_PARAM;
	if (parameters == NULL)
	{
		request.parameters.options = ZCL_COMM_RESET_OPT_CURRENT;
		request.parameters.index = 0;
	}
	else
	{
		request.parameters = *parameters;
	}

	envelope->length = sizeof request;
	return _zcl_comm_command_send( envelope);
}

/*** BeginHeader zcl_comm_clust_handler */
/*** EndHeader */
#include <stdlib.h>
#if RAND_MAX < 32767
	#error "Other code calling this function depend on RAND_MAX >= 32767."
#endif
// return a number from [0 to <range>] inclusive
int rand_range( int range)
{
	unsigned int perbucket;
	unsigned int max;
	unsigned int r;

	if (range < 0 || range > RAND_MAX)
	{
		return -EINVAL;
	}

	if (range == 0)
	{
		return 0;			// [0 .. 0], only choice is 0
	}
	if (range == RAND_MAX)
	{
		return rand();		// rand() defined to return [0 .. RAND_MAX]
	}

	/* If range is a string of low bits (1, 3, 7, 15, 31, etc.) we could just
		take rand() and bitwise AND it with range, but the low-order bits of
		many PRNGs are not very random.

		For other ranges, to ensure equal distribution, create <range>+1 buckets
		of <perbucket> values per bucket.  Discard random numbers outside the
		range of values, and then divide the result by the <perbucket> count.

		This has the added benefit of using high-order bits which are "more"
		random than low-order bits on many psuedo random number generators.

		(See comp.lang.c FAQ list, 13.16: http://c-faq.com/lib/randrange.html)
	*/

	// rand() generates values from 0 to RAND_MAX, inclusive (hence the +1u
	// and converting to unsigned int in case RAND_MAX == INT_MAX).  We need
	// range + 1 buckets (again, 0 to <range> inclusive).
	perbucket = (RAND_MAX + 1u) / (range + 1u);
	max = range * perbucket;
	do
	{
		r = rand();
	} while (r >= max);

	#ifdef ZCL_COMMISSIONING_VERBOSE
		printf( "%s: rand_range(%u) returning %u\n", __FUNCTION__,
			range, r / perbucket);
	#endif
	return (r / perbucket);
}

/**
	Partial implementation of a function to validate a Startup Attribute Set
	before trying to use it.

	@param[in]	comm	Startup Attribute Set to test.

	@retval	TRUE	Startup Attribute Set is valid
	@retval	FALSE	Startup Attribute Set is invalid and cannot be used

	@todo add more tests to this function
*/
bool_t zcl_comm_sas_is_valid( const zcl_comm_state_t FAR *comm)
{
	if (comm->sas.startup_control != ZCL_COMM_STARTUP_FROM_SCRATCH)
	{
		// Extended PAN ID is required for all but "from scratch"
		if (memcheck( &comm->sas.extended_panid, 0xFF, 8) == 0
			|| memcheck( &comm->sas.extended_panid, 0x00, 8) == 0)
		{
			return FALSE;
		}
	}

	switch (comm->sas.startup_control)
	{
		case ZCL_COMM_STARTUP_JOINED:
			// must have short address, panid, trust center, network key,
			// key seq num and key type
			if (comm->sas.short_address == WPAN_NET_ADDR_COORDINATOR)
			{
				// must support coordinator mode
			}
			else
			{
				// must support non-coordinator mode
			}
			break;

		case ZCL_COMM_STARTUP_COORDINATOR:
			break;

		case ZCL_COMM_STARTUP_REJOIN:
			break;

		case ZCL_COMM_STARTUP_FROM_SCRATCH:
			break;
	}

	return TRUE;
}

int zcl_comm_clust_handler( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	// Make sure frame is not manufacturer-specific, client-to-server and
	// a cluster command (not "profile-wide").
	if (envelope != NULL &&
		ZCL_CMD_MATCH( envelope->payload, GENERAL, CLIENT_TO_SERVER, CLUSTER))
	{
		const PACKED_STRUCT {
			zcl_header_nomfg_t	zcl;
			union {
				zcl_comm_restart_device_cmd_t		restart_dev;
				zcl_comm_save_startup_param_t		save_startup;
				zcl_comm_restore_startup_param_t	restore_startup;
				zcl_comm_reset_startup_param_t	reset_startup;
			} cmd;
		} FAR *request = envelope->payload;
		const zcl_attribute_tree_t FAR *tree = context;
		zcl_comm_state_t FAR *comm;
		uint32_t delay;

		// Context points to the attribute tree for this cluster.  First
		// server attribute is the first element of the zcl_comm_startup_param_t
		// structure.  Note that we intentionally cast away const on 'value'
		// since it is actually non-const.
		comm = (zcl_comm_state_t FAR *)tree[0].server->value;

		switch (request->zcl.command)
		{
			case ZCL_COMM_CMD_RESTART_DEVICE:
				// validate the statup configuration
				if (! zcl_comm_sas_is_valid( comm))
				{
					#ifdef ZCL_COMMISSIONING_VERBOSE
						printf( "%s: ignoring restart cmd due to inconsistent SAS\n",
							__FUNCTION__);
					#endif
					return zcl_comm_response( envelope,
								ZCL_STATUS_INCONSISTENT_STARTUP_STATE);
				}

				// Set a timer for when we should perform the restart.
				delay = request->cmd.restart_dev.delay * UINT32_C(1000) +
					rand_range( request->cmd.restart_dev.jitter * 80);
				#ifdef ZCL_COMMISSIONING_VERBOSE
					printf( "%s: restart scheduled in %" PRIu32 "ms\n",
						__FUNCTION__, delay);
				#endif

				comm->restart_ms = (xbee_millisecond_timer() + delay);
				comm->flags |= ZCL_COMM_FLAG_DELAYED_RESTART;

				if ((request->cmd.restart_dev.options
															& ZCL_COMM_RESTART_OPT_MODE_MASK)
					== ZCL_COMM_RESTART_OPT_MODE_SAVE_CHANGES)
				{
					comm->flags |= ZCL_COMM_FLAG_INSTALL;
				}
				else
				{
					// client changed its mind and we need to clear the install flag
					comm->flags &= ~ZCL_COMM_FLAG_INSTALL;
				}
				return zcl_comm_response( envelope, ZCL_STATUS_SUCCESS);

			case ZCL_COMM_CMD_RESET_STARTUP_PARAM:
				if (request->cmd.reset_startup.options
																& ZCL_COMM_RESET_OPT_CURRENT)
				{
					#ifdef ZCL_COMMISSIONING_VERBOSE
						printf( "%s: resetting to factory defaults\n",
							__FUNCTION__);
					#endif
					// reset and install factory defaults
					comm->flags |=
						ZCL_COMM_FLAG_DELAYED_RESTART |
						ZCL_COMM_FLAG_FACTORY_RESET |
						ZCL_COMM_FLAG_INSTALL;
					comm->restart_ms = xbee_millisecond_timer();
				}
				// if we implemented saved startup parameters, we would need
				// to process other options here

				return zcl_comm_response( envelope, ZCL_STATUS_SUCCESS);

			case ZCL_COMM_CMD_SAVE_STARTUP_PARAM:
			case ZCL_COMM_CMD_RESTORE_STARTUP_PARAM:
				// this implementation does not handle these optional commands
				break;
		}
	}

	// Allow General Command handler to process general
	// commands and send errors out for unsupported commands.
	return zcl_general_command( envelope, context);
}

/*** BeginHeader zcl_comm_sas_init */
/*** EndHeader */
void zcl_comm_sas_init( zcl_comm_startup_param_t *p)
{
	memset( p, 0, sizeof *p);
	memset( &p->extended_panid, 0xFF, sizeof(p->extended_panid));
	memset( &p->network_key_le, 0xFF, sizeof(p->network_key_le));
	memset( &p->preconfig_link_key_le, 0xFF, sizeof(p->preconfig_link_key_le));

	p->scan_attempts = ZCL_COMM_SCAN_ATTEMPTS_DEFAULT;
	p->time_between_scans = ZCL_COMM_TIME_BETWEEN_SCANS_DEFAULT;
	p->rejoin_interval = ZCL_COMM_REJOIN_INTERVAL_DEFAULT;
	p->max_rejoin_interval = ZCL_COMM_MAX_REJOIN_INTERVAL_DEFAULT;
}

/*** BeginHeader zcl_comm_factory_reset */
/*** EndHeader */
/**
	@brief Perform a locally-triggered factory reset to default SAS.

	@param[in]	ms_delay		initiate reset after \p ms_delay milliseconds
*/
void zcl_comm_factory_reset( uint16_t ms_delay)
{
	zcl_comm_state.restart_ms = xbee_millisecond_timer() + ms_delay;
	zcl_comm_state.flags |= ZCL_COMM_FLAG_DELAYED_RESTART
		| ZCL_COMM_FLAG_FACTORY_RESET | ZCL_COMM_FLAG_INSTALL;
}
