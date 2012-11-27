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
	@addtogroup SXA
	@{
	@file xbee_sxa.c
   Simple XBee API.
	An additional layer of wrapper functions for the XBee API.
   The wrappers provide simplified handling of
   . node discovery
   . XBee I/O response messages
   . Caching of AT command results

*/

/*** BeginHeader sxa_table, sxa_table_count, sxa_list_head, sxa_list_count,
				 sxa_local_table, sxa_xbee, sxa_wpan_address */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/discovery.h"
#include "xbee/io.h"
#include "xbee/sxa.h"

#ifndef __DC__
	#define _xbee_sxa_debug
	// Rabbit uses special malloc calls to access system (vs. user) memory
	#define _sys_calloc( s)		calloc( s, 1)
	#define _sys_malloc( s)		malloc( s)
	#define _sys_free( p)		free( p)
#elif defined XBEE_SXA_DEBUG
	#define _xbee_sxa_debug	__debug
#else
	#define _xbee_sxa_debug	__nodebug
#endif

#define SXA_OFFSET(base, ofs)  (void FAR *)((char FAR *)(base) + (ofs))

/*** EndHeader */

// The head of the linked list of nodes.  Our own (local) XBee nodes are
// also included in the list, keyed by their IEEE address, as usual.
sxa_node_t FAR *sxa_table = NULL;
int sxa_table_count = 0;
sxa_node_t FAR *sxa_local_table = NULL;

_xbee_sxa_debug
sxa_node_t FAR * (sxa_list_head)(void)
{
	return sxa_table;
}

_xbee_sxa_debug
int (sxa_list_count)(void)
{
	return sxa_table_count;
}

_xbee_sxa_debug
xbee_dev_t * (sxa_xbee)(sxa_node_t FAR *sxa)
{
	return sxa->xbee;
}

_xbee_sxa_debug
wpan_address_t FAR * (sxa_wpan_address)(sxa_node_t FAR *sxa)
{
	return sxa->addr_ptr;
}


/*** BeginHeader _sxa_default_cache_groups */
/*** EndHeader */

const FAR xbee_atcmd_reg_t _sxa_ni_query_regs[] = {
	XBEE_ATCMD_REG( 'N', 'I', XBEE_CLT_COPY, sxa_node_t, id.node_info),
   XBEE_ATCMD_REG_END
};

const FAR xbee_atcmd_reg_t _sxa_devinfo_query_regs[] = {
	XBEE_ATCMD_REG_MOVE_THEN_CB( 'A', 'O', XBEE_CLT_COPY_BE,
   					sxa_node_t, ao,
                  _sxa_process_version_info, 0),
	XBEE_ATCMD_REG( 'H', 'V', XBEE_CLT_COPY_BE, sxa_node_t, hardware_version),
	XBEE_ATCMD_REG_MOVE_THEN_CB( 'V', 'R', XBEE_CLT_COPY_BE,
   					sxa_node_t, firmware_version,
                  _sxa_process_version_info, 1),
	XBEE_ATCMD_REG( 'D', 'D', XBEE_CLT_COPY_BE, sxa_node_t, dd),
   XBEE_ATCMD_REG_END
};

const FAR xbee_atcmd_reg_t _sxa_dhdl_query_regs[] = {
	XBEE_ATCMD_REG( 'D', 'H', XBEE_CLT_COPY, sxa_node_t, dest_addr.l[0]),
	XBEE_ATCMD_REG( 'D', 'L', XBEE_CLT_COPY, sxa_node_t, dest_addr.l[1]),
   XBEE_ATCMD_REG_END
};


#define _SXA_CG_INIT(id, field, get, fn) \
	{ id, offsetof(sxa_node_t, field), get, fn }
#define _SXA_CG_INIT_END \
	{ 0, 0, NULL, NULL }

const FAR sxa_cached_group_t _sxa_default_cache_groups[] =
{
	_SXA_CG_INIT(SXA_CACHED_NODE_ID,		node_id_cf,			_sxa_ni_query_regs, 			NULL),
	_SXA_CG_INIT(SXA_CACHED_IO_CONFIG,	io_config_cf, 		NULL, 							_sxa_update_io_config_group),
	_SXA_CG_INIT(SXA_CACHED_DEVICE_INFO,device_info_cf,	_sxa_devinfo_query_regs,	NULL),
	_SXA_CG_INIT(SXA_CACHED_DHDL,			dhdl_cf, 			_sxa_dhdl_query_regs, 		NULL),
	_SXA_CG_INIT_END
};

/*** BeginHeader _sxa_cache_group_by_id */
/*** EndHeader */
const sxa_cached_group_t FAR * _sxa_cache_group_by_id(uint16_t id)
{
	int i;

   for (i = 0; _sxa_default_cache_groups[i].flags_offs; ++i)
   {
   	if (_sxa_default_cache_groups[i].id == id)
      {
      	return _sxa_default_cache_groups + i;
      }
   }
   return NULL;
}


/*** BeginHeader _sxa_process_version_info */
/*** EndHeader */
void _sxa_process_version_info(
	const xbee_cmd_response_t FAR 		*response,
   const struct xbee_atcmd_reg_t FAR	*reg,
	void FAR										*base)
{
	/* This is called on receipt of result for the ATVR (firmware version)
      register.  Also, ATHV (hardware version) should be available.
      'base' points to the SXA.

      This function basically fills in a bunch of device capability fields.
      Processing is derived from NDS code (ms_serial.c: msOpenRadio()).

      We set the sxa->dd field based on this information.  A 'real' DD
      command follows in the command list.  If this works, sxa->dd will
      be overwritten (with presumably the same value), otherwise the value
      we compute here will be untouched.
   */
   sxa_node_t FAR *sxa;
	uint16_t zbVer;
   uint16_t zbV;

   if (response == NULL || reg == NULL || base == NULL)
   {
   	return;
   }

   sxa = base;
   // Ignore ZigBee node type in version field
   zbVer = (uint16_t)sxa->firmware_version & ~XBEE_NODETYPE_MASK;
   zbV = zbVer & XBEE_PROTOCOL_MASK;
   if (!reg->flags)
   {
   	// Callback being invoked to handle AO result.  If error, mark
      // ao field with 0xFF to indicate AO command not supported.
	   if ((response->flags & XBEE_CMD_RESP_MASK_STATUS)
                                             != XBEE_AT_RESP_SUCCESS)
		{
      	sxa->ao = XBEE_ATAO_NOT_SUPPORTED;
      }
		return;
   }

   // Otherwise, callback is being invoked after AO,HV and VR commands.

   sxa->caps = 0;

   switch (sxa->hardware_version & XBEE_HARDWARE_MASK)
   {
   case XBEE_HARDWARE_900_PRO:
   	if (sxa->firmware_version >= 0x1800)
      {
      	sxa->dd = MS_MOD_XBEE900;
			sxa->caps |= ZB_CAP_DIGIMESH;
      }
      else
      {
      	sxa->dd = MS_MOD_XBEE900DP;
      }
   	sxa->caps |= ZB_CAP_ADV_ADDR|ZB_CAP_REMOTE_DDO|ZB_CAP_GW_FW;
		break;
   case XBEE_HARDWARE_868_PRO:
   	sxa->dd = MS_MOD_XBEE868;
   	sxa->caps |= ZB_CAP_ADV_ADDR|ZB_CAP_REMOTE_DDO|ZB_CAP_GW_FW;
		break;
	case XBEE_HARDWARE_S2:
   case XBEE_HARDWARE_S2_PRO:
   case XBEE_HARDWARE_S2B_PRO:
   	sxa->caps |= ZB_CAP_ADV_ADDR|ZB_CAP_GW_FW|ZB_CAP_ZIGBEE|ZB_CAP_CHILDREN;
      switch (zbV)
      {
      case XBEE_PROTOCOL_ZNET:
      	// ZNet 2.5 firmware
         sxa->dd = MS_MOD_ZNET25;
         sxa->caps |= ZB_CAP_REMOTE_DDO;
         break;
      case XBEE_PROTOCOL_ZB:
      	// ZB firmware
         sxa->dd = MS_MOD_ZB;
         sxa->caps |= ZB_CAP_ZDO|ZB_CAP_REMOTE_DDO|ZB_CAP_REMOTE_FW|ZB_CAP_ZBPRO;
			break;
      case XBEE_PROTOCOL_SMARTENERGY:
      	// Smart Energy firmware
         sxa->dd = MS_MOD_ZB;
         sxa->caps |= ZB_CAP_ZDO|ZB_CAP_ZBPRO|ZB_CAP_SE;
      	break;
      }
      break;
   case XBEE_HARDWARE_S2C_PRO:
   case XBEE_HARDWARE_S2C:
   	sxa->dd = MS_MOD_ZB_S2C;
      sxa->caps |= ZB_CAP_ADV_ADDR|ZB_CAP_GW_FW|ZB_CAP_ZIGBEE|
            			ZB_CAP_CHILDREN|ZB_CAP_ZDO|ZB_CAP_ZBPRO;
      switch (zbV)
      {
      case XBEE_PROTOCOL_ZB_S2C:
      	// ZB firmware
         sxa->caps |= ZB_CAP_REMOTE_DDO|ZB_CAP_REMOTE_FW;
         break;
      case XBEE_PROTOCOL_SE_S2C:
      	// Smart Energy firmware
         sxa->caps |= ZB_CAP_SE;
      	break;
      }
   	break;
   case XBEE_HARDWARE_S1:
   case XBEE_HARDWARE_S1_PRO:
   	sxa->dd = MS_MOD_SERIES1;
   	if (sxa->firmware_version >= 0x1080 && sxa->firmware_version < 0x10C0)
      {
			// 802.15.4 firmware
         sxa->caps |= ZB_CAP_GW_FW;
      }
      else if (sxa->firmware_version >= 0x10C4 && sxa->firmware_version < 0x8000)
      {
      	sxa->caps |= ZB_CAP_REMOTE_DDO|ZB_CAP_GW_FW;
      }
      else if (sxa->firmware_version >= 0x8000)
      {
			// ZigBee and DigiMesh use the same range of version numbers.
      	if (sxa->ao == XBEE_ATAO_NOT_SUPPORTED)
         {
         	// ZigBee firmware
            sxa->caps |= ZB_CAP_GW_FW|ZB_CAP_ZIGBEE|ZB_CAP_CHILDREN;
         }
         else
         {
         	// DigiMesh 2.4 firmware
            sxa->dd = MS_MOD_XBEE24;
            sxa->caps |= ZB_CAP_ADV_ADDR|ZB_CAP_REMOTE_DDO|ZB_CAP_GW_FW|ZB_CAP_DIGIMESH;
         }
      }
   	break;
   }
}





/*** BeginHeader _sxa_update_io_config_group */
/*** EndHeader */
void _sxa_update_io_config_group(
	sxa_node_t FAR			*sxa,
   const sxa_cached_group_t FAR	*cgroup)
{
}


/*** BeginHeader _sxa_disc_process_node_data */
/*** EndHeader */
/**
	@internal
	@brief
	Function shared by multiple functions that process Node Identification
	Messages (0x95 frames and ATND responses).

	@param[in]	xbee		device that received the message
	@param[in]	raw		pointer to the message
	@param[in]	length	length of the message

	@retval	0	successfully parsed
	@retval	!0	error parsing data or calling \a handler()
*/
_xbee_sxa_debug
int _sxa_disc_process_node_data( xbee_dev_t *xbee, const void FAR *raw,
	int length)
{
	xbee_node_id_t node_id;
   sxa_node_t FAR *sxa;

	// We may want to add code here to confirm that length is correct,
	// based on the length of the NI value in the xbee_node_id_t.

	if (length <= 0)
	{
	#ifdef XBEE_DISCOVERY_VERBOSE
   	printf( "%s: invalid node data of %d bytes:\n",
			__FUNCTION__, length);
   #endif
		return -EINVAL;
	}

	if (xbee_disc_nd_parse( &node_id, raw, length) == 0)
	{
		sxa = sxa_node_add( xbee, &node_id);
	#ifdef XBEE_DISCOVERY_VERBOSE
   	printf( "%s: new/updated SXA node\n",
			__FUNCTION__);
   #endif

      // Find out about I/O configuration if not already known
      if (sxa->io.clc.status == XBEE_COMMAND_LIST_ERROR)
      {
   		xbee_io_query(xbee, &sxa->io, sxa->addr_ptr);
		}
	}
	#ifdef XBEE_DISCOVERY_VERBOSE
   else
   {
		printf( "%s: invalid node description of %u bytes:\n",
			__FUNCTION__, length);
		hex_dump( raw, length, HEX_DUMP_FLAG_OFFSET);
   }
	#endif
	return -EINVAL;
}

/*** BeginHeader _sxa_disc_atnd_response */
/*** EndHeader */
/**
	@brief
	Frame handler for processing AT Command Response frames.

	Should only be called by the frame dispatcher and unit tests.

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.
*/
_xbee_sxa_debug
int _sxa_disc_atnd_response( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t nd = { { 'N', 'D' } };
	const xbee_frame_local_at_resp_t FAR *resp = raw;

	if (resp->header.command.w == nd.w &&
		XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
	{
		// this is a successful ATND response
	#ifdef XBEE_DISCOVERY_VERBOSE
   	printf( "%s\n", __FUNCTION__);
   #endif
		return _sxa_disc_process_node_data( xbee, resp->value,
			length - offsetof( xbee_frame_local_at_resp_t, value));
	}

	return 0;
}


/*** BeginHeader _sxa_disc_handle_frame_0x95 */
/*** EndHeader */
/**
	@brief
	Frame handler for processing Node Identification Indicator (0x95) frames.

	Should only be called by the frame dispatcher and unit tests.

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.
*/
// can register to receive 0x95 frames and dump to stdout
_xbee_sxa_debug
int _sxa_disc_handle_frame_0x95( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	const xbee_frame_node_id_t FAR *frame = raw;

	#ifdef XBEE_DISCOVERY_VERBOSE
   	printf( "%s\n", __FUNCTION__);
   #endif
	return _sxa_disc_process_node_data( xbee, &frame->node_data,
		length - offsetof( xbee_frame_node_id_t, node_data));
}



/*** BeginHeader sxa_node_by_addr */
/*** EndHeader */
// search the node table for a node by its IEEE address
_xbee_sxa_debug
sxa_node_t FAR *sxa_node_by_addr( const addr64 FAR *ieee_be)
{
	sxa_node_t FAR *rec;

   if (!ieee_be)
   {
   	return NULL;
   }

	for (rec = sxa_list_head(); rec; rec = rec->next)
	{
		if (addr64_equal( ieee_be, &rec->id.ieee_addr_be))
		{
			return rec;
		}
	}

	return NULL;
}

/*** BeginHeader sxa_local_node */
/*** EndHeader */
// search the node table for a local node keyed by xbee pointer
_xbee_sxa_debug
sxa_node_t FAR *sxa_local_node( const xbee_dev_t *xbee)
{
	sxa_node_t FAR *rec;

	for (rec = sxa_local_table; rec; rec = rec->next_local)
	{
		if (rec->xbee == xbee)
		{
			return rec;
		}
	}

	return NULL;
}

/*** BeginHeader sxa_node_by_name */
/*** EndHeader */
// search the node table for a node by its name (NI setting)
_xbee_sxa_debug
sxa_node_t FAR *sxa_node_by_name( const char FAR *name)
{
	sxa_node_t FAR *rec;

	for (rec = sxa_list_head(); rec; rec = rec->next)
	{
		if (strcmp( rec->id.node_info, name) == 0)
		{
			return rec;
		}
	}

	return NULL;
}


/*** BeginHeader sxa_node_by_index */
/*** EndHeader */
// search the node table for a node by its index (ordinal number from 0 on up).
// The index and IEEE address remain fixed over complete power-up cycle.
_xbee_sxa_debug
sxa_node_t FAR *sxa_node_by_index( int index)
{
	sxa_node_t FAR *rec;

	for (rec = sxa_list_head(); rec; rec = rec->next)
	{
		if (rec->index == index)
		{
			return rec;
		}
	}

	return NULL;
}


/*** BeginHeader sxa_node_add */
/*** EndHeader */
// copy node_id into the node table, possibly updating existing entry
_xbee_sxa_debug
sxa_node_t FAR *sxa_node_add( xbee_dev_t *xbee, const xbee_node_id_t FAR *node_id)
{
	sxa_node_t FAR *rec;
   bool_t is_local;

	for (rec = sxa_list_head(); rec; rec = rec->next)
	{
		if (addr64_equal( &node_id->ieee_addr_be, &rec->id.ieee_addr_be))
		{
			break;
		}
	}

	if (rec == NULL)
   {
	#ifdef XBEE_DISCOVERY_VERBOSE
   	printf( "%s: new entry\n", __FUNCTION__);
   #endif
   	rec = (sxa_node_t FAR *)_sys_calloc(sizeof(*rec));
      rec->node_id_cf = _SXA_CACHED_OK;	// Get this in the discovery data
		rec->next = sxa_list_head();
      rec->index = sxa_table_count++;
      rec->xbee = xbee;
      // For the _wpan_address_t, use 64-bit addressing only (since we have it)
      _f_memcpy(&rec->address.ieee, &node_id->ieee_addr_be, sizeof(rec->address.ieee));
      is_local = !memcmp(&rec->address.ieee, &xbee->wpan_dev.address.ieee,
      									sizeof(rec->address.ieee));
      if (is_local)
      {
      	rec->addr_ptr = NULL;	// This defines it as local device entry
         // Also add to special linked list of local.  Current hardware
         // supports only one local XBee, but let's be upward compatible.
         rec->next_local = sxa_local_table;
			sxa_local_table = rec;
      }
      else
      {
   		rec->addr_ptr = &rec->address;
      }
      rec->address.network = WPAN_NET_ADDR_UNDEFINED;
      rec->groups = _sxa_default_cache_groups;
      sxa_table = rec;
   }
	#ifdef XBEE_DISCOVERY_VERBOSE
   else
   {
   	printf( "%s: updated entry\n", __FUNCTION__);
   }
   #endif

	rec->id = *node_id;

   // Update the timestamp
   rec->stamp = xbee_seconds_timer();

	return rec;
}

/*** BeginHeader sxa_node_table_dump */
/*** EndHeader */
_xbee_sxa_debug
void sxa_node_table_dump( void)
{
	sxa_node_t FAR *rec;

	for (rec = sxa_list_head(); rec; rec = rec->next)
	{
   	// Will print in reverse order of index
		printf( "%2u:%c", rec->index, rec->addr_ptr ? ' ' : '*');
	   printf( "Addr:%08" PRIx32 "-%08" PRIx32 " 0x%04x  "
	      "par:0x%04x %6s\n",
	      be32toh( rec->id.ieee_addr_be.l[0]), be32toh( rec->id.ieee_addr_be.l[1]),
	      rec->id.network_addr, rec->id.parent_addr,
	      xbee_disc_device_type_str( rec->id.device_type));
      if (rec->device_info_cf == _SXA_CACHED_OK)
      {
	   	printf( "\tHV:%04X VR:%08" PRIx32 " DD:%08" PRIx32 "\n",
         	rec->hardware_version, rec->firmware_version, rec->dd);
         printf( "\tCAPS:%s%s%s%s%s%s%s%s%s%s\n",
         	rec->caps & ZB_CAP_ADV_ADDR ? " ADV_ADDR" : "",
         	rec->caps & ZB_CAP_ZDO ? " ZDO" : "",
         	rec->caps & ZB_CAP_REMOTE_DDO ? " REMOTE_DDO" : "",
         	rec->caps & ZB_CAP_GW_FW ? " GW_FW" : "",
         	rec->caps & ZB_CAP_REMOTE_FW ? " REMOTE_FW" : "",
         	rec->caps & ZB_CAP_ZIGBEE ? " ZIGBEE" : "",
         	rec->caps & ZB_CAP_ZBPRO ? " ZBPRO" : "",
         	rec->caps & ZB_CAP_DIGIMESH ? " DIGIMESH" : "",
         	rec->caps & ZB_CAP_CHILDREN ? " CHILDREN" : "",
         	rec->caps & ZB_CAP_SE ? " SE" : "");
      }
      if (rec->node_id_cf == _SXA_CACHED_OK)
      {
	   	printf( "\tNI:[%" PRIsFAR "]\n", rec->id.node_info);
      }
	}
}


/*** BeginHeader _sxa_disc_cluster_handler */
/*** EndHeader */
/**
	@internal
	@brief
	Cluster handler for "Digi Node Identification" cluster.

	Used in the
	cluster table of #WPAN_ENDPOINT_DIGI_DATA (0xE8) for cluster
	DIGI_CLUST_NODEID_MESSAGE (0x0095).

	@param[in]		envelope	information about the frame (addresses, endpoint,
									profile, cluster, etc.)
	@param[in,out]	context

	@retval	0	handled data
	@retval	!0	some sort of error processing data

	@sa wpan_aps_handler_fn()
*/
_xbee_sxa_debug
int _sxa_disc_cluster_handler( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	// note that it is safe to cast the wpan_dev_t in the envelope to an
	// xbee_dev_t because the wpan_dev_t is the first element of the xbee_dev_t

	#ifdef XBEE_DISCOVERY_VERBOSE
   	printf( "%s\n", __FUNCTION__);
   #endif
	return _sxa_disc_process_node_data( (xbee_dev_t *)envelope->dev,
		envelope->payload, envelope->length);
}



/*** BeginHeader _sxa_io_process_response */
/*** EndHeader */
/**
	@internal
	@brief
	Function shared by multiple functions that process I/O sample
	messages (0x92 frames and ATIS responses).

	@param[in]	xbee		device that received the message
	@param[in]	raw		pointer to the message (starting at sample count field)
	@param[in]	length	length of the message

	@retval	0	successfully parsed
	@retval	!0	error parsing data
*/
_xbee_sxa_debug
int _sxa_io_process_response( const addr64 FAR *ieee_be,
		const void FAR *raw, int length)
{
   sxa_node_t FAR * sxa = sxa_node_by_addr(ieee_be);

	// We may want to add code here to confirm that length is correct,
	// based on the length of the NI value in the xbee_node_id_t.

	if (!sxa || length < 4)
	{
		return -EINVAL;
	}

	if (xbee_io_response_parse( &sxa->io, raw) == 0)
	{
		sxa->stamp = xbee_seconds_timer();
	#ifdef XBEE_IO_VERBOSE
   	xbee_io_response_dump( &sxa->io);
   #endif
		return 0;
	}
	#ifdef XBEE_IO_VERBOSE
   else
   {
		printf( "%s: invalid I/O sample of %u bytes, or unrecognized node addr:\n",
			__FUNCTION__, length);
		hex_dump( raw, length, HEX_DUMP_FLAG_OFFSET);
   }
	#endif
	return -EINVAL;
}




/*** BeginHeader _sxa_io_response_handler */
/*** EndHeader */
/* IO Response (0x92) frame handler */
_xbee_sxa_debug
int _sxa_io_response_handler(xbee_dev_t *xbee, const void FAR *raw,
									 uint16_t length, void FAR *context)
{
	const xbee_frame_io_response_t FAR *frame = raw;
   int iolen;
   /* This handler only gets invoked if AO=0 on the local
      device.  Otherwise, xbee_io_cluster_handler() is invoked. */

   iolen = length - offsetof(xbee_frame_io_response_t, num_samples) + 1;
	_sxa_io_process_response(&frame->ieee_address_be,
		&frame->num_samples, iolen);

	return 0;
}

/*** BeginHeader _sxa_io_cluster_handler */
/*** EndHeader */
_xbee_sxa_debug
int _sxa_io_cluster_handler(const wpan_envelope_t FAR *envelope,
								   void FAR *context)
{
   /* This handler only gets invoked if AO=1 or higher on the local
      device.  Otherwise, xbee_io_response_handler() is invoked. */

	_sxa_io_process_response(&envelope->ieee_address,
		envelope->payload, envelope->length);

	return 0;
}

/*** BeginHeader _sxa_remote_is_cmd_response_handler */
/*** EndHeader */
_xbee_sxa_debug
int _sxa_remote_is_cmd_response_handler(xbee_dev_t *xbee, const void FAR *raw,
										uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t is = {{'I', 'S'}};
	const xbee_frame_remote_at_resp_t FAR *resp = raw;

   if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
   {
      if (resp->header.command.w == is.w)
      {
         /* I/O sample record */
	      _sxa_io_process_response(&resp->header.ieee_address,
					resp->value,
					length - offsetof( xbee_frame_remote_at_resp_t, value));
      }
   }

	return 0;
}

/*** BeginHeader _sxa_local_is_cmd_response_handler */
/*** EndHeader */
_xbee_sxa_debug
int _sxa_local_is_cmd_response_handler(xbee_dev_t *xbee, const void FAR *raw,
										uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t is = {{'I', 'S'}};
	const xbee_frame_local_at_resp_t FAR *resp = raw;

   if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
   {
      if (resp->header.command.w == is.w)
      {
         /* I/O sample record */
        // Use the local device's IEEE address...
	      _sxa_io_process_response(&xbee->wpan_dev.address.ieee,
					resp->value,
					length - offsetof( xbee_frame_local_at_resp_t, value));
      }
   }

	return 0;
}


/*** BeginHeader sxa_get_digital_input */
/*** EndHeader */
_xbee_sxa_debug
int sxa_get_digital_input( const sxa_node_t FAR *sxa, uint_fast8_t index)
{
	return xbee_io_get_digital_input( &sxa->io, index);
}

/*** BeginHeader sxa_get_digital_output */
/*** EndHeader */
_xbee_sxa_debug
int sxa_get_digital_output( const sxa_node_t FAR *sxa, uint_fast8_t index)
{
	return xbee_io_get_digital_output( &sxa->io, index);
}

/*** BeginHeader sxa_get_analog_input */
/*** EndHeader */
_xbee_sxa_debug
int16_t sxa_get_analog_input( const sxa_node_t FAR *sxa, uint_fast8_t index)
{
	return xbee_io_get_analog_input( &sxa->io, index);
}

/*** BeginHeader sxa_set_digital_output */
/*** EndHeader */
_xbee_sxa_debug
int sxa_set_digital_output( sxa_node_t FAR *sxa,
				uint_fast8_t index, enum xbee_io_digital_output_state state)
{
	return xbee_io_set_digital_output( sxa->xbee, &sxa->io, index, state,
   				sxa->addr_ptr);
}

/*** BeginHeader sxa_io_configure */
/*** EndHeader */
_xbee_sxa_debug
int sxa_io_configure( sxa_node_t FAR *sxa,
				uint_fast8_t index, enum xbee_io_type type)
{
	return xbee_io_configure( sxa->xbee, &sxa->io, index, type,
   				sxa->addr_ptr);
}

/*** BeginHeader sxa_io_set_options */
/*** EndHeader */
_xbee_sxa_debug
int sxa_io_set_options( sxa_node_t FAR *sxa,
			uint16_t sample_rate, uint16_t change_mask)
{
	return xbee_io_set_options( sxa->xbee, &sxa->io, sample_rate, change_mask,
   				sxa->addr_ptr);
}

/*** BeginHeader sxa_io_query */
/*** EndHeader */
_xbee_sxa_debug
int sxa_io_query( sxa_node_t FAR *sxa)
{
	return xbee_io_query( sxa->xbee, &sxa->io, sxa->addr_ptr);
}

/*** BeginHeader sxa_io_query_status */
/*** EndHeader */
_xbee_sxa_debug
int sxa_io_query_status( sxa_node_t FAR *sxa)
{
	return xbee_io_query_status( &sxa->io);
}


/*** BeginHeader sxa_io_dump */
/*** EndHeader */
_xbee_sxa_debug
void sxa_io_dump( sxa_node_t FAR *sxa)
{
	// Debug only
	xbee_io_t FAR * io = &sxa->io;
	int i;

   for (i = 0; i < 13; ++i)
   {
   	printf(" %d", io->config[i]);
   }
   printf("\n");
}

/*** BeginHeader sxa_init_or_exit */
/*** EndHeader */

const FAR xbee_atcmd_reg_t _sxa_init_query_regs[] = {
   // ATNO2 used so that ND command returns local node info as well as remote.
	XBEE_ATCMD_REG_SET_8( 'N', 'O', 2),
   XBEE_ATCMD_REG_END_CMD('N', 'D')
};

_xbee_sxa_debug
sxa_node_t FAR * sxa_init_or_exit(xbee_dev_t *xbee,
							const wpan_endpoint_table_entry_t *ep_table,
                     int verbose
                     )
{
	//xbee_command_list_context_t clc;
	xbee_node_id_t nid;
   sxa_node_t FAR *sxa;
	uint16_t t;
	int status;

   memset(&nid, 0, sizeof(nid));

	// give the XBee 500ms to wake up after resetting it (or exit if it
   // receives a packet)
	t = XBEE_SET_TIMEOUT_MS(500);
	while (! XBEE_CHECK_TIMEOUT_MS(t) && xbee_dev_tick( xbee) <= 0);

   xbee_wpan_init(xbee, ep_table);

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)
	xbee_cmd_init_device( xbee);
   if (verbose)
   {
		printf( "Waiting for driver to query the XBee device...\n");
   }
	do {
		wpan_tick( &xbee->wpan_dev);
		status = xbee_cmd_query_status( xbee);
	} while (status == -EBUSY);
	if (status)
	{
   	if (verbose)
      {
			printf( "Error %d waiting for query to complete.\n", status);
      	return NULL;
      }
      exit(1);
	}

   // Add initial SXA node for the local device.  We key it by its own
   // MAC address (rather than e.g. zero) since in future there could be
   // multiple local XBees.
   nid.ieee_addr_be = xbee->wpan_dev.address.ieee;
	sxa = sxa_node_add(xbee, &nid);
	// Nothing known as yet.  Since we set NO2, will discover local node
   // with ATND, and hence stuff will get filled in.
   _sxa_set_cache_status(sxa, NULL, SXA_CACHED_NODE_ID, _SXA_CACHED_UNKNOWN);

	xbee_cmd_list_execute(xbee,
   								//&clc,
                           &sxa->io.clc,
									_sxa_init_query_regs,
                           &sxa->id,
                           NULL);
	//while (xbee_cmd_list_status(&clc) == -EBUSY)
   //{
   //	sxa_tick();
   //}

	// report on the settings
   if (verbose)
   {
		xbee_dev_dump_settings( xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);
   }

   // For convenience, return the local node we just created.  The app is
   // likely to use it frequently.
   return sxa;
}



/*** BeginHeader _sxa_launch_update */
/*** EndHeader */

void _sxa_cmd_list_cache_cb(
	const xbee_cmd_response_t FAR 		*response,
   const struct xbee_atcmd_reg_t FAR	*reg,
	void FAR										*base)
{
	sxa_cached_t FAR *c;

   if (response != NULL && reg != NULL && base != NULL &&
   		(response->flags & XBEE_CMD_RESP_MASK_STATUS) != XBEE_AT_RESP_SUCCESS)
	{
   	// Command to set a cached entry failed.
      c = SXA_OFFSET(base, reg->offset - _SXA_CACHED_PREFIX_SIZE);
		c->flags = _SXA_CACHED_ERROR;
   }
}

void _sxa_launch_update(sxa_node_t FAR *sxa,
								const struct _xbee_reg_descr_t FAR *xrd,
                        uint16_t cache_group)
{
	// xrd parameter may be null if launching a group other than 'MISC'.
   const xbee_atcmd_reg_t FAR *list = NULL;
   const sxa_cached_group_t FAR * group = _sxa_cache_group_by_id(cache_group);

   if (group)
   {
   	list = group->get_list;
      sxa->doing_group = group;
      #ifdef SXA_CACHE_VERBOSE
      printf("%s: node %u group %u (list=%" PRIpFAR ")\n",
      	__FUNCTION__, sxa->index, group->id, list);
      #endif
   }
   else if (xrd != NULL)
   {
      // Single register update
      sxa->doing_group = NULL;
      _f_memset(sxa->misc_reg, 0, sizeof(sxa->misc_reg));
      sxa->misc_reg[0].command.w = *(uint16_t *)xrd->alias;
      sxa->misc_reg[0].callback = _sxa_cmd_list_cache_cb;
      sxa->misc_reg[0].bytes = xrd->sxa_len - _SXA_CACHED_PREFIX_SIZE;
      sxa->misc_reg[0].offset = xrd->sxa_offs + _SXA_CACHED_PREFIX_SIZE;
      if (sxa->misc_reg[0].bytes < 5 && strcmp(xrd->alias, "CC"))
      {
         // Fields 4 or less in length are always numbers (except CC command),
         // and hence we convert to host byte order.
         sxa->misc_reg[0].type = XBEE_CLT_COPY_BE;
      }
      else
      {
         // Everything else is a string or 64-bit address, so
         // no byte ordering change.
         sxa->misc_reg[0].type = XBEE_CLT_COPY;
      }
      #ifdef SXA_CACHE_VERBOSE
      printf("%s: node %u misc reg %s (len=%u)\n",
      	__FUNCTION__, sxa->index, xrd->alias, sxa->misc_reg[0].bytes);
      #endif
      list = sxa->misc_reg;
   }
   if (list)
   {
      xbee_cmd_list_execute(sxa->xbee,
                  &sxa->io.clc,
                  list,
                  sxa,
                  sxa->addr_ptr);
	}
   else if (cache_group == SXA_CACHED_IO_CONFIG)
   {
   	// Ugh: bit of an exception for I/O: call a function rather than
      // a command list.
		xbee_io_query(sxa->xbee, &sxa->io, sxa->addr_ptr);
   }
   _sxa_set_cache_status(sxa, xrd, cache_group, _SXA_CACHED_BUSY);
}


/*** BeginHeader sxa_tick */
/*** EndHeader */

void sxa_tick(void)
{
	sxa_node_t FAR *sxa;
   xbee_dev_t *xbee;
   const struct _xbee_reg_descr_t FAR * xrd;
   int err;

	// Iterate through the local devices, and drive their tick functions
   for (sxa = sxa_local_table; sxa; sxa = sxa->next_local)
   {
		xbee = sxa->xbee;
		wpan_tick( &xbee->wpan_dev);
   }
   // Also expire any finished commands
   xbee_cmd_tick();

   // Iterate all devices and update any missing information that has been
   // requested (and didn't get error last time requested).
   for (sxa = sxa_table; sxa; sxa = sxa->next)
   {
      if (!sxa->queued && !sxa->doing_group)
      {
      	// If nothing queued, see if basic cache groups are unknown
         // and (if so) launch an update
         if (sxa_cache_status(sxa, NULL, SXA_CACHED_DEVICE_INFO) ==
         		_SXA_CACHED_UNKNOWN)
				_sxa_launch_update(sxa, NULL, SXA_CACHED_DEVICE_INFO);
         else if (sxa_cache_status(sxa, NULL, SXA_CACHED_NODE_ID) ==
         		_SXA_CACHED_UNKNOWN)
				_sxa_launch_update(sxa, NULL, SXA_CACHED_NODE_ID);
         else if (sxa_cache_status(sxa, NULL, SXA_CACHED_IO_CONFIG) ==
         		_SXA_CACHED_UNKNOWN)
				_sxa_launch_update(sxa, NULL, SXA_CACHED_IO_CONFIG);
      }

      // Currently active register value update (if any)
      // (check for sxa->queued is actually redundant, since the 2nd
      // condition only holds if sxa->queued is not null)
      if (sxa->q_index < sxa->nqueued)
      {
			assert(sxa->queued != NULL);
			xrd = sxa->queued[sxa->q_index];
      }
      else
      {
      	xrd = NULL;
      }

      err = 0;

		switch (sxa->io.clc.status)
      {
      	case XBEE_COMMAND_LIST_RUNNING:
         	continue;
         case XBEE_COMMAND_LIST_ERROR:
         case XBEE_COMMAND_LIST_TIMEOUT:
            err = 1;
            sxa->io.clc.status = XBEE_COMMAND_LIST_DONE;
            // fall through...
         default:
            if (xrd)
            {
            	// We were processing an entry, and got result.  Set
               // status appropriately and advance to next entry.
               if (!err)
               {
               	// No overall error for the command list, however the
                  // register in the list may not have been read, so
                  // set err in this case
                  err = sxa_cache_status(sxa, xrd, xrd->sxa_cache_group) ==
                         _SXA_CACHED_ERROR;
               }
      #ifdef SXA_CACHE_VERBOSE
      			printf("%s: node %u got result from %s (err=%d)\n",
               		__FUNCTION__, sxa->index, xrd->alias, err);
      #endif
         		_sxa_set_cache_status(sxa, xrd, xrd->sxa_cache_group,
               		err ? _SXA_CACHED_ERROR : _SXA_CACHED_OK);
					++sxa->q_index;
            }
            else
            {
            	// Not processing an entry, but may have been processing
               // a general group update.
               if (sxa->doing_group)
               {
      #ifdef SXA_CACHE_VERBOSE
      				printf("%s: node %u got result from group %d (err=%d)\n",
                  	 __FUNCTION__, sxa->index, sxa->doing_group->id, err);
      #endif
	               _sxa_set_cache_status(sxa, NULL, sxa->doing_group->id,
	                     err ? _SXA_CACHED_ERROR : _SXA_CACHED_OK);
               }
            }

            sxa->doing_group = NULL;

            if (sxa->q_index >= sxa->nqueued)
            {
            	// Start at the beginning if reached end.  This is because we
               // allow old entries to be updated before the list is
               // finished executing.  (Otherwise, we would have been done).
            	sxa->q_index = 0;
				}

            while (sxa->q_index < sxa->nqueued)
            {
            	xrd = sxa->queued[sxa->q_index];
            	if (sxa_cache_status(sxa, xrd, xrd->sxa_cache_group) ==
               			_SXA_CACHED_PENDING)
               {
               	// Found entry which we want to update, so launch it
                  // and break out.  We need a switch statement here for
                  // cache groups; this is where the logic resides for
                  // initiating the appropriate query.  Note that when a
                  // register in a cache group is updated, then all other
                  // registers in the same group will have been updated
                  // too, so their entries subsequently will be skipped.
                  _sxa_launch_update(sxa, xrd, xrd->sxa_cache_group);
                  break;
               }
               ++sxa->q_index;
            }

            if (sxa->q_index >= sxa->nqueued)
            {
            	// After scanning, there were no updates launched, thus the
               // list is complete and we can reset it.
               sxa->nqueued = 0;
               if (sxa->queued)
               {
#ifdef SXA_CACHE_VERBOSE
						printf("%s: node %u queue completed\n",
                  	__FUNCTION__, sxa->index);
#endif
	               _sys_free(sxa->queued);
	               sxa->queued = NULL;
	               sxa->q_index = 0;
               }
            }
            break;
      }
   }

}


/*** BeginHeader sxa_cache_status */
/*** EndHeader */
sxa_cache_flags_t sxa_cache_status(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb,
                     uint16_t cache_group)
{
	sxa_cached_t FAR * c;
   const sxa_cached_group_t FAR * group;

   if (cache_group == SXA_CACHED_MISC)
   {
   	c = SXA_OFFSET(sxa, zb->sxa_offs);
   	return c->flags;
   }
   else if (cache_group == SXA_CACHED_NONE)
   {
   	return _SXA_CACHED_OK;
   }
   group = _sxa_cache_group_by_id(cache_group);
   if (!group)
   {
   	return _SXA_CACHED_BAD_GROUP;
   }
   return *(sxa_cache_flags_t FAR *)SXA_OFFSET(sxa, group->flags_offs);
}

/*** BeginHeader _sxa_set_cache_status */
/*** EndHeader */
void _sxa_set_cache_status(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb,
                     uint16_t cache_group,
                     sxa_cache_flags_t flags)
{
	sxa_cached_t FAR * c;
   const sxa_cached_group_t FAR * group;

   if (cache_group == SXA_CACHED_NONE || sxa == NULL)
   {
   	return;
   }
   else if (cache_group == SXA_CACHED_MISC && zb != NULL)
   {
   	c = SXA_OFFSET(sxa, zb->sxa_offs);
   	c->flags = flags;
   }

	group = _sxa_cache_group_by_id(cache_group);
   if (group)
		*(sxa_cache_flags_t FAR *)SXA_OFFSET(sxa, group->flags_offs) = flags;
}

/*** BeginHeader sxa_cached_value_ptr */
/*** EndHeader */
void FAR * sxa_cached_value_ptr(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb)
{
	sxa_cached_t FAR * c;

  	c = SXA_OFFSET(sxa, zb->sxa_offs);
   if (zb->sxa_cache_group == SXA_CACHED_MISC)
   {
   	return c+1;
   }
   return c;
}

/*** BeginHeader sxa_cached_value_len */
/*** EndHeader */
uint16_t sxa_cached_value_len(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb)
{
   if (zb->sxa_cache_group == SXA_CACHED_MISC)
   {
   	return zb->sxa_len - _SXA_CACHED_PREFIX_SIZE;
   }
   return zb->sxa_len;
}

/*** BeginHeader sxa_find_pending_cache */
/*** EndHeader */
int sxa_find_pending_cache(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb)
{
	uint16_t	i;

   for (i = 0; i < sxa->nqueued; ++i)
   {
   	if (sxa->queued[i] != zb)
      	continue;
      if (sxa_cache_status(sxa, zb, zb->sxa_cache_group) &
      			 (_SXA_CACHED_BUSY | _SXA_CACHED_PENDING))
      	return 1;
   }
	return 0;
}

/*** BeginHeader sxa_schedule_update_cache */
/*** EndHeader */
int sxa_schedule_update_cache(sxa_node_t FAR * sxa,
							const _xbee_reg_descr_t FAR * zb)
{
	if (!sxa->queued)
   {
   	sxa->queued = _sys_malloc(sizeof(void FAR *) * _SXA_MAX_QUEUED);
      sxa->q_index = 0;
      sxa->nqueued = 0;
   }
   // If alread enqueued and pending (or busy), don't add again
   if (sxa_find_pending_cache(sxa, zb))
   	return 1;
   if (sxa->nqueued >= _SXA_MAX_QUEUED)
   	return 0;
   // Else must be unknown, OK, or in error: add with pending status
   if (sxa->q_index >= sxa->nqueued)
   	// Keep index past end if nothing launched
   	++sxa->q_index;
   sxa->queued[sxa->nqueued++] = zb;
   _sxa_set_cache_status(sxa, zb, zb->sxa_cache_group, _SXA_CACHED_PENDING);
   return 1;
}


/*** BeginHeader sxa_is_digi */
/*** EndHeader */
int sxa_is_digi(const sxa_node_t FAR *sxa)
{
	// This function is for upward compatibility for when the SXA layer is
   // enhanced to support non-XBee devices.  For now, we only do ATND
   // discovery, so always return TRUE.
   return 1;
}




