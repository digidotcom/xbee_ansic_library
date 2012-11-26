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
	@addtogroup xbee_serial
	@{
	@file xbee/serial.h
	Platform-specific layer provieds a consistent serial API to upper layers
	of the driver.

	@section xbee_serial_overview XBee Serial API Overview

	This platform-specific layer maps a consistent
	serial API for the upper levels of the driver to the device's native
	serial API (e.g., serXread/serXwrite on the Rabbit, opening a COM port on a
	Windows PC).

	- sending and receiving serial data
		- xbee_ser_write()
		- xbee_ser_read()
		- xbee_ser_putchar()
		- xbee_ser_getchar()

	- checking the status of transmit and receive buffers
		- xbee_ser_tx_free()
		- xbee_ser_tx_used()
		- xbee_ser_tx_flush()
		- xbee_ser_rx_free()
		- xbee_ser_rx_used()
		- xbee_ser_rx_flush()

	- managing the serial port and control lines
		- xbee_ser_open()
		- xbee_ser_baudrate()
		- xbee_ser_close()
		- xbee_ser_break()
		- xbee_ser_flowcontrol()
		- xbee_ser_set_rts()
		- xbee_ser_get_cts()

	User code will not typically call these functions, unless they are
	not making use of the higher layers of the driver.

	Note that we may need some additional functions to support firmware updates.
	The firmware update code typically needs to be able to open the serial
	port at different baud rates, send a break on the Tx pin and control the
	reset pin.
*/

#ifndef __XBEE_SERIAL
#define __XBEE_SERIAL

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

/**
	@brief
	Helper function used by other xbee_serial functions to
	validate the \a serial parameter.

	Confirms that it is non-\c NULL and is set to a valid port.

	@param[in]	serial	XBee serial port

	@retval	1 \a serial is not a valid XBee serial port
	@retval	0 \a serial is a valid XBee serial port
*/
bool_t xbee_ser_invalid( xbee_serial_t *serial);


/**
	@brief
	Returns a human-readable string describing the serial port.

	For example, on a Windows machine this will be "COM1" or "COM999".
	On a Rabbit, it will be a single letter, "A" through "F".
	On Freescale HCS08, it will be something like "SCI1" or "SCI2".
	On POSIX, the name of the device (e.g., "/dev/ttyS0")

	Returns "(invalid)" if \a serial is invalid or not configured.

	@param[in]	serial	port

	@return	null-terminated string describing the serial port
*/
const char *xbee_ser_portname( xbee_serial_t *serial);


/**
	@brief
	Transmits \a length bytes from \a buffer to the XBee serial
	port \a serial.

	@param[in]	serial	XBee serial port

	@param[in]	buffer	source of bytes to send

	@param[in]	length	number of bytes to write

	@retval	>=0		The number of bytes successfully written to XBee
							serial port.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.
	@retval	-EIO		I/O error attempting to write to serial port.

	@see	xbee_ser_read(), xbee_ser_putchar(), xbee_ser_getchar()
*/
int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer,
	int length);


/**
	@brief
	Reads up to \a bufsize bytes from XBee serial port \a serial
	and into \a buffer.

	If there is no data available when the function is
	called, it will return immediately.

	@param[in]	serial	XBee serial port

	@param[out]	buffer	buffer to hold bytes read from XBee serial port

	@param[in]	bufsize	maximum number of bytes to read

	@retval	>=0		The number of bytes read from XBee serial port.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.
	@retval	-EIO		I/O error attempting to write to serial port.

	@see	xbee_ser_write(), xbee_ser_putchar(), xbee_ser_getchar()
*/
int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize);


/**
	@brief
	Transmits a single character, \a ch, to the XBee serial
	port \a serial.

	@param[in]	serial	XBee serial port

	@param[in]	ch			character to send

	@retval	0			Successfully sent (queued) character.
	@retval	-ENOSPC	The write buffer is full and the character wasn't sent.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_read(), xbee_ser_write(), xbee_ser_getchar()
*/
int xbee_ser_putchar( xbee_serial_t *serial, uint8_t ch);


/**
	@brief
	Reads a single character from the XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	0-255	character read from XBee serial port
	@retval	-ENODATA	There aren't any characters in the read buffer.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_read(), xbee_ser_write(), xbee_ser_getchar()
*/
int xbee_ser_getchar( xbee_serial_t *serial);


/**
	@brief
	Returns the number of bytes of unused space in the serial
	transmit buffer for XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	INT_MAX	The buffer size is unlimited (or unknown).
	@retval	>=0		The number of bytes it would take to fill the XBee
							serial port's serial transmit buffer.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_rx_free(), xbee_ser_rx_used(), xbee_ser_rx_flush(),
				xbee_ser_tx_used(), xbee_ser_tx_flush()
*/
int xbee_ser_tx_free( xbee_serial_t *serial);


/**
	@brief
	Returns the number of queued bytes in the serial transmit buffer
	for XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	0			The buffer size is unlimited (or space used is unknown).
	@retval	>0			The number of bytes queued in the XBee
							serial port's serial transmit buffer.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_rx_free(), xbee_ser_rx_used(), xbee_ser_rx_flush(),
				xbee_ser_tx_free(), xbee_ser_tx_flush()
*/
int xbee_ser_tx_used( xbee_serial_t *serial);


/**
	@brief
	Flushes (i.e., deletes and does not transmit) characters in the
	serial transmit buffer for XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	0			success
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_rx_free(), xbee_ser_rx_used(), xbee_ser_rx_flush(),
				xbee_ser_tx_free(), xbee_ser_tx_used()
*/
int xbee_ser_tx_flush( xbee_serial_t *serial);


/**
	@brief
	Returns the number of bytes of unused space in the serial
	receive buffer for XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	INT_MAX	The buffer size is unlimited (or unknown).
	@retval	>=0		The number of bytes it would take to fill the XBee
							serial port's serial receive buffer.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_tx_free(), xbee_ser_tx_used(), xbee_ser_tx_flush(),
				xbee_ser_rx_used(), xbee_ser_rx_flush()
*/
int xbee_ser_rx_free( xbee_serial_t *serial);


/**
	@brief
	Returns the number of queued bytes in the serial receive buffer
	for XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	>=0		The number of bytes queued in the XBee
							serial port's serial transmit buffer.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_tx_free(), xbee_ser_tx_used(), xbee_ser_tx_flush(),
				xbee_ser_rx_free(), xbee_ser_rx_flush()

	@note Unlike xbee_ser_tx_used(), this function MUST return the number
			of bytes available.  Some layers of the library wait until enough
			bytes are ready before continuing.

			We may expand on or replace this API.  On some platforms (like
			Win32) we have to do some of our own buffering in order to
			peek at data in the serial receive buffer.  Most of the driver
			only requires a check to see if some number of bytes are available
			or not.  Consider changing the API to something like:

	@code	int xbee_ser_rx_avail( xbee_serial_t *serial, uint8_t count)
	@endcode
			returns TRUE if \c count bytes are available, FALSE otherwise.
*/
int xbee_ser_rx_used( xbee_serial_t *serial);


/**
	@brief
	Deletes all characters in the serial receive buffer for XBee serial
	port \a serial.

	@param[in]	serial	XBee serial port

	@retval	0			success
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_tx_free(), xbee_ser_tx_used(), xbee_ser_tx_flush(),
				xbee_ser_rx_free(), xbee_ser_rx_used()
*/
int xbee_ser_rx_flush( xbee_serial_t *serial);


/**
	@brief
	Opens the serial port connected to XBee serial port \a serial at
	\a baudrate bits/second.

	@param[in]	serial	XBee serial port

	@param[in]	baudrate	Bits per second of serial data transfer speed.

	@retval	0			Opened serial port within 5% of requested baudrate.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.
	@retval	-EIO		Can't open serial port within 5% of requested baudrate.

	@see xbee_ser_baudrate(), xbee_ser_close(), xbee_ser_break()
*/
int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate);


/**
	@brief
	Change the baud rate of XBee serial port \a serial to
	\a baudrate bits/second.

	@param[in]	serial	XBee serial port

	@param[in]	baudrate	Bits per second of serial data transfer speed.

	@retval	0			Opened serial port within 5% of requested baudrate.
	@retval	-EINVAL	\a serial is not a valid XBee serial port.
	@retval	-EIO		Can't open serial port within 5% of requested baudrate.

	@see xbee_ser_open(), xbee_ser_close(), xbee_ser_break()
*/
int xbee_ser_baudrate( xbee_serial_t *serial, uint32_t baudrate);


/**
	@brief
	Close the serial port attached to XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@retval	0			closed serial port
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_open(), xbee_ser_baudrate(), xbee_ser_break()
*/
int xbee_ser_close( xbee_serial_t *serial);


/**
	@brief
	Disable the serial transmit pin and pull it low to send a break
	to the XBee serial port.

	@param[in]	serial	XBee serial port

	@param[in]	enabled	Set to 1 to start the break or 0 to end the break (and
								resume transmitting).

	@retval	0	Success
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_open(), xbee_ser_close()
*/
int xbee_ser_break( xbee_serial_t *serial, bool_t enabled);


/**
	@brief
	Enable or disable hardware flow control (CTS/RTS) on the serial
	port for XBee serial port \a serial.

	@param[in]	serial	XBee serial port

	@param[in]	enabled	Set to 0 to disable flow control or non-zero to enable
								flow control.

	@retval	0	Success
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_set_rts(), xbee_ser_get_cts()

*/
int xbee_ser_flowcontrol( xbee_serial_t *serial, bool_t enabled);


/**
	@brief
	Disable hardware flow control and manually set the RTS (ready to
	send) pin on the XBee device's serial port.

	Typically used to enter the XBee device's boot loader and initiate
	a firmware update.

	@param[in]	serial	XBee serial port

	@param[in]	asserted	Set to 1 to assert RTS (ok for XBee to send to us)
								or 0 to deassert RTS (tell XBee not to send to us).

	@retval	0	Success
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_flowcontrol(), xbee_ser_get_cts()
*/
int xbee_ser_set_rts( xbee_serial_t *serial, bool_t asserted);


/**
	@brief
	Read the status of the /CTS (clear to send) pin on the serial
	port connected to XBee serial port \a serial.

	Note that this
	function doesn't return the value of the pin -- it returns
	whether it's asserted (i.e., clear to send to the XBee serial
	port) or not.

	@param[in]	serial	XBee serial port

	@retval	1			it's clear to send
	@retval	0			it's not clear to send
	@retval	-EINVAL	\a serial is not a valid XBee serial port.

	@see	xbee_ser_flowcontrol(), xbee_ser_set_rts()
*/
int xbee_ser_get_cts( xbee_serial_t *serial);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
   #use "xbee_serial_rabbit.c"
#endif

#endif
