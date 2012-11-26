/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/***************************************************************************

   SXA-io.c

   This sample shows use of the Simple XBee API library, to discover
   remote XBee nodes and query/set their I/Os.

   To use:
      -  Open the File->Project->Open (Ctrl-P) to open the SXA-io project in
         Samples/XBee.  Then, select Options->Project Options, and go to
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
   	-  The program will print out a list of commands to choose from
   	-	Typing "menu" will redisplay the menu of commands.
      -  Set the target node (for remote AT commands) using the '>'
         or '<' commands.
      -  Manipulate I/O state on the selected target using the listed commands.
   	-	To exit, press F4.

   For additional diagnostics/debugging, add one or more of the
   following to the Options->Project Options "Defines" tab.  The VERBOSE
   macros print messages to the Dynamic C stdio window, and the DEBUG
   macros allow single-stepping through the elibrary code.
		XBEE_ATCMD_VERBOSE
		XBEE_DEVICE_VERBOSE
		XBEE_SERIAL_VERBOSE
		XBEE_IO_VERBOSE
		XBEE_DISCOVERY_VERBOSE
		XBEE_ATCMD_DEBUG
		XBEE_SERIAL_DEBUG
		XBEE_DEVICE_DEBUG
		XBEE_WPAN_DEBUG
		XBEE_SXA_DEBUG


*****************************************************************************/

#memmap xmem


#include <stdio.h>
#include <ctype.h>
#include <errno.h>

// Main header required for Simple XBee API
#include "xbee/sxa.h"

// Common code for handling AT command syntax and execution
#include "common/_atinter.h"

// Common code for handling SXA target device selection
#include "common/_sxa_select.h"



// Load configuration settings for board's serial port.
#include "xbee_config.h"

// An instance required (in root memory) for the local XBee
xbee_dev_t my_xbee;

/*
	Add fixed set of cluster IDs in order to handle discovery and I/O samples,
   no matter what the ATAO setting in the attached XBee.
*/

const wpan_cluster_table_entry_t digi_data_clusters[] = {

	/* Other clusters can go here, sorted by cluster ID... */
	XBEE_IO_CLIENT_CLUST_ENTRY,
	XBEE_DISC_CLIENT_CLUST_ENTRY,
	WPAN_CLUST_ENTRY_LIST_END
};

/* Used to track ZDO transactions in order to match responses to requests
   (#ZDO_MATCH_DESC_RESP). */
wpan_ep_state_t zdo_ep_state = { 0 };

const wpan_endpoint_table_entry_t endpoints_table[] = {
    /* Add your endpoints here */

    ZDO_ENDPOINT(zdo_ep_state),

    /* Digi endpoints */
    {
        WPAN_ENDPOINT_DIGI_DATA,  // endpoint
        WPAN_PROFILE_DIGI,        // profile ID
        NULL,                     // endpoint handler
        NULL,                     // ep_state
        0x0000,                   // device ID
        0x00,                     // version
        digi_data_clusters        // clusters
    },

    { WPAN_ENDPOINT_END_OF_LIST }
};


void help(void)
{
   printATCmds( sxa_xbee( sxa));
   sxa_select_help();
   printf("I/O setting:\n");
   printf(" {+|-}io[f]  (+ for hi, - for lo,\n");
   printf("                io number (0-5),\n");
   printf("                optional 'f' for force)\n");
   printf(" .io      (set io number (0-5) to input)\n");
   printf(" ;io      (set io number (0-3) to analog input)\n");
   printf(" {#|$}io  (set/clear io number (0-9) sample-on-change)\n");
   printf(" IS       (immediate sample I/O state)\n");
   printf("\n");
   printf(" menu     (print this command list)\n");
   printf(" quit     (quit this demo)\n");
}



/*
	main

	Initiate communication with the XBee module, then accept AT and
   other commands from STDIO, pass them to the XBee module via the
   SXA library, and print the results.
*/
int main( void)
{
   char cmdstr[80];
   int ionum;
   int status;

   // Required local device initialization
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin))
	{
		printf( "Failed to initialize device.\n");
		exit(1);
	}

   // Initialize the Simple XBee API layer.  SXA handles a lot of the
   // book-keeping for multiple remote nodes and XBee on-module I/Os.
   // It also helps to hide the difference between local and remote nodes.
   sxa = sxa_init_or_exit(&my_xbee, endpoints_table, 1);

   if (!sxa)
   {
   	printf("Failed to initialize Simple XBee API!\n");
      exit(2);
   }

   help();

   while (1)
   {
   	// Accumulate a command string, and call the tick function while waiting.
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
   		sxa_tick();
      }

      // Have a command, process it.
		if (! strncmpi( cmdstr, "menu", 4))
      {
      	help();
      }
      else if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
      else if ( cmdstr[0] == '+' || cmdstr[0] == '-')
      {
      	// Digital output on or off
			ionum = cmdstr[1] - '0';
         status = sxa_set_digital_output(
               sxa,
               ionum,
               (cmdstr[0] == '+' ? XBEE_IO_SET_HIGH : XBEE_IO_SET_LOW) |
               (cmdstr[2] == 'f' ? XBEE_IO_FORCE : 0)
               );
         printf("I/O command status %d\n", status);
      }
      else if (cmdstr[0] == '.' || cmdstr[0] == ';')
      {
      	// Set an I/O to digital or analog input
			ionum = cmdstr[1] - '0';
         status = sxa_io_configure(sxa, ionum,
         	cmdstr[0] == '.' ? XBEE_IO_TYPE_DIGITAL_INPUT :
            		XBEE_IO_TYPE_ANALOG_INPUT
            );
         printf("I/O command status %d\n", status);
      }
      else if (cmdstr[0] == '#' || cmdstr[0] == '$')
      {
      	// Set or clear automatic sample on digital I/O change
			ionum = cmdstr[1] - '0';
         if (cmdstr[0] == '#')
         	status = sxa_io_set_options(sxa, 0, sxa->io.change_mask | 1u<<ionum);
         else
         	status = sxa_io_set_options(sxa, 0, sxa->io.change_mask & ~(1u<<ionum));
         printf("I/O command status %d\n", status);
      }
      else if ( sxa_select_command( cmdstr))
      {
      	// empty, command done
      }
      else
      {
      	// Everything else gets executed as a manual AT command
			process_command_remote( sxa_xbee( sxa), cmdstr,
         	have_target ? &target_ieee : NULL);
	   }
   }
}



/*
	Define frame dispatch table.  The entries listed here are mandatory
	for this sample to work, since the library currently requires the
	frame dispatch table to be set up at compile-time.
*/
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	// Standard handlers (required) for AT command processing
	XBEE_FRAME_HANDLE_LOCAL_AT,
   XBEE_FRAME_HANDLE_REMOTE_AT,

	// Standard handlers (required) for SXA to work properly
	XBEE_FRAME_HANDLE_IS_RESPONSE,
   XBEE_FRAME_HANDLE_ND_RESPONSE,
   XBEE_FRAME_HANDLE_RX_EXPLICIT,

	XBEE_FRAME_TABLE_END
};




