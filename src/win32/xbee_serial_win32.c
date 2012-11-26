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

/**
	@addtogroup hal_win32
	@{
	@file xbee_serial_win32.c
	Serial Interface for XBee Module (Win32 Platform)

	@addtogroup hal_win32
	@{
*/
// NOTE: Documentation for these functions can be found in xbee/serial.h.

#include "xbee/serial.h"
#include <wtypes.h>
#include <limits.h>
#include <errno.h>
#include <stdio.h>

#define XBEE_SER_CHECK(ptr)	\
	do { if (xbee_ser_invalid(ptr)) return -EINVAL; } while (0)


int xbee_ser_invalid( xbee_serial_t *serial)
{
	if (serial)
	{
		return (serial->hCom == NULL);
	}

	return 1;
}


const char *xbee_ser_portname( xbee_serial_t *serial)
{
	static char portname[sizeof("COM9999")];

	if (serial != NULL && serial->comport > 0 && serial->comport < 10000)
	{
		snprintf( portname, sizeof (portname), "COM%u", serial->comport);
		return portname;
	}

	return "(invalid)";
}


int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer,
	int length)
{
	DWORD dwWrote;
	BOOL success;

	XBEE_SER_CHECK( serial);
	if (! buffer || length < 0)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: return -EINVAL (buffer=%p, length=%d)\n",
				__FUNCTION__, buffer, length);
		#endif
		return -EINVAL;
	}

	success = WriteFile( serial->hCom, buffer, length, &dwWrote, NULL);
	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: wrote %d of %d bytes (err=%lu)\n", __FUNCTION__,
			(int) dwWrote, length, success ? 0 : GetLastError());
		hex_dump( buffer, length, HEX_DUMP_FLAG_TAB);
	#endif

	if (! success)
	{
		return -EIO;
	}

	return (int) dwWrote;
}


int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize)
{
	DWORD dwRead;
	BOOL success;

	XBEE_SER_CHECK( serial);
	if (! buffer || bufsize < 0)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: return -EINVAL (buffer=%p, bufsize=%d)\n",
				__FUNCTION__, buffer, bufsize);
		#endif
		return -EINVAL;
	}

	success = ReadFile( serial->hCom, buffer, bufsize, &dwRead, NULL);
	#ifdef XBEE_SERIAL_VERBOSE
		if (! success)
		{
			printf( "%s: ReadFile error %lu\n", __FUNCTION__, GetLastError());
		}
		if (dwRead)
		{
			printf( "%s: read %d bytes\n", __FUNCTION__, (int) dwRead);
			hex_dump( buffer, (int) dwRead, HEX_DUMP_FLAG_TAB);
		}
	#endif

	if (! success)
	{
		return -EIO;
	}

	return (int) dwRead;
}


int xbee_ser_putchar( xbee_serial_t *serial, uint8_t ch)
{
	int retval;

	retval = xbee_ser_write( serial, &ch, 1);
	if (retval == 1)
	{
		return 0;
	}
	else if (retval == 0)
	{
		return -ENOSPC;
	}
	else
	{
		return retval;
	}
}


int xbee_ser_getchar( xbee_serial_t *serial)
{
	uint8_t ch = 0;
	int retval;

	retval = xbee_ser_read( serial, &ch, 1);
	if (retval != 1)
	{
		return retval ? retval : -ENODATA;
	}

	return ch;
}


int xbee_ser_tx_free( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	return XBEE_SER_TX_BUFSIZE - xbee_ser_tx_used( serial);
}


int xbee_ser_tx_used( xbee_serial_t *serial)
{
	COMSTAT	stat;

	XBEE_SER_CHECK( serial);

	if (ClearCommError( serial->hCom, NULL, &stat))
	{
		return (int) stat.cbOutQue;
	}

	return 0;
}


int xbee_ser_tx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	PurgeComm( serial->hCom, PURGE_TXCLEAR | PURGE_TXABORT);

	return 0;
}


int xbee_ser_rx_free( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	return XBEE_SER_RX_BUFSIZE - xbee_ser_rx_used( serial);
}


int xbee_ser_rx_used( xbee_serial_t *serial)
{
	COMSTAT	stat;

	XBEE_SER_CHECK( serial);

	if (ClearCommError( serial->hCom, NULL, &stat))
	{
		return (int) stat.cbInQue;
	}

	return 0;
}


int xbee_ser_rx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	PurgeComm( serial->hCom, PURGE_RXCLEAR | PURGE_RXABORT);

	return 0;
}


int xbee_ser_baudrate( xbee_serial_t *serial, uint32_t baudrate)
{
	DCB			dcb;

	if (serial == NULL || serial->hCom == NULL)
	{
		return -EINVAL;
	}

	memset( &dcb, 0, sizeof dcb);
	dcb.DCBlength = sizeof dcb;
	if (!GetCommState( serial->hCom, &dcb))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%lu initializing COM%u)\n",
				__FUNCTION__, "GetComState", GetLastError(), serial->comport);
		#endif
		return -EIO;
	}
	dcb.BaudRate = baudrate;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fAbortOnError = FALSE;

	if (!SetCommState( serial->hCom, &dcb))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%lu initializing COM%u)\n",
				__FUNCTION__, "SetComState", GetLastError(), serial->comport);
		#endif
		return -EIO;
	}

	serial->baudrate = baudrate;
	return 0;
}


int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate)
{
	char				buffer[sizeof("\\\\.\\COM9999")];
	HANDLE			hCom;
	COMMTIMEOUTS	timeouts;
	int				err;

	if (serial == NULL || serial->comport > 9999)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: -EINVAL (serial=%p)\n", __FUNCTION__, serial);
		#endif
		return -EINVAL;
	}

	// if XBee's existing hCom handle is not null, need to close it
	// (unless we're just changing the baud rate?)

	// Assume serial port has not changed if port is already open, and skip the
	// CreateFile step.

	if (serial->hCom)
	{
		// serial port is already open
		hCom = serial->hCom;
		serial->hCom = NULL;
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: port already open (hCom=%p)\n",
				__FUNCTION__, hCom);
		#endif
	}
	else
	{
		snprintf( buffer, sizeof buffer, "\\\\.\\COM%u", serial->comport);
		hCom = CreateFile( buffer, GENERIC_READ | GENERIC_WRITE,
			0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hCom == INVALID_HANDLE_VALUE)
		{
			#ifdef XBEE_SERIAL_VERBOSE
				printf( "%s: error %lu opening handle to %s\n", __FUNCTION__,
					GetLastError(), buffer);
			#endif
			return -EIO;
		}
	}

	// set up a transmit and receive buffers
	SetupComm( hCom, XBEE_SER_RX_BUFSIZE, XBEE_SER_TX_BUFSIZE);

	/*	Set the COMMTIMEOUTS structure.  Per MSDN documentation for
		ReadIntervalTimeout, "A value of MAXDWORD, combined with zero values
		for both the ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier
		members, specifies that the read operation is to return immediately
		with the bytes that have already been received, even if no bytes have
		been received."
	*/
	if (!GetCommTimeouts( hCom, &timeouts))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%lu initializing COM%u)\n",
				__FUNCTION__, "GetCommTimeouts", GetLastError(), serial->comport);
		#endif
		CloseHandle( hCom);
		return -EIO;
	}
	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;
	if (!SetCommTimeouts( hCom, &timeouts))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%lu initializing COM%u)\n",
				__FUNCTION__, "SetCommTimeouts", GetLastError(), serial->comport);
		#endif
		CloseHandle( hCom);
		return -EIO;
	}

	PurgeComm( hCom, PURGE_TXCLEAR | PURGE_TXABORT |
							PURGE_RXCLEAR | PURGE_RXABORT);

	serial->hCom = hCom;

	err = xbee_ser_baudrate( serial, baudrate);

	if (err)
	{
		serial->hCom = NULL;
		return err;
	}

	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: SUCCESS COM%u opened (hCom=%p, baud=%u)\n",
			__FUNCTION__, serial->comport, hCom, baudrate);
	#endif

	return 0;
}


int xbee_ser_close( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	CloseHandle( serial->hCom);
	serial->hCom = NULL;

	return 0;
}


int xbee_ser_break( xbee_serial_t *serial, int enabled)
{
	BOOL success;

	XBEE_SER_CHECK( serial);

	if (enabled)
	{
		success = SetCommBreak( serial->hCom);
	}
	else
	{
		success = ClearCommBreak( serial->hCom);
	}

	#ifdef XBEE_SERIAL_VERBOSE
		if (success == 0)
		{
			printf( "%s: {Set|Clear}CommBreak error %lu\n", __FUNCTION__,
					GetLastError());
		}
	#endif

	return success ? 0 : -EIO;
}


int xbee_ser_flowcontrol( xbee_serial_t *serial, int enabled)
{
	DCB dcb;

	XBEE_SER_CHECK( serial);

	memset( &dcb, 0, sizeof dcb);
	dcb.DCBlength = sizeof dcb;
	if (!GetCommState( serial->hCom, &dcb))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: GetComState error %lu\n", __FUNCTION__, GetLastError());
		#endif
		return -EIO;
	}

	dcb.fOutxCtsFlow = enabled ? TRUE : FALSE;
	dcb.fRtsControl = enabled ? RTS_CONTROL_HANDSHAKE : RTS_CONTROL_DISABLE;

	if (!SetCommState( serial->hCom, &dcb))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: SetComState error %lu\n", __FUNCTION__, GetLastError());
		#endif
		return -EIO;
	}

	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: flowcontrol %s\n", __FUNCTION__,
			enabled ? "enabled" : "disabled");
	#endif

	return 0;
}


int xbee_ser_set_rts( xbee_serial_t *serial, int asserted)
{
	BOOL success;

	XBEE_SER_CHECK( serial);

	// disable flow control so our manual setting will stick
	xbee_ser_flowcontrol( serial, 0);

	success = EscapeCommFunction( serial->hCom, asserted ? SETRTS : CLRRTS);

	#ifdef XBEE_SERIAL_VERBOSE
		if (success == 0)
		{
			printf( "%s: EscapeCommFunction error %lu\n", __FUNCTION__,
					GetLastError());
		}
	#endif

	return success ? 0 : -EIO;
}


int xbee_ser_get_cts( xbee_serial_t *serial)
{
	DWORD	dwModemStatus;

	XBEE_SER_CHECK( serial);

	if (!GetCommModemStatus( serial->hCom, &dwModemStatus))
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: GetCommModemStatus error %lu\n", __FUNCTION__,
					GetLastError());
		#endif
		return -EIO;
	}
	return (dwModemStatus & MS_CTS_ON) ? 1 : 0;
}

//@}
