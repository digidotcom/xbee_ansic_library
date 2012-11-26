/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/***************************************************************************

   SXA-stream.c

   This sample shows use of the Simple XBee API library, to discover
   remote XBee nodes and send data streams back and forth.

   Principle of operation

	The principle of operation is to use the built-in transparent serial data
   endpoint.  Thus, this sample is able to send and receive serial data to
   any remote XBee module, without depending on any special programming or
   attached host processor.  It can be used to communicate with a remote
   async serial device such as an old ascii terminal or printer, or a
   sensor with an RS232 interface.

   If you use the '$ nnn/ccc' command to send longer strings of data, you will
   quickly notice one of the main limitations of this method for sending data
   streams: there is a limit on the length of any one transmission, and there's
   no direct way of confirming that the remote device successfully received
   all of the data.  Because of this, it is necessary for the application to
   handle the possibility of missing data in either direction.  For example,
   ASCII-based communication may require CRLF characters at the end of each
   line, which would allow for communication to be re-synchronized at each
   CRLF delimiter.

   For true end-to-end reliability at the transport layer, see the sample
   SXA-socket.c.

   For a version which does not use SXA, see transparent.c.


   To use:

   It is recommended to first get the SXA-command sample working in order to
   become familiar with project file settings and ensure that basic
   XBee functionality is accessible.

	Set up a network with the Rabbit's XBee configured as the coordinator.  Set
	up multiple XBee modules configured as routers and joined to your
	coordinator.  Each router module should have its "DH" and "DL" parameters
	set to 0 so they will send serial data to the coordinator (Rabbit).  Give
	each router a unique name (the "NI" parameter).

	If you don't want the Rabbit to be the coordinator, you will need to
	configure the DH/DL values on each router to match the SH/SL (MAC address)
	of your Rabbit's XBee module.

	Connect each router up to a serial port on a PC and open a terminal to
	that serial port.  Make sure the baud rate matches the "BD" parameter on
	that XBee!

   Compile this sample to an XBee-enabled board.  If not one of
   the standard boards which support XBee (RCM45xxW, BL4S100 etc.)
   then you will need to define a set of configuration macros.  See
   xbee_config.h for details.

   Typing "menu" will redisplay the menu of commands.  Hit '?' to get
   a list of target nodes which have been discovered.

   Set the target node (for sending stream data or AT commands) using the
   '>' or '<' commands.

   AT commands can be issued to the local or remote node.  For example,
   ATND to the local node will cause another round of node discovery, and
   ATD04 will set the local (or remote) target XBee module I/O (DIO0) to a
   low output.

	Type a message (other than one of the above commands) on the Rabbit and
   it will go out to the selected node.

	Paste messages into the terminal window of each XBee router, and the Rabbit
	XBee will receive and display the message, prefixed with the node's name.

   To exit, press F4.

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


//#define SHOW_STATUS	// Uncomment to show transmit status


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

#include "xbee/transparent_serial.h"

// An instance required (in root memory) for the local XBee
xbee_dev_t my_xbee;


// function receiving data on transparent serial cluster
int xbee_transparent_rx( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	printf( "%u bytes from ", envelope->length);

	sxa = sxa_node_by_addr( &envelope->ieee_address);
	if (sxa != NULL)
	{
		printf( "'%ls':\n", sxa->id.node_info);
	}
	else
	{
		printf( "0x%04x:\n", envelope->network_address);
	}

	hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);

	return 0;
}

int send_data( const sxa_node_t FAR *sxa, void FAR *data, uint16_t length)
{
	int status;
	wpan_envelope_t env;

	wpan_envelope_create( &env, &my_xbee.wpan_dev, &sxa->id.ieee_addr_be,
		sxa->id.network_addr);
	env.payload = data;
	env.length = length;

	do
   {
	   status = xbee_transparent_serial( &env);

	   if (status == -EBUSY)
	   {
	      // For this sample, let's just wait a bit then retry.  Necessary
	      // to call sxa_tick() so that packets get processed otherwise we
	      // might wait here forever.
	      sxa_tick();
	   }
   } while (status == -EBUSY);

   return status;
}

int send_string( const sxa_node_t FAR *sxa, char FAR *str)
{
	return send_data( sxa, str, strlen( str));
}




/*
	Add fixed set of cluster IDs in order to handle discovery and I/O samples,
   no matter what the ATAO setting in the attached XBee.
*/

const wpan_cluster_table_entry_t digi_data_clusters[] = {

	/* Other clusters can go here, sorted by cluster ID... */
	{ DIGI_CLUST_SERIAL, xbee_transparent_rx, NULL,
		WPAN_CLUST_FLAG_INOUT | WPAN_CLUST_FLAG_NOT_ZCL },
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
   printf(" ATxx     (Issue AT command - local or remote)\n");
   printf(" xxx...   (Data to send to selected target - remote only)\n");
   printf(" $ nnn/ccc (Send test string of length nnn, chunk size ccc)\n");
   sxa_select_help();
   printf("\n");
   printf(" menu     (print this command list)\n");
   printf(" quit     (quit this demo)\n");
}

char qbf[] =
{
	"The quick brown fox jumps over the lazy dog. 012345678901234567\n"
	"The quick brown fox jumps over the lazy dog. 012345678901234567\n"
	"The quick brown fox jumps over the lazy dog. 012345678901234567\n"
	"The lazy black dog catches the fatigued fox. 012345678901234567\n"
};


/*
	main

	Initiate communication with the XBee module, then accept AT and
   other commands from STDIO, pass them to the XBee module via the
   SXA library, and print the results.
*/
int main( void)
{
   char cmdstr[80];
   char * slash;
   int ionum;
   int msglen;
   int msgsent;
   int status;
   int chunksize;

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
      	if (!have_target)
         {
      		printf("Need a remote target to send data\n");
         }
         else
         {
	         msglen = atoi(cmdstr+1);
            slash = strchr(cmdstr, '/');
            if (slash)
            {
            	chunksize = atoi(slash + 1);
               if (chunksize > sizeof(qbf) || chunksize < 1) chunksize = sizeof(qbf);
            }
            else
            {
            	chunksize = 64;
            }
            msgsent = 0;
	         while (msglen >= chunksize)
	         {
					status = send_data( sxa, qbf, chunksize);
               if (status)
               {
               	printf("Error sending %d bytes: status=%d\n", chunksize, status);
               }
               else
               {
               	msgsent += chunksize;
               }
               msglen -= chunksize;
	         }
            if (msglen)
            {
					status = send_data( sxa, qbf, msglen);
               if (status)
               {
               	printf("Error sending %d bytes: status=%d\n", msglen, status);
               }
               else
               {
               	msgsent += msglen;
               }
            }
            printf("Total %d sent\n", msgsent);
         }
      }
      else if (!strncasecmp(cmdstr, "at", 2))
      {
			process_command_remote( sxa_xbee( sxa), cmdstr,
         	have_target ? &target_ieee : NULL);
	   }
      else if (have_target)
      {
      	msglen = strlen(cmdstr);
	   	printf( "sending %u bytes to '%ls'\n", msglen, sxa->id.node_info);
	   	status = send_data( sxa, cmdstr, msglen);
         if (status)
         {
         	printf("Error sending %d bytes: status=%d\n", msglen, status);
         }
      }
      else
      {
      	printf("Need a remote target to send data\n");
      }
   }
}


#ifdef SHOW_STATUS
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
		case XBEE_TX_DISCOVERY_ADDRESS | XBEE_TX_DISCOVERY_ROUTE:
      	return "ADDR+ROUTE";
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
#endif // SHOW_STATUS


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
#ifdef SHOW_STATUS
	{ XBEE_FRAME_MODEM_STATUS, 0, my_modem_status, NULL },
   { XBEE_FRAME_TRANSMIT_STATUS, 0, my_handle_transmit_status, NULL },
#endif
	XBEE_FRAME_TABLE_END
};




