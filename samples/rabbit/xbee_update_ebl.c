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
	xbee_update_ebl.c

	Test firmware update for XBee modules that use .EBL files.

	Sample code to read firmware from ximported .ebl file, used to demonstrate
	new non-blocking firmware update API.

	This sample would be a good starting point for an update tool that
	downloads firmware from a website, or reads it from a FAT file.

	Note that the S2 doesn't actually need a seek function.  Maybe that should
	just be removed (set to NULL)?  We may need it in the future, so leave it in?
	There are benefits to not having it though -- can install updates straight
	from an HTTP download.
*/

#include "xbee/atcmd.h"
#include "xbee/device.h"
#include "xbee/firmware.h"

// Load configuration settings for board's serial port.
#include "xbee_config.h"

// Choose path to an EBL file here:
#ximport "/Program Files/Digi International/XCTU/update/ebl_files/"		\
			"XB24-ZB_2370.ebl" XBEE_FW_EBL

#define ximport_length(xaddr)		( *(const long far *) (xaddr) )
#define ximport_data(xaddr)		( (const char far *) ((xaddr) + 4) )

xbee_dev_t my_xbee;

int main()
{
	xbee_fw_buffer_t fw;
	char buffer[80];
	uint32_t t;
	uint16_t timeout;
	int i, result, last_state;
	int status;

	xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin);

	// Use helper API for reading firmware from a buffer (or ximported file).
	xbee_fw_buffer_init( &fw, ximport_length( XBEE_FW_EBL),
																ximport_data( XBEE_FW_EBL));

	printf( "sending %" PRIu32 " bytes from 0x%" PRIx32 "\n",
		ximport_length( XBEE_FW_EBL), ximport_data( XBEE_FW_EBL));

	xbee_fw_install_init( &my_xbee, NULL, &fw.source);
	do
	{
		t = xbee_millisecond_timer();
		last_state = fw.source.state;
		result = xbee_fw_install_ebl_tick( &fw.source);
		t = xbee_millisecond_timer() - t;
		if (t > 50)
		{
			printf( "!!! tick blocked for %" PRIu32 "ms in state %u "
				"(now state %u)\n", t, last_state, fw.source.state);
		}
		// clear to end of line and print new status
		printf( "\x1B[K" " %" PRIsFAR "\r",
			xbee_fw_status_ebl( &fw.source, buffer));
	} while (result == 0);

	if (result != 1)
	{
		printf( "firmware update failed with error %d\n", result);
		xbee_ser_close( &my_xbee.serport);

		return result;
	}

	printf( "firmware update completed successfully\n");

	// wait 2000 ms before trying to talk to new firmware load
	timeout = XBEE_SET_TIMEOUT_MS( 2000);
	while (! XBEE_CHECK_TIMEOUT_MS( timeout));

	// need to switch to 9600 baud and set configuration options
	// enable RTS/CTS; change baud rate; save changes; switch baudrate
	xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin);

	xbee_ser_baudrate( &my_xbee.serport, 9600);

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)

	// Doing this confirms that we can communicate with the XBee at 9600 baud.
	xbee_cmd_init_device( &my_xbee);
	printf( "Waiting for driver to query the XBee device...\n");
	do {
		xbee_dev_tick( &my_xbee);
		status = xbee_cmd_query_status( &my_xbee);
	} while (status == -EBUSY);
	if (status)
	{
		printf( "Error %d waiting for query to complete.\n", status);
		xbee_ser_close( &my_xbee.serport);
		return status;
	}

	// report on the settings
	xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

	printf( "switching to 115200 bps and enabling CTS/RTS\n");

	// enable CTS/RTS
	xbee_cmd_simple( &my_xbee, "D7", 1);
	xbee_cmd_simple( &my_xbee, "D6", 1);

	// After we send the baud rate change, we need to switch the baud rate
	// on the Rabbit as well.
	xbee_cmd_simple( &my_xbee, "BD", 7);

	// wait for characters to finish sending
	while (xbee_ser_tx_used( &my_xbee.serport));

	// wait 500ms before changing baud rate and sending ATWR
	timeout = XBEE_SET_TIMEOUT_MS( 500);
	while (! XBEE_CHECK_TIMEOUT_MS( timeout))
	{
		xbee_dev_tick( &my_xbee);
	}

	// change serial port to 115200, write changes out
	xbee_ser_baudrate( &my_xbee.serport, 115200);
	xbee_cmd_execute( &my_xbee, "WR", NULL, 0);

	printf( "firmware update complete\n");
	xbee_ser_close( &my_xbee.serport);

	return 0;
}

const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_TABLE_END
};

