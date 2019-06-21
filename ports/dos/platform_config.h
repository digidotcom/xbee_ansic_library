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
	@file xbee/platform_dos.h
	Header for DOS platform (using Watcom C cross compiler).

	This file is automatically included by xbee/platform.h.

*/
#ifndef __XBEE_PLATFORM_DOS
#define __XBEE_PLATFORM_DOS

	// macro used to declare a packed structure (no alignment of elements)
	#define PACKED_STRUCT		_Packed struct

	#define _f_memcpy		memcpy
	#define _f_memset		memset

	// stdint.h for int8_t, uint8_t, int16_t, etc. types
	#ifdef __WATCOMC__
		#include <stdint.h>
	#else
		typedef signed char		int8_t;
		typedef unsigned char	uint8_t;
		typedef short				int16_t;
		typedef unsigned short	uint16_t;
		typedef long				int32_t;
		typedef unsigned long	uint32_t;

		typedef unsigned short	uint_fast8_t;
		typedef signed short		int_fast8_t;

		#define __FUNCTION__ 	""
	#endif
	// This type isn't in stdint.h
	typedef int					bool_t;


	// Load platform's endian header to learn whether we're big or little.
	//#include <endian.h>	// Not available on Watcom DOS, so do manually:
	#define LITTLE_ENDIAN	1234
	#define BIG_ENDIAN		4321

	#define BYTE_ORDER		LITTLE_ENDIAN

// On the DOS platform, some pointers are "far".  Many platforms just have
// a single pointer type and will define FAR to nothing.
// For now, don't need far data, so use small or medium memory model.
	#define FAR

	#define INTERRUPT_ENABLE		_enable()
	#define INTERRUPT_DISABLE		_disable()

// Elements needed to keep track of serial port settings.  Must have a
// baudrate memember, other fields are platform-specific.
typedef struct xbee_serial_t {
	uint32_t		baudrate;
	uint16_t		flags;
		#define XST_INIT_OK	0x0001		// Ser port initialized OK
	uint16_t		comport;		// 1 = COM1, 2 = COM2 etc.
	uint16_t		port;			// I/O address of UART: COM1 = 0x3F8 etc.
	void (__interrupt FAR *prev_int)();		// Previous COM port interrupt handler
	uint16_t		intvec;		// Interrupt vector number: 0x0C for COM1 etc.
	uint16_t		picmask;		// PIC mask bit (corresponding to IRQ): 1<<4 for IRQ4
	uint8_t		lsr_mask;	// Mask of line status conditions that are enabled.
		// These determined by UART hardware...
		#define	XST_MASK_DR			0x01			// Data Ready
		#define	XST_MASK_OE			0x02			// Overrun error
		#define	XST_MASK_PE			0x04			// Parity error
		#define	XST_MASK_FE			0x08			// Frame error
		#define	XST_MASK_BI			0x10			// Break interrupt
		#define	XST_MASK_THRE		0x20			// Tx holding reg empty
		#define	XST_MASK_TEMT		0x40			// Tx shift register empty
		#define	XST_MASK_FIFOE		0x80			// Rx FIFO error
	struct xbee_cbuf_t *	txbuf;	// Transmit circular buffer
	struct xbee_cbuf_t *	rxbuf;	// Receive circular buffer
	uint16_t		isr_count;	// For debug - count UART interrupts
	uint16_t		isr_rent;	// For debug - count ISR reentry
	uint16_t		oe;			// Overrun error count
	uint16_t		pe;			// Parity error count
	uint16_t		fe;			// Frame error count
	uint16_t		bi;			// Break interrupt count
} xbee_serial_t;

// DOS epoch is 1/1/1970
#define ZCL_TIME_EPOCH_DELTA	ZCL_TIME_EPOCH_DELTA_1970

// our millisecond timer has a ?ms resolution
#include <time.h>
#define XBEE_MS_TIMER_RESOLUTION (1000 / CLOCKS_PER_SEC)

#define WPAN_MAX_CONVERSATIONS 6
#define WPAN_CONVERSATION_TIMEOUT 15

#include <dos.h>
#include <conio.h>
#include <stddef.h>

// console using ANSI.SYS escape codes
void gotoxy_ansi( int col, int row);
void clrscr_ansi( void);

#ifdef __WATCOMC__
	// console using BIOS calls
	void gotoxy00_bios( int col, int row);
	void getxy_bios( int *col, int *row);
	void clrscr_bios( void);

	// gotoxy is commonly 1-based, but BIOS calls in gotoxy00() are 0-based
	#define gotoxy00(x,y)	gotoxy00_bios(x,y)
	#define gotoxy(x,y)		gotoxy00_bios((x)-1, (y)-1)
	#define getxy(x,y)		getxy_bios(x,y)
	#define clrscr()			clrscr_bios()
#else
	// Borland C is 1's based, but we assume 0's based.
	#define gotoxy00(x,y) gotoxy((x)+1, (y)+1)
#endif

#endif		// __XBEE_PLATFORM_DOS

