/* Digi International, Copyright © 2005-2011.  All rights reserved. */

/*
	serial_bypass.c

	This program bridges the programming port of the Rabbit (Serial A) with
	the serial port connected to the XBee module.

	When this program is running, you can bypass the Rabbit and talk directly
	to the XBee module from a PC.  This allows use of X-CTU to configure and
	update the firmware on the XBee module.

	Instructions:

	1) From Dynamic C, choose "Compile To Target/Store Program in Flash" from
		the "Compile" menu.
	2) Choose "Close Connection" from the "Run" menu.
	3) Disconnect the Rabbit board's power.
	4) Switch from the PROG connector to the DIAG connector on the programming
		cable.
	5) Launch X-CTU and select the COM port associated with your programming
		cable.  Configure the serial port for 115200/8N1, no flow control and
		"Enable API".  Do not choose "Use escape characters".
	6) Switch to the Terminal tab in X-CTU.
	7) Reconnect the Rabbit board's power.

	You should see a message that the Rabbit has entered Serial Bypass mode at
	this point.

	8) Switch X-CTU to the Modem Configuration tab and press the "Read" button.

	X-CTU should identify the modem type and firmware version, then read the
	module's settings.
*/

// ------------------------------------------------------
//
// Serial Cable Communication:
//
// The following definitions redirect stdio to serial port A when
// the core module is not attached to the PROG connector of the
// programming cable.  Use the DIAG connector of the programming cable
// to access serial port A.  See Samples\STDIO_SERIAL.C for more details
// on redirecting stdio.
//

#define	STDIO_DEBUG_SERIAL	SADR
#define	STDIO_DEBUG_BAUD		115200
#define	STDIO_DEBUG_ADDCR

// ------------------------------------------------------

#memmap xmem

// Use ANSI-compliant puts() (appends newline) instead of legacy version
#define __ANSI_PUTS__

#include <stdio.h>
#include <ctype.h>
#include <errno.h>

// Load configuration settings for board's serial port.
#include "xbee_config.h"

#if defined XBEE_ON_SERB
   #define _XBEE_SER    B
#elif defined XBEE_ON_SERC
   #define _XBEE_SER    C
#elif defined XBEE_ON_SERD
   #define _XBEE_SER    D
#elif defined XBEE_ON_SERE
   #define _XBEE_SER    E
#elif defined XBEE_ON_SERF
   #define _XBEE_SER    F
#else
   #fatal "No XBEE_ON_SERx macros defined."
#endif
#define _XBEE_SER_DR			CONCAT( CONCAT( SER, _XBEE_SER), _TXPORT)
#if _XBEE_SER_DR == PCDR
	#define _XBEE_SER_FR		PCFR
#elif _XBEE_SER_DR == PDDR
	#define _XBEE_SER_FR		PDFR
#else
	#fatal "Don't know how to configure Tx pin to XBee as a digital output."
#endif
#define _XBEE_DRIVE_TXD    CONCAT( _XBEE_SER, DRIVE_TXD)
#define _XBEE_DRIVE_RXD    CONCAT( _XBEE_SER, DRIVE_RXD)

#define _HOST_DRIVE_TXD		6			// ADRIVE_TXD
#define _HOST_DRIVE_RXD		7			// ADRIVE_RXD

__nodebug
void serial_bypass( void)
{
	puts( "Entering bypass mode -- reset board to exit.");
	Disable_HW_WDT();
	#asm __nodebug
	.init:
				ipset	3					; disable interrupts

		; change Tx pin to host to normal digital output
				ld		hl, PCFR
		ioi	res	_HOST_DRIVE_TXD, (hl)

		; change Tx pin to XBee to normal digital output
				ld		hl, _XBEE_SER_FR
		ioi	res	_XBEE_DRIVE_TXD, (hl)

	         ld		hl, PCDR       		; Serial A (host) Tx and Rx
	         ld		ix, _XBEE_SER_DR


	.check_host_rx:
	   ioi   bit   _HOST_DRIVE_RXD, (hl)     ; read A's Rx pin
	         jr    z, .host_rx_zero
	.host_rx_one:
	   ioi   set	_XBEE_DRIVE_TXD, (ix)
	   		jr		.check_xbee_rx
	.host_rx_zero:
	   ioi   res	_XBEE_DRIVE_TXD, (ix)


	.check_xbee_rx:
		ioi	bit	_XBEE_DRIVE_RXD, (ix)
				jr		z, .xbee_rx_zero
	.xbee_rx_one:
	   ioi   set	_HOST_DRIVE_TXD, (hl)
	   		jr		.check_host_cts
	.xbee_rx_zero:
	   ioi   res	_HOST_DRIVE_TXD, (hl)


	.check_host_cts:
		; port A doesn't have CTS/RTS, so we'll just map Tx/Rx pins
	.check_xbee_cts:


	         jr .check_host_rx
	#endasm
}

#undef _XBEE_SER
#undef _XBEE_SER_TXPORT
#undef _XBEE_SER_RXPORT
#undef _XBEE_DRIVE_TXD
#undef _XBEE_DRIVE_RXD
#undef _HOST_DRIVE_TXD
#undef _HOST_DRIVE_RXD

/*
	main

	Initiate communication with the XBee module, then accept AT commands from
	STDIO, pass them to the XBee module and print the result.
*/
int main( void)
{
	xbee_serial_t serial = XBEE_SERPORT;

	// attempt to open serial port
	if (xbee_ser_open( &serial, 115200) == 0)
	{
		// enable flow control
		xbee_ser_flowcontrol( &serial, 1);

	   // take XBee out of reset
	   xbee_reset_pin( NULL, 0);

		serial_bypass();
	}
	else
	{
		puts( "error opening serial port");
		while(1);
	}
}


