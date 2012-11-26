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
/*
	For now, use on BL4S150.

	Test firmware update for XBee modules that use .OEM files.

	*** For this sample to work, you must add XBEE_DEVICE_ENABLE_ATMODE to
		your Project Defines.
*/

#if 0
	#define XBEE_DEVICE_VERBOSE
	#define XBEE_SERIAL_VERBOSE
	#define XBEE_FIRMWARE_VERBOSE
	#define XBEE_ATMODE_VERBOSE
#endif
#if 0
	#define XBEE_DEVICE_DEBUG
	#define XBEE_SERIAL_DEBUG
	#define XBEE_FIRMWARE_DEBUG
	#define XBEE_ATMODE_DEBUG
#endif

#include "xbee/device.h"
#include "xbee/firmware.h"

// Load configuration settings for board's serial port.
#include "xbee_config.h"

/*
	Sample code to read firmware from ximported .oem file, used to demonstrate
	new non-blocking firmware update API.

	This sample would be a good starting point for an update tool that
	downloads firmware from a website, or reads it from a FAT file.
*/

// Import a .OEM file created with X-CTU and MaxStreamOEMUpdate.dll.
#ximport "/Program Files/Digi International/XCTU/OEM/XBP09-DM_8044.oem" FIRMWARE

#define ximport_length(xaddr)		( *(const long far *) (xaddr) )
#define ximport_data(xaddr)		( (const char far *) ((xaddr) + 4) )

const char *settings[] = {
	"BD 7",		// 115,200 bps
	"D6 1",		// enable RTS on pin D6
	"AP 1",		// enable API mode
	"WR",			// write settings to NV memory
	"CN",			// exit command mode
};

int configure_modem( xbee_dev_t *xbee)
{
	char response[10];
	int bytesread;
	int i, mode, retval;

	// make sure we're at 9600 baud, default speed after firmware update
	xbee_ser_open( &xbee->serport, 9600);

	xbee_atmode_enter( xbee);
	do
	{
		mode = xbee_atmode_tick( xbee);
	} while ((mode != XBEE_MODE_COMMAND) && (mode != XBEE_MODE_IDLE));

	if (mode != XBEE_MODE_COMMAND)
	{
		// failed to enter command mode
		printf( "failed to enter command mode after firmware update\n");
		return -EIO;
	}

   // Configure XBee module for our library
   for (i = 0; i < sizeof(settings)/sizeof(*settings); ++i)
   {
		printf( "Setting AT%s\n", settings[i]);
		xbee_atmode_send_request( xbee, settings[i]);
		bytesread = 0;
		do {
			retval = xbee_atmode_read_response(
									xbee, response, sizeof(response), &bytesread);
		} while (retval == -EAGAIN);
		printf( "Received response: [%s], retval = %d\n", response, retval);
		if (retval < 0)
		{
			return -EIO;
		}
	}

	// switch to 115200 bps (to match "BD 7" setting we just made)
	xbee_ser_open( &xbee->serport, 115200);

	return 0;
}

xbee_dev_t my_xbee;

void main()
{
	xbee_fw_buffer_t fw;
	char buffer[80];
	unsigned long t;
	int i, result, last_state;

	xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin);

	// Use helper API for reading firmware from a buffer (or ximported file).
	xbee_fw_buffer_init( &fw, ximport_length( FIRMWARE),
																ximport_data( FIRMWARE));

	xbee_fw_install_init( &my_xbee, NULL, &fw.source);
	do
	{
		t = MS_TIMER;
		last_state = fw.source.state;
		result = xbee_fw_install_oem_tick( &fw.source);
		t = MS_TIMER - t;
		if (t > 50)
		{
			printf( "!!! tick blocked for %lums in state %u (now state %u)\n",
				t, last_state, fw.source.state);
		}
		// clear to end of line and print new status
		printf( "\x1B[K" " %ls\r", xbee_fw_status_oem( &fw.source, buffer));
	} while (result == 0);

	if (result == 1)
	{
		printf( "firmware update completed successfully\n");
		configure_modem( &my_xbee);
	}
	else
	{
		printf( "firmware update failed with error %d\n", result);
		i = xbee_ser_read( &my_xbee.serport, buffer, 80);
		if (i > 0)
		{
			printf( "extra bytes in buffer: [%.*s]\n", i, buffer);
		}
	}

	xbee_ser_close( &my_xbee.serport);
}

