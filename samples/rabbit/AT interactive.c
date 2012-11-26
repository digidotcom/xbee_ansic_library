/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/***************************************************************************

   AT interactive.c

   This sample shows how to set up and use AT commands with the XBee module.

   To use:
      -  Open the File->Project->Open (Ctrl-P) to open the AT interactive
         project in Samples/XBee.  Then, select Options->Project Options, and go to
         the "Communications" tab.  Make sure the serial port settings
         are set correctly for the intended target XBee-enabled board.
      -  Select Window->Project Explorer (Shift-F12) to bring up the Project
         Explorer window.
   	-	Select "Run Project" in the project explorer window to compile and run
         this sample on an XBee-enabled board.  If not one of
         the standard boards which support XBee (RCM45xxW, BL4S100 etc.)
         then you will need first to define a set of configuration macros.  See
         xbee_config.h for details.
      -  If desired, save the current project settings using
         File->Project->Save (Ctrl-R).
   	-  The program will print out a list of AT commands to choose from
         (although the list is not complete, consult the XBee manual for
         a full list).
   	-  You can type in either "ATxx" or just the "xx" part of the command
   	-  Using just the AT command, you can read any of the values
   	-  You can set any of the "set or read" values using the following format:
				Valid command formats (AT prefix is optional, CC is command):
				[AT]CC 0xXXXXXX (where XXXXXX is an even number of hexadecimal
            					  characters)
				[AT]CC YYYY (where YYYY is an integer, up to 32 bits)
				[AT]NI "Node ID String" (where quotes contain string data)
      -  Initially, commands are directed to the locally attached XBee module.
         You can use the '>' command to point to a remote XBee if you know
         its MAC address, and it is joined to this network.
   	-	Typing "menu" will redisplay the menu of commands.
   	-	To exit, press F4.

   Note:
   	This sample does not discover applicable remote nodes, thus it can
      be difficult to enter the correct parameter for the '>' command.
   	See SXA-command.c which uses the Simple XBee API layer libraries
      to display a list of remote (and local) nodes which can be
      selected easily.

   For additional diagnostics/debugging, add one or more of the
   following to the Options->Project Options "Defines" tab.  The VERBOSE
   macros print messages to the Dynamic C stdio window, and the DEBUG
   macros allow single-stepping through the elibrary code.
		XBEE_ATCMD_VERBOSE
		XBEE_DEVICE_VERBOSE
		XBEE_SERIAL_VERBOSE
		XBEE_ATCMD_DEBUG
		XBEE_SERIAL_DEBUG
		XBEE_DEVICE_DEBUG

*****************************************************************************/


#memmap xmem



#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "xbee/atcmd.h"
#include "xbee/wpan.h"

// Common code for handling AT command syntax and execution
#include "common/_atinter.h"

// Load configuration settings for board's serial port.
#include "xbee_config.h"

// An instance required (in root memory) for the local XBee
xbee_dev_t my_xbee;



addr64 target_ieee;
int have_target = 0;
int ever_had_target = 0;



int set_target(const char *str, addr64 FAR *address)
{
	uint_fast8_t i;
	uint8_t FAR *b;
	int ret;
   char buf[40];

	i = 8;			// bytes to convert
	if (str != NULL && address != NULL)
	{
		// skip over leading spaces
		while (isspace( (uint8_t) *str))
		{
			++str;
		}
		for (b = address->b; i; ++b, --i)
		{
			ret = hexstrtobyte( str);
			if (ret < 0)
			{
				break;
			}
			*b = (uint8_t)ret;
			str += 2;					// point past the encoded byte

			// skip over any separator, if present
			if (*str && ! isxdigit( (uint8_t) *str))
			{
				++str;
			}
		}
	}
   if (i == 4)
   {
   	for (i = 7; i >= 4; --i)
      {
      	address->b[i] = address->b[i-4];
      }
      address->b[0] = 0x00;
      address->b[1] = 0x13;
      address->b[2] = 0xA2;
      address->b[3] = 0x00;
      i = 0;
   }

	if (i != 0)
	{
   	printf("Target set to local device\n");
		return 0;
	}

   printf("Target now %" PRIsFAR "\n", addr64_format(buf, address));
   ever_had_target = 1;
	return 1;
}

/*
	main

	Initiate communication with the XBee module, then accept AT commands from
	STDIO, pass them to the XBee module and print the result.
*/
int main( void)
{
   char cmdstr[80];
	int status;
	uint16_t t;


	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

  	//xbee_dev_reset( &my_xbee);
	// give the XBee 500ms to wake up after resetting it (or exit if it
   // receives a packet)
	t = XBEE_SET_TIMEOUT_MS(500);
	while (! XBEE_CHECK_TIMEOUT_MS(t) && xbee_dev_tick( &my_xbee) <= 0);

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)
	xbee_cmd_init_device( &my_xbee);
	printf( "Waiting for driver to query the XBee device...\n");
	do {
		xbee_dev_tick( &my_xbee);
		status = xbee_cmd_query_status( &my_xbee);
	} while (status == -EBUSY);
	if (status)
	{
		printf( "Error %d waiting for query to complete.\n", status);
	}

	// report on the settings
	xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

   printATCmds( &my_xbee);
   printf("Target setting for remote commands:\n");
   printf(" > addr   (addr is hex ieee addr, high bytes assumed 0013a200)\n");
   printf(" >        (reset to local device)\n");
   printf(" <        (reinstate previous remote target)\n");

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	xbee_dev_tick( &my_xbee);
      }

		if (! strncmpi( cmdstr, "menu", 4))
      {
      	printATCmds( &my_xbee);
      }
      else if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
      else if ( cmdstr[0] == '>')
      {
			have_target = set_target( cmdstr+1, &target_ieee);
      }
      else if ( cmdstr[0] == '<')
      {
      	if (ever_had_target)
         {
         	have_target = 1;
            printf("Reinstating %" PRIsFAR "\n",
            	addr64_format(cmdstr, &target_ieee));
         }
         else
         {
         	printf("Nothing to reinstate\n");
         }
      }
      else
      {
      	have_target ?
				process_command_remote( &my_xbee, cmdstr, &target_ieee) :
				process_command( &my_xbee, cmdstr);
	   }
   }
}


int my_handle_transmit_status( xbee_dev_t *xbee,
	const void FAR *payload, uint16_t length, void FAR *context)
{
   const xbee_frame_transmit_status_t FAR *frame = payload;

	// it may be necessary to push information up to user code so they know when
	// a packet has been received or if it didn't make it out
   printf( "TS: id 0x%02x to 0x%04x retries=%d del=0x%02x disc=0x%02x\n",
      frame->frame_id,
      be16toh( frame->network_address_be), frame->retries, frame->delivery,
      frame->discovery);

	return 0;
}

int my_modem_status( xbee_dev_t *xbee,
	const void FAR *payload, uint16_t length, void FAR *context)
{
	const xbee_frame_modem_status_t FAR *frame = payload;

	xbee_frame_dump_modem_status(xbee, payload, length, context);

   if (frame->status == XBEE_MODEM_STATUS_WATCHDOG)
   {
   	have_target = 0;
      printf("Resetting target to local device\n");
   }
   return 0;
}


// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
   XBEE_FRAME_HANDLE_REMOTE_AT,
	{ XBEE_FRAME_MODEM_STATUS, 0, my_modem_status, NULL },
   { XBEE_FRAME_TRANSMIT_STATUS, 0, my_handle_transmit_status, NULL },
	XBEE_FRAME_TABLE_END
};




