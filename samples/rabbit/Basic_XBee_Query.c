/**************************************************************************
  Copyright (c)2011 Digi International Inc., All Rights Reserved

  Basic_XBee_Query.c

  This sample program shows the reading of two local registers on an XBee
  device.  It does not attempt to establish a network or do anything beyond
  the reading of this register.  Configuration is automatic for any Rabbit
  core module or SBC that comes with an integrated XBee module.  To run this
  sample on other hardware, please consult section 5.1.1 of the Intro to
  ZigBee manual for further information on custom configurations.

**************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "xbee/atcmd.h"

// Custom structure to receive responses from AT command list below
typedef struct xb_buf_t {
  unsigned int baud;
  unsigned int payload;
} xb_buf;

// Defines sequence of AT commands to be executed (Read baud & max payload)
const xbee_atcmd_reg_t query_regs[] = {
	XBEE_ATCMD_REG( 'B', 'D', XBEE_CLT_COPY_BE, struct xb_buf_t, baud),
	XBEE_ATCMD_REG( 'N', 'P', XBEE_CLT_COPY_BE, struct xb_buf_t, payload),
   XBEE_ATCMD_REG_END       // Mark list end
};

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

// Baud rate decoding table
const long xbee_baud[] = {1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200};

void main()
{
	int status;
	xbee_command_list_context_t clc;
   xb_buf buffer;

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
		xbee_dev_tick( &my_xbee );
		status = xbee_cmd_query_status( &my_xbee );
	} while (status == -EBUSY);
	if (status) //Check for valid response or error condition
   {
   	printf( "xbee_init failed. Result code: %d (%ls)\n", status,
                 error_message(status));
   }
   else
   {
   	printf("\nXBee initialization and query complete.\n\n");

      // Execute pre-defined list of commands and save results to buffer
		xbee_cmd_list_execute( &my_xbee,
   								  &clc,
									  query_regs,
                             &buffer,
                             NULL );
		while ((status = xbee_cmd_list_status(&clc)) == XBEE_COMMAND_LIST_RUNNING)
   	{
   		xbee_dev_tick( &my_xbee );      // Drive device layer to completion
   	}

      if (status == XBEE_COMMAND_LIST_TIMEOUT)   // Check for timeout condition
      {
      	printf("XBee command list timed out.\n");
      }
      else
      {  // Command list completed
	      if (status == XBEE_COMMAND_LIST_ERROR)   // Check for complete w/error
         {
            printf("XBee command list completed with errors.\n\n");
         }
         printf("XBee serial baud rate: %ld\n", xbee_baud[buffer.baud]);
         printf("XBee max network payload: %d\n", buffer.payload);
      }
   }

   while(1);	//Infinite loop
}