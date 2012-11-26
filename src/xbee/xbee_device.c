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
	@addtogroup xbee_device
	@{
	@file xbee_device.c
	Code related to processing XBee Frames.

	Need to update xbee_send and xbee_send_endpoint to accept a callback function
	and context.  The transmit_status message will dispatch a message to that
	function and callback (and then automatically delete the callback).
*/

/*** BeginHeader */
#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/device.h"

#ifndef __DC__
	#define _xbee_device_debug
#elif defined XBEE_DEVICE_DEBUG
	#define _xbee_device_debug __debug
#else
	#define _xbee_device_debug __nodebug
#endif

// Load library for sending and receiving frames over serial port.
#include "xbee/serial.h"
#include "wpan/aps.h"
/*** EndHeader */


/*** BeginHeader xbee_next_frame_id */
/*** EndHeader */
/**
	@brief
	Increment and return current frame ID for a given XBee device.

	Frame IDs go from 1 to 255 and then back to 1.

	@param[in] xbee	XBee device.

	@retval	1-255 Current frame ID (after incrementing) for device
	@retval	0 \a xbee is not a valid XBee device pointer.
*/
_xbee_device_debug
uint8_t xbee_next_frame_id( xbee_dev_t *xbee)
{
	if (! xbee)
	{
		return 0;
	}

	// frame_id ranges from 1 to 255; if incremented to 0, wrap to 1
	if (! ++xbee->frame_id)
	{
		xbee->frame_id = 1;
	}

	return xbee->frame_id;
}

/*** BeginHeader xbee_dev_init */
/*** EndHeader */
/**
	@brief
	Initialize the XBee device structure and open a serial connection to
	a local, serially-attached XBee module.

	This function does not actually initiate communications with the XBee
	module.  See xbee_cmd_init_device() for information on initializing
	the "AT Command" layer of the driver, which will read basic information
	from the XBee module.

@param[in]	xbee
					XBee device to initialize.

@param[in]	serport
					Pointer to an xbee_serial_t structure used to initialize
					\a xbee->serport.

@param[in]	is_awake
					Pointer to function that reads the XBee module's "ON" pin.  The
					function should return 1 if XBee is on and 0 if it is off.

			@code int is_awake( xbee_dev_t *xbee); @endcode

@param[in]	reset
					Pointer to function that asserts the XBee module's "/RESET" pin.
					If \a asserted is TRUE, puts the XBee into reset.
					If \a asserted is FALSE, takes it out of reset.  No return value.

			@code void xbee_reset( xbee_dev_t *xbee, bool_t asserted); @endcode

@retval	0 Success
@retval	-EINVAL Invalid parameter (\a xbee is NULL, \a serport is not valid, etc.)
@retval	-EIO Couldn't set serial port baudrate within 5% of \a serport->baudrate.
*/
/*
@note			Instead of passing function pointers, it may make more sense to
				have the caller set those elements after calling xbee_dev_init.
				Unless they are necessary during the init process, there's no need
				to clutter up the function call with extra parameters, especially if
				we need functions to set the RTS pin or read the CTS pin.

@note			Consider having a field in the structure to indicate when it has been
				properly initialized, and have various functions check that field
				before continuing.
*/
_xbee_device_debug
int xbee_dev_init( xbee_dev_t *xbee, const xbee_serial_t *serport,
											xbee_is_awake_fn is_awake, xbee_reset_fn reset)
{
	int error;

	if (! xbee)
	{
		return -EINVAL;
	}

	// try communicating with the XBee module
	// set xbee to all zeros, then
	// set up serial port and attempt communications with module
	memset( xbee, 0, sizeof( xbee_dev_t));

	// configuration for serial XBee
	xbee->is_awake = is_awake;	// function to read XBee's "ON" pin
	if (reset)
	{
		xbee->reset = reset;		// function to assert XBee's reset pin
		reset( xbee, 0);			// take XBee out of reset state
	}

	xbee->serport = *serport;
	error = xbee_ser_open( &xbee->serport, serport->baudrate);
	if (! error)
	{
		error = xbee_ser_flowcontrol( &xbee->serport, 1);
	}

	xbee->flags = XBEE_DEV_FLAG_USE_FLOWCONTROL;

	#ifdef XBEE_DEVICE_ENABLE_ATMODE
		// fill in default values for GT, CT and CC registers
		xbee->guard_time = 1000;
		xbee->escape_char = '+';
		xbee->idle_timeout = 100;
	#endif

	return error;
}


/*** BeginHeader xbee_dev_flowcontrol */
/*** EndHeader */
/**
	@brief Control whether xbee_frame_write() checks the XBee CTS signal before
			sending frames.

	Flow control is enabled in xbee_dev_init() by default.

	@param[in,out]		xbee		XBee to configure
	@param[in]			enabled	TRUE to enable checking CTS before sending

	@sa xbee_dev_init(), xbee_frame_write()
*/
_xbee_device_debug
void xbee_dev_flowcontrol( xbee_dev_t *xbee, bool_t enabled)
{
	if (xbee != NULL)
	{
		if (enabled)
		{
			xbee->flags |= XBEE_DEV_FLAG_USE_FLOWCONTROL;
		}
		else
		{
			xbee->flags &= ~XBEE_DEV_FLAG_USE_FLOWCONTROL;
		}
	}
}


/*** BeginHeader xbee_dev_dump_settings */
/*** EndHeader */
/**
	@brief
	Print information to stdout about the XBee device.

	Default behavior is to print the name of the serial port, the XBee
	module's hardware version (ATHV), firmware version (ATVR), IEEE
	address (ATSH/ATSL) and network address (ATMY).

	Assumes the user has already called xbee_cmd_init_device() and waited for
	xbee_cmd_query_status() to finish.

	@param[in]	xbee
	@param[in]	flags
			- XBEE_DEV_DUMP_FLAG_NONE: default settings

	@sa xbee_cmd_init_device(), xbee_cmd_query_status(), xbee_ser_portname()
*/
	// ideas for flags:
	//		show portname
	//		extended dump
	//		super dump?  (state information)
_xbee_device_debug
void xbee_dev_dump_settings( xbee_dev_t *xbee, uint16_t flags)
{
	char addr[ADDR64_STRING_LENGTH];

	// flags parameter included in API for future expansion; unused for now
	XBEE_UNUSED_PARAMETER( flags);

	printf( "XBee on %s: HV=0x%x  VR=0x%" PRIx32 "  IEEE=%" PRIsFAR
		"  net=0x%04x\n\n", xbee_ser_portname( &xbee->serport),
		xbee->hardware_version, xbee->firmware_version,
		addr64_format( addr, &xbee->wpan_dev.address.ieee),
		xbee->wpan_dev.address.network);
}


/*** BeginHeader xbee_dev_reset */
/*** EndHeader */
/**
	@brief
	Toggles the reset line of the XBee device.

	@param[in]	xbee	XBee device to reset.

	@retval	0			Successfully toggled reset.
	@retval	-EINVAL	Invalid xbee_dev_t passed to function.
	@retval	-EIO		This XBee device doesn't have an interface to the
							module's reset pin.
*/
_xbee_device_debug
int xbee_dev_reset( xbee_dev_t *xbee)
{
	uint16_t t;

	if (! xbee)
	{
		return -EINVAL;
	}
	if (! xbee->reset)
	{
		return -EIO;
	}

	xbee->reset( xbee, 1);
	// need to hold reset for 250ns
	// Wait for the millisecond timer to roll over twice to ensure
	// 250ns elapsed.  Consider adding high-resolution sleep function
	// to the platform files (usleep?).
	t = XBEE_SET_TIMEOUT_MS(XBEE_MS_TIMER_RESOLUTION + 1);
	while (! XBEE_CHECK_TIMEOUT_MS(t));
	xbee->reset( xbee, 0);
	#ifdef XBEE_DEVICE_ENABLE_ATMODE
		xbee->mode = XBEE_MODE_UNKNOWN;
	#endif

	return 0;
}

/*** BeginHeader xbee_dev_tick */
/*** EndHeader */
/**
	@brief
	Check for newly received frames on an XBee device and dispatch
	them to registered frame handlers.

	A program with an XBee
	interface needs to call this function often enough to keep up
	with inbound bytes.

	Execution time depends greatly on how long each frame handler
	takes to process its frame.

@warning			This function is NOT re-entrant and will return -EBUSY if it is
					called when already running.

@param[in]	xbee	XBee device to check for, and then dispatch, new frames.

@retval	>=0  		Number of frames received and dispatched.
@retval	-EINVAL	If \a xbee isn't a valid device structure.
@retval	-EBUSY	If xbee_dev_tick() was called when it's already running
							for this device.

*/
_xbee_device_debug
int xbee_dev_tick( xbee_dev_t *xbee)
{
	int frames;

	if (! xbee)
	{
		return -EINVAL;
	}

	INTERRUPT_DISABLE;

	// Prevent recursion -- _xbee_frame_load will dispatch frames as read.  If
	// a frame handler tries to call xbee_dev_tick, don't recurse into
	// _xbee_frame_load again.
	if (xbee->flags & XBEE_DEV_FLAG_IN_TICK)
	{
		INTERRUPT_ENABLE;
		return -EBUSY;
	}

	xbee->flags |= XBEE_DEV_FLAG_IN_TICK;

	INTERRUPT_ENABLE;

	frames = _xbee_frame_load( xbee);
	xbee->flags &= ~XBEE_DEV_FLAG_IN_TICK;

	return frames;
}


/*** BeginHeader _xbee_dispatch_table_dump */
/*** EndHeader */
/**
	@internal
	Dump the contents of the frame dispatch table for XBee
	device \a xbee.  Must have XBEE_DEVICE_VERBOSE defined.

	@param[in]	xbee	XBee device of table to dump.
*/
_xbee_device_debug
void _xbee_dispatch_table_dump( const xbee_dev_t *xbee)
{
#ifndef XBEE_DEVICE_VERBOSE
	// empty/ignored function unless VERBOSE output enabled
	XBEE_UNUSED_PARAMETER( xbee);
#else
	uint_fast8_t i;
	const xbee_dispatch_table_entry_t *entry;

	if (! xbee)
	{
		return;
	}

	puts( "Index\tType\tID\tHandler\tContext");
	entry = xbee_frame_handlers;
	for (i = 0; entry->frame_type != 0xFF; ++i, ++entry)
	{
		if (entry->frame_type)
		{
			printf( "%3d:\t0x%02x\t0x%02x\t0x%p\t%" PRIpFAR "\n", i,
				entry->frame_type, entry->frame_id, entry->handler, entry->context);
		}
		else
		{
			printf( "%3d:\t[empty]\n", i);
		}
	}
#endif
}

/*** BeginHeader _xbee_checksum */
/*** EndHeader */
/**
	@internal
	@brief
	Calculate the checksum for an XBee frame.

					This function actually
					subtracts bytes from 0xFF, in order to determine the proper
					checksum byte so all bytes added together result in 0x00.

					Frame checksums start with the frame type (first byte after
					frame length).

					If calculating a checksum for an outgoing packet, sum all of the
					bytes using this function, and send the low byte of the result.

					If verifying a received packet, summing all of the bytes after
					the length, including the packet's checksum byte, should result
					in 0x00.

					Should only be used internally by this library.

	@param[in]	bytes		Buffer of bytes to add to sum, assumed non-NULL.

	@param[in]	length	Number of bytes to add.

	@param[in]	initial	Starting checksum value.  Use 0xFF to start or the
								result  of the previous call if summing multiple
								blocks of data.

	@return	Value from 0x00 to 0xFF that, when added to all bytes previously
					checksummed, results in a low byte of 0x00.

*/
// Function name in parenthesis so platforms can provide an inline assembly
// version as a replacement, and include a function macro to override this
// function in their platform.h.  See Rabbit
_xbee_device_debug
uint8_t (_xbee_checksum)( const void FAR *bytes, uint_fast8_t length,
	uint_fast8_t initial)
{
	uint8_t i;
	uint8_t checksum;
	const char FAR *p;

	checksum = initial;
	for (p = (const char FAR *)bytes, i = length; i; ++p, --i)
	{
		checksum -= *p;
	}

	return checksum;
}

/*** BeginHeader xbee_frame_write */
/*** EndHeader */
/**
	@brief
	Copies a frame into the transmit serial buffer to send to an
	XBee module.

	Header should include bytes as they will be sent to the XBee.  Function
	accepts separate header and data to limit the amount of copying necessary
	to send requests.

	This function should only be called after \a xbee has been initialized by
	calling xbee_dev_init().

	By default, xbee_frame_write() checks the /CTS signal from the XBee before
	attempting to send, and will return -EBUSY if the XBee has deasserted CTS.
	Use xbee_dev_flowcontrol() to disable this check (necessary on a system
	without a connection to the XBee module's /CTS signal).

	@param[in]	xbee			XBee device to send to.

	@param[in]	header		Pointer to the header to send.  Header starts with
									the frame type (this function will pre-pend the
									0x7E start-of-frame and 16-bit length).
									Pass \c NULL if there isn't a header and the entire
									frame is in the payload (\a data and \a datalen).

	@param[in]	headerlen	Number of header bytes to send (starting with
									address passed in \a header).  Ignored if
									\a header is \c NULL.

	@param[in]	data			Address of frame payload or \c NULL if the entire
									frame content is stored in the header bytes
									(\a header and \a headerlen).

	@param[in]	datalen		Number of payload bytes to send (starting with
									address passed in \a data).  Ignored if \a data
									is \c NULL.

	@param[in]	flags			Optional flags
					- XBEE_WRITE_FLAG_NONE

	@retval	0				Successfully queued frame in transmit serial buffer.
	@retval	-EINVAL		\a xbee is \c NULL or invalid flags passed
	@retval	-ENODATA		No data to send (\a headerlen + \a datalen == 0).
	@retval	-EBUSY		Transmit serial buffer is full, or XBee is not
								accepting serial data (deasserting /CTS signal).
	@retval	-EMSGSIZE	Serial buffer can't ever send a frame this large.

	@sa xbee_dev_init(), xbee_serial_write(), xbee_dev_flowcontrol()
*/
#include "xbee/byteorder.h"

_xbee_device_debug
int xbee_frame_write( xbee_dev_t *xbee, const void FAR *header,
	uint16_t headerlen, const void FAR *data, uint16_t datalen, uint16_t flags)
{
	PACKED_STRUCT {
		uint8_t	start;
		uint16_t	length_be;
	} prefix;

	int cts, free, used, framesize;
	uint8_t checksum = 0xFF;
	#ifdef XBEE_DEVICE_VERBOSE
		uint8_t type, id;			// for debug messages
	#endif

	// flags parameter included in API for future expansion; unused for now
	XBEE_UNUSED_PARAMETER( flags);

	if (xbee == NULL || xbee_ser_invalid( &xbee->serport))
	{
		cts = -EINVAL;
	}
	else if (xbee->flags & XBEE_DEV_FLAG_USE_FLOWCONTROL)
	{
		cts = xbee_ser_get_cts( &xbee->serport);
	}
	else
	{
		cts = 1;
	}

	if (cts == -EINVAL)
	{
		// <xbee> is NULL, or xbee->serport is not valid
		#ifdef XBEE_DEVICE_VERBOSE
			printf( "%s: return -EINVAL (xbee = 0x%p, cts = %d)\n",
				__FUNCTION__, xbee, cts);
		#endif
		return -EINVAL;
	}

	if (! header)
	{
		headerlen = 0;		// if header is NULL, set headerlen to 0
	}
	if (! data)
	{
		datalen = 0;		// if data is NULL, set datalen to 0
	}
	if (! (headerlen || datalen))
	{
		#ifdef XBEE_DEVICE_VERBOSE
			printf( "%s: return -ENODATA (headerlen = %u, datalen = %u)\n",
				__FUNCTION__, headerlen, datalen);
		#endif
		return -ENODATA;
	}

	// Make sure XBee is asserting CTS and verify that the transmit serial buffer
	// has enough room for the frame (payload + 3-byte header + 1-byte checksum).
	free = xbee_ser_tx_free( &xbee->serport);
	used = xbee_ser_tx_used( &xbee->serport);
	framesize = headerlen + datalen + 3 + 1;
	if (! cts || free < framesize)
	{
		#ifdef XBEE_DEVICE_VERBOSE
			printf( "%s: return -EBUSY (cts = %s, free = %d, framesize = %d)\n",
				__FUNCTION__, cts ? "yes" : "no", free, framesize);
		#endif
		return (framesize - free > used) ? -EMSGSIZE : -EBUSY;
	}

	#ifdef XBEE_DEVICE_VERBOSE
		type = *(const char FAR *) (headerlen ? header : data);
		if (headerlen < 2)
		{
			// if headerlen is 1, id is first byte of data, otherwise second byte
			id = ((const char FAR *)data)[headerlen ? 0 : 1];
		}
		else
		{
			id = ((const char FAR *)header)[1];
		}
		printf( "%s: frame type 0x%02x, id 0x%02x (%u-byte payload)\n",
			__FUNCTION__, type, id, headerlen + datalen);
	#endif

	// Send 0x7E (start frame marker) and 16-bit length
	prefix.start = 0x7E;
	prefix.length_be = htobe16( headerlen + datalen);
	xbee_ser_write( &xbee->serport, &prefix, 3);

	// Send <headerlen> bytes from <header> if it is not NULL
	if (headerlen)
	{
		xbee_ser_write( &xbee->serport, header, headerlen);
		checksum = _xbee_checksum( header, (uint_fast8_t) headerlen, checksum);
	}

	// Send <datalen> bytes from <data> if it is not NULL
	if (datalen)
	{
		xbee_ser_write( &xbee->serport, data, datalen);
		checksum = _xbee_checksum( data, (uint_fast8_t) datalen, checksum);
	}

	// Send 1-byte checksum of bytes in payload
	xbee_ser_write( &xbee->serport, &checksum, 1);

	return 0;
}


/*** BeginHeader _xbee_frame_load */
/*** EndHeader */
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
/**
	@internal
	@brief
	Check XBee's serial buffer for complete frames and pass them
	off to the dispatcher.

	Should only be called after \a xbee has been initialized by calling
	xbee_dev_init().  Typically called by xbee_dev_tick().

	@param[in]	xbee	XBee device to read from.

	@retval	0		No new frames waiting.
	@retval	>0		Number of frames processed.
	@retval	<0 	Error.

	@see xbee_dev_init(), _xbee_frame_dispatch()
*/
_xbee_device_debug
int _xbee_frame_load( xbee_dev_t *xbee)
{
	// Use xbee_serial API to load multiple bytes at a time.

	// Based on state, do one of the following:

	// 1) Waiting for start of frame:
	// Scan through serial buffer until 0x7e byte is found.
	// Advance to next state.

	// 2) Waiting for length:
	// Wait until 2 bytes in serial buffer, read into xbee->rx.bytes_in_frame.

	// 3) Waiting for (<length> + 1) bytes of data:
	// Read as many bytes as possible from the serial buffer and into the
	// xbee->rx.frame_data[].  Once all bytes have been read, calculate and
	// verify checksum and then hand off to dispatcher.

	uint8_t ch;
	uint16_t length;
	int retval;
	int bytes_left, read;
	uint_fast8_t dispatched;
	xbee_serial_t	*serport;

	if (xbee == NULL || xbee_ser_invalid( (serport = &xbee->serport) ))
	{
		#ifdef XBEE_DEVICE_VERBOSE
			printf( "%s: return -EINVAL (xbee is %p)\n", __FUNCTION__, xbee);
		#endif
		return -EINVAL;
	}

	dispatched = 0;		// counter to keep track of frames processed

	for (;;)
	{
	   switch (xbee->rx.state)
	   {
	      case XBEE_RX_STATE_WAITSTART:    // waiting for initial 0x7E
	      	/*
					It may seem inefficient to read one byte at a time while looking
					for the 0x7E start byte, but in reality we almost always read it
					in on the first attempt (i.e., the buffer should be empty or will
					start with 0x7E).
	      	*/
	         do {
	            retval = xbee_ser_read( serport, &ch, 1);
	            if (retval != 1)
	            {
	            	return dispatched;
	            }
	         } while (ch != 0x7E);
	         #ifdef XBEE_DEVICE_VERBOSE
					printf( "%s: got start-of-frame\n", __FUNCTION__);
	         #endif
	         xbee->rx.state = XBEE_RX_STATE_LENGTH_MSB;
				// fall through to next state

	   	case XBEE_RX_STATE_LENGTH_MSB:
	   		// try to read a character from the serial port
	   		if (xbee_ser_read( serport, &ch, 1) == 0)
	   		{
	   			return dispatched;
	   		}
				if (ch == 0x7E)
				{
					// MSB of length can never be 0x7E, consider it to be the new
					// start-of-frame character and recheck for the length.
					#ifdef XBEE_DEVICE_VERBOSE
						printf( "%s: ignoring duplicate start-of-frame (0x7E)\n",
							__FUNCTION__);
					#endif
					break;
				}
				// set MSB of frame length
				xbee->rx.bytes_in_frame = ch << 8;
				xbee->rx.state = XBEE_RX_STATE_LENGTH_LSB;
			   // fall through to trying to read LSB of length
	   	case XBEE_RX_STATE_LENGTH_LSB:
	   		// try to read a character from the serial port
	   		if (xbee_ser_read( serport, &ch, 1) == 0)
	   		{
	   			return dispatched;
	   		}

	   		// set LSB of frame length, make local copy for range check
				length = (xbee->rx.bytes_in_frame += ch);
				if (length > XBEE_MAX_FRAME_LEN || length < 2)
				{
					// this isn't a valid frame, go back to looking for start marker
					#ifdef XBEE_DEVICE_VERBOSE
						printf( "%s: read bad frame length (%u ! [2 .. %u])\n",
							__FUNCTION__, length, XBEE_MAX_FRAME_LEN);
					#endif
					if (ch == 0x7E)
					{
						// Handle case of 0x7E 0xXX 0x7E where second 0x7E is actual
						// start of frame.
						xbee->rx.state = XBEE_RX_STATE_LENGTH_MSB;
					}
					else
					{
						xbee->rx.state = XBEE_RX_STATE_WAITSTART;
					}
					break;
				}
	         #ifdef XBEE_DEVICE_VERBOSE
					printf( "%s: got length %" PRIu16 "\n", __FUNCTION__, length);
	         #endif
				xbee->rx.state = XBEE_RX_STATE_RXFRAME;
				xbee->rx.bytes_read = 0;
				// fall through to next state

	      case XBEE_RX_STATE_RXFRAME:      // receiving frame & trailing checksum
	      	bytes_left = xbee->rx.bytes_in_frame - xbee->rx.bytes_read + 1;
				read = xbee_ser_read( serport,
							xbee->rx.frame_data + xbee->rx.bytes_read, bytes_left);
				if (read != bytes_left)
				{
					// Not enough bytes to finish reading current frame, record
					// number of bytes read and return.
					if (read > 0)
					{
						xbee->rx.bytes_read += read;
					}
					return dispatched;
				}

            // ready to load more frames on next pass
            xbee->rx.state = XBEE_RX_STATE_WAITSTART;

				if (_xbee_checksum( xbee->rx.frame_data,
														xbee->rx.bytes_in_frame + 1, 0xFF))
				{
					// checksum failed, throw out the frame
					#ifdef XBEE_DEVICE_VERBOSE
						printf( "%s: checksum failed\n", __FUNCTION__);
						hex_dump( xbee->rx.frame_data, xbee->rx.bytes_in_frame + 1,
							HEX_DUMP_FLAG_OFFSET);
					#endif

					/* At this point, we *could* look through the frame data for
						another start-of-frame (0x7E) marker, including considering
						the LSB of the length field.  We shouldn't have to though --
						assuming a good serial connection, we should stay in sync
						with XBee frames and not have to work too hard at resyncing.

						Also, we only sync to a select range of 3-byte sequences --
						0x7E followed by two-byte length of 0 to about 300.
					*/

					break;
				}
				else
				{
					// frame is ready for dispatch
					++dispatched;
					#ifdef XBEE_DEVICE_VERBOSE
						printf( "%s: dispatch frame #%d\n", __FUNCTION__,
							dispatched);
					#endif
					_xbee_frame_dispatch( xbee, xbee->rx.frame_data,
																	xbee->rx.bytes_in_frame);

					if (dispatched == XBEE_DEV_MAX_DISPATCH_PER_TICK)
					{
						return dispatched;
					}
				}
	         break;

			default:
				#ifdef XBEE_DEVICE_VERBOSE
					printf( "%s: invalid state %d\n", __FUNCTION__,
						xbee->rx.state);
				#endif
				xbee->rx.state = XBEE_RX_STATE_WAITSTART;
	   }
	}
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif


/*** BeginHeader _xbee_frame_dispatch */
/*** EndHeader */
/**
	@internal
	@brief
	Receive modem status frames and update flags/state of wpan_dev_t.

	@see xbee_frame_handler_fn()
*/
_xbee_device_debug
void _xbee_dev_modem_status( wpan_dev_t *wpan, uint_fast8_t status)
{
	uint16_t flags = wpan->flags;

	switch (status)
	{
		case XBEE_MODEM_STATUS_COORD_START:
			// We're the coordinator, so we know our network address and are
			// implicitly joined.  If EO is non-zero (which we aren't checking)
			// then we're authenticated as well.  Maybe add an EE check to
			// xbee_cmd_query_device and set another flag?
			wpan->address.network = WPAN_NET_ADDR_COORDINATOR;
			flags |= (WPAN_FLAG_JOINED | WPAN_FLAG_AUTHENTICATED);
			break;

		case XBEE_MODEM_STATUS_KEY_ESTABLISHED:
			flags |= WPAN_FLAG_AUTHENTICATED;
			// It's possible to go straight to Key Established from a watchdog
			// timeout, so fall through to the JOINED status as well.

		case XBEE_MODEM_STATUS_JOINED:
			flags |= WPAN_FLAG_JOINED;
			break;

		case XBEE_MODEM_STATUS_HW_RESET:
		case XBEE_MODEM_STATUS_WATCHDOG:
		case XBEE_MODEM_STATUS_DISASSOC:
			// no longer joined or authenticated
			flags &= ~(WPAN_FLAG_JOINED | WPAN_FLAG_AUTHENTICATED);
			wpan->address.network = WPAN_NET_ADDR_UNDEFINED;
			#ifdef XBEE_DEVICE_VERBOSE
				printf( "%s: marked network address as invalid (status=0x%02x)\n",
					__FUNCTION__, status);
			#endif
			break;
	}

	wpan->flags = flags;
}


/**
	@internal
	@brief
	Function called by _xbee_frame_load() to dispatch any frames read.

	Scans through xbee->frame_handlers, matching
	the frame_type and frame_id (if frame_id is not zero).  Passes
	the frame and context (from the frame handler table) to each
	matching handler.

	@param[in]	xbee		XBee device that received the frames.

	@param[in]	frame		Address of bytes in frame, starting with the frame
								type byte.  Note that the frame is still in network
								byte order -- it is exactly as received from the
								XBee module.

	@param[in]	length	Number of bytes in frame.

	@return	Currently returns 0 in all cases.  May be updated at some point
				with various error messages (e.g., no handlers are registered
				for the given frame type and ID) or the number of handlers it
				was dispatched to

	@retval -EINVAL	Invalid parameter
	@retval	>=0		number of handlers frame was dispatched to
*/
_xbee_device_debug
int _xbee_frame_dispatch( xbee_dev_t *xbee, const void FAR *frame,
	uint16_t length)
{
	uint_fast8_t frametype, frameid;
	bool_t dispatched;
	const xbee_dispatch_table_entry_t *entry;

	if (! (xbee && frame && length))
	{
		// Since this is an internal API, always called correctly, this should
		// not happen and could be changed to an assert.  Currently part of
		// unit tests for this module.
		return -EINVAL;
	}

	// first byte of <frame> is the frametype, second is frame ID
	frametype = ((const uint8_t FAR *)frame)[0];
	frameid = ((const uint8_t FAR *)frame)[1];

	if (frametype == XBEE_FRAME_MODEM_STATUS)
	{
		_xbee_dev_modem_status( &xbee->wpan_dev, frameid);
	}

	#ifdef XBEE_DEVICE_VERBOSE
		printf( "%s: dispatch frame type 0x%02x, id 0x%02x\n",
			__FUNCTION__, frametype, frameid);
		hex_dump( frame, length, HEX_DUMP_FLAG_NONE);
	#endif

	dispatched = 0;
	for (entry = xbee_frame_handlers; entry->frame_type != 0xFF; ++entry)
	{
		if (entry->frame_type == frametype)
		{
			if (! entry->frame_id || entry->frame_id == frameid)
			{
				++dispatched;
				// entry matches all frame IDs (0) or matches this frame's ID
	         #ifdef XBEE_DEVICE_VERBOSE
	            printf( "%s: calling frame handler @%p, w/context %" \
	            	PRIpFAR "\n", __FUNCTION__, entry->handler, entry->context);
	         #endif
				entry->handler( xbee, frame, length, entry->context);
			}
		}
	}

	#ifdef XBEE_DEVICE_VERBOSE
		if (! dispatched)
		{
			printf( "%s: no handlers for frame type 0x%02x, id 0x%02x\n",
				__FUNCTION__, frametype, frameid);
		}
	#endif

	return dispatched;
}


/*** BeginHeader xbee_frame_dump_modem_status */
/*** EndHeader */
#include <stdio.h>
// see xbee/device.h for documentation
_xbee_device_debug
int xbee_frame_dump_modem_status( xbee_dev_t *xbee,
	const void FAR *payload, uint16_t length, void FAR *context)
{
	const xbee_frame_modem_status_t FAR *frame = payload;
	char *status_str;

	// Standard frame handler callback API, but we only care about payload.
	XBEE_UNUSED_PARAMETER( xbee);
	XBEE_UNUSED_PARAMETER( length);
	XBEE_UNUSED_PARAMETER( context);

	// stack will never call us with a NULL frame, but user code might
	if (frame == NULL)
	{
		return -EINVAL;
	}

	// Instead of updating this lib to trigger changes in other layers of the
	// API, other libraries should register to receive modem status events
	// directly.

	switch (frame->status)
	{
		case XBEE_MODEM_STATUS_HW_RESET:
			status_str = "hardware reset";
			break;

		case XBEE_MODEM_STATUS_WATCHDOG:
			status_str = "watchdog timeout";
			break;

		case XBEE_MODEM_STATUS_JOINED:
			status_str = "joined";
			break;

		case XBEE_MODEM_STATUS_DISASSOC:
			status_str = "disassociated";
			break;

		case XBEE_MODEM_STATUS_COORD_START:
			status_str = "coordinator started";
			break;

		case XBEE_MODEM_STATUS_NETWORK_KEY_UPDATED:
			status_str = "key updated";
			break;

		case XBEE_MODEM_STATUS_OVERVOLTAGE:
			status_str = "overvoltage";
			break;

		case XBEE_MODEM_STATUS_WOKE_UP:
			status_str = "woke up";
			break;

		case XBEE_MODEM_STATUS_SLEEPING:
			status_str = "sleeping";
			break;

		case XBEE_MODEM_STATUS_KEY_ESTABLISHED:
			status_str = "key established";
			break;

		case XBEE_MODEM_STATUS_CONFIG_CHANGE_IN_JOIN:
			status_str = "config change during join";
			break;

		case XBEE_MODEM_STATUS_STACK_ERROR:
			status_str = "stack error";
			break;

		default:
			status_str = "undefined";
	}
	printf( "%s: status: %s (0x%02x)\n", __FUNCTION__,
		status_str, frame->status);

	return 0;
}

