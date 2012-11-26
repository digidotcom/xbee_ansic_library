/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/***************************************************************************

   SXA-socket.c

   This sample shows use of the Simple XBee API library, to discover
   remote XBee nodes and send data streams back and forth, using a reliable
   TCP socket-based protocol.


   Principle of operation

	This sample uses a custom ZigBee endpoint to implement a protocol
   similar to TCP/IP.  Since a custom endpoint is used, this will not
   work with non-XBee devices, and an external processor is required to
   handle the details of the protocol.  Thus, you will need to use
   (say) an RCM4510 on each end.

   For a simpler technique, which does not require a Rabbit processor on
   both ends, but is less robust, see the sample SXA-stream.c.

   The library uses TCP/IP sockets, and hence all of the socket API may be
   used, including lookahead, keepalives and even using TLS.
   Instead of using IP (Internet Protocol) as a network layer, a point-to-
   point ZigBee link is used.  The normal TCP and IP headers are replaced
   with sufficient header information to reliably get each data segment to
   its destination over the ZigBee stack.

   In order to open the socket, a "localhost" IP address is used as the
   destination.  The address looks like this:
   	127.1.x.y
   Where the initial '127' indicates a localhost address, the '1'
   indicates TCP over XBee, and 'x.y' indicates the target peer device.
   x.y encodes a 16-bit number, which is the index into the table of
   nodes discovered by SXA.  Usually, there will be far fewer nodes than this
   (maybe a dozen or so) so most of the time 'x' will be zero and y will be
   the non-zero index.  (x.y cannot be 0.0, since the zero entry is always the
   local XBee device, which cannot be used.  For a true loopback device,
   always use 127.0.0.0).  Note that the SXA_INDEX_TO_IP macro should
   be sued to generate these IP addresses.

   Currently, both source and destination port numbers must be in the
   range 61616..61871 inclusive.  It is possible to have multiple sockets
   open between the same pair of devices, although this sample shows only
   one socket, and the source and destination port numbers are always
   SXA_STREAM_PORT (61616).  Use the macros SXA_STREAM_PORT and
   SXA_MAX_STREAM_PORT (61871) to define the allowable port range.



   Further considerations for applications:

   This sample demonstrates both passive (listening) and active connections.
   Most real applications would choose one or the other.  For example, a
   data concentrator may actively open multiple sockets to sensor-equipped
   devices.  Each sensor device would have a passively opened socket, waiting
   for connections from the data concentrator.

   If a device is powered down, it is the nature of TCP that the connection
   may remain "open" since the remaining "live" end of the connection may not
   discover that the other end has terminated.  This sample demonstrates
   use of TCP keepalives in order to periodically test the connection, and
   gracefully close it if one of the peers has terminated unexpectedly.
   With Internet TCP, keepalives are normally sent infrequently to avoid
   undue amounts of traffic.  With this ZigBee-based network, keepalives
   affect only the local area.


   Expected throughput:

   ZigBee is not noted for high throughput.  If you use this sample to
   send a long data stream (e.g. use the "$100000" command) then the
   throughput will be about 1500 bytes per second (12,000 bits per second).


   To use:

   It is recommended to first get the SXA-command sample working in order to
   become familiar with project file settings and ensure that basic
   XBee functionality is accessible.

	Set up a network with one Rabbit+XBee configured as the coordinator.  Set
	up one or more other XBee+Rabbit modules configured as routers and joined
   to your coordinator.  All boards must run this sample code, and have
   the same extended PAN ID: use the ATID command as required, with
   the same number for each board.  If desired, save the ID in non-volatile
   storage on the XBee module using ATWR.

   Compile this sample to all Rabbit+XBee boards.  If not one of
   the standard boards which support XBee (RCM45xxW, BL4S100 etc.)
   then you will need to define a set of configuration macros.  See
   xbee_config.h for details.

   All but one of the boards should be connected to an ordinary terminal
   using the "diagnostic" connector on the programming cable.  The other
   board may use the Dynamic C stdio console.

   Reset the boards connected to the terminals.  All boards will default
   to listening passively for connections, including the board connected
   to Dynamic C.

   Typing "menu" will redisplay the menu of commands.  Hit '?' to get
   a list of target nodes which have been discovered.  Enter ATND on the
   console to get each board to rediscover each of its neighbors.  All
   neighbors with matching PAN IDs should be discovered.  [ATND can take
   about 6 seconds to discover all nodes, so quickly entering '?' after
   ATND might not show a complete list.]

   Actively open a socket (TCP connection) to another device in the
   discovered device list using the "@N" command.  '?' will list all
   devices, from which you select N as the appropriate index number 1..9.
   Zero is not allowed, since it is always the local device.
   Just entering '@' with no digit will set up the socket in passive
   listen mode.

   AT commands can be issued to the local or remote node.  For example,
   ATND to the local node will cause another round of node discovery, and
   ATD04 will set the local (or remote) target XBee module I/O (DIO0) to a
   low output.  Set the remote or local target for AT commands using the
   '>' command (which is independently selected with respect to the
   '@' command setting).

	Type a message (other than one of the above commands) on the Rabbit and
   it will go out to the selected node.  The receiving node will print the
   number of bytes received.

	You can send large numbers of bytes using the '$' command.

   You can test the action of keepalives by establishing a connection
   between two devices, then resetting one of the devices.  The device
   which was not reset will automatically close the connection after a
   few seconds.

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
      DCRTCP_DEBUG

*****************************************************************************/

#define TCPCONFIG		15				// Loopback interface required
#define LOOPBACK_HANDLERS 2		// Minimum required value for TCP over ZigBee

// Definitions for automatically detecting when peer has crashed or
// rebooted etc.
#define KEEPALIVE_WAITTIME 2		// Seconds to wait after initial timeout
#define KEEPALIVE_NUMRETRYS 3		// Number of retries


#memmap xmem

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

// Needs to be included first for TCP over ZigBee
#use "dcrtcp.lib"

// Main header required for Simple XBee API TCP sockets over ZigBee
#include "xbee/sxa_socket.h"

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

	/* Required endpoint for SXA socket */
	SXA_SOCKET_ENDPOINT,

	{ WPAN_ENDPOINT_END_OF_LIST }
};


void help(void)
{
   printf("Commands and data:\n");
   printf("  ATxx [parm] (Issue AT command - local or remote)\n");
   printf("  xxx...   (Send data over stream socket)\n");
   printf("  $ nnn    (Send test string of length nnn)\n");
   sxa_select_help();
   printf("Stream sockets:\n");
   printf("  @        (open socket for listening (initial default))\n");
   printf("  @ N      (open socket actively to specified node table index)\n");
   printf("  .        (close socket)\n");
   printf("Other:\n");
   printf("  menu     (print this command list)\n");
   printf("  quit     (quit this demo)\n");
}

char qbf[] =
{
	"The quick brown fox jumps over the lazy dog. 012345678901234567\n"
	"The quick brown fox jumps over the lazy dog. 012345678901234567\n"
	"The quick brown fox jumps over the lazy dog. 012345678901234567\n"
	"The lazy black dog catches the fatigued fox. 012345678901234567\n"
};

// The TCP socket to use.  Must be static.
tcp_Socket sock;

// Current state, also static.
enum {
	LISTEN,
   OPENING,
   ESTABLISHED_PASSIVE,
   ESTABLISHED_ACTIVE,
   CLOSING_PASSIVE,
   CLOSING_ACTIVE,
   CLOSED
} state = CLOSED;


int passive_open(void)
{
	sock_abort(&sock);

   printf("@@@ Passively opening socket\n");
   state = LISTEN;
   return tcp_extlisten(
   	&sock, IF_LOOPBACK, SXA_STREAM_PORT, 0, SXA_STREAM_PORT,
      NULL, 0, 0, 0);
}

int active_open(int sxa_index)
{
	sock_abort(&sock);
   printf("@@@ Opening socket to index %d\n", sxa_index);
	state = OPENING;
   // Use the SXA_INDEX_TO_IP macro to get the correct loopback IP address
   // for connecting to the device with the given SXA index.
   return tcp_extopen(
   	&sock, IF_LOOPBACK, SXA_STREAM_PORT, SXA_INDEX_TO_IP(sxa_index), SXA_STREAM_PORT,
      NULL, 0, 0);
}

int close(void)
{
   printf("@@@ Closing socket by command\n");
	sock_close(&sock);
   state = CLOSING_ACTIVE;
}

// This sample requires this state machine to be called frequently.
void state_machine(void)
{
	int rd;

	tcp_tick(NULL);
   switch (state)
   {
   case LISTEN:
   	if (!sock_waiting(&sock))
      {
   		printf("@@@ Listening socket established\n");
         tcp_keepalive(&sock, 5);
      	state = ESTABLISHED_PASSIVE;
      }
      break;
   case OPENING:
   	if (!sock_waiting(&sock))
      {
   		printf("@@@ Socket established\n");
         tcp_keepalive(&sock, 5);
      	state = ESTABLISHED_ACTIVE;
      }
      break;
   case ESTABLISHED_PASSIVE:
   	if (!sock_alive(&sock))
      {
   		printf("@@@ Listening socket reset\n");
         passive_open();
      }
   	else if (!sock_readable(&sock))
      {
   		printf("@@@ Listening socket closing\n");
      	sock_close(&sock);
      	state = CLOSING_PASSIVE;
      }
      break;
   case ESTABLISHED_ACTIVE:
   	if (!sock_alive(&sock))
      {
   		printf("@@@ Socket reset\n");
         state = CLOSED;
      }
   	else if (!sock_readable(&sock))
      {
   		printf("@@@ Socket closing\n");
      	sock_close(&sock);
      	state = CLOSING_ACTIVE;
      }
      break;
   case CLOSING_PASSIVE:
   	if (!sock_alive(&sock))
      {
   		printf("@@@ Listening socket closed\n");
      	passive_open();
      }
      break;
   case CLOSING_ACTIVE:
   	if (!sock_alive(&sock))
      {
   		printf("@@@ Socket closed\n");
      	state = CLOSED;
      }
      break;
   }

   if (state == ESTABLISHED_PASSIVE ||
       state == ESTABLISHED_ACTIVE)
   {
		rd = sock_readable(&sock);
      if (rd > 1)
      {
      	--rd;
         printf("@@@ got %d bytes\n", rd);
         sock_fastread(&sock, NULL, rd);
      }
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
   int ionum;
   long msglen;
   long msgsent;
   int status;
   long offs;
   long amt;
   long T;

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

   // Required for SXA sockets
   // This initializes the main networking stack.  It will be fairly
	// minimal if there are no other network devices.
   sock_init_or_exit(1);

   // Required for SXA sockets
   // Initialize the SXA socket handlers etc.
   sxa_socket_init();

   // Default to opening socket passively
   passive_open();

   help();

   while (1)
   {
   	// Accumulate a command string, and call the tick function while waiting.
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
   		sxa_tick();
   		state_machine();
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
      else if ( cmdstr[0] == '@')
      {
      	if ( isdigit(cmdstr[1]))
         {
         	active_open(cmdstr[1] - '0');
         }
         else
         {
         	passive_open();
         }
      }
      else if ( cmdstr[0] == '.')
      {
      	close();
      }
      else if ( !strncasecmp(cmdstr, "at", 2))
      {
			process_command_remote( sxa_xbee( sxa), cmdstr,
         	have_target ? &target_ieee : NULL);
	   }
      else if ( cmdstr[0] == '$')
      {
      	if (state != ESTABLISHED_PASSIVE &&
               state != ESTABLISHED_ACTIVE)
         {
      		printf("Socket is not open for sending data\n");
         }
         else
         {
	         msglen = strtol(cmdstr+1, NULL, 0);

            status = 0;
            msgsent = 0;
            T = MS_TIMER;
            while (msgsent < msglen)
            {
	            // While sending long amounts of data, it is imperative to
	            // call the tick functions, otherwise application may lock up.
	            sxa_tick();
	            state_machine();

               // Send the data, as much as the socket will accept
               offs = msgsent % sizeof(qbf);
               amt = sizeof(qbf) - offs;
               if (amt > msglen - msgsent)
               	amt = msglen - msgsent;
               status = sock_fastwrite( &sock, qbf + (int)offs, (int)amt);
               if (status < 0)
               	break;
               msgsent += status;

            }
            T = MS_TIMER - T;
            if (status < 0)
            {
               printf("Error: status=%d\n", status);
            }
            printf("Total %ld sent out of %ld in %ldms\n", msgsent, msglen, T);
         }
      }
      else if (state == ESTABLISHED_PASSIVE ||
               state == ESTABLISHED_ACTIVE)
      {
      	msglen = strlen(cmdstr);
	   	printf( "sending %ld bytes to '%ls'\n", msglen, sxa->id.node_info);
	   	status = sock_fastwrite( &sock, cmdstr, (int)msglen);
         if (status < (int)msglen)
         {
         	printf("Error: sock_fastwrite status=%d\n", status);
         }
      }
      else
      {
      	printf("Socket is not open for sending data\n");
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




