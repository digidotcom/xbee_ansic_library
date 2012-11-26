/**************************************************************************
  Copyright (c)2011 Digi International Inc., All Rights Reserved

  Basic_XBee_Init.c

  This sample program shows the basic initialization of an XBee device.
  It does not attempt to establish a network or do anything beyond the
  basic initialization.  Configuration is automatic for any Rabbit core
  module or SBC that comes with an integrated XBee module.  To run this
  sample on other hardware, please consult section 5.1.1 of the Intro to
  ZigBee manual for further information on custom configurations.

  On success, it displays the XBee hardware and firmware versions.
  This is a quick way to check for the role that the XBee is programmed
  to perform.  The following list shows the firmware versions for XBee
  roles.  The last two (..) digits are minor revision values.

     Coordinator Version   0x000021..
     Router Version        0x000023..
     End Device Version    0x000029..

**************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "xbee/atcmd.h"

// Automatically load configuration settings for attached board's serial port.
#include "xbee_config.h"

// Define simple dispatch table for local requests only
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_TABLE_END
};

// Declare the XBee device structure to manage the XBee device settings
xbee_dev_t my_xbee;

void main()
{
	int status;

	// initialize the serial and device layer for this XBee device
   // XBEE_SERPORT, xbee_awake_pin and xbee_reset_pin setup in xbee_config.h
   // based on the board type being used to run the sample
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin))
	{
		printf( "Failed to initialize device.\n");
		exit(0);
	}

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)
	xbee_cmd_init_device( &my_xbee);
	printf( "Waiting for driver to query the XBee device...\n");
	do {
		xbee_dev_tick( &my_xbee);
		status = xbee_cmd_query_status( &my_xbee);
	} while (status == -EBUSY);
	if (!status)  //Check for valid response or error condition
   {
   	printf("\nXBee initialization and query complete.\n\n");
      printf("XBee Hardware Version: 0x%04x\n", my_xbee.hardware_version);
      printf("XBee Firmware Version: 0x%08lx\n", my_xbee.firmware_version);
   }
   else
   {  // Error detected, display code and error message
   	printf( "xbee_init failed. Result code: %d (%ls)\n", status,
                 error_message(status));
   }
   while(1);	//Infinite loop
}