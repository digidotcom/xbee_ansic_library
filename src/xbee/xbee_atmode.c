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
	@addtogroup xbee_atmode
	@{
	@file xbee_atmode.c
	Code for working with XBee modules in AT command mode instead of API mode.

	For DigiMesh 900 modules, this is necessary when doing firmware updates
	(even when module is using API-mode firmware).

	Note that this mode is not very robust.  Once it AT mode, the XBee will
	return to idle mode after some amount of time (value of CT register * 100ms).
	If our calculation of that idle time doesn't match the XBee module's, our
	state machine won't match the actual state.  It may be possible to add some
	extra time on either end of that timeout to be certain, and it's always
	possible re-enter command mode just to send the command to exit (ATCN) and
	be sure we're out of it.
*/

/*** BeginHeader */
#ifdef XBEE_DEVICE_ENABLE_ATMODE

#include <errno.h>
#include <string.h>

#include "xbee/platform.h"
#ifdef XBEE_ATMODE_VERBOSE
	#include <stdio.h>
#endif

#include "xbee/atmode.h"

#ifndef __DC__
	#define _xbee_atmode_debug
#elif defined XBEE_ATMODE_DEBUG
	#define _xbee_atmode_debug __debug
#else
	#define _xbee_atmode_debug __nodebug
#endif
/*** EndHeader */

/*** BeginHeader xbee_atmode_enter */
/*** EndHeader */
/**
	@brief	Attempt to enter AT command mode (delay 1 second, send +++,
			delay 1 second).

			After calling xbee_atmode_enter(), you must
			continue to call xbee_atmode_tick() until it returns
			XBEE_MODE_COMMAND (successfully entered command mode) or
			XBEE_MODE_IDLE (failed to enter command mode).

	@param[in]	xbee	XBee device

	@retval	0			Initiated sequence to enter command mode.
	@retval	-EINVAL	Invalid XBee device passed to function.

	@sa xbee_atmode_exit, xbee_atmode_tick
*/
_xbee_atmode_debug
int xbee_atmode_enter( xbee_dev_t *xbee)
{
	if (xbee == NULL)
	{
		return -EINVAL;
	}

	switch (xbee_atmode_tick( xbee))
	{
		case XBEE_MODE_PRE_ESCAPE:
		case XBEE_MODE_POST_ESCAPE:
		case XBEE_MODE_COMMAND:
		case XBEE_MODE_WAIT_RESPONSE:
		case XBEE_MODE_WAIT_IDLE:
			// already in command mode, or on our way there
			#ifdef XBEE_ATMODE_VERBOSE
				printf( "%s: ignoring request, already in state %d\n",
					__FUNCTION__, xbee->mode);
			#endif
			return 0;
	}

	#ifdef XBEE_ATMODE_VERBOSE
		printf( "%s: entering command mode (guard=%u, escape=%c)\n",
			__FUNCTION__, xbee->guard_time, xbee->escape_char);
	#endif

	xbee->mode = XBEE_MODE_PRE_ESCAPE;
	xbee->mode_timer = xbee_millisecond_timer();

	return 0;
}

/*** BeginHeader xbee_atmode_exit */
/*** EndHeader */
/**
	@brief	Leave AT command mode and return to idle mode.

				XBee driver doesn't consider the XBee module to be in idle mode
				until xbee_atmode_tick() returns XBEE_MODE_IDLE.

	@param[in]	xbee	XBee device

	@retval	0			Already in idle mode, or sent command to return to it.
							Call xbee_atmode_tick() until it returns XBEE_MODE_IDLE.
	@retval	-EINVAL	Invalid XBee device passed to function.
	@retval	-EBUSY	Can't exit until receipt of response from previous
							command.  Call function again later.

	@sa xbee_atmode_enter, xbee_atmode_tick
*/
_xbee_atmode_debug
int xbee_atmode_exit( xbee_dev_t *xbee)
{
	if (xbee == NULL)
	{
		return -EINVAL;
	}

	switch (xbee_atmode_tick( xbee))
	{
		case XBEE_MODE_PRE_ESCAPE:
			// not actually in command-mode yet, just switch back to IDLE state
			#ifdef XBEE_ATMODE_VERBOSE
				printf( "%s: aborting pre-escape wait to enter command mode\n",
					__FUNCTION__);
			#endif
			xbee->mode = XBEE_MODE_IDLE;
			break;

		case XBEE_MODE_POST_ESCAPE:
		case XBEE_MODE_WAIT_RESPONSE:
			// Force the caller to wait until we're actually in command mode
			// before making the switch.
			#ifdef XBEE_ATMODE_VERBOSE
				printf( "%s: returning -EBUSY\n", __FUNCTION__);
			#endif
			return -EBUSY;

		case XBEE_MODE_COMMAND:
			#ifdef XBEE_ATMODE_VERBOSE
				printf( "%s: sending ATCN\n", __FUNCTION__);
			#endif
			// in command mode, send ATCN to get out
			xbee_atmode_send_request( xbee, "CN");
			xbee->mode = XBEE_MODE_WAIT_IDLE;
			break;

	#ifdef XBEE_ATMODE_VERBOSE
		default:
			// not in command mode, don't change state
			printf( "%s: mode unchanged (%d)\n", __FUNCTION__);
	#endif
	}

	return 0;
}

/*** BeginHeader xbee_atmode_tick */
/*** EndHeader */
/**
	@brief		Advance the XBee device state machine when entering or exiting
					AT Command Mode or waiting for a response to a command.

	@param[in]	xbee	XBee device

	@retval	>=0	Current state of XBee device.  See the function help for
						xbee_mode for a list of modes.
	@retval	-EINVAL	Invalid XBee device passed to function.
	@retval	< 0		Undocumented error.

	@sa xbee_atmode_enter, xbee_atmode_tick
*/
_xbee_atmode_debug
int xbee_atmode_tick( xbee_dev_t *xbee)
{
	char escape[3];		// used to send escape sequence (+++)
	const char ok_response[] = { 'O', 'K', '\r' };
	char buffer[sizeof ok_response];

	if (xbee == NULL)
	{
		return -EINVAL;
	}

	switch (xbee->mode)
	{
		case XBEE_MODE_PRE_ESCAPE:
			if (xbee_millisecond_timer() - xbee->mode_timer
																	> xbee->guard_time + 200)
			{
				// guard time has passed, send escape sequence (def. "+++")
				#ifdef XBEE_ATMODE_VERBOSE
					printf( "%s: guard time elapsed, sending escape sequence\n",
						__FUNCTION__);
				#endif
				memset( escape, xbee->escape_char, sizeof escape);
				xbee_ser_write( &xbee->serport, escape, sizeof escape);
				xbee->mode = XBEE_MODE_POST_ESCAPE;
				xbee->mode_timer = xbee_millisecond_timer();

				// flush receive buffer (may be leftover frames from API mode)
				xbee_ser_rx_flush( &xbee->serport);
			}
			break;

		case XBEE_MODE_POST_ESCAPE:
         // receive buffer should contain "OK\r"
         if (xbee_ser_rx_used( &xbee->serport) >= sizeof ok_response)
         {
            memset( buffer, 0, sizeof ok_response);
            xbee_ser_read( &xbee->serport, buffer, sizeof ok_response);
            #ifdef XBEE_ATMODE_VERBOSE
            	printf( "%s: read %.*s\n", __FUNCTION__, sizeof ok_response,
            			buffer);
            #endif
            if (memcmp( buffer, ok_response, sizeof ok_response) == 0)
            {
               xbee->mode_timer = xbee_millisecond_timer();
               xbee->mode = XBEE_MODE_COMMAND;
            }
            else
            {
					// we received something other than OK
					xbee->mode = XBEE_MODE_IDLE;
            }
         }
			else if (xbee_millisecond_timer() - xbee->mode_timer
																	> xbee->guard_time + 200)
			{
				#ifdef XBEE_ATMODE_VERBOSE
					printf( "%s: guard time expired before 'OK'\n", __FUNCTION__);
				#endif
				// guard time passed and we didn't receive OK
				xbee->mode = XBEE_MODE_IDLE;
			}
			break;

		case XBEE_MODE_WAIT_RESPONSE:
			if (xbee_millisecond_timer() - xbee->mode_timer > 2000)
			{
				#ifdef XBEE_ATMODE_VERBOSE
					printf( "%s: timed out waiting for response\n", __FUNCTION__);
				#endif
				xbee->mode = XBEE_MODE_COMMAND;
			}
			break;

		case XBEE_MODE_WAIT_IDLE:
			if (xbee_millisecond_timer() - xbee->mode_timer > 2000)
			{
				xbee->mode = XBEE_MODE_IDLE;
			}
			break;

		case XBEE_MODE_COMMAND:
			if (xbee_millisecond_timer() - xbee->mode_timer
																> xbee->idle_timeout * 100ul)
			{
				#ifdef XBEE_ATMODE_VERBOSE
					printf( "%s: timeout from command mode to idle\n", __FUNCTION__);
				#endif
				xbee->mode = XBEE_MODE_IDLE;
			}
			break;

		case XBEE_MODE_UNKNOWN:
		case XBEE_MODE_BOOTLOADER:
		case XBEE_MODE_API:
		case XBEE_MODE_IDLE:
			// nothing
			break;
	}

	return xbee->mode;
}

/*** BeginHeader xbee_atmode_send_request */
/*** EndHeader */
/**
	@brief		Send an AT request and wait for a response.

					Automatically prepends the request with "AT" and adds the
					trailing carriage-return.

	@param[in]	xbee		XBee device

	@param[in]	command	Command to send (without leading AT or trailing \r)

	@retval	0			Command sent
	@retval	-EINVAL	Invalid parameter passed to function.
	@retval	-ENOSPC	Not enough room in transmit buffer to send request.

	@sa xbee_atmode_read_response
*/
_xbee_atmode_debug
int xbee_atmode_send_request( xbee_dev_t *xbee, const char FAR *command)
{
	xbee_serial_t	*serport = &xbee->serport;
	int cmdlen;

	if (xbee == NULL || command == NULL)
	{
		return -EINVAL;
	}

	cmdlen = strlen( command);

	// Make sure there's enough room in the tx buffer to send the full command.
	// "AT" + cmdlen + "\r"
	if (xbee_ser_tx_free( serport) < cmdlen + 3)
	{
		return -ENOSPC;
	}

	// DEVNOTE: Although it might be nice to see if <command> starts with AT
	// and automatically skip over that, we would have to check for a caller
	// checking or setting the AT register (Guard Time After).
	// So, (cmdlen >= 4) && starts_with("AT") && ! is_digit( command[2])

	xbee_ser_write( serport, "AT", 2);
	xbee_ser_write( serport, command, cmdlen);
	xbee_ser_write( serport, "\r", 1);
	xbee->mode = XBEE_MODE_WAIT_RESPONSE;
	xbee->mode_timer = xbee_millisecond_timer();

	return 0;
}

/*** BeginHeader xbee_atmode_read_response */
/*** EndHeader */
/**
	@brief		Non-blocking function reads response to request sent with
					xbee_atmode_send_request to an XBee device in AT Command Mode.

					Reads up to trailing return (\r) before returning success (0).
					Does not include trailing return in response.

					Sample code for using this function:
@code
					char response[40];
					int bytesread, retval;

					*response = '\0';			// start with empty response
					bytesread = 0;

					do {
						retval = xbee_atmode_read_response( xbee, response,
													sizeof( response), &bytesread);
					} while (retval == -EAGAIN);

					if (retval < 0)
						printf( "error %d reading response\n", retval);
					else
						printf( "response is %s\n", response);
@endcode

	@param[in]		xbee			XBee device

	@param[out]		response		Buffer to hold the response.  Since this function
					is non-blocking, and may require multiple calls to read the
					entire response, the buffer is used to hold the partial
					response until the XBee sends a return (\r).

	@param[in]		resp_size	Size of response buffer passed in parameter 2.

	@param[in,out]	bytesread	Pointer to an integer that is tracking the number
					of bytes already read.  Function will read new bytes starting at
					\a &response[*bytesread] and will increment \a *bytesread for
					every byte added to \a response.

					If NULL, function will append new bytes to the end of
					\a response.

	@retval	0				received complete line
	@retval	-EINVAL		Invalid parameter passed to function.
	@retval	-EPERM		XBee isn't waiting for a response
	@retval	-ENOSPC		buffer filled before "\r" received
	@retval	-EAGAIN		haven't read entire response yet, call function again
	@retval	-ETIMEDOUT	timed out waiting for response

	@sa	xbee_atmode_send_request
*/
_xbee_atmode_debug
int xbee_atmode_read_response( xbee_dev_t *xbee, char FAR *response,
	int resp_size, int FAR *bytesread)
{
	char FAR *p;
	int ch;
	int bytes;
	int retval;

	if (xbee == NULL || response == NULL)
	{
		return -EINVAL;
	}

	// Return an error if we're not in WAIT_IDLE or WAIT_RESPONSE state.
	if (xbee->mode != XBEE_MODE_WAIT_IDLE &&
		xbee->mode != XBEE_MODE_WAIT_RESPONSE)
	{
		return -EPERM;
	}

	bytes = bytesread ? *bytesread : strlen( response);
	p = response + bytes;
	for (;;)
	{
		if (bytes + 1 >= resp_size)		// include extra byte for null terminator
		{
			retval = -ENOSPC;
			break;
		}
		ch = xbee_ser_getchar( &xbee->serport);
		if (ch < 0)
		{
			retval = (xbee_millisecond_timer() - xbee->mode_timer > 2000)
																		? -ETIMEDOUT : -EAGAIN;
			break;
		}
		if (ch == '\r')
		{
			retval = 0;
			break;
		}
		*p++ = ch;
		++bytes;
	}

	*p = '\0';		// make sure string is null-terminated
	if (bytesread)
	{
		*bytesread = bytes;
	}
	if (retval == -ETIMEDOUT || retval == 0)		// timed out, or got a response
	{
		if (xbee->mode == XBEE_MODE_WAIT_IDLE)
		{
			xbee->mode = XBEE_MODE_IDLE;		// response to ATCN, now in idle mode
		}
		else if (xbee->mode == XBEE_MODE_WAIT_RESPONSE)
		{
			xbee->mode = XBEE_MODE_COMMAND;	// got response, still in command mode
		}
      #ifdef XBEE_ATMODE_VERBOSE
         printf( "%s: %s waiting for response, now in %s mode\n", __FUNCTION__,
         	retval ? "timeout" : "success",
         	(xbee->mode == XBEE_MODE_IDLE) ? "idle" : "command");
      #endif
	}

	return retval;
}
/*** BeginHeader */
#endif /* XBEE_DEVICE_ENABLE_ATMODE */
/*** EndHeader */

