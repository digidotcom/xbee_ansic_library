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
	@addtogroup hal_posix
	@{
	@file xbee_serial_posix.c
	Serial Interface for XBee Module (POSIX Platform)

	This file was created by Tom Collins <Tom.Collins@digi.com> based on
	information from:

	http://www.easysw.com/~mike/serial/serial.html

	Serial Programming Guide for POSIX Operating Systems
	5th Edition, 6th Revision
	Copyright 1994-2005 by Michael R. Sweet

	@todo missing a way to hold Tx in break condition
*/
// NOTE: Documentation for these functions can be found in xbee/serial.h.

#include <limits.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "xbee/serial.h"

#define XBEE_SER_CHECK(ptr)	\
	do { if (xbee_ser_invalid(ptr)) return -EINVAL; } while (0)


int xbee_ser_invalid( xbee_serial_t *serial)
{
	if (serial && serial->fd >= 0)
	{
		return 0;
	}

	#ifdef XBEE_SERIAL_VERBOSE
		if (serial == NULL)
		{
			printf( "%s: serial=NULL\n", __FUNCTION__);
		}
		else
		{
			printf( "%s: serial=%p, serial->fd=%d (invalid)\n", __FUNCTION__,
				serial, serial->fd);
		}
	#endif

	return 1;
}


const char *xbee_ser_portname( xbee_serial_t *serial)
{
	if (serial == NULL)
	{
		return "(invalid)";
	}

	return serial->device;
}


int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer,
	int length)
{
	int result;

	XBEE_SER_CHECK( serial);
	if (length < 0)
	{
		return -EINVAL;
	}

	result = write( serial->fd, buffer, length);

	if (result < 0)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: error %d trying to write %d bytes\n", __FUNCTION__,
				errno, length);
		#endif
		return -errno;
	}

	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: wrote %d of %d bytes\n", __FUNCTION__, result, length);
		hex_dump( buffer, result, HEX_DUMP_FLAG_TAB);
	#endif

	return result;
}


int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize)
{
	int result;

	XBEE_SER_CHECK( serial);

	if (! buffer || bufsize < 0)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: buffer=%p, bufsize=%d; return -EINVAL\n", __FUNCTION__,
				buffer, bufsize);
		#endif
		return -EINVAL;
	}

	result = read( serial->fd, buffer, bufsize);
	if (result == -1)
	{
		if (errno == EAGAIN)
		{
			return 0;
		}
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: error %d trying to read %d bytes\n", __FUNCTION__,
				errno, bufsize);
		#endif
		return -errno;
	}

	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: read %d bytes\n", __FUNCTION__, result);
		hex_dump( buffer, result, HEX_DUMP_FLAG_TAB);
	#endif

	return result;
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
	return INT_MAX;
}


int xbee_ser_tx_used( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);
	return 0;
}


int xbee_ser_tx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	tcflush( serial->fd, TCOFLUSH);

	return 0;
}


int xbee_ser_rx_free( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);
	return INT_MAX;
}


int xbee_ser_rx_used( xbee_serial_t *serial)
{
	int bytes;

	XBEE_SER_CHECK( serial);

	if (ioctl( serial->fd,
#ifdef FIONREAD
		FIONREAD,
#else
		TIOCINQ,							// for Cygwin
#endif
		&bytes) == -1)
	{
		return -errno;
	}

	return bytes;
}


int xbee_ser_rx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	tcflush( serial->fd, TCIFLUSH);

	return 0;
}


#define _BAUDCASE(b)		case b: baud = B ## b; break
int xbee_ser_baudrate( xbee_serial_t *serial, uint32_t baudrate)
{
	struct termios options;
	unsigned int baud;

	XBEE_SER_CHECK( serial);

	switch (baudrate)
	{
		_BAUDCASE(0);
		_BAUDCASE(9600);
		_BAUDCASE(19200);
		_BAUDCASE(38400);
		_BAUDCASE(57600);
		_BAUDCASE(115200);
		default:
			return -EINVAL;
	}

	// Get the current options for the port...
	if (tcgetattr( serial->fd, &options) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcgetattr", errno);
		#endif
		return -errno;
	}

	// Set the baud rates...
	cfsetispeed( &options, baud);
	cfsetospeed( &options, baud);

	// Enable the receiver and set local mode...
	options.c_cflag |= (CLOCAL | CREAD);

	// Set 8-bit mode
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	// Set the new options for the port, waiting until buffered data is sent
	if (tcsetattr( serial->fd, TCSADRAIN, &options) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcsetattr", errno);
		#endif
		return -errno;
	}

	return 0;
}


int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate)
{
	int err;

	if (serial == NULL)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: NULL parameter, return -EINVAL\n", __FUNCTION__);
		#endif
		return -EINVAL;
	}

	// make sure device name is null terminated
	serial->device[(sizeof serial->device) - 1] = '\0';

	serial->fd = open( serial->device, O_RDWR | O_NOCTTY | O_NDELAY);
	if (serial->fd < 0)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: open('%s') failed (errno=%d)\n", __FUNCTION__,
				serial->device, errno);
		#endif
		return -errno;
	}

	// Configure file descriptor to not block on read() if there aren't
	// any characters available.
	fcntl( serial->fd, F_SETFL, FNDELAY);

	// reset port before setting actual baudrate
	xbee_ser_baudrate( serial, 0);

	err = xbee_ser_baudrate( serial, baudrate);

	if (err)
	{
		return err;
	}

	return 0;
}


int xbee_ser_close( xbee_serial_t *serial)
{
	int result = 0;

	XBEE_SER_CHECK( serial);

	if (close( serial->fd) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: close(%d) failed (errno=%d)\n", __FUNCTION__,
				serial->fd, errno);
		#endif
		result = -errno;
	}
	serial->fd = -1;

	return result;
}


int xbee_ser_break( xbee_serial_t *serial, int enabled)
{
	XBEE_SER_CHECK( serial);

	/* Devnote regarding use of ioctl() over tcsendbreak():
		"The effect of a nonzero duration with tcsendbreak() varies. SunOS
		specifies a break of duration * N seconds, where N is at least 0.25,
		and not more than 0.5. Linux, AIX, DU, Tru64 send a break of duration
		milliseconds. FreeBSD and NetBSD and HP-UX and MacOS ignore the value
		of duration. Under Solaris and UnixWare, tcsendbreak() with nonzero
		duration behaves like tcdrain()."
		- http://linux.die.net/man/3/tcsendbreak
	*/

	if (ioctl( serial->fd, enabled ? TIOCSBRK : TIOCCBRK) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
				"TIOCMGET", errno);
		#endif
		return -errno;
	}

	return 0;
}


int xbee_ser_flowcontrol( xbee_serial_t *serial, int enabled)
{
	struct termios options;

	XBEE_SER_CHECK( serial);

	// Get the current options for the port...
	if (tcgetattr( serial->fd, &options) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcgetattr", errno);
		#endif
		return -errno;
	}

	if (enabled)
	{
		options.c_cflag |= CRTSCTS;
	}
	else
	{
		options.c_cflag &= ~CRTSCTS;
	}

	// Set the new options for the port immediately
	if (tcsetattr( serial->fd, TCSANOW, &options) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: %s failed (%d)\n", __FUNCTION__, "tcsetattr", errno);
		#endif
		return -errno;
	}

	return 0;
}


int xbee_ser_set_rts( xbee_serial_t *serial, int asserted)
{
	int status;

	XBEE_SER_CHECK( serial);

	// disable flow control so our manual setting will stick
	xbee_ser_flowcontrol( serial, 0);

	if (ioctl( serial->fd, TIOCMGET, &status) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
				"TIOCMGET", errno);
		#endif
		return -errno;
	}

	if (asserted)
	{
		status |= TIOCM_RTS;
	}
	else
	{
		status &= ~TIOCM_RTS;
	}

	if (ioctl( serial->fd, TIOCMSET, &status) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
				"TIOCMSET", errno);
		#endif
		return -errno;
	}

	return 0;
}


int xbee_ser_get_cts( xbee_serial_t *serial)
{
	int status;

	XBEE_SER_CHECK( serial);

	if (ioctl( serial->fd, TIOCMGET, &status) == -1)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: ioctl %s failed (errno=%d)\n", __FUNCTION__,
				"TIOCMGET", errno);
		#endif
		return -errno;
	}

	return (status & TIOCM_CTS) ? 1 : 0;
}

//@}
