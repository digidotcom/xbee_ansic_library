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



#use "xbee_driver_config.lib"

#define XBEE_DEVICE_VERBOSE
#define XBEE_DEVICE_DEBUG

#define XBEE_SERIAL_VERBOSE
#define XBEE_SERIAL_DEBUG

// NULL out a function called by xbee_dev_tick()
#define _xbee_param_request_tick(xbee)

#use "xbee_device.lib"
#use "xbee_param.lib"

xbee_dev_t xbee;

const char *cmdlist[] = { "VR", "HV", "SH", "SL", "NP", "ID", "CH" };

int handle_atresp( const xbee_dev_t *xbee, const char far *frame, word length,
	void far *context)
{
	char buffer[100];
	char *p;
	char far *b;
	int ch, i;

	p = buffer;
	p += sprintf( p, "  %c%c = 0x", frame[2], frame[3]);
	for (i = 5, b = &frame[5]; i < length; ++i)
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

   len = snprintf( buffer, 10, "\x08%c%s", xbee_next_frame_id( &xbee), reg);
   xbee_frame_write( &xbee, buffer, len, NULL, 0, 0);
}

void main()
{
	char buffer[80];
	unsigned long t;
	int i, ch;

	#if _BOARD_TYPE_ == RCM4510W
		// on 4510W, enable the chip select
		_xb_cs(1);
	#endif

	xbee_dev_init( &xbee, XBEE_SERPORT, 115200, xbee_awake, xbee_reset);
	xbee_param_init( &xbee);

	_xbee_dispatch_table_dump( &xbee);

	xbee_frame_handler_add( &xbee, XBEE_FRAME_LOCAL_AT_RESPONSE, 0,
		handle_atresp, NULL);

	for (;;)
	{
		if (kbhit())
		{
			ch = getchar();
			switch (ch)
			{
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
		xbee_dev_tick( &xbee);
	}
}
