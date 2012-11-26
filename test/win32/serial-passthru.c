/*
 * Copyright (c) 2011-2012 Digi International Inc.,
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
	Serial Bridge

	Bridge two serial ports at a given baudrate and pass characters
	through while dumping out to the screen.

	Also log changes in flow control signals.
*/

#include <stdio.h>
#include <unistd.h>
#include "xbee/platform.h"
#include "xbee/serial.h"

xbee_serial_t port[2];

int lastrx = -1;
char buffer[16];
int rxbytes = 0;
uint32_t rxoffset = 0;

void _header( int i)
{
	printf( "@%u: COM%u -> COM%u\n", xbee_millisecond_timer(),
		port[i].comport, port[1-i].comport);
}
void dump_bytes( int i)
{
	// no changes, still less than 16 bytes
	if (lastrx == i && rxbytes < 16)
	{
		return;
	}

	if (lastrx == -1 && i != -1)
	{
		// first bytes from port[i] after a timeout
		_header( i);
		lastrx = i;
	}

	// no bytes to print yet
	if (rxbytes == 0)
	{
		return;
	}

	printf( "%06x:\t", rxoffset);
	hex_dump( buffer, rxbytes, HEX_DUMP_FLAG_NONE);

	if (i == lastrx)
	{
		rxoffset += rxbytes;
	}
	else
	{
		if (i != -1)
		{
			// transition from port[lastrx] to port[i], new header
			_header( i);
		}
		lastrx = i;
		rxoffset = 0;
	}
	rxbytes = 0;
}

int usage( const char *progname)
{
	// temp override argv[0] since it contains the complete path
	progname = "serial-passthru.exe";

	printf( "  usage: %s <COMa> <COMb> <baudrate>\n", progname);
	printf( "example: %s COM1 COM19 9600\n", progname);

	return EXIT_FAILURE;
}

int main( int argc, char *argv[])
{
	int i;
	int err;
	int temp;
	int cts[2] = { -1, -1 };
	int read;
	char *tail;
	uint32_t baudrate;
	uint32_t rxtime = 0;

	if (argc != 4)
	{
		return usage( argv[0]);
	}

	for (i = 1; i < 3; ++i)
	{
		port[i-1].comport = (int) strtoul( &argv[i][3], &tail, 10);
		if (tail == NULL || strncmp( argv[i], "COM", 3) != 0)
		{
			return usage( argv[0]);
		}
	}
	baudrate = (uint32_t) strtoul( argv[3], &tail, 10);
	if (tail == NULL)
	{
		return usage( argv[0]);
	}

	printf( "bridging COM%u to COM%u at %" PRIu32 "bps\n",
		port[0].comport, port[1].comport, baudrate);
	for (i = 0; i < 2; ++i)
	{
		err = xbee_ser_open( &port[i], baudrate);
		if (err)
		{
			printf( "error %d opening COM%d\n", err, port[i].comport);
			return EXIT_FAILURE;
		}

		// manually control flow control
		xbee_ser_flowcontrol( &port[i], FALSE);
	}

	while (1)
	{
		for (i = 0; i < 2; ++i)
		{
			temp = xbee_ser_get_cts( &port[i]);
			if (temp != cts[i])
			{
				cts[i] = temp;
				dump_bytes( -1);
				xbee_ser_set_rts( &port[1-i], temp);
				printf( "PORT%d: CTS %sasserted\n", i, temp ? "" : "de");
			}
		}

		for (i = 0; i < 2; ++i)
		{
			if (xbee_ser_rx_used( &port[i]))
			{
				// flush accumulated bytes to make room
				dump_bytes( i);

				rxtime = xbee_millisecond_timer();
				read = xbee_ser_read( &port[i], &buffer[rxbytes], 16 - rxbytes);
				xbee_ser_write( &port[1-i], &buffer[rxbytes], read);
				rxbytes += read;
			}
		}
		if (rxbytes && (xbee_millisecond_timer() - rxtime > 100))
		{
			// flush bytes if serial port was idle
			dump_bytes( -1);
		}
		usleep( 1000);	// sleep 1ms to reduce CPU usage
	}

	return EXIT_SUCCESS;
}
