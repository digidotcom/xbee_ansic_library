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

//FIXME: this currently assumes DOS.  Needs work to make platform-independent.

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/cbuf.h"
#include "xbee/serial.h"

#include <conio.h>

xbee_serial_t ser;


/*
	main

	Open serial port, then send keystrokes to it and print received chars.
	This is a very basic test of serial port functionality.

	At the simplest, you can loop back Tx and Rx on the wire, and whatever
	you type in should get echoed.
*/


int main( int argc, char *argv[])
{
	int rc;
	char c;

#if 0		// Variable COM port supported...
	int i, rc;
	char buffer[80];

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
		printf( "Connect to which COMPORT? ");
		fgets( buffer, sizeof buffer, stdin);
		xbee->comport = (int) strtoul( buffer, NULL, 10);
	}
#endif

	rc = xbee_ser_open(&ser, 115200);	// Baud rate

	if (rc)
	{
		printf("Failed to open serial port rc=%d\n", rc);
		exit(1);
	}

	printf("Transmitter buffer free: %d\n", xbee_cbuf_free(ser.txbuf));
	printf("Transmitter buffer used: %d\n", xbee_cbuf_used(ser.txbuf));
	printf("Receiver buffer free: %d\n", xbee_cbuf_free(ser.rxbuf));
	printf("Receiver buffer used: %d\n", xbee_cbuf_used(ser.rxbuf));

	printf("Characters will be echoed if loopback.  Start typing.\n");
	printf("Press ESC to exit.\n");
	printf("Press ? to print stats.\n");

	while (1)
	{
		if (kbhit())
		{
			c = getch();
			if (c == 27)
			{
				break;
			}
			else if (c == '?')
			{
				printf("\nISR count=%hu, rent=%hu\n", ser.isr_count, ser.isr_rent);
				printf("IER %02X IIR %02X LCR %02X MCR %02X LSR %02X MSR %02X\n"
					, inp(ser.port + 1)
					, inp(ser.port + 2)
					, inp(ser.port + 3)
					, inp(ser.port + 4)
					, inp(ser.port + 5)
					, inp(ser.port + 6)
					);
				printf("OE %hd, PE %hd, FE %hd, BI %hd\n",
					ser.oe, ser.pe, ser.fe, ser.bi);
				continue;
			}
			xbee_ser_putchar(&ser, c);
			//outp(ser.port, c);
		}
		rc = xbee_ser_getchar(&ser);
		if (rc >= 0)
		{
			putch(rc);
		}
	}
	xbee_ser_close(&ser);

	return 0;


}

