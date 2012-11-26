/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/***************************************************************************

   SXA-command.c

   This sample shows use of the Simple XBee API library, to discover
   remote XBee nodes and issue AT commands, both interactively and
   programmatically.

	Programmatic issuance of AT commands uses the xbee_cmd_list_execute()
   function.


   To use:

      -  Open the File->Project->Open (Ctrl-P) to open the SXA-command project
         in Samples/XBee.  Then, select Options->Project Options, and go to
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
   	-  You can type in either "ATxx" or just the "xx" part of the command
   	-  Using just the AT command, you can read any of the values
   	-  You can set any of the "set or read" values using the following format:
				Valid command formats (AT prefix is optional, CC is command):
				[AT]CC 0xXXXXXX (where XXXXXX is an even number of hexadecimal
            					  characters)
				[AT]CC YYYY (where YYYY is an integer, up to 32 bits)
				[AT]NI "Node ID String" (where quotes contain string data)
      -  The '$' command issues a canned list of AT commands.  See the
         code for additional commentary (issue_commands() function).
   	-	Typing "menu" will redisplay the menu of commands.
      -  Set the target node (for remote AT commands) using the '>'
         or '<' commands.
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
	XBEE_DISC_CLIENT_CLUST_ENTRY,
	WPAN_CLUST_ENTRY_LIST_END
};

/* Used to track ZDO transactions in order to match responses to requests
   (#ZDO_MATCH_DESC_RESP). */
wpan_ep_state_t zdo_ep_state = { 0 };

const wpan_endpoint_table_entry_t endpoints_table[] = {
	/* Add your endpoints here */

	/* Required endpoint for discovery to work */
	ZDO_ENDPOINT(zdo_ep_state),

	/* Digi endpoints.  If you are not doing anything fancy, this is
      boilerplate which allows discovery responses to flow through
      to the correct handler.  */
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
   printATCmds( &my_xbee);
   printf("Commands and data:\n");
   printf("  [AT]xx [parm] (Issue AT command - local or remote)\n");
   printf("  $             (Issue canned list of commands and show result)\n");
   sxa_select_help();
   printf("Other:\n");
   printf("  menu     (print this command list)\n");
   printf("  quit     (quit this demo)\n");
}

/*
	issue_commands()

	This function and supporting data shows how to set up a list of commands
   to be executed, along with storing the results in a structure.
*/

// Sample structure to fill out from command list results
struct my_type {
	int		ch;		// ATCH (channel)
   uint8_t	op[8];	// ATOP (operating extended PAN ID)
   uint16_t	oi;		// ATOI (operating 16-bit PAN ID)
   uint16_t	my;		// ATMY (16-bit network address)
   uint8_t	sc[16];	// ATSC (Scan channels, expanded out into array of
   						//  booleans via the my_sc_callback() function
   // Can also go the other way i.e. use the following fields to *set*
   // parameter values in the module.
   uint8_t	ni[20];	// ATNI (node identifier string)
   uint32_t	old_dd;	// ATDD (Device type identifier)
   uint32_t new_dd;
   uint32_t verify_dd;
} my_struct;
// Sample callback function for the 'SC' command
void my_sc_callback(	const xbee_cmd_response_t FAR 		*response,
   						const struct xbee_atcmd_reg_t FAR	*reg,
							void FAR										*base)
{
	uint16_t k, n;
	struct my_type FAR *m = base;	// Cast to the real type
	// response->value contains the value, in host byte order.  For the
   // SC command, this is a 16-bit value, which we want to expand into
   // an array of 16 bytes (just to demo the sorts of thigs callbacks
   // can do).
	for (n = 0, k = 1; n < 16; ++n, k <<= 1)
   {
   	m->sc[n] = (k & response->value) != 0;
   }
}

// Sample list of commands to execute
const xbee_atcmd_reg_t my_command_list[] = {
	// Straight copies of resulting numeric data.  The _BE copy variant
   // makes sure the result is in host byte order, which is what you
   // normally want for numbers.  Otherwise, it is copied in over-the-air
   // order, which is used for 64-bit addresses.
	XBEE_ATCMD_REG( 'C', 'H', XBEE_CLT_COPY_BE, struct my_type, ch),
	XBEE_ATCMD_REG( 'O', 'P', XBEE_CLT_COPY,    struct my_type, op),
	XBEE_ATCMD_REG( 'O', 'I', XBEE_CLT_COPY_BE, struct my_type, oi),
	XBEE_ATCMD_REG( 'M', 'Y', XBEE_CLT_COPY_BE, struct my_type, my),

   // This shows use of a callback function.  use a callback if the
   // straight numeric or binary format of the result is not quite
   // what you want.  This sample expands a bitfield into an array
   // of booleans.
   XBEE_ATCMD_REG_CB( 'S', 'C', my_sc_callback, 0),

   // Set a parameter value to a string.  In this sample, manually
   // issue the 'NI' command after executing the command list to confirm
   // that the new value has been set.
	XBEE_ATCMD_REG( 'N', 'I', XBEE_CLT_SET_STR, struct my_type, ni),

   // This shows how to get a parameter value, set it to a new value,
   // then confirm the result.
	XBEE_ATCMD_REG( 'D', 'D', XBEE_CLT_COPY_BE, struct my_type, old_dd),
	XBEE_ATCMD_REG( 'D', 'D', XBEE_CLT_SET_BE, struct my_type, new_dd),
	XBEE_ATCMD_REG( 'D', 'D', XBEE_CLT_COPY_BE, struct my_type, verify_dd),

   // If you wanted to save any parameter changes in the module's non-
   // volatile memory, you might issue the following command.
   // XBEE_ATCMD_REG_END_CMD means that the command is issued at the
   // end of all comands, and there is no waiting for a result.
   	//XBEE_ATCMD_REG_END_CMD('W', 'R')
   XBEE_ATCMD_REG_END
};

void issue_commands(sxa_node_t FAR *sxa)
{
	int i;
	xbee_command_list_context_t	my_clc;			// Context structure

   // Set up parameter data for any SET commands
   my_struct.new_dd = 0x0F111111;
   strcpy(my_struct.ni, "MyNodeID");

   // Start issuing commands in list
	xbee_cmd_list_execute( sxa_xbee( sxa),			// Get the XBee device from sxa
   								&my_clc,					// The command list context
									my_command_list,		// The command list itself
                           &my_struct,				// The struct to fill in
                           sxa_wpan_address( sxa));	// Target Xbee address

   // For this sample, block until complete.  Real application would be
   // doing something else in the meantime, inside the loop.
	while (xbee_cmd_list_status(&my_clc) == -EBUSY)
   {
   	sxa_tick();
   }

   // Now look at the final status
   switch (xbee_cmd_list_status(&my_clc))
   {
   	case XBEE_COMMAND_LIST_DONE:
      	printf("Completed successfully:\n");
         printf(" CH = %d\n", my_struct.ch);
         printf(" OP =\n");
			hex_dump( my_struct.op, sizeof(my_struct.op), HEX_DUMP_FLAG_TAB);
         printf(" OI = 0x%04X\n", my_struct.oi);
         printf(" MY = 0x%04X\n", my_struct.my);
         printf(" SC =");
         for (i = 0; i < 16; ++i)
         {
         	printf("%d", my_struct.sc[i]);
         }
         printf(" (ch 11..26)\n");
         printf(" DD (old) = 0x%08lX\n", my_struct.old_dd);
         printf(" DD (new) = 0x%08lX\n", my_struct.verify_dd);
         break;
      case XBEE_COMMAND_LIST_TIMEOUT:
      	printf("Timed out\n");
         break;
      case XBEE_COMMAND_LIST_ERROR:
      default:
      	printf("Completed partially or with errors\n");
         break;
   }
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
      else if ( sxa_select_command( cmdstr))
      {
      	// empty, command done
      }
      else if ( cmdstr[0] == '$')
      {
      	issue_commands( sxa);
      }
      else
      {
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
   XBEE_FRAME_HANDLE_ND_RESPONSE,
   XBEE_FRAME_HANDLE_RX_EXPLICIT,

	XBEE_FRAME_TABLE_END
};




