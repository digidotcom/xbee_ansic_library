/*
 * Copyright (c) 2010-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */
/*
	Testbed for new xbee param interface

*/

#define XBEE_DEVICE_VERBOSE
#define XBEE_SERIAL_VERBOSE

#define XBEE_SERIAL_DEBUG
#define XBEE_DEVICE_DEBUG

#include <xbee/atcmd.h>

#use "xbee_driver_config.lib"

xbee_dev_t my_xbee;

int xbee_cmd_callback_sample( const xbee_cmd_response_t FAR *response)
{
	printf( "response to request 0x%04x: AT%.*ls = ", response->handle,
		2, response->command.str);
	if (response->value_length <= 4)
	{
		printf( "0x%lX (%lu)\n", response->value, response->value);
	}
	else
	{
		printf( "\"%.*ls\"\n", response->value_length, response->value_bytes);
	}

	return XBEE_ATCMD_DONE;
}


char *commands[] = { "VR", "ID", "OI" };

void test_api()
{
	int request;
	int i;

	for (i = 0; i < sizeof(commands)/sizeof(*commands); ++i)
	{
		// Read VR from local XBee
		request = xbee_cmd_create( &my_xbee, commands[i]);
		xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
		printf( "sending request 0x%04x: %s to local XBee\n", request,
			commands[i]);
		xbee_cmd_send( request);
	}

/*
	// Read NI from remote XBee (using IEEE address)
	request = xbee_cmd_create( &my_xbee, "NI");
	xbee_cmd_set_target( request,
		(addr64 *)"\x00\x13\xa2\x00\x40\x0a\x01\x17", WPAN_NET_ADDR_UNDEFINED);
	xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	printf( "sending request 0x%04x: %s\n", request, "NI to 40-0a-01-17");
	xbee_cmd_send( request);
*/
	// Set NI on local XBee
	request = xbee_cmd_create( &my_xbee, "NI");
	xbee_cmd_set_param_str( request, "Jumping Wizards");
	xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	printf( "sending request 0x%04x: %s\n", request, "set NI on local");
	xbee_cmd_send( request);
/*
	// Read NI from local XBee
	request = xbee_cmd_create( &my_xbee, "NI");
	xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	printf( "sending request 0x%04x: %s\n", request, "read NI from local");
	xbee_cmd_send( request);
*/
/*
	// Blocking request with timeout
	result = xbee_cmd_send_blocking( request, &response, timeout);
*/
}


void main(void)
{
	char buffer[80];
	unsigned long t;
	int i, ch;

	int request;

	#if _BOARD_TYPE_ == RCM4510W
		// on 4510W, enable the chip select
		_xb_cs(1);
	#endif

	xbee_dev_init( &my_xbee, XBEE_SERPORT, 115200,
								xbee_awake_pin, xbee_reset_pin);

	// How can we get this to happen automatically?  xbee_cmd_create() clears the
	// global request table the first time it's called, but unless we keep track
	// of which xbee_dev_t objects have been used, we don't know whether it's
	// necessary to register these two frame handlers or not.  Registering them
	// on every call of xbee_cmd_create() would be inefficient.
	xbee_frame_handler_add( &my_xbee, XBEE_FRAME_LOCAL_AT_RESPONSE, 0,
		xbee_cmd_handle_response, NULL);
	xbee_frame_handler_add( &my_xbee, XBEE_FRAME_REMOTE_AT_RESPONSE, 0,
		xbee_cmd_handle_response, NULL);

	// for debugging, display the contents of the table
	_xbee_dispatch_table_dump( &my_xbee);

	for (;;)
	{
		if (kbhit())
		{
			ch = getchar();
			switch (ch)
			{
				case 't':		// test the api
					test_api();
					break;

				case 'v':		// request VR
					printf( "send version request\n");
					//request( "VR");
					break;

				case 'n':		// request ND
					printf( "send node discovery\n");
					request = xbee_cmd_create( &my_xbee, "ND");
					printf( "sending request 0x%04x: %s to local XBee\n", request,
						"ND");
					xbee_cmd_send( request);
					break;

				case 'r':
					printf( "send network reset\n");
					request = xbee_cmd_create( &my_xbee, "NR");
					xbee_cmd_set_param( request, 1);
					xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
					printf( "sending request 0x%04x: %s to local XBee\n", request,
						"NR1");
					xbee_cmd_send( request);
					break;

				case 'f':		// flood the XBee with a bunch of requests
					printf( "send a bunch of requests\n");
					/*
					for (i = 0; i < sizeof(cmdlist)/sizeof(*cmdlist); ++i)
					{
						request( cmdlist[i]);
					}
					// wait 1000ms for requests to go out so we can grab multiple
					// responses in a single call to xbee_frame_tick
					t = TICK_TIMER;
					while (TICK_TIMER - t < 100);
					*/
					break;

	         case '1':
	            // Read SL from remote XBee (coordinator)
	            request = xbee_cmd_create( &my_xbee, "SL");
	            xbee_cmd_set_target( request,
	                                 WPAN_IEEE_ADDR_COORDINATOR, WPAN_NET_ADDR_UNDEFINED);
	            xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	            printf( "sending request 0x%04x: %s\n", request, "SL to coordinator");
	            xbee_cmd_send( request);
	            break;

	         case '2':
	            // Read SH from remote XBee (coordinator)
	            request = xbee_cmd_create( &my_xbee, "SH");
	            xbee_cmd_set_target( request,
	                                 WPAN_IEEE_ADDR_COORDINATOR, WPAN_NET_ADDR_COORDINATOR);
	            xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	            printf( "sending request 0x%04x: %s\n", request, "SH to coordinator");
	            xbee_cmd_send( request);
	            break;

	         case '3':
	            // Read DL from remote XBee (coordinator)
	            request = xbee_cmd_create( &my_xbee, "DL");
	            xbee_cmd_set_target( request,
	                                 WPAN_IEEE_ADDR_BROADCAST, WPAN_NET_ADDR_COORDINATOR);
	            xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	            printf( "sending request 0x%04x: %s\n", request, "DL to coordinator");
	            xbee_cmd_send( request);
	            break;

	         case '4':
	            // Read DH from remote XBee (coordinator)
	            request = xbee_cmd_create( &my_xbee, "DH");
	            xbee_cmd_set_target( request,
	               (addr64 *)"\x00\x13\xa2\x00\x40\x0a\x13\x0a", WPAN_NET_ADDR_COORDINATOR);
	            xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	            printf( "sending request 0x%04x: %s\n", request, "DL to coordinator");
	            xbee_cmd_send( request);
	            break;

	         case '5':
	            // Read SL from remote XBee (coordinator)
	            request = xbee_cmd_create( &my_xbee, "SL");
	            xbee_cmd_set_target( request,
	               (addr64 *)"\x00\x13\xa2\x00\x40\x0a\x13\x0a", WPAN_NET_ADDR_UNDEFINED);
	            xbee_cmd_set_callback( request, xbee_cmd_callback_sample, NULL);
	            printf( "sending request 0x%04x: %s\n", request, "SL to coordinator");
	            xbee_cmd_send( request);
	            break;
			}
		}
		xbee_dev_tick( &my_xbee);
	}
}


