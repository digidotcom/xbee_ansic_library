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
	@addtogroup util_cbuf
	@{
	@file xbee/cbuf.h
	Circular buffer data type used by the OTA (Over-The-Air) firmware update
	client and transparent serial cluster.
*/

#ifndef XBEE_CBUF_H
#define XBEE_CBUF_H

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

/// Circular buffer used by transparent serial cluster handler.  Buffer is
/// empty when \c head == \c tail and full when \c head == \c (tail - 1).
typedef struct xbee_cbuf_t {
	uint8_t			lock;
	uint8_t			head;		// front, or head index of data
	uint8_t			tail;		// back, or tail index of data
	uint8_t			mask;		// 2^n - 1
	uint8_t			data[1];	// variable length (<mask> bytes + 1 separator byte)
} xbee_cbuf_t;

/**
	XBEE_CBUF_OVERHEAD is used when allocating memory for a circular buffer.
	For a cbuf that can hold X bytes, you need (X + CBUF_OVERHEAD) bytes of
	memory.

	XBEE_CBUF_OVERHEAD includes a 4-byte header, plus a separator byte in the
	buffered data.

	For example, to set up a 31-byte circular buffer:
@code
	#define MY_BUF_SIZE 31			// must be 2^n -1
	char my_buf_space[MY_BUF_SIZE + XBEE_CBUF_OVERHEAD];
	xbee_cbuf_t *my_buf;

	my_buf = (xbee_cbuf_t *)my_buf_space;
	xbee_cbuf_init( my_buf, MY_BUF_SIZE);

	// or

	struct {
		xbee_cbuf_t		cbuf;
		char				buf_space[MY_BUF_SIZE];
	} my_buf;

	xbee_cbuf_init( &my_buf.cbuf, MY_BUF_SIZE);
@endcode
*/
#define XBEE_CBUF_OVERHEAD sizeof(xbee_cbuf_t)

/**
	@brief
	Initialize the fields of the circular buffer.

	@note
	You must initialize the xbee_cbuf_t structure before using it with any
	other xbee_cbuf_xxx() functions.  If you have ISRs pushing data
	into the buffer, don't enable them until AFTER you've called
	xbee_cbuf_init.

	@param[in,out]	cbuf
			Address of buffer to use for the circular buffer.  This buffer
			must be (datasize + CBUF_OVEREAD) bytes long to hold the locks,
			head, tail, size and buffered bytes.

	@param[in]		datasize
			Maximum number of bytes to store in the circular buffer.  This
			size must be at least 3, no more than 255, and
			a power of 2 minus 1 (2^n - 1).

	@retval	0			success
	@retval	-EINVAL	invalid parameter
*/
int xbee_cbuf_init( xbee_cbuf_t FAR *cbuf, uint_fast8_t datasize);

/**
	@brief
	Append a single byte to the circular buffer (if not full).

	@param[in,out]	cbuf	Pointer to circular buffer.

	@param[in]		ch		Byte to append.

	@retval	0 buffer is full
	@retval	1	the byte was appended

	@sa xbee_cbuf_getch, xbee_cbuf_get, xbee_cbuf_put
*/
int xbee_cbuf_putch( xbee_cbuf_t FAR *cbuf, uint_fast8_t ch);


/**
	@brief
	Remove and return the first byte of the circular buffer.

	@param[in]	cbuf	Pointer to circular buffer.

	@retval	-1		buffer is empty
	@retval	0-255	byte removed from the head of the buffer

	@sa	xbee_cbuf_putch, xbee_cbuf_get, xbee_cbuf_put

*/
int xbee_cbuf_getch( xbee_cbuf_t FAR *cbuf);


/**
	@brief
	Returns the capacity of the circular buffer.

	@param[in]	cbuf	Pointer to circular buffer.

	@retval	0-255		Maximum number of bytes that can
							be stored in the circularbuffer.

	@sa	xbee_cbuf_used, xbee_cbuf_free
*/
#define xbee_cbuf_length(cbuf)	((cbuf)->mask)


/**
	@brief
	Returns the number of bytes stored in the circular buffer.

	@param[in]	cbuf	Pointer to circular buffer.

	@retval	0-255	Number of bytes stored in the circular buffer.

	@sa	xbee_cbuf_length, xbee_cbuf_free

*/
uint_fast8_t xbee_cbuf_used( xbee_cbuf_t FAR *cbuf);


/**
	@brief
	Returns the number of additional bytes that can be stored in
	the circular buffer.

	@param[in]	cbuf	Pointer to circular buffer.

	@retval	0-255		Number of unused bytes in the circular buffer.

	@sa	xbee_cbuf_length, xbee_cbuf_free

*/
uint_fast8_t xbee_cbuf_free( xbee_cbuf_t FAR *cbuf);


/**
	@brief
	Flush the contents of the circular buffer.

	@param[in,out]	cbuf	Pointer to circular buffer.

*/
void xbee_cbuf_flush( xbee_cbuf_t FAR *cbuf);


/**
	@brief
	Append multiple bytes to the end of a circular buffer.

	@param[in,out]	cbuf		circular buffer
	@param[in]		buffer	data to copy into circular buffer
	@param[in]		length	number of bytes to copy

	@retval	0-255		number of bytes copied (may be less than length if buffer
							is full)
*/
uint_fast8_t xbee_cbuf_put( xbee_cbuf_t FAR *cbuf, const void FAR *buffer,
																		uint_fast8_t length);
/**
	@brief
	Read (and remove) multiple bytes from circular buffer.

	@param[in,out]	cbuf		circular buffer
	@param[out]		buffer	destination to copy data from circular buffer
	@param[in]		length	number of bytes to copy

	@retval	0-255		number of bytes copied (may be less than length if buffer
							is empty)
*/
uint_fast8_t xbee_cbuf_get( xbee_cbuf_t *cbuf, void FAR *buffer,
																		uint_fast8_t length);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_cbuf.c"
#endif

#endif		// XBEE_CBUF_H defined
