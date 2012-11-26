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
	@addtogroup hal_dos
	@{
	@file xbee_serial_dos.c
	Serial Interface for XBee Module (DOS Platform)

	Warning: This module is incomplete.  It only supports COM1.
*/
// NOTE: Documentation for these functions can be found in xbee/serial.h.

// Great reference on interfacing with Serial UART hardware:
// http://www.lammertbies.nl/comm/info/serial-uart.html

#include <errno.h>
#include <limits.h>
#include <stdio.h>

#include "xbee/serial.h"
#include "xbee/cbuf.h"

// Registers
#define UART_RBR		0			//< Receive Buffer				DLAB = 0, read
#define UART_THR		0			//< Transmitter Holding			DLAB = 0, write
#define UART_DLL		0			//< Divisor Latch LSB			DLAB = 1
#define UART_IER		1			//< Interrupt Enable				DLAB = 0
#define UART_DLM		1			//< Divisor Latch MSB			DLAB = 1
#define UART_IIR		2			//< Interrupt Identification	read
#define UART_FCR		2			//< FIFO Control					write
#define UART_LCR		3			//< Line Control
#define UART_MCR		4			//< Modem Control
#define UART_LSR		5			//< Line Status					read
#define UART_MSR		6			//< Modem Status					read
#define UART_SCR		7			//< Scratch

// Bitmasks for Interrupt Enable Register
#define IER_NONE						0
#define IER_RX_DATA					(1<<0)
#define IER_TX_EMPTY					(1<<1)
#define IER_LSR_CHANGE				(1<<2)
#define IER_MSR_CHANGE				(1<<3)

#define IER_DEFAULT	(IER_RX_DATA | IER_TX_EMPTY | IER_LSR_CHANGE)

// Bitmasks for FIFO Control Register (present in 16550 and later)
#define FCR_DISABLE_FIFOS			(0<<0)
#define FCR_ENABLE_FIFOS			(1<<0)
#define FCR_CLEAR_RX_FIFO			(1<<1)
#define FCR_CLEAR_TX_FIFO			(1<<2)
#define FCR_DMA_MODE_0				(0<<3)
#define FCR_DMA_MODE_1				(1<<3)
#define FCR_ENABLE_64BYTE			(1<<5)
#define FCR_INT_TRIGGER_1BYTE		(0<<6)
#define FCR_INT_TRIGGER_4BYTE		(1<<6)
#define FCR_INT_TRIGGER_8BYTE		(2<<6)
#define FCR_INT_TRIGGER_14BYTE	(3<<6)

#define FCR_DEFAULT	(FCR_ENABLE_FIFOS | FCR_DMA_MODE_0)

// Bitmasks for Line Control Register
#define LCR_5BIT						(0<<0)
#define LCR_6BIT						(1<<0)
#define LCR_7BIT						(2<<0)
#define LCR_8BIT						(3<<0)
#define LCR_1STOP						(0<<2)
#define LCR_2STOP						(1<<2)
#define LCR_PARITY_NONE				(0<<3)
#define LCR_PARITY_ODD				(1<<3)
#define LCR_PARITY_EVEN				(3<<3)
#define LCR_PARITY_MARK				(5<<3)
#define LCR_PARITY_SPACE			(7<<3)
#define LCR_BREAK_ENABLE			(1<<6)
#define LCR_DIVISOR_LATCH			(1<<7)

#define LCR_DEFAULT	(LCR_8BIT | LCR_1STOP | LCR_PARITY_NONE)

// Bitmasks for Modem Control Register
#define MCR_NONE						0
#define MCR_DTR						(1<<0)
#define MCR_RTS						(1<<1)
#define MCR_OUT1						(1<<2)
#define MCR_OUT2						(1<<3)
#define MCR_LOOPBACK					(1<<4)
#define MCR_AUTOFLOW					(1<<5)

// "Output 2 is sometimes used in circuitry which controls the interrupt
// process on a PC."
#define MCR_DEFAULT	(MCR_DTR | MCR_RTS | MCR_OUT2)

// Bitmask for Modem Status Register
#define MSR_CTS_CHANGED				(1<<0)
#define MSR_DSR_CHANGED				(1<<1)
#define MSR_RI_EDGE					(1<<2)
#define MSR_CD_CHANGED				(1<<3)
#define MSR_CTS						(1<<4)
#define MSR_DSR						(1<<5)
#define MSR_RI							(1<<6)
#define MSR_CD							(1<<7)

// Could change XBEE_SER_CHECK to an assert, or even ignore it if not in debug.
#if defined XBEE_SERIAL_DEBUG
	#define XBEE_SER_CHECK(ptr)	\
		do { if (xbee_ser_invalid(ptr)) return -EINVAL; } while (0)
#else
	#define XBEE_SER_CHECK(ptr)
#endif

// Since we only support one COM port, it is expedient to simply save the
// pointer passed to xbee_ser_open here.  The ISR needs the info herein.
static xbee_serial_t * xbee_ser_com1 = NULL;

// Similarly, have some static circular buffers
// CBUF_SIZE must be power of 2 minus 1, <= 255 for circular buffer size
#define CBUF_SIZE	255
static struct
{
	xbee_cbuf_t		cb;
	uint8_t			buf[CBUF_SIZE];	// 1 byte is already at end of the cb struct
} txbuf, rxbuf;


void __interrupt __far xbee_ser_isr()
{
	uint_fast8_t mask, byte;
	int txbyte;
#ifdef DEBUG_ISR
	static int in_isr = 0;
#endif

	if (!xbee_ser_com1) {
		return;
	}
#ifdef DEBUG_ISR
	++xbee_ser_com1->isr_count;
	if (in_isr) {
		++xbee_ser_com1->isr_rent;
	}
	++in_isr;
#endif
	do
	{
		mask = inp(xbee_ser_com1->port + UART_LSR) & xbee_ser_com1->lsr_mask;
		if (mask & XST_MASK_DR)
		{
			// Data ready.  Read it and add to cbuf
			byte = inp(xbee_ser_com1->port + UART_RBR);
			xbee_cbuf_putch(xbee_ser_com1->rxbuf, byte);

		}
		if (mask & XST_MASK_THRE)
		{
			// Transmitter holding register empty
			txbyte = xbee_cbuf_getch(xbee_ser_com1->txbuf);
			if (txbyte >= 0)
			{
				outp(xbee_ser_com1->port + UART_THR, txbyte);
			}
			else
			{
				// Tx now idle (nothing in circ buffer).
				// Turn off tx interrupt, app has to restart.
				xbee_ser_com1->lsr_mask &= ~XST_MASK_THRE;
				outp(xbee_ser_com1->port + UART_IER, IER_DEFAULT & ~IER_TX_EMPTY);
			}
		}
		if (mask & (XST_MASK_OE | XST_MASK_PE | XST_MASK_FE | XST_MASK_BI))
		{
			// Line status errors.  Already reset because LSR read above
			if (mask & XST_MASK_OE)
			{
				++xbee_ser_com1->oe;
			}
			if (mask & XST_MASK_PE)
			{
				++xbee_ser_com1->pe;
			}
			if (mask & XST_MASK_FE)
			{
				++xbee_ser_com1->fe;
			}
			if (mask & XST_MASK_BI)
			{
				++xbee_ser_com1->bi;
			}
		}
	} while (mask);

	// Non-specific End Of Interrrupt to PIC 1.
	outp(0x20, 0x20);

#ifdef DEBUG_ISR
	--in_isr;
#endif
}


int xbee_ser_invalid( xbee_serial_t *serial)
{
	if (serial)
	{
		return !(serial->flags & XST_INIT_OK);
	}

	return 1;
}


const char *xbee_ser_portname( xbee_serial_t *serial)
{
	// DOS serial driver only works on COM1; see xbee_serial_win32.c for
	// method of using a static buffer and snprintf() to create the "COMx"
	// string for any given port.
	if (serial != NULL && serial->comport == 1)
	{
		return "COM1";
	}

	return "(invalid)";
}


int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer,
	int length)
{
	int wrote;

	XBEE_SER_CHECK( serial);
	if (! buffer || length < 0)
	{
		return -EINVAL;
	}
	if (!length)
	{
		return 0;
	}
#if 0		// Blocking Tx
	for (wrote = 0; wrote < length; ++wrote) {
		while (!(inp(serial->port + UART_LSR) & XST_MASK_THRE));
		outp(serial->port + UART_THR, ((const char FAR *)buffer)[wrote]);
	}
#else
	wrote = xbee_cbuf_free(serial->txbuf);
	if (wrote > length)
	{
		wrote = length;
	}
	if (!wrote)
	{
		return 0;
	}
	xbee_cbuf_put(serial->txbuf, buffer, wrote);
	INTERRUPT_DISABLE;
	if (!(serial->lsr_mask & XST_MASK_THRE))
	{
		// Transmitter not enabled.  Give it a kick
		serial->lsr_mask |= XST_MASK_THRE;
		// Get 1st char and write it directly to UART
		outp(serial->port + UART_THR, xbee_cbuf_getch(serial->txbuf));
		outp(serial->port + UART_IER, IER_DEFAULT);	// LSR, Rx and Tx interrupts
	}
	INTERRUPT_ENABLE;

#endif
	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: wrote %d of %d bytes \n", __FUNCTION__,
			wrote, length);
		hex_dump( buffer, length, HEX_DUMP_FLAG_TAB);
	#endif

	return wrote;
}

int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize)
{
	int read;

	XBEE_SER_CHECK( serial);
	if (! buffer)
	{
		return -EINVAL;
	}

	read = xbee_cbuf_used(serial->rxbuf);
	if (read > bufsize)
	{
		read = bufsize;
	}

	xbee_cbuf_get(serial->rxbuf, buffer, read);

	#ifdef XBEE_SERIAL_VERBOSE
		if (read)
		{
			printf( "%s: read %d bytes\n", __FUNCTION__, read);
			hex_dump( buffer, read, HEX_DUMP_FLAG_TAB);
		}
	#endif
	return read;
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

	return xbee_cbuf_free(serial->txbuf);
}


int xbee_ser_tx_used( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);
	return xbee_cbuf_used(serial->txbuf);
}


int xbee_ser_tx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	INTERRUPT_DISABLE;
	// Reset Tx FIFO
	outp(serial->port + UART_FCR, FCR_DEFAULT | FCR_CLEAR_TX_FIFO);

	// Turn off tx interrupt, app has to restart.
	serial->lsr_mask &= ~XST_MASK_THRE;
	outp(serial->port + UART_IER, IER_DEFAULT & ~IER_TX_EMPTY);
	INTERRUPT_ENABLE;
	xbee_cbuf_flush(serial->txbuf);
	return 0;
}


int xbee_ser_rx_free( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);
	return xbee_cbuf_free(serial->rxbuf);
}


int xbee_ser_rx_used( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);
	return xbee_cbuf_used(serial->rxbuf);
}


int xbee_ser_rx_flush( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

	INTERRUPT_DISABLE;
	// Reset Rx FIFO
	outp(serial->port + UART_FCR, FCR_DEFAULT | FCR_CLEAR_RX_FIFO);
	// Read pending char and LSR to clear interrupts
	inp(serial->port + UART_RBR);
	inp(serial->port + UART_LSR);
	// Clear Rx buffer
	xbee_cbuf_flush(serial->rxbuf);
	INTERRUPT_ENABLE;

	return 0;
}


int xbee_ser_baudrate( xbee_serial_t *serial, uint32_t baudrate)
{
	uint16_t div;

	if (serial == NULL)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: -EINVAL (serial=%p, baud=%" PRIu32 ") - bad param\n",
				__FUNCTION__, serial, baudrate);
		#endif
		return -EINVAL;
	}

	if (baudrate > 115200 || baudrate < 1)
	{
		baudrate = 115200;
	}
	if (baudrate < 115)
	{
		// Direct divisor specification if 1..114
		div = (uint16_t)baudrate;
	}
	else
	{
		// Direct BPS rate specification
		div = (uint16_t)(115200 / baudrate);
	}
	serial->baudrate = 115200 / div;

   outp(serial->port + UART_LCR, LCR_DEFAULT | LCR_DIVISOR_LATCH);
	outp(serial->port + UART_DLL, div & 0xFF);
	outp(serial->port + UART_DLM, div >> 8);
   outp(serial->port + UART_LCR, LCR_DEFAULT);

	return 0;
}


int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate)
{
	if (serial == NULL)
	{
		#ifdef XBEE_SERIAL_VERBOSE
			printf( "%s: -EINVAL (serial=%p, baud=%" PRIu32 ") - bad param\n",
				__FUNCTION__, serial, baudrate);
		#endif
		return -EINVAL;
	}
	if (xbee_ser_com1)
	{
		// Only support one open at a time.
		xbee_ser_close(xbee_ser_com1);
	}


	// Important to save pointer for ISR
	xbee_ser_com1 = serial;

	serial->flags = XST_INIT_OK;
	serial->isr_count = 0;
	serial->isr_rent = 0;
	serial->oe = 0;
	serial->pe = 0;
	serial->fe = 0;
	serial->bi = 0;

	/* COM  port   vec   IRQ*/
	/* ---- -----  ----  ---*/
  	/* COM1 0x3F8  0x0C  4  */
  	/* COM2 0x2F8	0x0B	3  */
  	/* COM3 0x3E8	0x0C	4  */
  	/* COM4 0x2E8	0x0B	3  */
	switch (serial->comport)
	{
		default:
			serial->comport = 1;
			serial->port = 0x3F8;
			serial->intvec = 0x0C;
			serial->picmask = 1<<4;
			break;
		case 2:
			serial->port = 0x2F8;
			serial->intvec = 0x0B;
			serial->picmask = 1<<3;
			break;
		case 3:
			serial->port = 0x3E8;
			serial->intvec = 0x0C;
			serial->picmask = 1<<4;
			break;
		case 4:
			serial->port = 0x2E8;
			serial->intvec = 0x0B;
			serial->picmask = 1<<3;
			break;
	}

	// Point to buffers (static for now, since only COM1)
	serial->txbuf = &txbuf.cb;
	serial->rxbuf = &rxbuf.cb;
	xbee_cbuf_init(&txbuf.cb, CBUF_SIZE);
	xbee_cbuf_init(&rxbuf.cb, CBUF_SIZE);


	// Halt interrupts, save old vector, set new vector
	outp(serial->port + UART_IER, IER_NONE);
	serial->prev_int = _dos_getvect(serial->intvec);
	_dos_setvect(serial->intvec, xbee_ser_isr);

	xbee_ser_baudrate( serial, baudrate);

	outp(serial->port + UART_LCR, LCR_DEFAULT);
	outp(serial->port + UART_FCR,
			FCR_DEFAULT | FCR_CLEAR_RX_FIFO | FCR_CLEAR_TX_FIFO);

	outp(serial->port + UART_MCR, MCR_DEFAULT);

	// Enable interrupts for rx and line status only at this point (nothing to tx)
	serial->lsr_mask = XST_MASK_DR | XST_MASK_OE | XST_MASK_PE | XST_MASK_FE | XST_MASK_BI;
	outp(serial->port + UART_IER, IER_DEFAULT & ~IER_TX_EMPTY);

	// Clear pending garbage chars and status
	inp(serial->port + UART_RBR);
	inp(serial->port + UART_LSR);
	inp(serial->port + UART_MSR);

	outp(0x21,(inp(0x21) & ~serial->picmask));  // Enable IRQ

	#ifdef XBEE_SERIAL_VERBOSE
		printf( "%s: COM1 port opened (baud=%lu, 8N1)\n",
			__FUNCTION__, baudrate);
	#endif

	return 0;
}


int xbee_ser_close( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);
	outp(serial->port + UART_MCR, MCR_OUT2);	// Turn off DTR, RTS
	outp(0x21,(inp(0x21) | serial->picmask));  // Disable IRQ
	outp(serial->port + UART_IER, IER_NONE);
	_dos_setvect(serial->intvec, serial->prev_int);

	xbee_ser_com1 = NULL;
	serial->flags = 0;

	return 0;
}


int xbee_ser_break( xbee_serial_t *serial, int enabled)
{
	XBEE_SER_CHECK( serial);

	// FIXME: this is not a really clean way of doing it, since the
	// UART may still be part way through the last char.  But it doesn't
	// hurt in this application I think...
	if (enabled)
	{
		outp(serial->port + UART_LCR, LCR_DEFAULT | LCR_BREAK_ENABLE);
	}
	else
	{
		outp(serial->port + UART_LCR, LCR_DEFAULT & ~LCR_BREAK_ENABLE);
	}

	return 0;
}


int xbee_ser_flowcontrol( xbee_serial_t *serial, int enabled)
{
	XBEE_SER_CHECK( serial);

	// FIXME: interpret this to mean turn on RTS, DTR
	outp(serial->port + UART_MCR, MCR_DEFAULT);  /* Turn on DTR, RTS */

	return 0;
}


int xbee_ser_set_rts( xbee_serial_t *serial, int asserted)
{
	XBEE_SER_CHECK( serial);
	if (asserted)
	{
		outp(serial->port + UART_MCR, MCR_DEFAULT | MCR_RTS);
	}
	else
	{
		outp(serial->port + UART_MCR, MCR_DEFAULT & ~MCR_RTS);
	}

	return 0;
}


int xbee_ser_get_cts( xbee_serial_t *serial)
{
	XBEE_SER_CHECK( serial);

#ifdef XBEE_SER_DISABLE_CTS
	return 1;	// <-- hack to ignore CTS for miswired modems
#else
	return inp(serial->port + UART_MSR) & MSR_CTS ? 1 : 0;
#endif
}

//@}
