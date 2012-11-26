/*
 * Copyright (c) 2008-2012 Digi International Inc.,
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
	@addtogroup xbee_firmware
	@{
	@file xbee_firmware.c
	API related to updating XBee firmware on the local XBee module.
	This could grow to support OTA (Over The Air) updates, via an expanded API.

	Include helper API for initiating an update via firmware loaded to a
	far char buffer.  An additional xbee_firmware_fat.lib could include
	helper functions for installing firmware from a file on the FAT filesystem.

	Define XBEE_FIRMWARE_VERBOSE for debugging messages.

	Define XBEE_FIRMWARE_DEBUG to make the functions in this file debuggable.

*/

/*** BeginHeader */
#include <errno.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atmode.h"
#include "xbee/byteorder.h"
#include "xbee/firmware.h"

#ifndef __DC__
	#define _xbee_firmware_debug
#elif defined XBEE_FIRMWARE_DEBUG
	#define _xbee_firmware_debug __debug
#else
	#define _xbee_firmware_debug __nodebug
#endif

enum {
	XBEE_FW_STATE_INIT,				// initial state
	XBEE_FW_STATE_RESET,				// reset and attempt to enter boot loader
	XBEE_FW_STATE_TOGGLE,			// hold reset line for 50ms
	XBEE_FW_STATE_BREAK,				// end serial break after 150ms

	// states for .EBL files (e.g., ZNet, ZigBee)
	XBEE_FW_STATE_PROMPT,			// waiting for '>' prompt
	XBEE_FW_STATE_XMODEM_WAIT,		// waiting for 'C' to start Xmodem send
	XBEE_FW_STATE_XMODEM_SEND,		// sending firmware via Xmodem
	XBEE_FW_STATE_XMODEM_DONE,		// Xmodem complete, wait 3 seconds and send CR
	XBEE_FW_STATE_FINAL_PROMPT,	// wait for '>' prompt and we're done!

	// states for .OEM files (e.g., DigiMesh 900)
	XBEE_FW_STATE_CMD_PENDING,		// waiting to enter command mode
	XBEE_FW_STATE_CMD,				// in command mode
	XBEE_FW_STATE_PARSE_RESPONSE,	// general state for parsing an AT response
	XBEE_FW_STATE_CMD_HV,			// waiting for ATHV response
	XBEE_FW_STATE_CMD_COMPAT,		// waiting for AT%C response
	XBEE_FW_STATE_CMD_VR,			// waiting for ATVR response
	XBEE_FW_STATE_CMD_CF1,			// waiting for ATCF1 response
	XBEE_FW_STATE_CMD_SL,			// waiting for ATSL response
	XBEE_FW_STATE_CMD_PROG,			// waiting for AT%Pxxxx response
	XBEE_FW_STATE_CMD_FR,				// waiting for ATFR response

	XBEE_FW_STATE_BOOTLOADER,		// in the bootloader, ready to send
	XBEE_FW_STATE_TX_START,			// looking for bootloader response

	XBEE_FW_STATE_TX_BLOCK,			// start sending a block
	XBEE_FW_STATE_SENDING,			// actually sending the block
	XBEE_FW_STATE_RX_BLOCK,			// receiving a block
	XBEE_FW_STATE_RX_FAIL,			// receive failed

	XBEE_FW_STATE_DONE,				// done and going from command to idle mode
	XBEE_FW_STATE_FAILURE,			// upload aborted
	XBEE_FW_STATE_SUCCESS,			// finished upload and all is good
};

/*** EndHeader */

/*** BeginHeader xbee_fw_install_init */
/*** EndHeader */
/**
	@brief
					Prepare to install new firmware on an attached XBee module.

					The host must be able to control the reset pin of the XBee
					module.

	@param[in]	xbee		XBee device to install firmware on.  Must have been
					set up with xbee_dev_init().

	@param[in]	target	The current version of this library can only update the
					local XBee module, so this parameter should always be NULL.  When
					over-the-air (OTA) updating is supported, this paramter will be
					the address of a remote module to update.

	@param[in]	source	Structure with function pointers for seeking and reading
					from the new firmware image.

@code
					// Function prototypes for functions that will provide firmware
					// bytes when called by xbee_fw_install_tick().
					int my_firmware_seek( uint32_t offset);
					int my_firmware_read( void FAR *buffer, int16_t bytes);

					xbee_fw_source_t	fw;
					xbee_dev_t			xbee;

					fw.seek = &my_firmware_seek;
					fw.read = &my_firmware_read;

					xbee_dev_init( &xbee, ...);
					xbee_fw_install_init( &xbee, &fw);
@endcode

	@retval	0			success
	@retval	-EINVAL	NULL parameter passed to function
	@retval	-EIO		XBee device passed to function doesn't have a
								callback for controlling the reset pin on the XBee
								module.

*/
_xbee_firmware_debug
int xbee_fw_install_init( xbee_dev_t *xbee, const wpan_address_t FAR *target,
	xbee_fw_source_t *source)
{
	if (! (xbee && source))
	{
		return -EINVAL;
	}

	if (target)
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: return -EINVAL, over-the-air updates not supported yet\n",
				__FUNCTION__);
		#endif
		return -EINVAL;
	}

	if (! xbee->reset)
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: can't control reset pin, can't enter bootloader\n",
				__FUNCTION__);
		#endif
		return -EIO;
	}
	source->xbee = xbee;
	source->state = XBEE_FW_STATE_INIT;
	source->bootloader_cmd = '1';

	return 0;
}


/*** BeginHeader xbee_fw_install_ebl_state */
/*** EndHeader */
/**
	@brief
	Returns a unique value indicating the state of the .EBL install process.

	@param[in]	source	object used to track state of transfer

	@return unique value that changes whenever the transfer state changes

	@sa xbee_fw_status_ebl
*/
_xbee_firmware_debug
unsigned int xbee_fw_install_ebl_state( xbee_fw_source_t *source)
{
	return (source->u.xbxm.packet_num << 6) | source->state;
}


/*** BeginHeader xbee_fw_install_hcs08_tick */
/*** EndHeader */
/**
	@brief
	Drive the firmware update process for the HCS08 application on Programmable
	XBee modules.

	@param[in,out]	source	object used to track state of transfer

	@retval	0			update is in progress
	@retval	1			update completed successfully
	@retval	-EINVAL	\a source is invalid
	@retval	-EIO		general failure
	@retval	-ETIMEDOUT	connection timed out waiting for data from target

	@sa xbee_fw_install_init, xbee_fw_install_ebl_state, xbee_fw_status_ebl
*/
_xbee_firmware_debug
int xbee_fw_install_hcs08_tick( xbee_fw_source_t *source)
{
	int result;

	if (source != NULL && source->state == XBEE_FW_STATE_INIT)
	{
		// PXBee bootloader uses 'F' instead of '1' to start XMODEM
		source->bootloader_cmd = 'F';
	}

	// this function call takes care of parameter checking
	result = xbee_fw_install_ebl_tick( source);
	if (result >= 0)
	{
		switch (source->state)
		{
			case XBEE_FW_STATE_FINAL_PROMPT:
				// no final prompt, can go straight to SUCCESS
				source->state = XBEE_FW_STATE_SUCCESS;
				return 1;
		}
	}

	return result;
}


/*** BeginHeader xbee_fw_install_ebl_tick */
/*** EndHeader */
#define _TIME_ELAPSED(x)  (xbee_millisecond_timer() - source->timer > (x))
/**
	@brief
	Drive the firmware update process for boards that use .EBL files to store
	their firmware.

	@param[in,out]	source	object used to track state of transfer

	@retval	0			update is in progress
	@retval	1			update completed successfully
	@retval	-EINVAL	\a source is invalid
	@retval	-EIO		general failure
	@retval	-ETIMEDOUT	connection timed out waiting for data from target

	@sa xbee_fw_install_init, xbee_fw_install_ebl_state, xbee_fw_status_ebl
*/
_xbee_firmware_debug
int xbee_fw_install_ebl_tick( xbee_fw_source_t *source)
{
	int result;
	xbee_dev_t *xbee;
	xbee_serial_t *serport;

	if (! source)
	{
		return -EINVAL;
	}

	xbee = source->xbee;
	serport = &xbee->serport;

	switch (source->state)
	{
		case XBEE_FW_STATE_INIT:
			source->tries = 0;
			// fall through to reset state

		case XBEE_FW_STATE_RESET:
	      // make sure reset line starts out not reset (high)
	      xbee->reset( xbee, 0);

	      #ifdef XBEE_FIRMWARE_VERBOSE
	         printf( "%s: open serial port\n", __FUNCTION__);
	      #endif

	      // Open serial port to bootloader's baud rate.
	      xbee_ser_open( serport, 115200);
	      xbee_ser_flowcontrol( serport, 1);

	      // clear RTS
	      #ifdef XBEE_FIRMWARE_VERBOSE
	         printf( "%s: RTS off\n", __FUNCTION__);
	      #endif
	      xbee_ser_set_rts( serport, 0);

	      // Begin serial break condition by setting Tx line low (space) while
	      // resetting XBee module.
	      #ifdef XBEE_FIRMWARE_VERBOSE
	         printf( "%s: start serial break & toggle reset low\n",
	         	__FUNCTION__);
	      #endif
	      xbee_ser_break( serport, 1);

			xbee_dev_reset( xbee);
			source->timer = xbee_millisecond_timer();
			source->state = XBEE_FW_STATE_BREAK;
			break;

		case XBEE_FW_STATE_BREAK:
			// keep break condition for 150ms, then re-open serial port
			if (_TIME_ELAPSED( 150))
			{
	         #ifdef XBEE_FIRMWARE_VERBOSE
	            printf( "%s: clear break and assert RTS\n",
	            	__FUNCTION__);
	         #endif

	         // clear break condition and set RTS
	         xbee_ser_break( serport, 0);
	         xbee_ser_flowcontrol( serport, 1);
	         xbee_ser_set_rts( serport, 1);

	         // Send CR to radio and wait for a prompt to come back
	         xbee_ser_putchar( serport, '\r');

	         source->timer = xbee_millisecond_timer();
	         source->state = XBEE_FW_STATE_PROMPT;
			}
			break;

		case XBEE_FW_STATE_PROMPT:
			if (xbee_ser_getchar( serport) == '>')
			{
	         #ifdef XBEE_FIRMWARE_VERBOSE
	            printf( "%s: entered bootloader, initiate upload\n",
	            	__FUNCTION__);
	         #endif
				xbee_ser_putchar( serport, source->bootloader_cmd);
	         source->timer = xbee_millisecond_timer();
				source->state = XBEE_FW_STATE_XMODEM_WAIT;
			}
			break;

		case XBEE_FW_STATE_XMODEM_WAIT:
			if (xbee_ser_getchar( serport) == 'C')
			{
	         #ifdef XBEE_FIRMWARE_VERBOSE
	            printf( "%s: starting Xmodem send\n", __FUNCTION__);
	         #endif
	         xbee_xmodem_use_serport( &source->u.xbxm, serport);
	         xbee_xmodem_set_source( &source->u.xbxm, source->buffer,
	         	source->read, source->context);
				result = xbee_xmodem_tx_init( &source->u.xbxm,
					XBEE_XMODEM_FLAG_128 | XBEE_XMODEM_FLAG_FORCE_CRC);
				source->state =
					result ? XBEE_FW_STATE_FAILURE : XBEE_FW_STATE_XMODEM_SEND;
				return result;
			}
			break;

		case XBEE_FW_STATE_XMODEM_SEND:
			result = xbee_xmodem_tx_tick( &source->u.xbxm);
			if (result == 1)
			{
	         #ifdef XBEE_FIRMWARE_VERBOSE
	            printf( "%s: Xmodem send complete\n", __FUNCTION__);
	         #endif

				source->state = XBEE_FW_STATE_XMODEM_DONE;
				source->timer = xbee_millisecond_timer();
				return 0;
			}
			else if (result)
			{
				// error during xmodem, abort transfer
				source->state = XBEE_FW_STATE_FAILURE;
			}
			return result;

		case XBEE_FW_STATE_XMODEM_DONE:
			if (! _TIME_ELAPSED( 3000))
			{
				return 0;
			}
			xbee_ser_putchar( serport, '\r');
			source->timer = xbee_millisecond_timer();
			source->state = XBEE_FW_STATE_FINAL_PROMPT;
			// fall through to STATE_FINAL_PROMPT

		case XBEE_FW_STATE_FINAL_PROMPT:
			if (xbee_ser_getchar( serport) != '>')
			{
				break;
			}
         #ifdef XBEE_FIRMWARE_VERBOSE
            printf( "%s: running new firmware\n", __FUNCTION__);
         #endif
         // do we need to make sure it's running?
			xbee_ser_putchar( serport, '2');
         source->timer = xbee_millisecond_timer();
			source->state = XBEE_FW_STATE_SUCCESS;
			// fall through to STATE_DONE

		case XBEE_FW_STATE_SUCCESS:
			return 1;

		case XBEE_FW_STATE_FAILURE:
			return -EIO;

		default:
         #ifdef XBEE_FIRMWARE_VERBOSE
            printf( "%s: invalid state %d\n", __FUNCTION__, source->state);
         #endif
			return -EBADMSG;
	}

	// any state that hasn't already returned uses this shared timeout feature
	if (_TIME_ELAPSED( XBEE_FW_LOAD_TIMEOUT))
	{
		source->tries += 1;

      #ifdef XBEE_FIRMWARE_VERBOSE
         printf( "%s: timeout #%d\n", __FUNCTION__, source->tries);
      #endif

		if (source->tries < 3)
		{
			// try again
			source->state = XBEE_FW_STATE_RESET;
			source->timer = xbee_millisecond_timer();
		}
		else
		{
      	return -ETIMEDOUT;
      }
	}

	return 0;
}
#undef _TIME_ELAPSED

/*** BeginHeader xbee_fw_status_ebl */
/*** EndHeader */
/**
	@brief
			Update \a buffer with the current install status of \a source.
			Note that this string will only change if the return value of
			xbee_fw_install_ebl_state() has changed.

	@param[in]	source	State variable to generate status string for.

	@param[out]	buffer	Buffer (at least 80 bytes) to receive dynamic status
								string.

	@return	Returns parameter 2 or a pointer to a fixed status string.

	@sa xbee_fw_install_ebl_state
*/
_xbee_firmware_debug
char FAR *xbee_fw_status_ebl( xbee_fw_source_t *source, char FAR *buffer)
{
	if (! source || ! source->xbee)
	{
		return "Invalid source.";
	}

	switch (source->state)
	{
		case XBEE_FW_STATE_SUCCESS:
		case XBEE_FW_STATE_DONE:
			return "Install successful.";

		case XBEE_FW_STATE_FAILURE:
			return "Install failed.";

		case XBEE_FW_STATE_INIT:
		case XBEE_FW_STATE_RESET:
			return "Starting update.";

		case XBEE_FW_STATE_BREAK:
			return "Generating serial break on transmit pin.";

		case XBEE_FW_STATE_PROMPT:
			return "Waiting for response from bootloader.";

		case XBEE_FW_STATE_XMODEM_WAIT:
			return "Waiting to start XMODEM send.";

		case XBEE_FW_STATE_XMODEM_SEND:
			switch (source->u.xbxm.state)
			{
				case XBEE_XMODEM_STATE_FLUSH:
				case XBEE_XMODEM_STATE_START:
					return "XMODEM: Starting transfer...";

				case XBEE_XMODEM_STATE_SEND:
				case XBEE_XMODEM_STATE_RESEND:
				case XBEE_XMODEM_STATE_SENDING:
					sprintf( buffer, "XMODEM Packet %" PRIu16 ": data.",
						source->u.xbxm.packet_num);
					return buffer;

				case XBEE_XMODEM_STATE_WAIT_ACK:
					sprintf( buffer, "XMODEM Packet %" PRIu16 ": data, checksum, ACK.",
						source->u.xbxm.packet_num);
					return buffer;

				case XBEE_XMODEM_STATE_EOF:
				case XBEE_XMODEM_STATE_FINAL_ACK:
					return "XMODEM: done sending, wait for final ACK.";

				case XBEE_XMODEM_STATE_SUCCESS:
					return "XMODEM: transfer successful.";

				case XBEE_XMODEM_STATE_FAILURE:
					return "XMODEM: transfer failed.";
			}
			sprintf( buffer, "XMODEM: unknown state %d.", source->u.xbxm.state);
			return buffer;

		case XBEE_FW_STATE_XMODEM_DONE:
		case XBEE_FW_STATE_FINAL_PROMPT:
			return "Update complete, rebooting into new firmware.";

	}

	sprintf( buffer, "Invalid state %d.", source->state);
	return buffer;
}

/*** BeginHeader xbee_fw_install_oem_tick */
/*** EndHeader */
/////////////////
// .OEM Firmware Files

// Multi-byte values are stored MSB first (big-endian) in the file.

// Summing all bytes in 16-bit chunks should result in 0x0000.  The two-byte
// checksum at offset 2 provides the necessary offset to get all bytes in the
// file to add up to 0x0000.

// Firmware files start with 0x93 0xFD
#define XBEE_OEM_MAGIC_NUMBER				0x93FD

#define XBEE_OEM_OFS_MAGIC_NUMBER		0		// 0x93 0xFD (XBEE_OEM_MAGIC_NUMBER)
#define XBEE_OEM_OFS_CHECKSUM				2		// 16-bit value
#define XBEE_OEM_OFS_IMAGE_LEN			4		// 32-bit value
#define XBEE_OEM_OFS_HEADER_LEN			8		// 16-bit value
#define XBEE_OEM_OFS_MODULE_ID			10		// 8-bit value, must match upper
															// byte of XBee's ATHV response
#define XBEE_OEM_OFS_SW_COMPAT_LEVEL	11		// 8-bit value, must match XBee's
															// AT%C response
#define XBEE_OEM_OFS_FW_VERSION			12		// variable-length string?  Must
															// match XBee's ATVR response.
															// If ATVR=8042, OEM file contains
															// 0x38 0x30 0x34 0x32.

typedef PACKED_STRUCT _xbee_oem_header
{
	uint16_t			magic;					// 0x93 0xFD (XBEE_OEM_MAGIC_NUMBER)
	uint16_t			checksum;
	uint32_t			image_len;				// bytes in file, including header
	uint16_t			header_len;				// offset to first block
	uint8_t			module_id;				// must match upper byte of XBee's
													// HV response
	uint8_t			software_id;			// must match XBee's AT%C response
	// header is followed by multi-byte version string (usually 4 bytes?)
} xbee_oem_header_t;

/*
	char				version_str[4];		// must match ATVR response
													// If ATVR=8042, OEM file contains
													// 0x38 0x30 0x34 0x32.
*/

// Don't change these constants.
#define XBEE_BOOTLOADER_ACK	'U'		// for all bootloaders.
#define XBEE_BOOTLOADER_NAK	0x11		// for XStream

#define XBEE_OEM_FLAG_NONE				0x0000
#define XBEE_OEM_FLAG_FORCEUPDATE	0x0001

// ----------------------------------------------------------------------------
// These routines are for accessing the update image.
// ----------------------------------------------------------------------------

/// @todo check return value of source->read here and in _uint16 and _uint32?
_xbee_firmware_debug
int xbee_fw_read_byte( xbee_fw_source_t *source)
{
	uint8_t b = 0;

	source->read( source->context, &b, 1);

	return b;
}

_xbee_firmware_debug
uint16_t xbee_fw_read_uint16( xbee_fw_source_t *source)
{
	uint16_t temp = 0;

	source->read( source->context, &temp, 2);

	return be16toh( temp);
}

_xbee_firmware_debug
uint32_t xbee_fw_read_uint32( xbee_fw_source_t *source)
{
	uint32_t temp = 0;

	source->read( source->context, &temp, 4);

	return be32toh( temp);
}

#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
// This routine assumes valid hex only in upper case.
_xbee_firmware_debug
uint16_t xbee_fw_hex2word(const char *pString)
{
	uint16_t value;
	int ch;
	int i;

	value = 0;
	for (i = 0; (i < 4) && ((ch = *pString) >= '0'); ++pString, ++i)
	{
		value = (value << 4) | ( ch - (ch >= 'A' ? 'A' - 10 : '0') );
	}
	return value;
}
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

/** @internal
					Validate the firmware image stored in \p source.  Checks the
					header, length and checksum.

	@param[in]	source	Firmware source initialized with \c seek and \c read
								functions.

	@retval	0			Source contains a valid firmware image.
	@retval	-EILSEQ	Source does not contain a valid firmware image.
	@retval	-EIO		I/O error reading from firmware image
	@retval	-EBADMSG	Firmware checksum failed, image is bad.
*/
// DEVNOTE: improve this function by walking the firmware block-by-block and
// confirming that all blocksizes are valid (and that the file is therefore
// properly formatted).
_xbee_firmware_debug
int _xbee_oem_verify( xbee_fw_source_t *source)
{
	char 						buffer[256];
	xbee_oem_header_t		oem_header;
	int i, bytes;
	uint16_t Checksum, *w;
	uint32_t Length;

	if (source->seek( source->context, 0) != 0
		|| source->read( source->context, &oem_header, sizeof oem_header)
			!= sizeof oem_header)
	{
		return -EIO;
	}

	Checksum = be16toh( oem_header.magic);
	if (XBEE_OEM_MAGIC_NUMBER != Checksum)
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: firmware doesn't start with magic number\n",
				__FUNCTION__);
		#endif
		return -EILSEQ;
	}
	Checksum += be16toh( oem_header.checksum);

	Length = be32toh( oem_header.image_len);
	Checksum += (uint16_t)(Length>>16);
	Checksum += (uint16_t)Length;

	Checksum += be16toh( oem_header.header_len);
	Checksum += (oem_header.module_id << UINT16_C(8)) | oem_header.software_id;

	// Length is always even and between 32 and 128KB
	// XTend max is 128K, XBee max is 64K, and XStream max is 32K.

	if (32 > Length || 128 * UINT32_C(1024) < Length || Length & 1 )
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: firmware header indicates invalid length (%" PRIu32 ")\n",
				__FUNCTION__, Length);
		#endif
		return -EILSEQ;
	}

	// Length includes header that we've already read and checksummed, so remove
	// those bytes from the count to read and checksum.
	Length -= sizeof(oem_header);
	while (Length)
	{
		bytes = sizeof(buffer);
		if (bytes > Length)
		{
			bytes = (int) Length;
		}
		source->read( source->context, buffer, bytes);
		for (w = (uint16_t *)buffer, i = bytes >> 1; i; ++w, --i)
		{
			Checksum += be16toh( *w);
		}
		Length -= bytes;
	}
	if (Checksum)
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: firmware checksum bad\n", __FUNCTION__);
		#endif
		return -EBADMSG;
	}

   #ifdef XBEE_FIRMWARE_VERBOSE
      printf( "%s: firmware verified\n", __FUNCTION__);
   #endif
	return 0;
}

_xbee_firmware_debug
int _xbee_fw_send_request( xbee_fw_source_t *source, const FAR char *request,
	int next_state)
{
	#ifdef XBEE_FIRMWARE_VERBOSE
		printf( "%s: sending AT%" PRIsFAR "\n", __FUNCTION__, request);
	#endif
	source->buffer[0] = '\0';
	xbee_atmode_send_request( source->xbee, request);
	source->state = XBEE_FW_STATE_PARSE_RESPONSE;
	source->next_state = next_state;

	return 0;
}

#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DISABLE C5909		// Assignment in condition is OK
#endif
#define _TIME_ELAPSED(x)  (xbee_millisecond_timer() - source->timer > (x))
/**
	@brief
	Install the firmware image stored in \a source.

	You must call xbee_fw_install_init() on the source before calling this tick
	function.

	If successful, XBee will be running the new firmware at 115,200 baud.

	@param[in]	source	Firmware source initialized with seek and read
								functions.

	@retval	1			Successfully installed new firmware.
	@retval	0			Firmware installation in progress (incomplete).
	@retval	-EILSEQ	\a firmware does not contain a valid firmware image.
	@retval	-EBADMSG	Firmware checksum failed, image is bad.
	@retval	-EIO		Couldn't establish communications with XBee module.
	@retval	-ENOTSUP	Firmware is not compatible with this hardware.

*/
_xbee_firmware_debug
int xbee_fw_install_oem_tick( xbee_fw_source_t *source)
{
	uint16_t flags;
	xbee_dev_t	*xbee;
	xbee_serial_t	*serport;
	int err;
	char response[10];
	char *p;
	int ch, i, retval;
	int bytes;
	uint16_t hv, tempword;

	// temporary...
	flags = XBEE_OEM_FLAG_FORCEUPDATE;

	xbee = source->xbee;
	serport = &xbee->serport;

	// break out of switch to return default of 0 (in progress)
	switch (source->state)
	{
		case XBEE_FW_STATE_SUCCESS:
			return 1;
		case XBEE_FW_STATE_FAILURE:
			return -EIO;

		case XBEE_FW_STATE_INIT:
			// Check the firmware image for validity, leaves the library set up
			// for reading from this image.
		   #ifdef XBEE_FIRMWARE_VERBOSE
		      printf( "%s: verify firmware image\n", __FUNCTION__);
		   #endif
			if ( (err = _xbee_oem_verify( source)) )
			{
				source->state = XBEE_FW_STATE_FAILURE;
				return err;
			}
			source->state = XBEE_FW_STATE_RESET;
		   source->tries = 0;
			break;

		case XBEE_FW_STATE_RESET:
			xbee_ser_open( serport, source->tries ? 9600 : 115200);
			xbee_ser_flowcontrol( serport, 1);

		   // Put the radio in command mode.
		   #ifdef XBEE_FIRMWARE_VERBOSE
		      printf( "%s: enter command mode at %skbps\n", __FUNCTION__,
		      												source->tries ? "9.6" : "115");
		   #endif
		   xbee_atmode_enter( xbee);
		   source->state = XBEE_FW_STATE_CMD_PENDING;
			break;

		case XBEE_FW_STATE_CMD_PENDING:		// waiting to complete command mode
			switch (xbee_atmode_tick( xbee))
			{
				case XBEE_MODE_COMMAND:
					// successfully entered command mode
					_xbee_fw_send_request( source, "HV", XBEE_FW_STATE_CMD_HV);
					break;

				case XBEE_MODE_IDLE:
					// failed to enter command mode, try again at 9600?
					if (! source->tries)
					{
					   #ifdef XBEE_FIRMWARE_VERBOSE
					      printf( "%s: trying again at default baud\n",
					      	__FUNCTION__);
					   #endif
					   source->tries++;
						source->state = XBEE_FW_STATE_RESET;
						break;
					}
				   #ifdef XBEE_FIRMWARE_VERBOSE
				      printf( "%s: failed to enter command mode\n",
				      	__FUNCTION__);
				   #endif
				   source->state = XBEE_FW_STATE_FAILURE;
					return -EIO;
			}
			break;

		case XBEE_FW_STATE_PARSE_RESPONSE:
			retval = xbee_atmode_read_response( xbee, source->buffer,
				sizeof(source->buffer), NULL);
			if (retval == -EAGAIN)
			{
				break;
			}
			else if (retval < 0)
			{
				#ifdef XBEE_FIRMWARE_VERBOSE
					printf( "%s: error %d waiting for response\n", __FUNCTION__,
						retval);
				#endif
				source->state = XBEE_FW_STATE_FAILURE;
				return -EIO;
			}
			#ifdef XBEE_FIRMWARE_VERBOSE
				printf( "%s: response is [%s]\n", __FUNCTION__, source->buffer);
			#endif
			source->state = source->next_state;
			break;

		case XBEE_FW_STATE_CMD_HV:
		   hv = xbee_fw_hex2word( source->buffer);
		   source->seek( source->context, XBEE_OEM_OFS_MODULE_ID);
		   if ((hv >> 8) != xbee_fw_read_byte( source))
		   {
		      return -ENOTSUP;
		   }
			_xbee_fw_send_request( source, "%C", XBEE_FW_STATE_CMD_COMPAT);
			break;

		case XBEE_FW_STATE_CMD_COMPAT:
		   // If the radio does not support this command, then can't check it.
		   // XBee and XTend support it, and XStream does not.
		   if (strcmp( source->buffer, "ERROR"))
		   {  // response to AT%C is not "ERROR"
		      source->seek( source->context, XBEE_OEM_OFS_SW_COMPAT_LEVEL);
		      if (xbee_fw_hex2word( source->buffer) != xbee_fw_read_byte( source))
		      {
		         return -ENOTSUP;
		      }
		   }
			_xbee_fw_send_request( source, "VR", XBEE_FW_STATE_CMD_VR);
			break;

		case XBEE_FW_STATE_CMD_VR:
		   // Now, compare the version.
		   source->seek( source->context, XBEE_OEM_OFS_FW_VERSION);
		   for (i = 0; i < 4; ++i)
		   {
		      if (xbee_fw_read_byte( source) != source->buffer[i])
		      {
		         // It does not match. Needs update.
		         flags |= XBEE_OEM_FLAG_FORCEUPDATE;
		         break;
		      }
		   }

		   // If we don't need an update, return the radio to operational mode.
		   if (! flags & XBEE_OEM_FLAG_FORCEUPDATE)
		   {
	         // exit command mode
	         xbee_atmode_send_request( xbee, "CN");

				source->state = XBEE_FW_STATE_DONE;
				break;
		   }

		   // We're going to update the firmware.
		   // Tell the radio to return the serial number in hex.

		   // This command is only needed for XTend, and will get an ERROR on
		   // XStream and XBee.  We ignore the response.
			_xbee_fw_send_request( source, "CF1", XBEE_FW_STATE_CMD_CF1);
			break;

		case XBEE_FW_STATE_CMD_CF1:
		   // Now get the serial number of the unit so we can force it into the
		   // bootloader.  This is needed for XTend and XStream, but not XBee.
			_xbee_fw_send_request( source, "SL", XBEE_FW_STATE_CMD_SL);
			break;

		case XBEE_FW_STATE_CMD_SL:
		   // Munge the serial number and enter programming mode via AT command.
		   sprintf( response, "%%P%04" PRIX16,
		      (xbee_fw_hex2word( source->buffer) + 0xDB8A) & 0x3fff);
			_xbee_fw_send_request( source, response, XBEE_FW_STATE_CMD_PROG);
			break;

		case XBEE_FW_STATE_CMD_PROG:
			if (strcmp( source->buffer, "OK"))
			{	// Response to AT%Pxxxx was not "OK", so trigger a firmware
		      // reset (ATFR) and use RTS/serial break to enter bootloader.
				_xbee_fw_send_request( source, "FR", XBEE_FW_STATE_CMD_FR);
			}
			else
			{
				// response to AT%Pxxxx was OK, so we're in the bootloader
				source->state = XBEE_FW_STATE_BOOTLOADER;
			}
			break;

		case XBEE_FW_STATE_CMD_FR:
			// ignore response to ATFR
	      // de-assert RTS and go into serial break to reset board
	      xbee_ser_set_rts( serport, 0);
	      xbee_ser_break( serport, 1);
	      source->timer = xbee_millisecond_timer();
	      source->state = XBEE_FW_STATE_BREAK;
			break;

		case XBEE_FW_STATE_BREAK:		// 400ms break and then we're in bootloader
			if (! _TIME_ELAPSED( 400))
			{
				break;
			}
			// fall through to BOOTLOADER state

		case XBEE_FW_STATE_BOOTLOADER:		// We should be in the boot loader.
		   // boot loader runs at 38400
			xbee_ser_open( serport, 38400);

			// Verify that we're in the bootloader: send '+' and look for ACK/NAK
			xbee_ser_putchar( serport, '+');

			source->state = XBEE_FW_STATE_TX_START;
			// fall through to TX_START state

		case XBEE_FW_STATE_TX_START:
			ch = xbee_ser_getchar( serport);
			if ((ch == -ENODATA) && ! _TIME_ELAPSED( 1000))
			{
				break;
			}
			#ifdef XBEE_FIRMWARE_VERBOSE
				printf( "%s: bootloader response is 0x%02x\n", __FUNCTION__, ch);
			#endif
			if (ch != XBEE_BOOTLOADER_ACK && ch != XBEE_BOOTLOADER_NAK)
			{
				source->state = XBEE_FW_STATE_FAILURE;
				return -EIO;
			}

			// Flash the blocks.
			source->tries = 0;

			#if XBEE_OEM_OFS_HEADER_LEN != (XBEE_OEM_OFS_IMAGE_LEN+4)
				#fatal "Update function, header_len no longer follows image_len."
			#endif
			source->seek( source->context, XBEE_OEM_OFS_IMAGE_LEN);
			source->u.oem.firmware_length = xbee_fw_read_uint32( source);
			source->u.oem.block_offset = xbee_fw_read_uint16( source);

			source->state = XBEE_FW_STATE_TX_BLOCK;
			break;

		case XBEE_FW_STATE_TX_BLOCK:
			source->seek( source->context, source->u.oem.block_offset);
			tempword = xbee_fw_read_uint16( source);

	      // !!! avoid overflow by validating the block size.  Should fit within
	      // the remaining bytes of the file.

			// End of firmware image?
			if (! tempword)
			{
		      #ifdef XBEE_FIRMWARE_VERBOSE
		         printf( "%s: firmware update complete, resetting radio\n",
		         	__FUNCTION__);
		      #endif

				// toggle reset line
				xbee_dev_reset( xbee);

				// switch back to 9600 baud, default baud rate for new firmware
		      xbee_ser_open( serport, 9600);
		      xbee_ser_flowcontrol( serport, 1);

				source->timer = xbee_millisecond_timer();
				source->state = XBEE_FW_STATE_FINAL_PROMPT;
				break;
			}

			source->u.oem.block_length = tempword;
			source->u.oem.cur_offset = source->u.oem.block_offset + 2 + tempword;

		   #ifdef XBEE_FIRMWARE_VERBOSE
		      printf( "%s: sending %" PRIu16 "-byte block @ offset %" PRIu32 "\n",
		      	__FUNCTION__, tempword, source->u.oem.block_offset);
		   #endif

			source->state = XBEE_FW_STATE_SENDING;
			break;

		case XBEE_FW_STATE_SENDING:
			bytes = xbee_ser_tx_free( serport);
			if (bytes > 0)
			{
				if (bytes > 128)
				{
					bytes = 128;
				}
				if (bytes > source->u.oem.block_length)
				{
					bytes = source->u.oem.block_length;
				}
			   #ifdef XBEE_FIRMWARE_VERBOSE
			      printf( "%s: sending %" PRIu16 " bytes\n", __FUNCTION__, bytes);
			   #endif
				// !!! need error checking here on both the read and the write!
				source->read( source->context, source->buffer, bytes);
				xbee_ser_write( serport, source->buffer, bytes);
				source->u.oem.block_length -= bytes;
			}
			if (source->u.oem.block_length)			// more bytes to send
			{
				break;
			}

			// read the block of bytes that should match the modem's response
			source->u.oem.block_length = tempword = xbee_fw_read_uint16( source);
			source->u.oem.cur_offset += 2 + tempword;
	      #ifdef XBEE_FIRMWARE_VERBOSE
	         printf( "%s: expecting %" PRIu16 "-byte response\n", __FUNCTION__,
	         	tempword);
	      #endif
	      // !!! avoid overflow by validating the block size.  Should fit within
	      // the remaining bytes of the file.
			source->timer = xbee_millisecond_timer();
			source->state = XBEE_FW_STATE_RX_BLOCK;
			break;

		case XBEE_FW_STATE_RX_BLOCK:
			bytes = source->u.oem.block_length;
			if (bytes > 128)
			{
				bytes = 128;
			}
			p = source->buffer;
			bytes = xbee_ser_read( serport, p, bytes);
			if (bytes > 0)
			{
				for (i = 0; i < bytes; ++i, ++p)
				{
					if (*p != xbee_fw_read_byte( source))
					{
						source->state = XBEE_FW_STATE_RX_FAIL;
						return 0;
					}
				}

				// successfully received <bytes> bytes of response
				source->u.oem.block_length -= bytes;
				if (source->u.oem.block_length)			// more to receive
				{
					break;
				}
				// response was good, send the next block
		      #ifdef XBEE_FIRMWARE_VERBOSE
		         printf( "%s: response complete\n", __FUNCTION__);
		      #endif
				source->tries = 0;
				source->u.oem.block_offset = source->u.oem.cur_offset;
				source->state = XBEE_FW_STATE_TX_BLOCK;
				break;
			}
			// wait up to 4 seconds to receive the response block
			if (! _TIME_ELAPSED( 4000))
			{
				break;
			}
			// fall through (receive timeout failure)

		case XBEE_FW_STATE_RX_FAIL:
			// timeout on receive or characters didn't match, resend
			if (++source->tries <= 3)
			{
	         #ifdef XBEE_FIRMWARE_VERBOSE
	            printf( "%s: invalid response, resend block\n", __FUNCTION__);
	         #endif
				source->state = XBEE_FW_STATE_TX_BLOCK;
				break;
			}

			// Already tried the block too many times
         #ifdef XBEE_FIRMWARE_VERBOSE
            printf( "%s: too many invalid responses, aborting transfer\n",
            	__FUNCTION__);
         #endif
			source->state = XBEE_FW_STATE_FAILURE;
			return -EIO;

		case XBEE_FW_STATE_FINAL_PROMPT:
			// wait for reboot to finish
			if (! _TIME_ELAPSED( 3000))
			{
				break;
			}
			// fall through to DONE

		case XBEE_FW_STATE_DONE:
			source->state = XBEE_FW_STATE_SUCCESS;
         return 1;

		default:
         #ifdef XBEE_FIRMWARE_VERBOSE
            printf( "%s: invalid state %d\n", __FUNCTION__, source->state);
         #endif
         source->state = XBEE_FW_STATE_FAILURE;
         return -EBADMSG;

	}	// end of big switch

	return 0;
}
#undef _TIME_ELAPSED
#ifdef __XBEE_PLATFORM_HCS08
	#pragma MESSAGE DEFAULT C5909		// restore C5909 (Assignment in condition)
#endif

/*** BeginHeader xbee_fw_status_oem */
/*** EndHeader */
/**
	@brief
	Update \a buffer with the current install status of \a source.

	@param[in]	source	State variable to generate status string for.

	@param[out]	buffer	Buffer (at least 80 bytes) to receive dynamic status
								string.

	@return		Returns parameter 2 or a pointer to a fixed status string.

*/
_xbee_firmware_debug
char FAR *xbee_fw_status_oem( xbee_fw_source_t *source, char FAR *buffer)
{
	if (! source || ! source->xbee)
	{
		return "Invalid source.";
	}

	switch (source->state)
	{
		case XBEE_FW_STATE_SUCCESS:
		case XBEE_FW_STATE_DONE:
			return "Install successful.";

		case XBEE_FW_STATE_FAILURE:
			return "Install failed.";

		case XBEE_FW_STATE_INIT:
			return "Starting update.";

		case XBEE_FW_STATE_RESET:
			return "Verified firmware image.";

		case XBEE_FW_STATE_CMD_PENDING:
			sprintf( buffer, "Trying to enter command mode at %skbps.",
				source->tries ? "9.6" : "115");
			return buffer;

		case XBEE_FW_STATE_PARSE_RESPONSE:
			return "Waiting for response to AT command.";

		case XBEE_FW_STATE_CMD_HV:
			sprintf( buffer, "Received response to AT%s command.", "HV");
			return buffer;

		case XBEE_FW_STATE_CMD_COMPAT:
			sprintf( buffer, "Received response to AT%s command.", "%C");
			return buffer;

		case XBEE_FW_STATE_CMD_VR:
			sprintf( buffer, "Received response to AT%s command.", "VR");
			return buffer;

		case XBEE_FW_STATE_CMD_CF1:
			sprintf( buffer, "Received response to AT%s command.", "CF1");
			return buffer;

		case XBEE_FW_STATE_CMD_SL:
			sprintf( buffer, "Received response to AT%s command.", "SL");
			return buffer;

		case XBEE_FW_STATE_CMD_PROG:
			sprintf( buffer, "Received response to AT%s command.", "%Pxxxx");
			return buffer;

		case XBEE_FW_STATE_CMD_FR:
			sprintf( buffer, "Received response to AT%s command.", "FR");
			return buffer;

		case XBEE_FW_STATE_BREAK:
			return "Generating serial break on transmit pin.";

		case XBEE_FW_STATE_BOOTLOADER:
		case XBEE_FW_STATE_TX_START:
			return "Waiting for response from bootloader.";

		case XBEE_FW_STATE_TX_BLOCK:
		case XBEE_FW_STATE_SENDING:
		case XBEE_FW_STATE_RX_BLOCK:
		case XBEE_FW_STATE_RX_FAIL:
			sprintf( buffer, "Sending firmware (%" PRIu32 "/%" PRIu32 ").",
				source->u.oem.block_offset, source->u.oem.firmware_length);
			return buffer;

		case XBEE_FW_STATE_FINAL_PROMPT:
			return "Update complete, rebooting into new firmware.";
	}

	sprintf( buffer, "Invalid state %d.", source->state);
	return buffer;
}

/*** BeginHeader xbee_fw_buffer_init */
/*** EndHeader */
/**	@internal
	@brief
	Helper function used when the source firmware image is entirely held
	in a buffer.

	@param[in]	context	address of xbee_fw_buffer_t object
	@param[in]	offset	offset to new position

	@retval	-EINVAL	offset greater than length of file
	@retval	0			seek successful
*/
_xbee_firmware_debug
int _xbee_fw_buffer_seek( void FAR *context, uint32_t offset)
{
	xbee_fw_buffer_t FAR *fw;

	fw = (xbee_fw_buffer_t FAR *) context;

	if (offset > fw->length)
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: return -EINVAL; attempting to seek past end of buffer\n",
				__FUNCTION__);
		#endif
		return -EINVAL;
	}

   #ifdef XBEE_FIRMWARE_VERBOSE
      printf( "%s: seeked to offset %" PRIu32 "\n", __FUNCTION__, offset);
   #endif

   fw->offset = offset;
   return 0;
}

/**	@internal
	@brief
	Helper function used when the source firmware image is entirely held
	in a buffer.

	@param[in]	context	address of xbee_fw_buffer_t object
	@param[out]	buffer	destination buffer to read bytes into
	@param[in]	bytes		number of bytes to read

	@retval	-ENODATA		no more bytes available to read
	@retval	0-bytes		number of bytes read
*/
_xbee_firmware_debug
int _xbee_fw_buffer_read( void FAR *context, void FAR *buffer, int16_t bytes)
{
   uint32_t file_offset;
	uint32_t bytes_left;
	xbee_fw_buffer_t FAR *fw;

	fw = (xbee_fw_buffer_t FAR *) context;

   file_offset = fw->offset;
   bytes_left = fw->length - file_offset;
	if (! bytes_left)
	{
		#ifdef XBEE_FIRMWARE_VERBOSE
			printf( "%s: return -ENODATA; buffer empty\n", __FUNCTION__);
		#endif
		return -ENODATA;
	}

   if (bytes > bytes_left)
   {
   	bytes = (int) bytes_left;
   }

	_f_memcpy( buffer, &fw->address[file_offset], bytes);
   fw->offset += bytes;

   #ifdef XBEE_FIRMWARE_VERBOSE
      printf( "%s: read %u bytes\n", __FUNCTION__, bytes);
   #endif

	return bytes;
}

/**
	@brief
	Helper function for setting up an xbee_fw_buffer_t for use with a source
	firmware image held entirely in a buffer.

	@param[out]	fw		structure to configure for reading firmware from a buffer
	@param[in]	length	number of bytes in firmware image
	@param[in]	address	address of buffer containing firmware

	@retval	0 			success
	@retval	-EINVAL	invalid parameter
*/
_xbee_firmware_debug
int xbee_fw_buffer_init( xbee_fw_buffer_t *fw, uint32_t length,
	const char FAR *address)
{
	if (fw == NULL || length == 0 || address == NULL)
	{
		return -EINVAL;
	}

	fw->length = length;
	fw->address = address;
	fw->offset = 0;

	fw->source.context = (void FAR *) fw;
	fw->source.seek = &_xbee_fw_buffer_seek;
	fw->source.read = &_xbee_fw_buffer_read;

	return 0;
}

