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
	For now, use on BL4S150.

*/



// In a traditional C environment, these macros would have to be set before
// compiling xbee_driver.c to a .o file.  With this setup, they can be set
// in the program that makes use of xbee_driver.c.

#define XBEE_DEVICE_VERBOSE
#define XBEE_DEVICE_DEBUG

#define XBEE_SERIAL_VERBOSE
#define XBEE_SERIAL_DEBUG

// New-style #include.  xbee/device.h will automatically #use "xbee_device.c".
#include <xbee/device.h>

#use "xbee_driver_config.lib"

xbee_dev_t my_xbee;

const char *cmdlist[] = { "VR", "HV", "SH", "SL", "NP", "ID", "CH" };

int handle_atresp( const xbee_dev_t *xbee, const void far *frame, word length,
	void far *context)
{
	char buffer[100];
	char *p;
	char far *b;
	int ch, i;

	const char far *framebytes;

	framebytes = (const char far *)frame;
	p = buffer;
	p += sprintf( p, "  %c%c = 0x", framebytes[2], framebytes[3]);
	for (i = 5, b = &framebytes[5]; i < length; ++i)
	{
		ch = *b++;
		*p++ = _hexits_lower[ch >> 4];
		*p++ = _hexits_lower[ch & 0x0F];
	}
	*p = '\0';

	printf( "%s\n", buffer);
}

void request( const char *reg)
{
	char buffer[10];
	int len;

   len = snprintf( buffer, 10, "\x08%c%s", xbee_next_frame_id( &my_xbee), reg);
   xbee_frame_write( &my_xbee, buffer, len, NULL, 0, 0);
}

void testRTSCTS(void)
{
	int rts, cts;

	rts = BitRdPortI( PEDR, 6);
	cts = BitRdPortI( PDDR, 5);
	printf( "RTS: %s (%d)   CTS: %s (%d)\n", rts ? "disable" : "enable", rts,
		cts ? "disable" : "enable", cts);
}

void main(void)
{
	char buffer[80];
	unsigned long t;
	int i, ch;

	#if _BOARD_TYPE_ == RCM4510W
		// on 4510W, enable the chip select
		_xb_cs(1);
	#endif

	xbee_dev_init( &my_xbee, XBEE_SERPORT, 115200,
								xbee_awake_pin, xbee_reset_pin);
	_xbee_dispatch_table_dump( &my_xbee);

	xbee_frame_handler_add( &my_xbee, XBEE_FRAME_LOCAL_AT_RESPONSE, 0,
		handle_atresp, NULL);

	for (;;)
	{
		if (kbhit())
		{
			ch = getchar();
			switch (ch)
			{
				case 't':		// display values of CTS/RTS pins
					testRTSCTS();
					break;

				case 'v':		// request VR
					printf( "send version request\n");
					request( "VR");
					break;

				case 'n':		// request ND
					printf( "send node discovery\n");
					request( "ND");
					break;

				case 'r':
					printf( "send network reset\n");
					request( "NR\x01");
					break;

				case 'f':		// flood the XBee with a bunch of requests
					printf( "send a bunch of requests\n");
					for (i = 0; i < sizeof(cmdlist)/sizeof(*cmdlist); ++i)
					{
						request( cmdlist[i]);
					}
					// wait 1000ms for requests to go out so we can grab multiple
					// responses in a single call to xbee_frame_tick
					t = TICK_TIMER;
					while (TICK_TIMER - t < 100);
					break;

			}
		}
		xbee_dev_tick( &my_xbee);
	}
}
