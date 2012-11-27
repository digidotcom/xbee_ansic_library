/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/***************************************************************************

   AT_remote.c
	Digi International, Copyright (C) 2011.  All rights reserved.

   This sample shows how to set up and use AT commands with the XBee module.
   Unlike AT_interactive.c, this sample allows discovery of remote nodes
   and sending AT commands to them.

   To use:
      -  Open the File->Project->Open (Ctrl-P) to open the AT_remote project in
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
   	-  The program will print out a list of AT commands to choose from
   	-  You can type in either "ATxx" or just the "xx" part of the command
   	-  Using just the AT command, you can read any of the values
   	-  You can set any of the "set or read" values using the following format:
				Valid command formats (AT prefix is optional, CC is command):
				[AT]CC 0xXXXXXX (where XXXXXX is an even number of hexadecimal
            					  characters)
				[AT]CC YYYY (where YYYY is an integer, up to 32 bits)
				[AT]NI "Node ID String" (where quotes contain string data)
   	-	Typing "menu" will redisplay the menu of commands.
      -  Set the target node (for remote AT commands) using the '>'
         or '<' commands.
   	-	To exit, press F4.

	This sample relies on shared files in the common directory.

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


// ------------------------------------------------------

#memmap xmem


#include <stdio.h>
#include <ctype.h>
#include <errno.h>

#include "xbee/atcmd.h"
#include "xbee/wpan.h"

// Common code for handling AT command syntax and execution
#include "common/_atinter.h"

// Common code for handling remote node tables
#include "common/_nodetable.h"

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
   xbee_node_id_t * node;
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
         	if (isdigit(*str))
            {
            	node = node_by_index( *str - '0');
               if (node)
               {
               	i = 0;
                  _f_memcpy(address, &node->ieee_addr_be, 8);
               }
               else
               {
               	printf("Table entry not found.  Select from:\n");
                  node_table_dump();
               }
            }
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
	return 1;
}


//#include "zigbee/zdo.h"

int xbee_discovery_cluster_handler(const wpan_envelope_t FAR *envelope,
								   void FAR *context)
{
	xbee_node_id_t node_id;

   printf("Discovery cluster handler\n");

	if (xbee_disc_nd_parse(&node_id, envelope->payload, envelope->length) == 0) {
		node_add(&node_id);
		xbee_disc_node_id_dump(&node_id);
	}

	return 0;
}

const wpan_cluster_table_entry_t digi_data_clusters[] = {

    /* Other clusters should go here, sorted by cluster ID... */
    // transparent serial goes here

	{DIGI_CLUST_NODEID_MESSAGE, xbee_discovery_cluster_handler, NULL,	/* cluster 0x0095 */
			WPAN_CLUST_FLAG_INPUT | WPAN_CLUST_FLAG_NOT_ZCL},
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

	// give the XBee 500ms to wake up after resetting it (or exit if it
   // receives a packet)
	t = XBEE_SET_TIMEOUT_MS(500);
	while (! XBEE_CHECK_TIMEOUT_MS(t) && xbee_dev_tick( &my_xbee) <= 0);

   xbee_wpan_init(&my_xbee, endpoints_table);

	// Initialize the AT Command layer for this XBee device and have the
	// driver query it for basic information (hardware version, firmware version,
	// serial number, IEEE address, etc.)
	xbee_cmd_init_device( &my_xbee);
	printf( "Waiting for driver to query the XBee device...\n");
	do {
		wpan_tick( &my_xbee.wpan_dev);
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
   printf(" > addr   (addr is hex ieee addr 8 or 16 digits,\n");
   printf("           high bytes assumed 0013a200 if 8 digits)\n");
   printf(" > N      (single digit: select node from table by index 0..9)\n");
   printf(" >        (reset to local device)\n");
   printf(" <        (reinstate previous remote target)\n");

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
			wpan_tick( &my_xbee.wpan_dev);
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
         if (have_target)
         {
         	ever_had_target = 1;
         }
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

const char * xbee_delivery_status_str(uint8_t delivery)
{
	switch (delivery)
   {
		case XBEE_TX_DELIVERY_SUCCESS:
      	return "OK";
		case XBEE_TX_DELIVERY_MAC_ACK_FAIL:
      	return "ACK-FAIL";
		case XBEE_TX_DELIVERY_CCA_FAIL:
      	return "CCA-FAIL";
		case XBEE_TX_DELIVERY_BAD_DEST_EP:
      	return "BAD-DEST-EP";
		case XBEE_TX_DELIVERY_NO_BUFFERS:
      	return "NO-BUFFERS";
		case XBEE_TX_DELIVERY_NET_ACK_FAIL:
      	return "NET-ACK-FAIL";
		case XBEE_TX_DELIVERY_NOT_JOINED:
      	return "NOT-JOINED";
		case XBEE_TX_DELIVERY_SELF_ADDRESSED:
      	return "SELF-ADDRESSED";
		case XBEE_TX_DELIVERY_ADDR_NOT_FOUND:
      	return "ADD-NOT-FOUND";
		case XBEE_TX_DELIVERY_ROUTE_NOT_FOUND:
      	return "ROUTE-NOT-FOUND";
		case XBEE_TX_DELIVERY_BROADCAST_NOT_HEARD:
      	return "BCAST-NOT-HEARD";
		case XBEE_TX_DELIVERY_INVALID_BINDING_INDX:
      	return "INVALID-BINDING-INDX";
		case XBEE_TX_DELIVERY_INVALID_EP:
      	return "INVALID-EP";
		case XBEE_TX_DELIVERY_RESOURCE_ERROR:
      	return "DEL-RSRC-ERROR";
		case XBEE_TX_DELIVERY_PAYLOAD_TOO_BIG:
      	return "PAYLOAD-TOO-BIG";
		case XBEE_TX_DELIVERY_INDIRECT_NOT_REQ:
      	return "INDIR-NOT-REQ";
		case XBEE_TX_DELIVERY_KEY_NOT_AUTHORIZED:
      	return "DEL-KEY-NOT-AUTH";
      default:
      	return "<unknown>";
   }
}

const char * xbee_discovery_status_str(uint8_t discovery)
{
	switch (discovery)
   {
		case XBEE_TX_DISCOVERY_NONE:
      	return "None";
		case XBEE_TX_DISCOVERY_ADDRESS:
      	return "ADDR";
		case XBEE_TX_DISCOVERY_ROUTE:
      	return "ROUTE";
		case XBEE_TX_DISCOVERY_EXTENDED_TIMEOUT:
      	return "EXTENDED-TIMEOUT";
      default:
      	return "<unknown>";
   }
}

int my_handle_transmit_status( xbee_dev_t *xbee,
	const void FAR *payload, uint16_t length, void FAR *context)
{
   const xbee_frame_transmit_status_t FAR *frame = payload;

	// it may be necessary to push information up to user code so they know when
	// a packet has been received or if it didn't make it out
   printf( "TS: id 0x%02x to 0x%04x retries=%d del=0x%02x=%s disc=0x%02x=%s\n",
      frame->frame_id,
      be16toh( frame->network_address_be), frame->retries,
      frame->delivery, xbee_delivery_status_str(frame->delivery),
      frame->discovery, xbee_discovery_status_str(frame->discovery));

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

int xbee_nd_cmd_response_handler(xbee_dev_t *xbee, const void FAR *raw,
										uint16_t length, void FAR *context)
{
	static const xbee_at_cmd_t nd = {{'N', 'D'}};
	const xbee_frame_local_at_resp_t FAR *resp = raw;
	xbee_node_id_t node_id;
	xbee_node_id_t * tab_node;

   if (XBEE_AT_RESP_STATUS( resp->header.status) == XBEE_AT_RESP_SUCCESS)
   {
      if (resp->header.command.w == nd.w)
      {
         /* this is a successful ATND response, parse the response
          * and add the discovered node to our node table */
         if (xbee_disc_nd_parse(&node_id, resp->value,
         		length - offsetof( xbee_frame_local_at_resp_t, value)) == 0)
         {
            tab_node = node_add(&node_id);
            if (tab_node)
            {
               printf( "%2u: ", (int)(tab_node - node_table));
            }
            xbee_disc_node_id_dump(&node_id);
         }
      }
   }

	return 0;
}



// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	{ XBEE_FRAME_LOCAL_AT_RESPONSE, 0, xbee_nd_cmd_response_handler, NULL },
   XBEE_FRAME_HANDLE_REMOTE_AT,
   XBEE_FRAME_HANDLE_RX_EXPLICIT,
	{ XBEE_FRAME_MODEM_STATUS, 0, my_modem_status, NULL },
   { XBEE_FRAME_TRANSMIT_STATUS, 0, my_handle_transmit_status, NULL },
	XBEE_FRAME_TABLE_END
};




