/*
 * Copyright (c) 2007-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

// NOTE: add -D HANDHELD to compiler options when compiling for
// hand-held meter reader (with 24 x 16 display)

#include <stdio.h>
#include <stdlib.h>
#include "../common/_atinter.h"

xbee_dev_t my_xbee;

// ------------------------------------------------------
// printATcommands
//
// Prints out a partial list of the AT commands available.
// Custom version of generic printATCmds() from common/_atinter.c

void print_commands(void)
{
   int i;
   int flags;
	const command_t *command;
#ifdef HANDHELD
	int row, col;
#endif

	// Set flags to match commands from the command list that are valid for
	// this XBee module.
   flags = 0;

	// This logic needs to move into the library somewhere.  Not sure of exact
	// location (wpan?  xbee?)
   switch (my_xbee.hardware_version & XBEE_HARDWARE_MASK)
   {
   	case XBEE_HARDWARE_900_PRO:
   		switch ((unsigned) (my_xbee.firmware_version & XBEE_PROTOCOL_MASK))
   		{
   			case XBEE_PROTOCOL_DIGIMESH:
					flags = FLAG_DIGIMESH900;
					break;

				default:
					printf( "Only DigiMesh supported on this hardware.\n");
			}
			break;

		case XBEE_HARDWARE_S2:
		case XBEE_HARDWARE_S2_PRO:
			switch ((unsigned) (my_xbee.firmware_version & XBEE_PROTOCOL_MASK))
			{
				case XBEE_PROTOCOL_ZNET:	flags |= FLAG_ZNET;				break;
				case XBEE_PROTOCOL_ZB:		flags |= FLAG_ZB;					break;
				default:
					printf( "Unknown protocol for firmware version 0x%04x.\n",
						my_xbee.firmware_version);
			}
			switch ((unsigned) (my_xbee.firmware_version & XBEE_NODETYPE_MASK))
			{
				case XBEE_NODETYPE_COORD:	flags |= FLAG_COORDINATOR;		break;
				case XBEE_NODETYPE_ROUTER:	flags |= FLAG_ROUTER;			break;
				case XBEE_NODETYPE_ENDDEV:	flags |= FLAG_ENDDEV;			break;
				default:
					printf( "Unknown node type for firmware version 0x%04x.\n",
						my_xbee.firmware_version);
			}
			break;
   }

	if (! flags)
	{
		printf( "Can't display menu for this hardware/firmware combination.\n");
	}
	else
	{
#ifdef HANDHELD
#else
		printf( "Command\tDescription\n===================\n");
		for (i = 0, command = command_list; i < command_count; ++i, ++command)
		{
			// must match firmware and node type
			if (flags == (command->flags & flags))
			{
				// only display commands that are valid for this XBee
				printf( "  %s\t%s\n", command->command, command->desc);
			}
   	}
#endif
	}

#ifdef HANDHELD
	printf( "\nValid command formats:\n");
	printf( " [AT]CC 0xXXXXXX (hex)\n");
	printf( " [AT]CC YYYY     (dec)\n");
	printf( " [AT]NI \"Node ID Str\"\n");
	row = 4;
	col = 0;
	if (flags)
	{
		printf( "Cmds: ");
		col = 6;
		for (i = 0, command = command_list; i < command_count; ++i, ++command)
		{

			// must match firmware and node type
			if (flags == (command->flags & flags))
			{
				// only display commands that are valid for this XBee
				printf( " %s", command->command);
				col += 3;
				if (col > 20)
				{
					printf("\n");
					++row;
					col = 0;
				}
			}
   	}
		if (col)
		{
			printf("\n");
			++row;
		}
	}
#else
	printf( "\n\n");
	printf( "Valid command formats (AT prefix is optional, CC is command):\n\n");
	printf( " [AT]CC 0xXXXXXX (where XXXXXX is an even number of " \
		"hexadecimal characters)\n");
	printf( " [AT]CC YYYY (where YYYY is an integer, up to 32 bits)\n");
	printf( " [AT]NI \"Node ID String\" (where quotes contain string data)\n\n");
#endif
}

/*
	main

	Initiate communication with the XBee module, then accept AT commands from
	STDIO, pass them to the XBee module and print the result.
*/

#include "xbee/byteorder.h"

void parse_arguments( int argc, char *argv[], xbee_serial_t *xbee)
{
	int i;
	uint32_t baud;
	#ifndef HANDHELD
		char buffer[80];
	#endif

	memset( xbee, 0, sizeof *xbee);

	// default baud rate
	xbee->baudrate = 115200;

	for (i = 1; i < argc; ++i)
	{
		if (strncmp( argv[i], "COM", 3) == 0)
		{
			xbee->comport = (int) strtoul( argv[i] + 3, NULL, 10);
		}
		if ( (baud = (uint32_t) strtoul( argv[i], NULL, 0)) > 0)
		{
			xbee->baudrate = baud;
		}
	}

	while (xbee->comport == 0)
	{
#ifdef HANDHELD
		xbee->comport = 1;
#else
		printf( "Connect to which COMPORT? ");
		fgets( buffer, sizeof buffer, stdin);
		xbee->comport = (int) strtoul( buffer, NULL, 10);
#endif
	}
}

int main( int argc, char *argv[])
{
   char cmdstr[80];
	int status;
	xbee_serial_t XBEE_SERPORT;

	parse_arguments( argc, argv, &XBEE_SERPORT);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize device.\n");
		return 0;
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
	if (status)
	{
		printf( "Error %d waiting for query to complete.\n", status);
	}

	// report on the settings
	xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

   print_commands();

   while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	xbee_dev_tick( &my_xbee);
      }

		if (! strncmp( cmdstr, "menu", 4))
      {
			clrscr();
      	print_commands();
      }
      else if (! strcmp( cmdstr, "quit"))
      {
			return 0;
		}
      else
      {
			process_command( &my_xbee, cmdstr);
	   }
   }
}

// Since we're not using a dynamic frame dispatch table, we need to define
// it here.
#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
#include "xbee/device.h"		// for XBEE_FRAME_HANDLE_TX_STATUS
#include "xbee/wpan.h"			// for XBEE_FRAME_HANDLE_RX_EXPLICIT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_TABLE_END
};

