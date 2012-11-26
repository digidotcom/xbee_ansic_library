/*
 * Copyright (c) 2007-2012 Digi International Inc.,
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
	@addtogroup util_xmodem
	@{
	@file xbee_xmodem.c
	Xmodem send implementation, used for XBee firmware updates.

	@todo Have timeout values adjust based on link latency.  Start out with a
			high timeout (10 seconds?) and adjust down based on actual response
			time (maybe 150% of last ACK's delay?)  Timeout for EOT should start
			at timeout value from last block.
*/

/*** BeginHeader */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/xmodem.h"

#ifndef __DC__
	#define _xmodem_debug
#elif defined XMODEM_DEBUG
   #define _xmodem_debug __debug
#else
   #define _xmodem_debug __nodebug
#endif

/*** EndHeader */

/*** BeginHeader _xbee_xmodem_ser_read, _xbee_xmodem_ser_write */
int _xbee_xmodem_ser_read( void FAR *context, void FAR *buffer,
																					int16_t bytes);
int _xbee_xmodem_ser_write( void FAR *context, const void FAR *buffer,
																					int16_t bytes);
/*** EndHeader */
// Stub functions for sending Xmodem to serially-attached XBee.  Intentionally
// downcasting FAR context to near pointer since xbee_serial_t objects are
// always near.

_xmodem_debug
int _xbee_xmodem_ser_read( void FAR *context, void FAR *buffer, int16_t bytes)
{
	return xbee_ser_read( CAST_FAR_TO_NEAR(context), buffer, bytes);
}
_xmodem_debug
int _xbee_xmodem_ser_write( void FAR *context, const void FAR *buffer,
																					int16_t bytes)
{
	return xbee_ser_write( CAST_FAR_TO_NEAR(context), buffer, bytes);
}

/*** BeginHeader xbee_xmodem_use_serport */
/*** EndHeader */
// documented in xbee/xmodem.h
_xmodem_debug
int xbee_xmodem_use_serport( xbee_xmodem_state_t *xbxm, xbee_serial_t *serport)
{
	return xbee_xmodem_set_stream( xbxm, _xbee_xmodem_ser_read,
		_xbee_xmodem_ser_write, serport);
}

/*** BeginHeader xbee_xmodem_set_stream */
/*** EndHeader */
// documented in xbee/xmodem.h
_xmodem_debug
int xbee_xmodem_set_stream( xbee_xmodem_state_t *xbxm,
	xbee_xmodem_read_fn read, xbee_xmodem_write_fn write,
	const void FAR *context)
{
	if (xbxm == NULL || read == NULL || write == NULL)
	{
		return -EINVAL;
	}

	xbxm->stream.read = read;
	xbxm->stream.write = write;

	// Cast away const to allow for const and non-const context.  We do this
	// to allow user code to set up read/write functions with a const or a
	// non-const context.  If they want to use const, this function needs to
	// accept const (and their read/write functions should be written as const).
	// If they want to use a non-const context, this function promotes it to
	// const unnecessarily.
	xbxm->stream.context = (void FAR *) context;

	return 0;
}

/*** BeginHeader xbee_xmodem_set_source */
/*** EndHeader */
// documented in xbee/xmodem.h
_xmodem_debug
int xbee_xmodem_set_source( xbee_xmodem_state_t *xbxm,
	void FAR *buffer, xbee_xmodem_read_fn read, const void FAR *context)
{
	if (xbxm == NULL || buffer == NULL || read == NULL)
	{
		return -EINVAL;
	}

	xbxm->buffer = buffer;
	xbxm->file.read = read;

	// Cast away const to allow for const and non-const context.  See comment
	// in xbee_xmodem_set_stream for reasoning behind const/non-const context.
	xbxm->file.context = (void FAR *) context;

	return 0;
}

/*** BeginHeader _xbee_xmodem_getchar, _xbee_xmodem_putchar */
int _xbee_xmodem_getchar( xbee_xmodem_state_t *xbxm);
int _xbee_xmodem_putchar( xbee_xmodem_state_t *xbxm, uint8_t ch);
/*** EndHeader */
/**
	@internal
	Helper function to read a single character from the stream.

	@param[in]	xbxm		state structure

	@retval	0-255			character read from stream
	@retval	-ENODATA		stream is empty
	@retval	<0				error reading from stream
*/
_xmodem_debug
int _xbee_xmodem_getchar( xbee_xmodem_state_t *xbxm)
{
	uint8_t ch = 0;
	int retval;

	retval = xbxm->stream.read( xbxm->stream.context, &ch, 1);
	if (retval != 1)
	{
		return retval ? retval : -ENODATA;
	}

	return ch;
}

/**
	@internal
	Helper function to write a single character to the stream.

	@param[in]	xbxm		state structure
	@param[in]	ch			character to write

	@retval	0				character was written to stream
	@retval	-ENOSPC		could not write to stream
	@retval	<0				error writing to stream
*/
_xmodem_debug
int _xbee_xmodem_putchar( xbee_xmodem_state_t *xbxm, uint8_t ch)
{
	int retval;

	retval = xbxm->stream.write( xbxm->stream.context, &ch, 1);
	if (retval == 1)
	{
		return 0;
	}
	else if (retval == 0)
	{
		return -ENOSPC;
	}
	else
	{
		return retval;
	}
}

/*** BeginHeader xbee_xmodem_tx_init */
/*** EndHeader */
// documented in xbee/xmodem.h
_xmodem_debug
int xbee_xmodem_tx_init( xbee_xmodem_state_t *xbxm, uint16_t flags)
{
	if (! xbxm)
	{
		return -EINVAL;
	}

	xbxm->flags = flags & XBEE_XMODEM_FLAG_USER;
	xbxm->packet_num = 0;
	xbxm->state = XBEE_XMODEM_STATE_START;

	return 0;
}

/*** BeginHeader xbee_xmodem_tx_tick */
/*** EndHeader */
// Dynamic C has crc16_calc() built in (part of crc16.lib).  On other
// platforms bring it in via its header.
#ifndef __DC__
	#include "xbee/xmodem_crc16.h"
#endif
// <0 for failure, 1 for file done, 0 for OK to send
_xmodem_debug
int _assemble_packet( xbee_xmodem_state_t *xbxm, uint16_t block_size)
{
	int err;
	uint8_t packet_num;
	uint16_t checksum;
	int ch;
	uint16_t i;
	char FAR *p;

	// Assemble a packet to send -- 3-byte header; 64, 128 or 1K xmodem
	// block from source file; and then 1-byte checksum or 2-byte CRC.

	xbxm->tries = 3;
	xbxm->buffer[0] =
			(xbxm->flags & XBEE_XMODEM_FLAG_1K) ? XMODEM_STX : XMODEM_SOH;
	packet_num = (uint8_t) xbxm->packet_num;
	xbxm->buffer[1] = packet_num;
	xbxm->buffer[2] = ~packet_num;
	err = xbxm->file.read( xbxm->file.context, &xbxm->buffer[3], block_size);
	if (err == -ENODATA || err == 0)
	{
		#ifdef XBEE_XMODEM_VERBOSE
			printf( "%s: reached end of file\n", __FUNCTION__);
		#endif
		xbxm->state = XBEE_XMODEM_STATE_EOF;
		return 1;
	}
	else if (err < 0)
	{
		#ifdef XBEE_XMODEM_VERBOSE
			printf( "%s: error %d reading file\n", __FUNCTION__, err);
		#endif
		return err;
	}
	else if (err < block_size)
	{
		// if necessary, we could wrap this all up into a while loop that
		// uses multiple reads, if necessary, to fill xbxm->buffer.
		if (xbxm->file.read( xbxm->file.context, &ch, 1) == 1)
		{
			#ifdef XBEE_XMODEM_VERBOSE
				printf( "%s: short read of %d bytes\n", __FUNCTION__, err);
			#endif
			return -EIO;
		}

		// short read, but it's the last block from the file -- set
		// remaining bytes to 0xFF
		_f_memset( &xbxm->buffer[3 + err], 0xFF, block_size - err);
	}

	if (xbxm->flags & XBEE_XMODEM_FLAG_CRC)
	{
		checksum = crc16_calc( &xbxm->buffer[3], block_size, 0);
		// append 16-bit CRC, MSB-first
		xbxm->buffer[3 + block_size] = checksum >> 8;
		xbxm->buffer[4 + block_size] = checksum & 0x00FF;
	}
	else
	{
		checksum = 0;
		for (p = &xbxm->buffer[3], i = block_size; i; --i)
		{
			checksum += *p++;
		}
		xbxm->buffer[3 + block_size] = (uint8_t) checksum;
	}

	return 0;
}

_xmodem_debug
uint16_t _block_size( xbee_xmodem_state_t *xbxm)
{
	switch (xbxm->flags & XBEE_XMODEM_MASK_BLOCKSIZE)
	{
		case XBEE_XMODEM_FLAG_64:			return 64;
		default:
		case XBEE_XMODEM_FLAG_128:			return 128;
		case XBEE_XMODEM_FLAG_1024:		return 1024;
	}
}

// documented in xbee/xmodem.h
_xmodem_debug
int xbee_xmodem_tx_tick( xbee_xmodem_state_t *xbxm)
{
	static const char CANCAN[] = { XMODEM_CAN, XMODEM_CAN };
	int err = -ETIMEDOUT;			// default error
	int retval;
	int sent;
	int ch;
	uint16_t packet_size, block_size;

	if (xbxm == NULL || xbxm->buffer == NULL)
	{
		return -EINVAL;
	}

	block_size = _block_size( xbxm);

	switch (xbxm->state)
	{
		case XBEE_XMODEM_STATE_FLUSH:
			// flush the receive buffer
			while (xbxm->stream.read( xbxm->stream.context, xbxm->buffer, 64) > 0)
			{
				// read from target until buffer is empty, then look for start char
			}
			xbxm->state = XBEE_XMODEM_STATE_START;
			// fall through to start state
		case XBEE_XMODEM_STATE_START:
			ch = _xbee_xmodem_getchar( xbxm);
			if (ch == XMODEM_NAK && !(xbxm->flags & XBEE_XMODEM_FLAG_FORCE_CRC))
			{
				xbxm->flags |= XBEE_XMODEM_FLAG_CHECKSUM;
			}
			else if (ch == XMODEM_CRC)
			{
				xbxm->flags |= XBEE_XMODEM_FLAG_CRC;
			}
			else
			{
				break;
			}

			#ifdef XBEE_XMODEM_VERBOSE
				printf( "%s: starting XMODEM%s\n", __FUNCTION__,
					(ch == XMODEM_CRC) ? "-CRC" : "");
			#endif
			// got start character, can start sending packets
			xbxm->state = XBEE_XMODEM_STATE_SEND;
			xbxm->packet_num = 1;
			// fall through

		case XBEE_XMODEM_STATE_SEND:
			retval = _assemble_packet( xbxm, block_size);
			if (retval < 0)
			{
				err = retval;
				goto failure;
			}
			else if (retval > 0)
			{
				break;			// transfer complete
			}

			while (_xbee_xmodem_getchar( xbxm) != -ENODATA)
			{
				// flush any extra characters received from other end
			}

			// fall through to start of sending packet

		case XBEE_XMODEM_STATE_RESEND:
			#ifdef XBEE_XMODEM_VERBOSE
				printf( "%s: sending packet %d\n", __FUNCTION__, xbxm->packet_num);
			#endif
			#ifdef XBEE_XMODEM_TESTING
				if (xbxm->flags &
						(XBEE_XMODEM_FLAG_BAD_CRC | XBEE_XMODEM_FLAG_RESTORE_CRC))
				{
					bool_t restore;

					// break CRC on first pass, restore it on second pass
					xbxm->buffer[3 + block_size] ^= 0xBA;
					xbxm->buffer[4 + block_size] ^= 0xAD;

					// handle transition of BAD_CRC set, RESTORE_CRC set, both clear
					restore = (xbxm->flags & XBEE_XMODEM_FLAG_BAD_CRC) != 0;
					xbxm->flags &=
							~(XBEE_XMODEM_FLAG_BAD_CRC | XBEE_XMODEM_FLAG_RESTORE_CRC);
					if (restore)
					{
						xbxm->flags |= XBEE_XMODEM_FLAG_RESTORE_CRC;
					}
				}
			#endif
			xbxm->offset = 0;
			// fall through

		case XBEE_XMODEM_STATE_SENDING:
			if (xbxm->flags & XBEE_XMODEM_FLAG_CRC)
			{
				packet_size = block_size + 5;			// 3-byte header, 2-byte CRC
			}
			else
			{
				packet_size = block_size + 4;			// 3-byte header, 1-byte checksum
			}

			#ifdef XBEE_XMODEM_TESTING
				if (xbxm->flags & XBEE_XMODEM_FLAG_DROP_FRAME)
				{
					xbxm->flags &= ~XBEE_XMODEM_FLAG_DROP_FRAME;
					sent = packet_size - xbxm->offset;
				}
				else
			#endif
				{
					sent = xbxm->stream.write( xbxm->stream.context,
						&xbxm->buffer[xbxm->offset], packet_size - xbxm->offset);
				}

			if (sent < 0)
			{
				err = sent;
				goto failure;
			}
			xbxm->offset += sent;
			if (xbxm->offset == packet_size)
			{
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: buffered packet, waiting for ACK\n",
						__FUNCTION__);
				#endif
				xbxm->state = XBEE_XMODEM_STATE_WAIT_ACK;
				xbxm->timer = (uint16_t) xbee_millisecond_timer();
			}
			break;

		case XBEE_XMODEM_STATE_WAIT_ACK:
			ch = _xbee_xmodem_getchar( xbxm);

			#ifdef XBEE_XMODEM_TESTING
				if (ch == XMODEM_ACK && (xbxm->flags & XBEE_XMODEM_FLAG_DROP_ACK))
				{
					xbxm->flags &= ~XBEE_XMODEM_FLAG_DROP_ACK;
					ch = -1;
				}
			#endif

			if (ch == XMODEM_ACK)
			{
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: received ACK for packet %d\n", __FUNCTION__,
						xbxm->packet_num);
				#endif
				++xbxm->packet_num;

				// read and send the next packet
				xbxm->state = XBEE_XMODEM_STATE_SEND;
				return 0;
			}
			else if (ch < 0)
			{
				if ((uint16_t) xbee_millisecond_timer() - xbxm->timer < 10000)
				{
					return 0;
				}
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: timeout waiting for ACK\n", __FUNCTION__);
				#endif
			}
			else
			{
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: received 0x%02x waiting for ACK\n",
						__FUNCTION__, ch);
				#endif
			}

			if (--xbxm->tries)
			{
				xbxm->state = XBEE_XMODEM_STATE_RESEND;
				return 0;
			}

			#ifdef XBEE_XMODEM_VERBOSE
				printf( "%s: abort transfer, no ACK\n", __FUNCTION__);
			#endif
			goto failure;

		case XBEE_XMODEM_STATE_EOF:
			#ifdef XBEE_XMODEM_VERBOSE
				printf( "%s: send EOT and wait for ACK\n", __FUNCTION__);
			#endif
			_xbee_xmodem_putchar( xbxm, XMODEM_EOT);
			xbxm->state = XBEE_XMODEM_STATE_FINAL_ACK;
			xbxm->timer = (uint16_t) xbee_millisecond_timer();
			break;

		case XBEE_XMODEM_STATE_FINAL_ACK:
			ch = _xbee_xmodem_getchar( xbxm);
			if (ch == XMODEM_ACK)
			{
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: received final ACK, transfer complete\n",
						__FUNCTION__);
				#endif
				xbxm->state = XBEE_XMODEM_STATE_SUCCESS;
				return 1;
			}
			else if (ch < 0)
			{
				if ((uint16_t) xbee_millisecond_timer() - xbxm->timer < 3000)
				{
					return 0;
				}
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: timeout waiting for ACK\n", __FUNCTION__);
				#endif
			}
			else
			{
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: received 0x%02x waiting for ACK\n",
						__FUNCTION__, ch);
				#endif
			}
			if (--xbxm->tries)
			{
				xbxm->state = XBEE_XMODEM_STATE_EOF;
			}
			else
			{
				#ifdef XBEE_XMODEM_VERBOSE
					printf( "%s: abort transfer, didn't get final ACK\n",
						__FUNCTION__);
				#endif
				goto failure;
			}
			break;

		case XBEE_XMODEM_STATE_SUCCESS:
			return 1;

		case XBEE_XMODEM_STATE_FAILURE:
			return -ETIMEDOUT;
	}

	return 0;

failure:
	// abort transfer and exit
	xbxm->stream.write( xbxm->stream.context, CANCAN, 2);
	xbxm->state = XBEE_XMODEM_STATE_FAILURE;

	return err;
}

