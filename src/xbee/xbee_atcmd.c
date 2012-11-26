/*
 * Copyright (c) 2009-2012 Digi International Inc.,
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
	@file xbee_atcmd.c
	Code related to sending AT command requests and processing the responses.

	@todo move xbee_cmd_simple and xbee_identify into xbee_device layer?

	@todo Add code to return an error (and disable the stack) if the XBee
			responds with an IEEE address not in Digi's allocation?

	@todo review todo list at the top of the file -- lots to do

	- Need different levels of VERBOSE output.
		- Errors only
		- Errors + program flow?
		- Lots of details
		- Everything including packet dumps?

	- Add flag for "multiple responses expected" -- taken care of by callback
	returning 1?

	- Add a function to change the timeout for a request (ATND, ATIS?).

	- Create unit tests
		- error on full table, insert dupe into table, purge expired entries from
		table, dispatcher purges entry on 0 return, dispatcher keeps entry on
		-1 return
*/

/*** BeginHeader */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/atcmd.h"

#ifndef __DC__
	#define _xbee_atcmd_debug
#elif defined XBEE_ATCMD_DEBUG
	#define _xbee_atcmd_debug	__debug
#else
	#define _xbee_atcmd_debug	__nodebug
#endif
/*** EndHeader */


/*** BeginHeader xbee_cmd_request_table */
/*** EndHeader */
/// Table used to keep track of outstanding requests.
FAR xbee_cmd_request_t xbee_cmd_request_table[XBEE_CMD_REQUEST_TABLESIZE];


/*** BeginHeader xbee_cmd_tick */
/*** EndHeader */
/**
	@brief
	This function should be called periodically (at least every few seconds)
	to expire old entries from the AT Command Request table.

	@return	>0	number of requests expired
	@return	0	none of the requests in the table expired
*/
_xbee_atcmd_debug
int xbee_cmd_tick( void)
{
	static uint8_t					last_time;
	uint8_t							now;
	uint_fast8_t					i;
	int								reuse;
	xbee_cmd_request_t	FAR *request;
	xbee_cmd_response_t			expired;
	xbee_dev_t						*device;
	int								count = 0;

	// only process expirations once per second
	now = (uint8_t) xbee_seconds_timer();
	if (last_time == now)
	{
		return 0;
	}
	last_time = now;

	memset( &expired, 0, sizeof expired);
	// set the timeout flag, but also make sure STATUS is invalid
	expired.flags = XBEE_CMD_RESP_FLAG_TIMEOUT | XBEE_CMD_RESP_MASK_STATUS;
	// walk through the table of requests, looking for expired entries
	for (request = xbee_cmd_request_table, i = XBEE_CMD_REQUEST_TABLESIZE;
		i; ++request, --i)
	{
		if (request->device && XBEE_CHECK_TIMEOUT_SEC(request->timeout))
		{
			++count;
			expired.handle =
				((XBEE_CMD_REQUEST_TABLESIZE - i) << 8) | request->sequence;

			#ifdef XBEE_ATCMD_VERBOSE
				printf( "%s: request 0x%04x timed out\n", __FUNCTION__,
					expired.handle);
			#endif
			reuse = XBEE_ATCMD_DONE;
			if (request->callback)
			{
				expired.device = request->device;
				expired.context = request->context;
				expired.command.w = request->command.w;

				#ifdef XBEE_ATCMD_VERBOSE
					printf( "%s: dispatch to handler @%p w/context %" PRIpFAR "\n",
						__FUNCTION__, request->callback, expired.context);
				#endif
				reuse = request->callback( &expired);
			}
			if (reuse != XBEE_ATCMD_REUSE)
			{
				#ifdef XBEE_ATCMD_VERBOSE
					printf( "%s: releasing expired request 0x%04x\n",
						__FUNCTION__, expired.handle);
				#endif

				device = request->device;
				_xbee_cmd_release_request( request);
				if (device->flags & XBEE_DEV_FLAG_QUERY_REFRESH)
				{
					// If we're waiting to refresh network settings due to a full
					// command table, now's our chance since we just opened a slot.
					xbee_cmd_query_device( device, 1);
				}
			}
		}
	}

	return count;
}

/*** BeginHeader _xbee_cmd_handle_to_address */
xbee_cmd_request_t FAR *_xbee_cmd_handle_to_address( int16_t handle);
/*** EndHeader */
/**
	@internal

	Convert a handle (16-bit int) to a pointer into the command
	request table.  Ensures that the handle is valid (correct
	index and sequence bytes).

	@param[in]	handle	Handle returned from xbee_cmd_create().

	@return	Pointer into the request table or NULL if handle is invalid.

	@see	xbee_cmd_create()

*/
_xbee_atcmd_debug
xbee_cmd_request_t FAR *_xbee_cmd_handle_to_address( int16_t handle)
{
	xbee_cmd_request_t FAR *request;

	if (handle < 0 || handle >= (XBEE_CMD_REQUEST_TABLESIZE << 8))
	{
		return NULL;
	}

	request = &xbee_cmd_request_table[handle >> 8];

	if (request->sequence != (handle & 0x00FF))
	{
		return NULL;
	}

	return request;
}



/*** BeginHeader xbee_cmd_list_execute */
/*** EndHeader */
/**	@internal
	Helper function to send off the next command in the list.
*/
_xbee_atcmd_debug
int _xbee_cmd_issue_list( xbee_dev_t *xbee,
									xbee_command_list_context_t FAR *clc,
                           const xbee_cmd_response_t FAR *response
									)
{
	uint8_t FAR 	*devptr;
	int16_t			request;
	uint32_t			param;
   const xbee_atcmd_reg_t FAR *reg = clc->list + clc->index;

#ifdef XBEE_ATCMD_VERBOSE
   printf( "%s: next command AT%c%c\n", __FUNCTION__,
         reg->command.str[0], reg->command.str[1]);
#endif
   if (response)
   {
   	request = response->handle;
   }
   else
   {
   	request = xbee_cmd_create( xbee, clc->list->command.str);
      if (request < 0)
      {
#ifdef XBEE_ATCMD_VERBOSE
   		printf( "%s: can't create request (%d)\n", __FUNCTION__,
         	request);
#endif
      	return request;
      }
   }

	xbee_cmd_set_command( request, reg->command.str);
	devptr = (uint8_t FAR *)clc->base + reg->offset;
	switch (reg->type)
	{
	   default:
	      // This is not a setting command, clear the parameter.
	      xbee_cmd_set_param_bytes( request, NULL, 0);
	      break;
	   case XBEE_CLT_SET_8:
	      xbee_cmd_set_param( request, reg->bytes);
	      break;
	   case XBEE_CLT_SET:
	      xbee_cmd_set_param_bytes( request, devptr, reg->bytes);
	      break;
	   case XBEE_CLT_SET_STR:
	      xbee_cmd_set_param_str( request, (char FAR *)devptr);
	      break;
	   case XBEE_CLT_SET_BE:
	      switch (reg->bytes)
	      {
	         default:
	         case 1:
	            param = *(uint8_t FAR *)devptr;
	            break;

	         case 2:
	            param = *(uint16_t FAR *)devptr;
	            break;

	         case 4:
	            param = *(uint32_t FAR *)devptr;
	            break;
	      }
	      xbee_cmd_set_param( request, param);
	      break;
	}

   return request;
}

/**	@internal
	Callback used by xbee_cmd_query_device (and possibly other
	functions) to learn about the attached XBee device
	(hardware/firmware version, address, etc.)

	View function help for xbee_cmd_set_callback() for details on
	xbee_cmd_response_t structure.

	*** This callback should only be called by xbee_cmd_handle_response(). ***
*/
/*
	@todo Figure out timeouts for this table.
		What if there's a timeout?  Should we resend or move on to the
		next register to query? Currently, it halts the entire list.
*/
_xbee_atcmd_debug
int _xbee_cmd_list_callback( const xbee_cmd_response_t FAR *response)
{
	xbee_dev_t *xbee;
	uint8_t FAR *devptr;
	const xbee_atcmd_reg_t FAR *reg;
   xbee_command_list_context_t FAR *clc = response->context;

	int16_t			request;
	uint16_t			command;

	xbee = response->device;

	if (response->flags & XBEE_CMD_RESP_FLAG_TIMEOUT)
	{
#ifdef XBEE_ATCMD_VERBOSE
		 printf( "%s: timed out\n", __FUNCTION__);
#endif
   	clc->status = XBEE_COMMAND_LIST_TIMEOUT;
      // Find 'end handler' callback...
		for (reg = clc->list; XBEE_ATCMD_REG_VALID(reg); ++reg);
   	if (reg->callback)
      {
      	// A final callback was specified (via XBEE_ATCMD_REG_END_CB)
#ifdef XBEE_ATCMD_VERBOSE
		 	printf( "%s: found final callback\n", __FUNCTION__);
#endif
         reg->callback(response, reg, clc->base);
      }
		return XBEE_ATCMD_DONE;			// give up
	}

	request = response->handle;
	command = response->command.w;

   reg = clc->list + clc->index;

	// Basic sanity check that response was for the command we last issued
   if (command == reg->command.w)
   {
#ifdef XBEE_ATCMD_VERBOSE
      printf( "%s: matched command result AT%c%c\n", __FUNCTION__,
            reg->command.str[0],reg->command.str[1]);
#endif
      // found it in the list, copy the response if status is success
      if ((response->flags & XBEE_CMD_RESP_MASK_STATUS)
                                                == XBEE_AT_RESP_SUCCESS)
      {
         // First do 'copy' action if specified, then do callback.
         devptr = (uint8_t FAR *)clc->base + reg->offset;
         switch (reg->type)
         {
            default:
               // Was not a copy command.
               break;
            case XBEE_CLT_COPY:
               // Copy minimum of response length and receiving field length
               _f_memcpy( devptr, response->value_bytes,
                  response->value_length < reg->bytes ?
                     response->value_length : reg->bytes);
               // Set the remainder, if any, to nulls
               if (response->value_length < reg->bytes)
               {
                  _f_memset( devptr + response->value_length,
                              0,
                              reg->bytes - response->value_length);
               }
               break;
            case XBEE_CLT_COPY_BE:
               switch (reg->bytes)
               {
                  case 1:
                     // Even though one byte doesn't technically need a
                     // swap, support it here for convenience of truncating
                     // a longer response value.
                     *(uint8_t FAR *)devptr = (uint8_t) response->value;
                     break;

                  case 2:
                     *(uint16_t FAR *)devptr = (uint16_t) response->value;
                     break;

                  case 4:
                     *(uint32_t FAR *)devptr = (uint32_t) response->value;
                     break;
               }
               break;
         }
      }
      if (reg->callback)
      {
         reg->callback(response, reg, clc->base);
      }
   }
   else
   {
   	// Command did not match
   	clc->status = XBEE_COMMAND_LIST_ERROR;
	   if (reg->callback)
	   {
	      // A final callback was specified (via XBEE_ATCMD_REG_END_CB)
         // Call with NULL reg, since this is an error and doesn't
         // correspond to any list entry.
#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: final callback (error proc)\n", __FUNCTION__);
#endif
	      reg->callback(response, NULL, clc->base);
	   }
		#ifdef XBEE_ATCMD_VERBOSE
			if (response->value_length <= 4)
			{
				printf( "%s: Unexpected response AT%.*" PRIsFAR \
					" = 0x%0*" PRIX32 "\n",
					__FUNCTION__, 2, response->command.str,
					response->value_length * 2, response->value);
			}
			else
			{
				printf( "%s: Unexpected %d-byte response to AT%.*" PRIsFAR "\n",
					__FUNCTION__, response->value_length, 2, response->command.str);
			}
		#endif
      // Give up
		return XBEE_ATCMD_DONE;
   }

   // Try next list entry
   ++clc->index;
   reg = clc->list + clc->index;
   if (XBEE_ATCMD_REG_VALID(reg))
   {
      request = _xbee_cmd_issue_list( xbee, clc, response);

	   xbee_cmd_send( request);

	   if (reg->type == XBEE_CLT_LAST)
	   {
	      // Last command, with no response processing, so release
	      // handle after sending.
	      xbee_cmd_release_handle( request);
	   }
		else
		{
      	return XBEE_ATCMD_REUSE;         // keep this request alive
      }
   }

	// Got through to end of list.
#ifdef XBEE_ATCMD_VERBOSE
   printf( "%s: done\n", __FUNCTION__);
#endif
   clc->status = XBEE_COMMAND_LIST_DONE;
   if (reg->callback)
   {
      // A final callback was specified (via XBEE_ATCMD_REG_END_CB)
#ifdef XBEE_ATCMD_VERBOSE
      printf( "%s: final callback\n", __FUNCTION__);
#endif
      reg->callback(response, reg, clc->base);
   }

	// Finished last command, tell dispatcher that we're done and it can delete
	// the request entry.
	return XBEE_ATCMD_DONE;
}


/**
	@brief
	Execute a list of AT commands.

   @param[in,out] xbee Device to execute commands
	@param[out]	clc	List head to set up.  This must be static since callback
   						functions access it asynchronously.
   @param[in]  list  First entry of list of commands to execute.  The list
                     must be terminated by either #XBEE_ATCMD_REG_END or
                     #XBEE_ATCMD_REG_END_CB.  List entries are created using
                     #XBEE_ATCMD_REG macros etc.
   @param[in]	base	Base address of a structure to fill in with command
   						results or as a source of values to set.
	@param[in]	address Remote address, or NULL if local device.

	@retval	0	started sending commands from the list
	@retval	-ENOSPC	the AT command table is full (increase the compile-time
							macro XBEE_CMD_REQUEST_TABLESIZE)
	@retval	-EINVAL	an invalid parameter was passed to the function

	@see	xbee_cmd_list_status()
*/
_xbee_atcmd_debug
int xbee_cmd_list_execute(
			xbee_dev_t *xbee,
 			xbee_command_list_context_t FAR *clc,
         const xbee_atcmd_reg_t FAR *list,
         void FAR *base,
			const wpan_address_t FAR *address
         )
{
	int16_t request;
	int error;

   clc->base = base;
   clc->list = list;
   clc->status = XBEE_COMMAND_LIST_RUNNING;
   clc->index = 0;
	request = _xbee_cmd_issue_list( xbee, clc, NULL);
	if (request < 0)
	{
		return request;
	}
   if (address)
   {
   	xbee_cmd_set_target(request, &address->ieee, address->network);
   }
	error = xbee_cmd_set_callback( request, _xbee_cmd_list_callback, clc);

	return error ? error : xbee_cmd_send( request);
}

/*** BeginHeader xbee_cmd_list_status */
/*** EndHeader */
/**
	@brief
	Determine status of command list execution.

	@param[in]	clc	List head passed to xbee_cmd_list_execute().

	@retval	XBEE_COMMAND_LIST_RUNNING currently executing commands
	@retval	XBEE_COMMAND_LIST_DONE successfully completed
	@retval	XBEE_COMMAND_LIST_TIMEOUT timed out
	@retval	XBEE_COMMAND_LIST_ERROR completed with error(s)

	@see	xbee_cmd_list_execute()
*/
_xbee_atcmd_debug
enum xbee_command_list_status (xbee_cmd_list_status)(
			xbee_command_list_context_t FAR *clc)
{
	return clc->status;
}

/*** BeginHeader xbee_cmd_init_device */
/*** EndHeader */
/**
	@brief
	Initialize the AT Command layer for an XBee device.

	You need to call this function before any of the other xbee_cmd_* functions.

	@param[in]	xbee	XBee device on which to enable the AT Command layer.
							This function will automatically call
					xbee_cmd_query_device if it hasn't already been called for this
					device.

	@retval	0	the XBee device was successfully configured to send and
							receive AT commands
	@retval	-EINVAL	an invalid parameter was passed to the function


	@see	xbee_cmd_query_device(), xbee_cmd_create(),
			xbee_cmd_set_command(), xbee_cmd_set_callback(), xbee_cmd_set_target(),
			xbee_cmd_set_param(), xbee_cmd_set_param_bytes(),
			xbee_cmd_set_param_str(), xbee_cmd_send()
*/
_xbee_atcmd_debug
int xbee_cmd_init_device( xbee_dev_t *xbee)
{
	int retval = 0;

	if (! xbee)
	{
		return -EINVAL;
	}

	if (xbee->flags & XBEE_DEV_FLAG_CMD_INIT)
	{
		// already registered, no need to add frame handlers and query device
		return 0;
	}

	xbee->flags |= XBEE_DEV_FLAG_CMD_INIT;

	// automatically query device if it hasn't been queried yet
	if (! (xbee->flags & XBEE_DEV_FLAG_QUERY_BEGIN))
	{
		retval = xbee_cmd_query_device( xbee, 0);
	}

	return retval;
}

/*** BeginHeader xbee_cmd_query_device, xbee_cmd_query_status */
/*** EndHeader */

_xbee_atcmd_debug
void _xbee_cmd_query_handle_eo(
			const xbee_cmd_response_t FAR *response,
   		const struct xbee_atcmd_reg_t FAR	*reg,
			void FAR										*base
			)
{
	xbee_dev_t FAR *xbee = base;

	// standard xbee_command_list_fn	API, but we don't use the 'reg' parameter
	XBEE_UNUSED_PARAMETER( reg);

	if ((response->flags & XBEE_CMD_RESP_MASK_STATUS) == XBEE_AT_RESP_SUCCESS)
	{
		if (response->value & XBEE_CMD_ATEO_USE_AUTHENTICATION)
		{
			xbee->wpan_dev.flags |= WPAN_FLAG_AUTHENTICATION_ENABLED;
		}
		else
		{
			xbee->wpan_dev.flags &= ~WPAN_FLAG_AUTHENTICATION_ENABLED;
		}
	}
#ifdef XBEE_ATCMD_VERBOSE
	else
	{
		printf( "%s: EO error 0x%02X\n", __FUNCTION__,
				response->flags & XBEE_CMD_RESP_MASK_STATUS);
	}
#endif
}

_xbee_atcmd_debug
void _xbee_cmd_query_handle_ai(
			const xbee_cmd_response_t FAR *response,
   		const struct xbee_atcmd_reg_t FAR	*reg,
			void FAR										*base
			)
{
	xbee_dev_t FAR *xbee = base;

	// standard xbee_command_list_fn	API, but we don't use the 'reg' parameter
	XBEE_UNUSED_PARAMETER( reg);

	if ((response->flags & XBEE_CMD_RESP_MASK_STATUS) == XBEE_AT_RESP_SUCCESS)
	{
		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: AI=%" PRIu32 "\n", __FUNCTION__,
				response->value);
		#endif
		if (response->value == 0)
		{
			xbee->wpan_dev.flags |= WPAN_FLAG_JOINED;
			if (xbee->wpan_dev.flags & WPAN_FLAG_AUTHENTICATION_ENABLED)
			{
				xbee->wpan_dev.flags |= WPAN_FLAG_AUTHENTICATED;
			}
		}
	}
#ifdef XBEE_ATCMD_VERBOSE
	else
	{
		printf( "%s: AI error 0x%02X\n", __FUNCTION__,
				response->flags & XBEE_CMD_RESP_MASK_STATUS);
	}
#endif
}

_xbee_atcmd_debug
void _xbee_cmd_query_handle_end(
			const xbee_cmd_response_t FAR *response,
   		const struct xbee_atcmd_reg_t FAR	*reg,
			void FAR										*base
			)
{
	#ifdef XBEE_ATCMD_VERBOSE
		char buffer[ADDR64_STRING_LENGTH];
	#endif

	// the original pointer passed to xbee_cmd_list_execute was near
	xbee_dev_t *xbee = CAST_FAR_TO_NEAR(base);

	// query process completed (maybe not successfully, but it is done)
	xbee->flags &= ~XBEE_DEV_FLAG_QUERY_INPROGRESS;

   if (!reg)
   {
	   xbee->flags |= XBEE_DEV_FLAG_QUERY_ERROR;
   }
   else if (response->flags & XBEE_CMD_RESP_FLAG_TIMEOUT)
   {
		xbee->flags |= XBEE_DEV_FLAG_QUERY_ERROR;
	#ifdef XBEE_ATCMD_VERBOSE
   	printf("%s: AT%c%c timed out!\n", __FUNCTION__,
      			response->command.str[0], response->command.str[1]);
   #endif
   }
   else
   {
	   xbee->flags |= XBEE_DEV_FLAG_QUERY_DONE;
   }

	if (xbee->flags & XBEE_DEV_FLAG_QUERY_ERROR)
	{
		if (WPAN_DEV_IS_JOINED( &xbee->wpan_dev) &&
			xbee->wpan_dev.address.network == WPAN_NET_ADDR_UNDEFINED)
		{
			xbee->flags |= XBEE_DEV_FLAG_QUERY_REFRESH;
		}
	}

	if (xbee->flags & XBEE_DEV_FLAG_QUERY_REFRESH)
	{
		// either a need to refresh the settings came in during the last refresh,
		// or a timeout/error occured and we need to try again
		xbee_cmd_query_device( xbee, 1);
	}

   #ifdef XBEE_ATCMD_VERBOSE
      printf( "%s: HV=0x%x  VR=0x%" PRIx32 "  IEEE=%" PRIsFAR \
         "  net=0x%04" PRIx16 "\n", __FUNCTION__,
         xbee->hardware_version, xbee->firmware_version,
         addr64_format( buffer, &xbee->wpan_dev.address.ieee),
         xbee->wpan_dev.address.network);
   #endif
}

/**	@internal
	List of XBee registers to query at startup; used to learn about the
	attached XBee module.
*/

const xbee_atcmd_reg_t _xbee_atcmd_query_regs[] = {
	XBEE_ATCMD_REG( 'H', 'V', XBEE_CLT_COPY_BE, xbee_dev_t, hardware_version),
	XBEE_ATCMD_REG( 'V', 'R', XBEE_CLT_COPY_BE, xbee_dev_t, firmware_version),
	XBEE_ATCMD_REG( 'S', 'H', XBEE_CLT_COPY, xbee_dev_t, wpan_dev.address.ieee.l[0]),
	XBEE_ATCMD_REG( 'S', 'L', XBEE_CLT_COPY, xbee_dev_t, wpan_dev.address.ieee.l[1]),
#ifdef XBEE_DEVICE_ENABLE_ATMODE
	XBEE_ATCMD_REG( 'G', 'T', XBEE_CLT_COPY_BE, xbee_dev_t, guard_time),
	XBEE_ATCMD_REG( 'C', 'T', XBEE_CLT_COPY_BE, xbee_dev_t, idle_timeout),
	XBEE_ATCMD_REG( 'C', 'C', XBEE_CLT_COPY, xbee_dev_t, escape_char),
#endif
	XBEE_ATCMD_REG_CB( 'E', 'O', _xbee_cmd_query_handle_eo, 0),
	XBEE_ATCMD_REG_CB( 'A', 'I', _xbee_cmd_query_handle_ai, 0),
	// Start here when refreshing values after MODEM_STATUS changes to JOINED.
	XBEE_ATCMD_REG( 'N', 'P', XBEE_CLT_COPY_BE, xbee_dev_t, wpan_dev.payload),
	XBEE_ATCMD_REG( 'M', 'Y', XBEE_CLT_COPY_BE, xbee_dev_t, wpan_dev.address.network),
   XBEE_ATCMD_REG_END_CB(_xbee_cmd_query_handle_end, 0)
};

// Set dynamically
xbee_command_list_context_t _xbee_atcmd_query_regs_head;


/// Offset into _xbee_atcmd_query_regs to use when refreshing the xbee_dev_t/
/// wpan_dev_t structure.
#ifdef XBEE_DEVICE_ENABLE_ATMODE
	#define XBEE_ATCMD_REG_REFRESH_IDX	(6+3)
#else
	#define XBEE_ATCMD_REG_REFRESH_IDX	6
#endif

/**
	@brief
	Learn about the underlying device by sending a series of
	commands and storing the results in the xbee_dev_t.

	This function will likely get called by the XBee stack
	at some point in the startup/initialization phase.

	Use xbee_cmd_query_status() to check on the progress of
	querying the device.

	@param[in,out]	xbee	XBee device to query.
	@param[in]		refresh	if non-zero, just refresh the volatile values
									(e.g., network settings, as opposed to device
									serial number)

	@retval	0			Started querying device.
	@retval	-EBUSY	Transmit serial buffer is full, or XBee is not accepting
							serial data (deasserting /CTS signal).

	@see xbee_cmd_init_device(), xbee_cmd_query_status()

*/
_xbee_atcmd_debug
int xbee_cmd_query_device( xbee_dev_t *xbee, uint_fast8_t refresh)
{
	int error;

	if (xbee == NULL)
	{
		return -EINVAL;
	}

	if (refresh)
	{
		// set flag indicating that we need to refresh the registers
		xbee->flags |= XBEE_DEV_FLAG_QUERY_REFRESH;
		refresh = XBEE_ATCMD_REG_REFRESH_IDX;
	}

	if (xbee->flags & XBEE_DEV_FLAG_QUERY_INPROGRESS)
	{
		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: aborting; query already in progress\n", __FUNCTION__);
		#endif
		// note that if we were trying to refresh, we've set a flag
		// (QUERY_REFRESH) and can restart the refresh after the current
		// pass finishes
		return 0;
	}

   error = xbee_cmd_list_execute(xbee,
   								&_xbee_atcmd_query_regs_head,
									_xbee_atcmd_query_regs + refresh,
                           xbee, NULL);

	// successfully started query
	if (error == 0)
	{
		xbee->flags |= XBEE_DEV_FLAG_QUERY_BEGIN |
							XBEE_DEV_FLAG_QUERY_INPROGRESS;
		xbee->flags &=
			~(XBEE_DEV_FLAG_QUERY_ERROR
				| XBEE_DEV_FLAG_QUERY_DONE
				| XBEE_DEV_FLAG_QUERY_REFRESH);
	}

	return error;
}

/**
	@brief
	Check the status of querying an XBee device, as initiated by
	xbee_cmd_query_device().

	@param[in]	xbee	device to check

	@retval	0				query completed
	@retval	-EINVAL		\a xbee is NULL
	@retval	-EBUSY		query underway
	@retval	-ETIMEDOUT	query timed out
   @retval  -EIO     	halted, but query may not have completed
   								(unexpected response)
*/
_xbee_atcmd_debug
int xbee_cmd_query_status( xbee_dev_t *xbee)
{
	if (! xbee)
	{
		return -EINVAL;
	}

	xbee_cmd_tick();

   return xbee_cmd_list_status(&_xbee_atcmd_query_regs_head);
}


/*** BeginHeader xbee_cmd_create */
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C1855		// Function intentially uses recursion
#endif
/**
	@brief
	Allocate an AT Command request.

	@param[in]	xbee	XBee device to use as the target (local) or to send
					through (remote).  This function will automatically call
					xbee_cmd_init() if it hasn't already been called for this
					device.

	@param[in]	command	Two-letter AT Command to send
								(e.g., "VR", "NI", etc.).

	@retval	>0			a "handle" to a request that can be built up and sent
							to a local (serially-attached) or remote XBee module.
	@retval	-ENOSPC	the AT command table is full (increase the compile-time
							macro XBEE_CMD_REQUEST_TABLESIZE)
	@retval	-EINVAL	an invalid parameter was passed to the function

	@see		xbee_cmd_init_device(), xbee_cmd_query_device(),
				xbee_cmd_set_command(), xbee_cmd_set_callback(),
				xbee_cmd_set_target(), xbee_cmd_set_param(),
				xbee_cmd_set_param_bytes(), xbee_cmd_set_param_str(),
				xbee_cmd_send()
*/
_xbee_atcmd_debug
int16_t xbee_cmd_create( xbee_dev_t *xbee, const char FAR command[3])
{
	static uint8_t initialized = 0;
	int_fast8_t index;
	int16_t handle;
	xbee_cmd_request_t FAR *request;

	if (! (xbee && command))
	{
		return -EINVAL;
	}

	if (! initialized)
	{
		initialized = 1;
		_f_memset( xbee_cmd_request_table, 0, sizeof(xbee_cmd_request_table));
	}

	if (! (xbee->flags & XBEE_DEV_FLAG_CMD_INIT))
	{
		// user hasn't called xbee_cmd_init() yet, do it for them
		xbee_cmd_init_device( xbee);
	}

	request = xbee_cmd_request_table;
	for (index = 0; index < XBEE_CMD_REQUEST_TABLESIZE; ++request, ++index)
	{
		if (request->device == NULL)
		{
			break;
		}
	}

	if (index == XBEE_CMD_REQUEST_TABLESIZE)
	{
		// all slots in table are in use

		// re-use `initialized` static to limit recursion
		if (initialized != 2)
		{
			initialized = 2;
			xbee_cmd_tick();		// try to free up a slot
			handle = xbee_cmd_create( xbee, command);
			initialized = 1;

			return handle;
		}
		return -ENOSPC;
	}

	handle = (index << 8) | request->sequence;

	// clear out most of the entry (preserve sequence)
	_f_memset( &request->timeout, 0,
					sizeof(*request) - offsetof(xbee_cmd_request_t, timeout));

	request->device = xbee;
	// allow 2 seconds to finish building command and successfully send it
	request->timeout = XBEE_SET_TIMEOUT_SEC(2);
	request->command.w = *(uint16_t FAR *)command;

	return handle;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C1855		// restore C1855 (Recursive function call)
#endif

/*** BeginHeader _xbee_cmd_release_request */
/*** EndHeader */
/**
	@internal
	@brief
	Release an entry from the request table by marking it as
	available.

	For internal driver use only -- userspace code
	should use xbee_cmd_release_handle() instead.

	@param[in]	request	Address of the table entry.

	@retval	0			request freed
	@retval	-EINVAL	address (\a request) is not valid

	@see	xbee_cmd_create(), xbee_cmd_release_handle()

	@todo	add bounds checking on request to make sure it's in the table?
*/
_xbee_atcmd_debug
int _xbee_cmd_release_request( xbee_cmd_request_t FAR *request)
{
	if (! request)
	{
		return -EINVAL;
	}

	request->device = NULL;			// free up entry in the table
	++request->sequence;				// alter sequence to expire old handles

	return 0;
}

/*** BeginHeader xbee_cmd_release_handle */
/*** EndHeader */
/**
	@brief
	Release an entry from the request table by marking it as
	available.

	@param[in]	handle	handle to the request (as returned by xbee_cmd_create)

	@retval	0			request freed
	@retval	-EINVAL	\a handle is not valid

	@see	xbee_cmd_create(), xbee_cmd_release_handle()
*/
_xbee_atcmd_debug
int xbee_cmd_release_handle( int16_t handle)
{
	// Invalid handle caught by _xbee_cmd_handle_to_address(), which will
	// pass NULL to _xbee_cmd_release_request() which returns -EINVAL.
	return _xbee_cmd_release_request( _xbee_cmd_handle_to_address( handle));
}

/*** BeginHeader xbee_cmd_set_command */
/*** EndHeader */
/**
	@brief
	Change the command associated with a request.

	@param[in]	handle	handle to the request, as returned by xbee_cmd_create()

	@param[in]	command	two-letter AT Command to send (e.g., "VR", "NI", etc.)

	@retval	0			command changed
	@retval	-EINVAL	\a handle is not valid

	@see	xbee_cmd_create(), xbee_cmd_set_callback(), xbee_cmd_set_target(),
			xbee_cmd_set_param(), xbee_cmd_set_param_bytes(),
			xbee_cmd_set_param_str(), xbee_cmd_send()
*/
_xbee_atcmd_debug
int xbee_cmd_set_command( int16_t handle, const char FAR command[3])
{
	xbee_cmd_request_t FAR *request;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	request->command.w = *(uint16_t FAR *)command;

	return 0;
}

/*** BeginHeader xbee_cmd_set_callback */
/*** EndHeader */
/**
	@brief
	Associate a callback with a given AT Command request.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().

	@param[in]	callback
					Callback function to receive the AT Command response.  This
					function should take a single parameter (pointer to an
					xbee_cmd_response_t) and return either #XBEE_ATCMD_DONE (if done
					with the request handle) or #XBEE_ATCMD_REUSE (if more responses
					are expected, or the request handle is going to be reused).
		@code
	int atcmd_callback( const xbee_cmd_response_t FAR *response)
		@endcode

	@param[in]	context
					Context (or "userdata") value to pass to the callback along with
					the AT Command response when it arrives.  Should be set to NULL
					if not used.

	@retval	0			callback set
	@retval	-EINVAL	\a handle is not valid

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_target(),
			xbee_cmd_set_param(), xbee_cmd_set_param_bytes(),
			xbee_cmd_set_param_str(), xbee_cmd_send()

*/
_xbee_atcmd_debug
int xbee_cmd_set_callback( int16_t handle, xbee_cmd_callback_fn callback,
	void FAR *context)
{
	xbee_cmd_request_t FAR *request;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	request->callback = callback;
	request->context = context;

	return 0;
}

/*** BeginHeader xbee_cmd_set_target */
/*** EndHeader */
/**
	@brief
	Associate a remote XBee device with a given AT Command request.
	By default, xbee_cmd_create() configures the request as a local
	AT Command for the serially-attached XBee module.  Use this
	function to send the command to a remote device.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().

	@param[in]	ieee
					Pointer to 64-bit IEEE hardware address of target, or NULL
					to switch back to the local XBee device.
					-	#WPAN_IEEE_ADDR_BROADCAST for broadcast.
					-	#WPAN_IEEE_ADDR_COORDINATOR for the coordinator (use
						#WPAN_NET_ADDR_UNDEFINED for the network address).
					-	#WPAN_IEEE_ADDR_UNDEFINED to use the 16-bit network address.

	@param[in]	network
					16-bit network address of target.
					-	#WPAN_NET_ADDR_UNDEFINED if the node's network address
						isn't known or this is a DigiMesh network.
					-	#WPAN_NET_ADDR_COORDINATOR for the coordinator (use the
						coordinator's actual IEEE address for the \a ieee parameter).
					-	#WPAN_NET_ADDR_BCAST_NOT_ASLEEP to broadcast to all nodes
						that aren't sleeping.
					-	#WPAN_NET_ADDR_BCAST_ROUTERS to broadcast to the coordinator
						and all routers.

	@retval	0			target set
	@retval	-EINVAL	\a handle is not valid
	@retval	-ENOSYS	function not implemented (XBEE_CMD_DISABLE_REMOTE defined)

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_param(), xbee_cmd_set_param_bytes(),
			xbee_cmd_set_param_str(), xbee_cmd_send()
*/

_xbee_atcmd_debug
int xbee_cmd_set_target( int16_t handle, const addr64 FAR *ieee,
	uint16_t network_address)
{
#ifdef XBEE_CMD_DISABLE_REMOTE
	return -ENOSYS;
#else
	xbee_cmd_request_t FAR *request;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	if (ieee == NULL)
	{
		_f_memset( &request->address, 0, sizeof request->address);
		request->flags &= ~XBEE_CMD_FLAG_REMOTE;
	}
	else
	{
		request->address.ieee = *ieee;
		request->address.network = network_address;
		request->flags |= XBEE_CMD_FLAG_REMOTE;
	}

	return 0;
#endif
}


/*** BeginHeader _xbee_cmd_encode_param */
uint8_t _xbee_cmd_encode_param( void FAR *buffer, uint32_t value);
/*** EndHeader */
/**
	@internal
	@brief
	Store an unsigned integer in 1, 2 or 4 bytes as a parameter to an
	AT command.

	@param[in]	buffer	buffer to store value
	@param[in]	value	value to store

	@return	Number of bytes required (1, 2 or 4) to hold value.
*/
_xbee_atcmd_debug
uint8_t _xbee_cmd_encode_param( void FAR *buffer, uint32_t value)
{
	if (value & 0xFFFF0000)				// setting 32-bit value
	{
		*(uint32_t FAR *) buffer = htobe32( value);
		return 4;
	}

	if ((uint16_t) value & 0xFF00)		// setting 16-bit value
	{
		*(uint16_t FAR *) buffer = htobe16( (uint16_t) value);
		return 2;
	}

	// setting 8-bit value
	*(uint8_t FAR *)buffer = (uint8_t) value;
	return 1;
}


/*** BeginHeader xbee_cmd_set_flags */
/*** EndHeader */
/**
	@brief
	Set the flags for a given AT Command request.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().
	@param[in]	flags		One or more of the following:
					- XBEE_CMD_FLAG_QUEUE_CHANGE don't apply changes until ATAC
						or another command without this flag is sent

	@retval	0			flags set
	@retval	-EINVAL	\a handle is not valid

	@see	xbee_cmd_clear_flags
*/
_xbee_atcmd_debug
int xbee_cmd_set_flags( int16_t handle, uint16_t flags)
{
	xbee_cmd_request_t FAR *request;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	request->flags |= (flags & XBEE_CMD_FLAG_USER_MASK);

	return 0;
}


/*** BeginHeader xbee_cmd_clear_flags */
/*** EndHeader */
/**
	@brief
	Clear the flags for a given AT Command request.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().
	@param[in]	flags		One or more of the following:
					- XBEE_CMD_FLAG_QUEUE_CHANGE don't apply changes until ATAC
						or another command without this flag is sent

	@retval	0			flags cleared
	@retval	-EINVAL	\a handle is not valid

	@see	xbee_cmd_set_flags
*/
_xbee_atcmd_debug
int xbee_cmd_clear_flags( int16_t handle, uint16_t flags)
{
	xbee_cmd_request_t FAR *request;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	request->flags &= ~(flags & XBEE_CMD_FLAG_USER_MASK);

	return 0;
}


/*** BeginHeader xbee_cmd_set_param */
/*** EndHeader */
/**
	@brief
	Set the parameter (up to 32-bits) for a given AT Command request.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().

	@param[in]	value
					Value to use as the parameter to the AT Command.  For negative
					values, or values > 0xFFFFFFFF, use xbee_cmd_set_param_bytes().
					For string parameters (e.g., for the "NI" command), use
					xbee_cmd_set_param_str().

	@retval	0			parameter set
	@retval	-EINVAL	\a handle is not valid

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_target(), xbee_cmd_set_param_bytes(),
			xbee_cmd_set_param_str(), xbee_cmd_send()
*/
_xbee_atcmd_debug
int xbee_cmd_set_param( int16_t handle, uint32_t value)
{
	xbee_cmd_request_t FAR *request;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	request->param_length = _xbee_cmd_encode_param( request->param, value);

	return 0;
}

/*** BeginHeader xbee_cmd_set_param_bytes */
/*** EndHeader */
/**
	@brief
	Set the parameter for a given AT Command request to a sequence of bytes.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().

	@param[in]	data		Pointer to bytes (MSB-first) to copy into request.

	@param[in]	length	Number of bytes to copy.
								0 < \a length <= #XBEE_CMD_MAX_PARAM_LENGTH

	@retval	0				parameter set
	@retval	-EINVAL		\a handle or \a length is not valid
	@retval	-EMSGSIZE	\a length is greater than #XBEE_CMD_MAX_PARAM_LENGTH

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_target(), xbee_cmd_set_param(), xbee_cmd_set_param_str(),
			xbee_cmd_send()
*/
_xbee_atcmd_debug
int xbee_cmd_set_param_bytes( int16_t handle, const void FAR *data,
	uint8_t length)
{
	xbee_cmd_request_t FAR *request;

	if (length > XBEE_CMD_MAX_PARAM_LENGTH)
	{
		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: ERROR -EMSGSIZE (length %d > %d)\n", __FUNCTION__,
				length, XBEE_CMD_MAX_PARAM_LENGTH);
		#endif
		return -EMSGSIZE;
	}

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: ERROR -EINVAL (handle=0x%04x (%s))\n",
				__FUNCTION__, handle, request ? "valid" : "invalid");
		#endif
		return -EINVAL;
	}

	request->param_length = length;
	if (length > 0)
	{
		_f_memcpy( request->param, data, length);
	}

	return 0;
}

/*** BeginHeader xbee_cmd_set_param_str */
/*** EndHeader */
/**
	@brief
	Set a string parameter for a given AT Command request (e.g., the
	"NI" node identifier command).

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().

	@param[in]	str
					String to use as the parameter.  Must be less than
					#XBEE_CMD_MAX_PARAM_LENGTH characters long.

	@retval	0				parameter set
	@retval	-EINVAL		\a handle is not valid
	@retval	-EMSGSIZE	\a str is more than #XBEE_CMD_MAX_PARAM_LENGTH
								characters.

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_target(), xbee_cmd_set_param(),
			xbee_cmd_set_param_bytes(), xbee_cmd_send()
*/
_xbee_atcmd_debug
int xbee_cmd_set_param_str( int16_t handle, const char FAR *str)
{
	int length = strlen( str);

	if (length > XBEE_CMD_MAX_PARAM_LENGTH)
	{
		return -EMSGSIZE;
	}

	#if XBEE_CMD_MAX_PARAM_LENGTH > 255
		#error This code assumes XBEE_CMD_MAX_PARAM_LENGTH is <= 255.
	#endif
	return xbee_cmd_set_param_bytes( handle, str, (uint8_t) length);
}

/*** BeginHeader xbee_cmd_send */
/*** EndHeader */
/**
	@brief
	Send an AT Command to a local or remote XBee device.

	@param[in]	handle
					Handle to the request, as returned by xbee_cmd_create().

	@retval	0			frame sent
	@retval	-EINVAL	handle is not valid
	@retval	-EBUSY	transmit serial buffer is full, or XBee is not accepting
							serial data (deasserting /CTS signal).

	@note If the request does not have a callback set, it will be automatically
			released if xbee_cmd_send() returns 0.

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_target(), xbee_cmd_set_param(),
			xbee_cmd_set_param_bytes(), xbee_cmd_set_param_str()
*/
_xbee_atcmd_debug
int xbee_cmd_send( int16_t handle)
{
	xbee_cmd_request_t FAR *request;
	xbee_header_at_request_t header;
	size_t header_size;
	int error;

	request = _xbee_cmd_handle_to_address( handle);
	if (! request)
	{
		return -EINVAL;
	}

	if (! request->callback)
	{
		// Nothing cares about the response, so use Frame ID 0 so the target
		// device won't send a response.
		request->frame_id = 0;
	}
	else
	{
		// set the request's frame ID
		// DEVNOTE: What if this is a resend?  Should we keep the same frame ID?
		request->frame_id = (uint8_t) xbee_next_frame_id( request->device);
	}

#ifndef XBEE_CMD_DISABLE_REMOTE
	if (request->flags & XBEE_CMD_FLAG_REMOTE)
	{
		header.frame_type = XBEE_FRAME_REMOTE_AT_CMD;
		header.remote.frame_id = request->frame_id;
		header.remote.ieee_address = request->address.ieee;
		header.remote.network_address_be = htobe16( request->address.network);
		header.remote.options = (request->flags & XBEE_CMD_FLAG_QUEUE_CHANGE) ?
			XBEE_REMOTE_AT_OPT_QUEUE : XBEE_REMOTE_AT_OPT_IMMEDIATE;
		header.remote.command.w = request->command.w;
		header_size = sizeof(xbee_header_remote_at_req_t);
	}
	else
#endif
	{
		header.frame_type = (request->flags & XBEE_CMD_FLAG_QUEUE_CHANGE) ?
			XBEE_FRAME_LOCAL_AT_CMD_Q : XBEE_FRAME_LOCAL_AT_CMD;
		header.local.frame_id = request->frame_id;
		header.local.command.w = request->command.w;
		header_size = sizeof(xbee_header_local_at_req_t);
	}

	#ifdef XBEE_ATCMD_VERBOSE
		puts( "atcmd header is:");
		hex_dump( &header, header_size, HEX_DUMP_FLAG_TAB);
		if (request->param_length)
		{
			puts( "atcmd param is:");
			hex_dump( request->param, request->param_length, HEX_DUMP_FLAG_TAB);
		}
	#endif

	error = xbee_frame_write( request->device, &header, header_size,
		request->param, request->param_length, XBEE_DEV_FLAG_NONE);

	if (! error)
	{
		if (request->frame_id != 0)
		{
			// we're expecting a response, so update the timeout value
			request->timeout =
#ifdef XBEE_CMD_DISABLE_REMOTE
				XBEE_SET_TIMEOUT_SEC( XBEE_CMD_LOCAL_TIMEOUT);
#else
				XBEE_SET_TIMEOUT_SEC( request->flags & XBEE_CMD_FLAG_REMOTE
						? XBEE_CMD_REMOTE_TIMEOUT : XBEE_CMD_LOCAL_TIMEOUT);
#endif
		}
		else if (! (request->flags & XBEE_CMD_FLAG_REUSE_HANDLE))
		{
			_xbee_cmd_release_request( request);
		}
	}

	return error;
}


/*** BeginHeader _xbee_cmd_handle_response */
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
/**	@internal
	@brief
	Callback handler registered for frame types
	#XBEE_FRAME_LOCAL_AT_RESPONSE (0x88) and
	#XBEE_FRAME_REMOTE_AT_RESPONSE (0x97).

	Should only be called by the frame dispatcher and unit tests.

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.
*/
_xbee_atcmd_debug
int _xbee_cmd_handle_response( xbee_dev_t *xbee, const void FAR *rawframe,
	uint16_t length, void FAR *context)
{
	xbee_cmd_response_t response;			// response passed to callback

	const xbee_frame_at_response_t FAR *frame = rawframe;
	xbee_at_cmd_t command;
	const uint8_t FAR *value;
	uint_fast8_t value_length;

	bool_t is_local;
	uint8_t status, frame_id;

	uint_fast8_t index;
	xbee_cmd_request_t FAR *request;

	wpan_address_t sender;

	#ifdef XBEE_ATCMD_VERBOSE
		// variables used to dump responses that don't match a callback
		uint8_t buffer[100];
		char *p;
		const uint8_t FAR *b;
		int ch;
		uint16_t i;
	#endif

	// standard frame handler API, but we don't make use of context
	XBEE_UNUSED_PARAMETER( context);

	if (frame == NULL)
	{
		// extra checking, since stack always passes a valid frame
		return -EINVAL;
	}
	switch (frame->frame_type)
	{
		case XBEE_FRAME_LOCAL_AT_RESPONSE:
			is_local = TRUE;
			value = frame->local.value;
			value_length = (uint_fast8_t)
				(length - offsetof(xbee_frame_local_at_resp_t, value));
			command = frame->local.header.command;
			status = frame->local.header.status;
			break;

#ifndef XBEE_CMD_DISABLE_REMOTE
		case XBEE_FRAME_REMOTE_AT_RESPONSE:
			is_local = FALSE;
			value = frame->remote.value;
			value_length = (uint_fast8_t)
				(length - offsetof(xbee_frame_remote_at_resp_t, value));
			command = frame->remote.header.command;
			status = frame->remote.header.status;
			break;
#endif

		default:
			// Shouldn't get to this point since the handler is only registered
			// for local and remote AT Response frames.
			return -EINVAL;
	}

	// local.frame_id and remote.frame_id are at the same offset in the struct
	frame_id = frame->local.header.frame_id;

	// Look for the frame in the table of pending requests.
	request = xbee_cmd_request_table;
	for (index = 0; index < XBEE_CMD_REQUEST_TABLESIZE; ++request, ++index)
	{
		// Match device, frame_id, command and local/remote
		if (request->device == xbee &&				// sent on this device
			request->frame_id == frame_id &&			// frame ID matches
#ifndef XBEE_CMD_DISABLE_REMOTE
			is_local == !(request->flags & XBEE_CMD_FLAG_REMOTE) &&
#endif
			request->command.w == command.w)		// command matches
		{
			break;
		}
	}

	if (index == XBEE_CMD_REQUEST_TABLESIZE)
	{
		// didn't find request in table
		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: couldn't match %.*s response to request table\n",
				__FUNCTION__, 2, command.str);
			if (value_length)
			{
				p = (char *) buffer;
				b = value;
				p += sprintf( p, " = 0x");
				for (i = value_length; i; --i)
				{
					ch = *b++;
					*p++ = "0123456789abcdef"[ch >> 4];
					*p++ = "0123456789abcdef"[ch & 0x0F];
				}
				*p = '\0';

				printf( "%s\n", buffer);
			}
		#endif

		return 0;
	}

	#ifdef XBEE_ATCMD_VERBOSE
		printf( "%s: response matched request %d\n", __FUNCTION__, index);
	#endif

	// build response to pass to callback
	if (! request->callback)
	{
		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: no callback registered for %d\n", __FUNCTION__, index);
		#endif
	}
	else
	{
      if (is_local)
      {
      	response.source = NULL;
      }
      else
      {
      	sender.ieee = frame->remote.header.ieee_address;
         sender.network = be16toh( frame->remote.header.network_address_be);
      	response.source = &sender;
      }
		response.device = xbee;
		response.context = request->context;
		response.handle = (index << 8) | request->sequence;
		response.command.w = request->command.w;
		response.flags = status;
		response.value_bytes = value;
		response.value_length = value_length;
		if (value_length == 0)
		{
			response.value = 0;
		}
		else
		{
			switch (value_length)
			{
				case 1:
					response.value = value[0];
					break;

				case 2:
					response.value = be16toh( *(uint16_t FAR *)value);
					break;

				case 4:
					response.value = be32toh( *(uint32_t FAR *)value);
					break;

				default:
					response.value = 0xFFFFFFFF;
					break;
			}
		}

		#ifdef XBEE_ATCMD_VERBOSE
			printf( "%s: dispatch to handler @%p w/context %" PRIpFAR "\n",
				__FUNCTION__, request->callback, response.context);
		#endif

		if (request->callback( &response) == XBEE_ATCMD_REUSE)
		{
			// keep the request alive (possibly resent by the callback)
			request->timeout = XBEE_SET_TIMEOUT_SEC(5);
			return 0;
		}
	}

	// @todo Only release if the appropriate flag is set (ignore response?)
	//			Then add a function to check the status of a request
	_xbee_cmd_release_request( request);
	return 0;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

/*** BeginHeader _xbee_cmd_modem_status */
/*** EndHeader */
/**
	@internal
	@brief
	Receive modem status frames and update our network address (and payload
	size) on state changes.

	@see xbee_frame_handler_fn()
*/
_xbee_atcmd_debug
int _xbee_cmd_modem_status( xbee_dev_t *xbee,
	const void FAR *payload, uint16_t length, void FAR *context)
{
	uint8_t status;

	// standard frame handler API, but we don't make use of length or context
	XBEE_UNUSED_PARAMETER( length);
	XBEE_UNUSED_PARAMETER( context);

	if (payload == NULL)
	{
		return -EINVAL;
	}

	status = ((const xbee_frame_modem_status_t FAR *)payload)->status;
	if (status == XBEE_MODEM_STATUS_KEY_ESTABLISHED ||
		status == XBEE_MODEM_STATUS_JOINED ||
		status == XBEE_MODEM_STATUS_COORD_START)
	{
		// need to kick off discovery of my address
		#ifndef XBEE_ATCMD_VERBOSE
			xbee_cmd_query_device( xbee, 1);
		#else
			int error;

			printf( "%s: looking up my network address (flags=0x%04X)\n",
				__FUNCTION__, xbee->flags);
			error = xbee_cmd_query_device( xbee, 1);
			if (error)
			{
				printf( "%s: error %d querying device (flags=0x%04X)\n",
					__FUNCTION__, error, xbee->flags);
			}
		#endif
	}

	return 0;
}

/*** BeginHeader xbee_cmd_simple */
/*** EndHeader */
/**
	@brief
	Simple interface for sending a command with a parameter to the local XBee
	without checking for a response.

	@param[in]	xbee	XBee device to use as the target.

	@param[in]	command	Two-letter AT Command to send
								(e.g., "ID", "CH", etc.).

	@param[in]	value
					Value to use as the parameter to the AT Command.

	@retval	0			command sent
	@retval	-EINVAL	an invalid parameter was passed to the function
	@retval	-EBUSY	transmit serial buffer is full, or XBee is not accepting
							serial data (deasserting /CTS signal).

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_target(), xbee_cmd_set_param(),
			xbee_cmd_set_param_bytes(), xbee_cmd_set_param_str(),
			xbee_cmd_execute()
*/
_xbee_atcmd_debug
int xbee_cmd_simple( xbee_dev_t *xbee, const char FAR command[3],
	uint32_t value)
{
	PACKED_STRUCT {
		xbee_header_local_at_req_t	header;
		uint8_t							param[4];
	} request;
	uint16_t request_size;

	request.header.frame_type = XBEE_FRAME_LOCAL_AT_CMD;
	request.header.frame_id = 0;		// don't need a response
	request.header.command.w = *(uint16_t FAR *)command;

	request_size = sizeof(request.header) +
		_xbee_cmd_encode_param( request.param, value);

	#ifdef XBEE_ATCMD_VERBOSE
		printf( "%s: sending AT%.2s=%" PRIu32 " (0x%" PRIx32 "), frame ID 0\n",
														__FUNCTION__, command, value, value);
	#endif

	return xbee_frame_write( xbee, &request, request_size, NULL, 0,
																		XBEE_DEV_FLAG_NONE);
}


/*** BeginHeader xbee_cmd_execute */
/*** EndHeader */
/**
	@brief
	Simple interface for sending a command with an optional parameter to the
	local XBee without checking for a response.

	For an asynchronous method of
	sending AT commands and getting the response, see xbee_cmd_create, the
	xbee_cmd_set_* functions and xbee_cmd_send.

	@param[in]	xbee	XBee device to use as the target.

	@param[in]	command	Two-letter AT Command to send
								(e.g., "ID", "CH", etc.).

	@param[in]	data		Optional big-endian (MSB-first) value to assign to
								\a command.  Use NULL if \a command doesn't take a
								parameter.

	@param[in]	length	Number of bytes in \a data; ignored if \a data is NULL.

	@retval	0			command sent
	@retval	-EINVAL	an invalid parameter was passed to the function
	@retval	-EBUSY	transmit serial buffer is full, or XBee is not accepting
							serial data (deasserting /CTS signal).

	@see	xbee_cmd_create(), xbee_cmd_set_command(), xbee_cmd_set_callback(),
			xbee_cmd_set_target(), xbee_cmd_set_param(),
			xbee_cmd_set_param_bytes(), xbee_cmd_set_param_str(), xbee_cmd_simple()
*/
_xbee_atcmd_debug
int xbee_cmd_execute( xbee_dev_t *xbee, const char FAR command[3],
	const void FAR *data, uint8_t length)
{
	xbee_header_local_at_req_t	request;

	request.frame_type = XBEE_FRAME_LOCAL_AT_CMD;
	// If this is an ND command, we'll need a non-zero frame ID.
	request.frame_id = (uint8_t) xbee_next_frame_id( xbee);
	request.command.w = *(uint16_t FAR *)command;

	#ifdef XBEE_ATCMD_VERBOSE
		printf( "%s: sending AT%.2s, frame ID 0x%02x\n",
												__FUNCTION__, command, request.frame_id);
	#endif

	return xbee_frame_write( xbee, &request, sizeof(request), data, length,
																		XBEE_DEV_FLAG_NONE);
}

/*** BeginHeader xbee_identify */
/*** EndHeader */
// documented in xbee/atcmd.h
void xbee_identify( xbee_dev_t *xbee, bool_t identify)
{
	static bool_t last_id = FALSE;

	// on state change -- toggle LT between 0 (default rate) and 10 (fast flash)
	if (identify != last_id)
	{
		last_id = identify;
		xbee_cmd_simple( xbee, "LT", last_id ? 10 : 0);
	}
}


