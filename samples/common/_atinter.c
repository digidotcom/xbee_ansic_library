/*
 * Copyright (c) 2008-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"

#include "_atinter.h"

/* ------------------------------------------------------
	parseParameter

	Parse the parameter entered by the user and associate it with the provided
	request handle.

	Valid parameter formats:

	[AT]CC 0xXXXXXX (where XXXXXX are valid hexadecimal characters)
	[AT]CC YYYY (where YYYY is an integer, up to 32 bits)
	[AT]NI "Node ID String" (where quotes contain string data)

	Parameter 1: string to parse
	Parameter 2: request handle to associate parameter to

	Return value: -EINVAL if unable to parse parameter or handle is invalid
						0 if successful
*/
int parseParameter (const char *paramstr, int16_t request)
{
	unsigned long temp;
	int16_t j;
	uint8_t length;
	char param[XBEE_CMD_MAX_PARAM_LENGTH];
	char *q;

	// skip leading spaces
	while (isspace( (unsigned char)*paramstr))
	{
		++paramstr;
	}

	if (! *paramstr)
	{
		return 0;			// no parameter
	}

	if (paramstr[0] == '"')
	{
		// parse string (for Node ID)
		paramstr++;
		q = strchr( paramstr, '"');
		if (q)
		{
			length = (uint8_t) (q - paramstr);
			return xbee_cmd_set_param_bytes( request, paramstr, length);
		}
		else
		{
			puts( "Missing closing quote (\") in parameter.");
			return -EINVAL;
		}
	}
	else if (paramstr[0] == '0'
		&& tolower( (unsigned char)paramstr[1]) == 'x')
	{
		// parse hex
		length = 0;
		paramstr += 2;			// point to start of hex
		q = param;
		while (*paramstr && length < sizeof(param))
		{
			j = hexstrtobyte( paramstr);
			if (j < 0)
			{
				puts( "error parsing hex string");
				return -EINVAL;
			}
			*q++ = (uint8_t) j;
			paramstr += 2;
			++length;
		}
		return xbee_cmd_set_param_bytes( request, param, length);
	}

	// parse integer
	temp = strtoul( paramstr, NULL, 10);
	return xbee_cmd_set_param( request, temp);

} //parseParameter()
/*
	xbee_cmd_callback

	Function called by XBee driver upon receipt of an AT Command Reponse.  Use
	the fields of the response to display the result of the command to the user.
*/
int xbee_cmd_callback( const xbee_cmd_response_t FAR *response)
{
	bool_t printable;
	uint_fast8_t length, i;
	uint8_t status;
	const uint8_t FAR *p;

	printf( "AT%.*" PRIsFAR " ", 2, response->command.str);

	if (response->flags & XBEE_CMD_RESP_FLAG_TIMEOUT)
	{
		puts( "(timed out)");
		return XBEE_ATCMD_DONE;
	}

	status = response->flags & XBEE_CMD_RESP_MASK_STATUS;
	if (status != XBEE_AT_RESP_SUCCESS)
	{
		printf( "(error: %s)\n",
			(status == XBEE_AT_RESP_ERROR) ? "general" :
			(status == XBEE_AT_RESP_BAD_COMMAND) ? "bad command" :
			(status == XBEE_AT_RESP_BAD_PARAMETER) ? "bad parameter" :
			(status == XBEE_AT_RESP_TX_FAIL) ? "Tx failure" :
														  "unknown error");
		return XBEE_ATCMD_DONE;
	}

	length = response->value_length;
	if (! length)		// command sent successfully, no value to report
	{
		puts( "(success)");
		return XBEE_ATCMD_DONE;
	}

	// check to see if we can print the value out as a string
	printable = 1;
	p = response->value_bytes;
	for (i = length; i; ++p, --i)
	{
		if (! isprint( *p))
		{
			printable = 0;
			break;
		}
	}

	if (printable)
	{
		printf( "= \"%.*" PRIsFAR "\" ", length, response->value_bytes);
	}
	if (length <= 4)
	{
		// format hex string with (2 * number of bytes in value) leading zeros
		printf( "= 0x%0*" PRIX32 " (%" PRIu32 ")\n", length * 2, response->value,
			response->value);
	}
	else
	{
		printf( "= %d bytes:\n", length);
		hex_dump( response->value_bytes, length, HEX_DUMP_FLAG_TAB);
	}

	return XBEE_ATCMD_DONE;
}

void process_command_remote( xbee_dev_t *xbee, const char *cmdstr,
			const addr64 FAR *ieee)
{
   char cmdptr[2];
   const char *param;
	int16_t request;

	if (! *cmdstr)
	{
		puts( "Enter an AT command");
		return;
	}

	param = cmdstr;
	do {
		// convert command to upper case
		cmdptr[0] = (char) toupper( (unsigned char)param[0]);
		cmdptr[1] = (char) toupper( (unsigned char)param[1]);
		param += 2;			// advance beyond two-letter command
	} while (memcmp( cmdptr, "AT", 2) == 0);		// skip leading AT

	request = xbee_cmd_create( xbee, cmdptr);
	if (request < 0)
	{
		// Note that strerror() expects the positive error value
		// (what would have been stored in errno) so we have to
		// negate the xbee_cmd_create() return value.
      printf( "Error creating request: %d (%" PRIsFAR ") \n",
      	request, strerror( -request));
	}
	else
	{
		// allow for "ATXX=1234" syntax
		if (*param == '=' || *param == ' ')
		{
			++param;
		}
      if (parseParameter( param, request) == 0)
		{
      	if (ieee)
         {
      		xbee_cmd_set_target( request, ieee, WPAN_NET_ADDR_UNDEFINED);
         }
			xbee_cmd_set_callback( request, xbee_cmd_callback, NULL);
			xbee_cmd_send( request);
		}
	}
}


void process_command( xbee_dev_t *xbee, const char *cmdstr)
{
	process_command_remote(xbee, cmdstr, NULL);
}




// ------------------------------------------------------
//
// AT Commands
//
// The following struct and table define the AT commands we'll be using.

const command_t command_list[] =
{
	{ "HV",	OUTPUT_HEX | FLAG_ANYDEVICE,
				"Hardware Version",
				"Read modem hardware verison number." },

	{ "VR",	OUTPUT_HEX | FLAG_ANYDEVICE,
				"Firmware Version",
				"Read modem firmware version number." },

	{ "VL",	OUTPUT_STRING | FLAG_DIGIMESH900,
				"Version (Long)",
				"Read detailed version information, including build date/time." },

	// Networking Registers, ZNet/ZB

	{ "ID",	OUTPUT_HEX | FLAG_ZNET | FLAG_ROUTER,
				"PAN ID",
				"Set or read the PAN ID.  If you set the ID, you must write it\n" \
				"\tto non-volatile memory (\"WR\") and then reset the network\n" \
				"\tsoftware (\"NR\").  (0-0x3FFF or 0xFFFF for random)" },

	{ "ID",	OUTPUT_HEX | FLAG_ZB | FLAG_SE | FLAG_ROUTER,
				"PAN ID",
				"Set or read the PAN ID.  If you set the ID, you must write it\n" \
				"\tto non-volatile memory (\"WR\") and then reset the network\n" \
				"\tsoftware (\"NR\").  (any 64-bit value, 0 for random)" },

	{ "SC",	OUTPUT_HEX | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Scan Channels",
				"Set or read the list of channels to scan when joining or\n" \
				"\tsetting up a network."  },

	{ "CH",	OUTPUT_HEX | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Operating Channel",
				"Read the current channel.  Will be zero if we are not\n" \
				"\tassociated with a network." },

	{ "OP",	OUTPUT_HEX | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Operating PAN ID",
				"Read the operating PAN ID." },

	{ "OI",	OUTPUT_HEX | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Operating 16-bit PAN ID",
				"Read the operating 16-bit PAN ID (ZigBee networks)." },

	// Networking Registers, DigiMesh

	{ "ID",	OUTPUT_HEX | FLAG_DIGIMESH900,
				"Network ID",
				"Set or read the Network ID.  If you set the ID, you must\n" \
				"\twrite it to non-volatile memory (\"WR\") and then reset the\n" \
				"\tnetwork software (\"NR\").  (0-0x7FFF or 0xFFFF for factory)" },

	{ "HP",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Channel Hopping Pattern",
				"Set or read which of 8 hopping patterns (0-7) is used.  All\n" \
				"\tmodules on a network must have the same hop pattern (HP) and\n" \
				"\tnetwork identifier (ID).\n" },

	// Networking Registers, All Protocols

	{ "BH",	OUTPUT_DECIMAL | FLAG_ANYDEVICE,
				"Broadcast Radius",
				"Set or read the maximum number of broadcast hops." },

	// Addressing

	{ "SH",	OUTPUT_HEX | FLAG_ANYDEVICE,
				"Serial Number (High)",
				"Read the upper half of the 64-bit IEEE address." },

	{ "SL",	OUTPUT_HEX | FLAG_ANYDEVICE,
				"Serial Number (Low)",
				"Read the lower half of the 64-bit IEEE address." },

	{ "MY",	OUTPUT_HEX | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Network Address",
				"Read the 16-bit network address.  Will be 0xFFFE if not\n" \
				"\tassociated with a network." },

	{ "MP",	OUTPUT_HEX | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ENDDEV,
				"Parent Address",
				"Read the 16-bit network address of our parent node."},

	{ "NI",	OUTPUT_STRING | (FLAG_ANYDEVICE ^ FLAG_SE),
				"Node Identifier",
				"Set or read the node identifier (up to 20 characters)." },

	{ "NT",	OUTPUT_DECIMAL | FLAG_ANYDEVICE,
				"Node Discovery Timeout",
				"Set or read the node discovery timeout (0-255 x 100ms)." },

	{ "SD",	OUTPUT_DECIMAL | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Scan Duration",
				"Set or read the channel scan duration (0-7)." },

	{ "NJ",	OUTPUT_DECIMAL | FLAG_ZNET | FLAG_ZB | FLAG_SE | FLAG_ANYNODETYPE,
				"Node Join Time",
				"Set or read the node join time (0-255 seconds)." },

	{ "JV",	OUTPUT_DECIMAL | FLAG_ZNET | FLAG_ZB | FLAG_ROUTER,
				"Channel Verification",
				"Set or read the channel verification flag.  If enabled (1), a\n" \
				"\trouter will verify a coordinator exists on the same channel\n" \
				"\tafter power cycling." },

	// Sleep Commands

	{ "SM",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Sleep Mode",
				"Set or read the Sleep Mode setting (0, 7 or 8)." },

	{ "SO",	OUTPUT_HEX | FLAG_DIGIMESH900,
				"Sleep Options",
				"Set or read the Sleep Options setting (bitfield)." },

	{ "ST",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Wake Time",
				"Set or read the Wake Time setting (milliseconds)." },

	{ "OW",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Operational Wake Time",
				"Read the Wake Time (in millesconds) that the node is currently\n" \
				"\tusing.  May be different than ST if synchronized with a\n" \
				"\tsleeping router network." },

	{ "SP",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Sleep Period",
				"Set or read the Sleep Period setting (units of 10ms)." },

	{ "OS",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Operational Sleep Period",
				"Read the Sleep Period (units of 10ms) that the node is\n" \
				"\tcurrently using.  May be different than SP if synchronized\n" \
				"\twith a sleeping router network." },

	{ "SS",	OUTPUT_HEX | FLAG_DIGIMESH900,
				"Sleep Status",
				"Read the Sleep Status bitfield." },

	// Diagnostic Commands

	{ "AI",	OUTPUT_HEX | FLAG_ANYDEVICE,
				"Assocation Indicator",
				"Read the association progress indicator (0 = associated)." },

	{ "NP",	OUTPUT_DECIMAL | FLAG_ANYDEVICE,
				"Network Payload",
				"Read the maximum number of RF payload bytes that can be sent\n" \
				"\tin a single transmission." },

	{ "%V",	OUTPUT_DECIMAL | FLAG_ANYDEVICE,
				"Supply Voltage",
				"Read voltage on Vcc pin in mV." },

	{ "TP",	OUTPUT_DECIMAL | FLAG_DIGIMESH900,
				"Temperature",
				"Read module temperature (in Celsius)." },

};
#define COMMAND_COUNT (sizeof(command_list)/sizeof(command_list[0]))
const int command_count = COMMAND_COUNT;

// ------------------------------------------------------
// printATcommands
//
// Prints out a partial list of the AT commands available.  Requires long
// strings, stored in an array of "commands" struct.

void printATCmds( xbee_dev_t *xbee)
{
   int i;
   int flags;
	const command_t *command;

	// Set flags to match commands from the command list that are valid for
	// this XBee module.
   flags = 0;

	// This logic needs to move into the library somewhere.  Not sure of exact
	// location (wpan?  xbee?)
   switch (xbee->hardware_version & XBEE_HARDWARE_MASK)
   {
   	case XBEE_HARDWARE_900_PRO:
   		switch ((unsigned) (xbee->firmware_version & XBEE_PROTOCOL_MASK))
   		{
   			case XBEE_PROTOCOL_DIGIMESH:
					flags = FLAG_DIGIMESH900;
					break;

				default:
					printf( "Only DigiMesh supported on this hardware.\n");
					break;
			}
			break;

		case XBEE_HARDWARE_S2:
		case XBEE_HARDWARE_S2_PRO:
		case XBEE_HARDWARE_S2B_PRO:
			switch ((unsigned) (xbee->firmware_version & XBEE_PROTOCOL_MASK))
			{
				case XBEE_PROTOCOL_ZNET:			flags |= FLAG_ZNET;		break;
				case XBEE_PROTOCOL_ZB:				flags |= FLAG_ZB;			break;
				case XBEE_PROTOCOL_SMARTENERGY:	flags |= FLAG_SE;			break;
				default:
					printf( "Unknown protocol for firmware version 0x%04x.\n",
						xbee->firmware_version);
					break;
			}
			switch ((unsigned) (xbee->firmware_version & XBEE_NODETYPE_MASK))
			{
				case XBEE_NODETYPE_COORD:	flags |= FLAG_COORDINATOR;		break;
				case XBEE_NODETYPE_ROUTER:	flags |= FLAG_ROUTER;			break;
				case XBEE_NODETYPE_ENDDEV:	flags |= FLAG_ENDDEV;			break;
				default:
					printf( "Unknown node type for firmware version 0x%04x.\n",
						xbee->firmware_version);
					break;
			}
			break;
   }

	if (! flags)
	{
		printf( "Can't display menu for this hardware/firmware combination.\n");
	}
	else
	{
		printf( "Command\tDescription\n===================\n");
		for (i = 0, command = command_list; i < COMMAND_COUNT; ++i, ++command)
		{
			// must match firmware and node type
			if (flags == (command->flags & flags))
			{
				// only display commands that are valid for this XBee
				printf( "  %s\t%s\n", command->command, command->desc);
			}
   	}
	}

	printf( "\n\n");
	printf( "Valid command formats (AT prefix is optional, CC is command):\n\n");
	printf( " [AT]CC 0xXXXXXX (where XXXXXX is an even number of " \
		"hexadecimal characters)\n");
	printf( " [AT]CC YYYY (where YYYY is an integer, up to 32 bits)\n");
	printf( " [AT]NI \"Node ID String\" (where quotes contain string data)\n\n");
}


