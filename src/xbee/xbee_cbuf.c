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
	@file xbee_cbuf.c

	Circular buffer data type used by the OTA (Over-The-Air) firmware update
	client and transparent serial cluster.

	Write to tail, read from head.
*/
/*** BeginHeader */
#include "xbee/cbuf.h"
#include <string.h>
/*** EndHeader */

/*		Functions are documented in xbee/cbuf.h		*/

/*** BeginHeader xbee_cbuf_init */
/*** EndHeader */
int xbee_cbuf_init( xbee_cbuf_t FAR *cbuf, unsigned int datasize)
{
	if (! cbuf || (datasize < 3) || (datasize & (datasize + 1)))
	{
		return -EINVAL;
	}
	cbuf->mask = datasize;
	cbuf->head = cbuf->tail = 0;

	return 0;
}

/*** BeginHeader xbee_cbuf_putch */
/*** EndHeader */
int xbee_cbuf_putch( xbee_cbuf_t FAR *cbuf, uint_fast8_t ch)
{
	unsigned int t;

	t = cbuf->tail;
	cbuf->data[t] = ch;
	t = (t + 1) & cbuf->mask;
	if (t == cbuf->head)
	{
		return 0;		// buffer is full
	}
	cbuf->tail = t;
	return 1;
}

/*** BeginHeader xbee_cbuf_getch */
/*** EndHeader */
int xbee_cbuf_getch( xbee_cbuf_t FAR *cbuf)
{
	unsigned int	h;
	uint8_t	retval;

	h = cbuf->head;
	if (h == cbuf->tail)
	{
		return -1;		// buffer is empty
	}
	retval = cbuf->data[h];
	cbuf->head = (h + 1) & cbuf->mask;

	return retval;
}

/*** BeginHeader xbee_cbuf_used */
/*** EndHeader */
unsigned int xbee_cbuf_used( xbee_cbuf_t FAR *cbuf)
{
	return (cbuf->tail - cbuf->head) & cbuf->mask;
}

/*** BeginHeader xbee_cbuf_free */
/*** EndHeader */
unsigned int xbee_cbuf_free( xbee_cbuf_t FAR *cbuf)
{
	return (cbuf->head - cbuf->tail - 1) & cbuf->mask;
}

/*** BeginHeader xbee_cbuf_flush */
/*** EndHeader */
void xbee_cbuf_flush( xbee_cbuf_t FAR *cbuf)
{
	cbuf->head = cbuf->tail;
}

/*** BeginHeader xbee_cbuf_put */
/*** EndHeader */
unsigned int xbee_cbuf_put( xbee_cbuf_t FAR *cbuf, const void FAR *buffer,
																			unsigned int length)
{
	unsigned int copied = 0; 
	unsigned int buf_free = xbee_cbuf_free(cbuf);
	unsigned int end_space = cbuf->mask + 1 - cbuf->tail;
	
	if (length > buf_free) 
	{
		length = buf_free;
	}
	
	if (length > end_space) 
	{
		memcpy(cbuf->data + cbuf->tail, buffer, end_space);
		buffer = ((char FAR *) buffer) + end_space;
		cbuf->tail = 0;
		length -= end_space;
		copied += end_space;
	}
	
	memcpy(cbuf->data + cbuf->tail, buffer, length);
	cbuf->tail = cbuf->tail + length;
	copied += length;
	
	#ifdef XBEE_CBUF_VERBOSE
		printf( "%s: %u bytes in\n", __FUNCTION__, copied);
	#endif
	
	return copied;
}

/*** BeginHeader xbee_cbuf_get */
/*** EndHeader */
unsigned int xbee_cbuf_get( xbee_cbuf_t *cbuf, void FAR *buffer, unsigned int length)
{
	unsigned int copied = 0;
	unsigned int buf_used = xbee_cbuf_used(cbuf);
	unsigned int end_space = cbuf->mask + 1 - cbuf->head;
	

	if (length > buf_used) 
	{
		length = buf_used;
	}
	
	if (length > end_space) 
	{
		memcpy(buffer, cbuf->data + cbuf->head, end_space);
		buffer = ((char FAR *) buffer) + end_space;
		cbuf->head = 0;
		length -= end_space;
		copied += end_space;
	}
	
	memcpy(buffer, cbuf->data + cbuf->head, length);
	cbuf->head = cbuf->head + length;
	copied += length;
	
	#ifdef XBEE_CBUF_VERBOSE
		printf( "%s: %u bytes out\n", __FUNCTION__, copied);
	#endif
	
	return copied;
}
