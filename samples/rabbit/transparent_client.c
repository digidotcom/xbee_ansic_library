/* Copyright (c)2007-2011 Digi International Inc., All Rights Reserved */

/*
	Transparent Serial Client sample

	This sample demonstrates sending simple streams of data between XBee nodes
	on a network.  It uses the Digi Transparent Serial cluster; the same
	cluster used by XBee modules running "AT firmware" instead of "API firmware".

	"Smart" Network Nodes:
	Run this sample on one or more Rabbit modules, with one configured as the
	coordinator and others as routers.  Each of the smart nodes can send to any
	other node on the network

	"Dumb" Network Nodes:
	Set up multiple XBee modules with "AT mode" or "Transparent Serial"
	firmware, configured as routers and joined to the coordinator.  Each router
	module should have its "DH" and "DL" parameters set to 0 so they will send
	serial data to the coordinator (Rabbit).  Give each router a unique name
	(the "NI" parameter).

	If you don't want the coordinator to receive data from the "dumb" nodes,
	you will need to configure the DH/DL values on each router to match the
	SH/SL (MAC address) of the node to receive the data.

	Connect each "dumb node" up to a serial port on a PC and open a terminal to
	that serial port.  Make sure the baud rate matches the "BD" parameter on
	that XBee!


	Data entered into the serial port of the "dumb nodes" is automatically sent
	to the XBee module specified in that node's DH/DL registers.  The "smart
	nodes" can search the network (via the "ND" command) and select any node
	by its Node ID (NI) to send data to.

	Smart nodes display received data with the name of the device that sent it.
	Dumb nodes just display the data in its raw form.


   Note: this sample does not use the Simple XBee API (SXA) library, in order
   to show how things are done "manually".  SXA makes things easier by
   managing node discovery.  See SXA-stream.c for an SXA-based sample.

   To use:
      -  Open the File->Project->Open (Ctrl-P) to open the "transparent_client"
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
   	-  The program will print out a list of commands to choose from.
		-  Use the "target" command to select one of them as a target for
      	your messages.
		-	Type a message on the Rabbit and it will go out to the selected node.
		-	Paste messages into the terminal window of each XBee router, and the
      	Rabbit XBee will receive and display the message, prefixed with
         the node's name.

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
*/

#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>

#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/discovery.h"
#include "xbee/transparent_serial.h"
#include "xbee/wpan.h"
#include "wpan/types.h"
#include "wpan/aps.h"
#include "zigbee/zdo.h"

// Common code for handling AT command syntax and execution
#include "common/_atinter.h"

// Common code for handling remote node lists
#include "common/_nodetable.h"

// Load configuration settings for board's serial port.
#include "xbee_config.h"

// An instance required (in root memory) for the local XBee
xbee_dev_t my_xbee;


void transparent_dump( const addr64 FAR *ieee, const void FAR *payload,
		uint16_t length)
{
	xbee_node_id_t *node_id;
	char buffer[ADDR64_STRING_LENGTH];
	const uint8_t FAR *message = payload;
	uint16_t i;

	printf( "%u bytes from ", length);

	node_id = node_by_addr( ieee);
	if (node_id != NULL)
	{
		printf( "'%s':\n", node_id->node_info);
	}
	else
	{
		printf( "%s:\n", addr64_format( buffer, ieee));
	}

	for (i = 0; i < length && isprint( message[i]); ++i);

	if (i == length)
	{
		// all characters of message are printable
		printf( "\t%.*s\n", length, message);
	}
	else
	{
		hex_dump( message, length, HEX_DUMP_FLAG_TAB);
	}
}

// function receiving data on transparent serial cluster when ATAO != 0
int transparent_rx( const wpan_envelope_t FAR *envelope, void FAR *context)
{
	transparent_dump( &envelope->ieee_address, envelope->payload,
		envelope->length);

	return 0;
}

// function receiving data on transparent serial cluster when ATAO=0
int receive_handler( xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	const xbee_frame_receive_t FAR *rx_frame = raw;

	if (length >= offsetof( xbee_frame_receive_t, payload))
	{
		transparent_dump( &rx_frame->ieee_address, rx_frame->payload,
				length - offsetof( xbee_frame_receive_t, payload));
	}

	return 0;
}

void node_discovered( xbee_dev_t *xbee, const xbee_node_id_t *rec)
{
	if (rec != NULL)
	{
		node_add( rec);
		xbee_disc_node_id_dump( rec);
	}
}

int send_data( const xbee_node_id_t *node_id, void FAR *data, uint16_t length)
{
	wpan_envelope_t env;

	wpan_envelope_create( &env, &my_xbee.wpan_dev, &node_id->ieee_addr_be,
		node_id->network_addr);
	env.payload = data;
	env.length = length;

	return xbee_transparent_serial( &env);
}

int send_string( const xbee_node_id_t *node_id, char FAR *str)
{
	return send_data( node_id, str, strlen( str));
}

/////// use the endpoint table when AO is non-zero

// must be sorted by cluster ID
const wpan_cluster_table_entry_t digi_data_clusters[] =
{
	// transparent serial goes here (cluster 0x0011)
	{ DIGI_CLUST_SERIAL, transparent_rx, NULL,
		WPAN_CLUST_FLAG_INOUT | WPAN_CLUST_FLAG_NOT_ZCL },

	// handle join notifications (cluster 0x0095) when ATAO is not 0
	XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY,

	WPAN_CLUST_ENTRY_LIST_END
};

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };

const wpan_endpoint_table_entry_t sample_endpoints[] = {
	ZDO_ENDPOINT(zdo_ep_state),

	// Endpoint/cluster for transparent serial and OTA command cluster
	{	WPAN_ENDPOINT_DIGI_DATA,		// endpoint
		WPAN_PROFILE_DIGI,				// profile ID
		NULL,									// endpoint handler
		NULL,									// ep_state
		0x0000,								// device ID
		0x00,									// version
		digi_data_clusters				// clusters
	},

	{ WPAN_ENDPOINT_END_OF_LIST }
};

////////// end of endpoint table, only necessary if ATAO is not 0

const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,

	XBEE_FRAME_HANDLE_ATND_RESPONSE,		// for processing ATND responses

	// this entry is for when ATAO is not 0
	XBEE_FRAME_HANDLE_RX_EXPLICIT,		// rx messages via endpoint table

	// next two entries are used when ATAO is 0
	XBEE_FRAME_HANDLE_AO0_NODEID,			// for processing NODEID messages
	{ XBEE_FRAME_RECEIVE, 0, receive_handler, NULL },		// rx messages direct
	XBEE_FRAME_TABLE_END
};

void print_menu( void)
{
	puts( "help                 This list of options.");
	puts( "nd                   Initiate node discovery.");
	puts( "nd <node id string>  Search for a specific node ID.");
	puts( "atXX[=YY]            AT command to get/set an XBee parameter.");
	puts( "    Optional value can be decimal (YYY), hex (0xYYYY)"
																" or string \"YYY\"");
	puts( "target               Show the list of known targets.");
	puts( "target SOME STRING   Set target based on its NI value.");
	puts( "");
	puts( "   All other commands are sent to the current target.");
	puts( "");
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
	int i;
	xbee_node_id_t *target = NULL;


	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, xbee_awake_pin, xbee_reset_pin))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	// Initialize the WPAN layer of the XBee device driver.  This layer enables
	// endpoints and clusters, and is required for all ZigBee layers.
	xbee_wpan_init( &my_xbee, sample_endpoints);

	// Register handler to receive Node ID messages
	xbee_disc_add_node_id_handler( &my_xbee, &node_discovered);

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

	print_menu();
	xbee_disc_discover_nodes( &my_xbee, NULL);

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	wpan_tick( &my_xbee.wpan_dev);
      	xbee_cmd_tick();
      }

		if (! strcmpi( cmdstr, "help") || ! strcmp( cmdstr, "?"))
		{
			print_menu();
		}
		else if (! strncmpi( cmdstr, "nd", 2))
      {
			// Initiate discovery for a specified node id (as parameter in command
			// or all node IDs.
			if (cmdstr[2] == ' ')
			{
				printf( "Looking for node [%s]...\n", &cmdstr[3]);
				xbee_disc_discover_nodes( &my_xbee, &cmdstr[3]);
			}
			else
			{
				puts( "Discovering nodes...");
				xbee_disc_discover_nodes( &my_xbee, NULL);
			}
      }
      else if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
      else if (! strncmpi( cmdstr, "AT", 2))
      {
			process_command( &my_xbee, cmdstr);
	   }
	   else if (! strcmpi( cmdstr, "target"))
	   {
	   	node_table_dump();
	   }
	   else if (! strncmpi( cmdstr, "target", 6))
	   {
	   	target = node_by_name( &cmdstr[7]);
	   	if (target == NULL)
	   	{
	   		printf( "couldn't find a target named '%s'\n", &cmdstr[7]);
	   	}
	   	else
	   	{
	   		printf( "target: ");
	   		xbee_disc_node_id_dump( target);
	   	}
	   }
	   else
	   {
	   	if (target == NULL)
	   	{
	   		puts( "you must first select a target with the `target` command");
	   	}
	   	else
	   	{
	   		i = strlen( cmdstr);
	   		printf( "sending %u bytes to '%s'\n", i, target->node_info);
	   		send_data( target, cmdstr, i);
	   	}
	   }
   }
}

