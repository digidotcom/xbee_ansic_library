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
/**
	@addtogroup hal_rabbit
	@{
	@file xbee_serial_rabbit.c

	Serial Interface for XBee Module (Rabbit Platform)

	Define XBEE_ON_SERX for every serial port that will/may have an XBee, so
	we can compile in serXopen/serXclose functions for those ports.

	@todo
	Need code to make sure IN and OUT buffers are large enough for XBee.
	Does the out buffer need to be large enough to hold an entire frame?

*/
// NOTE: Documentation for these functions can be found in xbee/serial.h.
/*** BeginHeader */
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/serial.h"

#ifdef XBEE_SERIAL_DEBUG
	#define _xbee_serial_debug __debug
#else
	#define _xbee_serial_debug __nodebug
#endif

// Could change XBEE_SER_CHECK to an assert, or even ignore it if not in debug.
#define XBEE_SER_CHECK(ptr)   \
   do { if (xbee_ser_invalid(ptr)) return -EINVAL; } while (0)
/*** EndHeader */

/*** BeginHeader xbee_ser_invalid */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_invalid( xbee_serial_t *serial)
{
	int disabled;

	if (serial)
	{
		disabled = 1;
		switch (serial->port)
		{
			case SER_PORT_A:
#ifdef XBEE_ON_SERA
				disabled = 0;
#endif
				break;

			case SER_PORT_B:
#ifdef XBEE_ON_SERB
				disabled = 0;
#endif
				break;

			case SER_PORT_C:
#ifdef XBEE_ON_SERC
				disabled = 0;
#endif
				break;

			case SER_PORT_D:
#ifdef XBEE_ON_SERD
				disabled = 0;
#endif
				break;

			case SER_PORT_E:
#ifdef XBEE_ON_SERE
				disabled = 0;
#endif
				break;

			case SER_PORT_F:
#ifdef XBEE_ON_SERF
				disabled = 0;
#endif
				break;

			default:
				return 1;
		}

		#ifdef XBEE_SERIAL_VERBOSE
	      if (disabled)
	      {
				printf( "ERROR: Support for port %c not compiled in.  " \
					"Define XBEE_ON_SER%c and recompile.\n", serial->port + 'A',
					serial->port + 'A');
	      }
	   #endif
		return disabled;
	}

	return 1;
}


/*** BeginHeader xbee_ser_portname */
/*** EndHeader */
_xbee_serial_debug
const char *xbee_ser_portname( xbee_serial_t *serial)
{
	if (serial)
	{
		switch (serial->port)
		{
#ifdef XBEE_ON_SERA
			case SER_PORT_A:	return "A";
#endif
#ifdef XBEE_ON_SERB
			case SER_PORT_B:	return "B";
#endif
#ifdef XBEE_ON_SERC
			case SER_PORT_C:	return "C";
#endif
#ifdef XBEE_ON_SERD
			case SER_PORT_D:	return "D";
#endif
#ifdef XBEE_ON_SERE
			case SER_PORT_E:	return "E";
#endif
#ifdef XBEE_ON_SERF
			case SER_PORT_F:	return "F";
#endif
		}
	}

	return "(invalid)";
}

/*** BeginHeader xbee_ser_write */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer, int length)
{
	XBEE_SER_CHECK( serial);
	if (! buffer || length < 0)
	{
		return -EINVAL;
	}
	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: write %d bytes\n", __FUNCTION__, length);
		hex_dump( buffer, length, HEX_DUMP_FLAG_TAB);
	#endif
	return serXfastwrite( serial->port, buffer, length);
}

/*** BeginHeader xbee_ser_read*/
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize)
{
	int read;

	XBEE_SER_CHECK( serial);
	if (! buffer || bufsize < 0)
	{
		return -EINVAL;
	}
	read = serXread( serial->port, buffer, bufsize, 0);

	#ifdef XBEE_SERIAL_VERBOSE
		if (read)
		{
			printf( "%s: read %d bytes\n", __FUNCTION__, read);
			hex_dump( buffer, read, HEX_DUMP_FLAG_TAB);
		}
	#endif

	return read;
}

/*** BeginHeader xbee_ser_putchar */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_putchar( xbee_serial_t *serial, uint8_t ch)
{
	XBEE_SER_CHECK( serial);

	if (serXputc( serial->port, ch))
	{
		return 0;
	}
	else
	{
		return -ENOSPC;
	}
}
/*** BeginHeader xbee_ser_getchar */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_getchar( xbee_serial_t *serial)
{
	int ch;

	XBEE_SER_CHECK( serial);

	ch = serXgetc( serial->port);

	return (ch < 0) ? -ENODATA : ch;
}

/*** BeginHeader xbee_ser_tx_free */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_tx_free( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	return serXwrFree( serial->port);
}

/*** BeginHeader xbee_ser_tx_used */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_tx_used( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	return serXwrUsed( serial->port);
}

/*** BeginHeader xbee_ser_tx_flush */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_tx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	serXwrFlush( serial->port);
	return 0;
}

/*** BeginHeader xbee_ser_rx_free */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_rx_free( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	return serXrdFree( serial->port);
}

/*** BeginHeader xbee_ser_rx_used */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_rx_used( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

// return bytes used in rx buffer -- will this be difficult on some platforms
// (like PC) where we can't peek at the stream?  It may be necessary to create
// a small buffer for the program to buffer up to 512 bytes from COM port.
	return serXrdUsed( serial->port);
}

/*** BeginHeader xbee_ser_rx_flush */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_rx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	serXrdFlush( serial->port);
	return 0;
}

/*** BeginHeader xbee_ser_open */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate)
{
	int baud_ok;

	XBEE_SER_CHECK( serial);

	// Due to how the Rabbit's RS232 library is designed*, it is necessary
	// to configure this library to support only a certain number of serial
	// ports.  (*calling serXopen for a given serial port results in TX/RX
	// buffers getting allocated and ISR functions getting compiled in.)
	switch (serial->port)
	{
#ifdef XBEE_ON_SERA
		case SER_PORT_A:	baud_ok = serAopen( baudrate);	break;
#endif
#ifdef XBEE_ON_SERB
		case SER_PORT_B:	baud_ok = serBopen( baudrate);	break;
#endif
#ifdef XBEE_ON_SERC
		case SER_PORT_C:	baud_ok = serCopen( baudrate);	break;
#endif
#ifdef XBEE_ON_SERD
		case SER_PORT_D:	baud_ok = serDopen( baudrate);	break;
#endif
#ifdef XBEE_ON_SERE
		case SER_PORT_E:	baud_ok = serEopen( baudrate);	break;
#endif
#ifdef XBEE_ON_SERF
		case SER_PORT_F:	baud_ok = serFopen( baudrate);	break;
#endif
		default:				return -EINVAL;
	}

	if (baud_ok)
	{
		serial->baudrate = baudrate;
		return 0;
	}
	else
	{
		xbee_ser_close( serial);
		return -EIO;
	}
}

/*** BeginHeader xbee_ser_close */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_close( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	switch (serial->port)
	{
#ifdef XBEE_ON_SERA
		case SER_PORT_A:	serAclose();	break;
#endif
#ifdef XBEE_ON_SERB
		case SER_PORT_B:	serBclose();	break;
#endif
#ifdef XBEE_ON_SERC
		case SER_PORT_C:	serCclose();	break;
#endif
#ifdef XBEE_ON_SERD
		case SER_PORT_D:	serDclose();	break;
#endif
#ifdef XBEE_ON_SERE
		case SER_PORT_E:	serEclose();	break;
#endif
#ifdef XBEE_ON_SERF
		case SER_PORT_F:	serFclose();	break;
#endif
		default:				return -EINVAL;
	}

	return 0;
}

/*** BeginHeader xbee_ser_break */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_break( xbee_serial_t *serial, int enabled)
{
	XBEE_SER_CHECK( serial);

	if (enabled)
	{
		switch (serial->port)
		{
#ifdef XBEE_ON_SERA
			case SER_PORT_A:		serAtxBreak( 0);		break;
#endif
#ifdef XBEE_ON_SERB
			case SER_PORT_B:		serBtxBreak( 0);		break;
#endif
#ifdef XBEE_ON_SERC
			case SER_PORT_C:		serCtxBreak( 0);		break;
#endif
#ifdef XBEE_ON_SERD
			case SER_PORT_D:		serDtxBreak( 0);		break;
#endif
#ifdef XBEE_ON_SERE
			case SER_PORT_E:		serEtxBreak( 0);		break;
#endif
#ifdef XBEE_ON_SERF
			case SER_PORT_F:		serFtxBreak( 0);		break;
#endif
			default:					return -EINVAL;
		}
	}
	else
	{
		(*(sxd[serial->port]->xEnable))();
	}

	return 0;
}

/*** BeginHeader xbee_ser_flowcontrol */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_flowcontrol( xbee_serial_t *serial, int enabled)
{
	XBEE_SER_CHECK( serial);

	if (enabled)
	{
		switch (serial->port)
		{
#ifdef XBEE_ON_SERA
			case SER_PORT_A:	serAflowcontrolOn();	break;
#endif
#ifdef XBEE_ON_SERB
			case SER_PORT_B:	serBflowcontrolOn();	break;
#endif
#ifdef XBEE_ON_SERC
			case SER_PORT_C:	serCflowcontrolOn();	break;
#endif
#ifdef XBEE_ON_SERD
			case SER_PORT_D:	serDflowcontrolOn();	break;
#endif
#ifdef XBEE_ON_SERE
			case SER_PORT_E:	serEflowcontrolOn();	break;
#endif
#ifdef XBEE_ON_SERF
			case SER_PORT_F:	serFflowcontrolOn();	break;
#endif
			default:				return -EINVAL;
		}
	}
	else
	{
		serXflowcontrolOff( serial->port);
	}

	return 0;
}

/*** BeginHeader xbee_ser_set_rts */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_set_rts( xbee_serial_t *serial, int asserted)
{
	XBEE_SER_CHECK( serial);

	// disable flow control so our manual setting will stick
	serXflowcontrolOff( serial->port);

	if (asserted)
	{
		sxd[serial->port]->xRTSon();
	}
	else
	{
		sxd[serial->port]->xRTSoff();
	}

	return 0;
}

/*** BeginHeader xbee_ser_get_cts */
/*** EndHeader */
_xbee_serial_debug
int xbee_ser_get_cts( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	// the xCheckCTS returns the pin's value, negate it since CTS is active low.
	return ! sxd[serial->port]->xCheckCTS();
}

//@}
