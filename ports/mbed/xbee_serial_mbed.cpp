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
    @addtogroup hal_kl25
    @{
    @file xbee_serial_kl25.c
    Serial Interface for Freescale KL25 using mbed compiler.
*/

// NOTE: Documentation for these functions can be found in xbee/serial.h.

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include "xbee_platform.h"
#include "xbee_serial.h"
#include "xbee_cbuf.h"

#include "mbed.h"

Serial xbee_ser(PTD3, PTD2);
#define MY_BUF_SIZE 127
struct {
    xbee_cbuf_t        cbuf;
    char               buf_space[MY_BUF_SIZE];
} xbee_buf;

// minimum number of uint8_t free in serial buffer to assert RTS
#define RTS_ASSERT_BYTES    8

// Could change XBEE_SER_CHECK to an assert, or even ignore it if not in debug.
#if defined XBEE_SERIAL_DEBUG
    #define XBEE_SER_CHECK(ptr) \
        do { if (xbee_ser_invalid(ptr)) return -EINVAL; } while (0)
#else
    #define XBEE_SER_CHECK(ptr)
#endif

bool_t xbee_ser_invalid( xbee_serial_t *serial)
{
    return (serial == NULL);
}


const char *xbee_ser_portname( xbee_serial_t *serial)
{
    // probably some way to access the name passed into the Serial constructor
    if (serial != NULL)
    {
        return "(unknown)";
    }

    return "(invalid)";
}


int xbee_ser_write( xbee_serial_t *serial, const void FAR *buffer,
    int length)
{
    int wrote = length;
    const uint8_t FAR *b = (const uint8_t FAR *) buffer;
    
    XBEE_SER_CHECK( serial);

    if (length < 0)
    {
        return -EINVAL;
    }

    while (length--)
    {
        while (! xbee_ser.writeable())
        {
            // does KL25 on mbed have a watchdog?  If so, hit it here...
        }
        xbee_ser.putc( *b++);
    }

    return wrote;
}

int xbee_ser_read( xbee_serial_t *serial, void FAR *buffer, int bufsize)
{
    int retval;

    XBEE_SER_CHECK( serial);

    if (bufsize == 0)
    {
        return 0;
    }

    if (bufsize < 0)
    {
        return -EINVAL;
    }

    retval = xbee_cbuf_get( &xbee_buf.cbuf, buffer,
        (bufsize > 255) ? 255 : (uint8_t) bufsize);
#ifdef XBEE_SERIAL_VERBOSE
    if (retval > 0)
    {
        printf( "read %d bytes\n", retval);
        hex_dump( buffer, retval, HEX_DUMP_FLAG_NONE);
    }
#endif

    return retval;
}


int xbee_ser_putchar( xbee_serial_t *serial, uint8_t ch)
{
    int retval;

    // the call to xbee_ser_write() does XBEE_SER_CHECK() for us
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
    int retval;

    XBEE_SER_CHECK( serial);
    retval = xbee_cbuf_getch( &xbee_buf.cbuf);

    if (retval < 0)
    {
        return -ENODATA;
    }

    return retval;
}


// Since we're using blocking transmit, there isn't a transmit buffer.
// Therefore, have xbee_ser_tx_free() and xbee_ser_tx_used() imply that
// we have an empty buffer that can hold an unlimited amount of data.

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
    return 0;
}


int xbee_ser_rx_free( xbee_serial_t *serial)
{
    XBEE_SER_CHECK( serial);
    return xbee_cbuf_free( &xbee_buf.cbuf);
}


int xbee_ser_rx_used( xbee_serial_t *serial)
{
    XBEE_SER_CHECK( serial);
    return xbee_cbuf_used( &xbee_buf.cbuf);
}


int xbee_ser_rx_flush( xbee_serial_t *serial)
{
    XBEE_SER_CHECK( serial);
    xbee_cbuf_flush( &xbee_buf.cbuf);
    return 0;
}


int xbee_ser_baudrate( xbee_serial_t *serial, uint32_t baudrate)
{
    XBEE_SER_CHECK( serial);

    xbee_ser.baud( baudrate);
    serial->baudrate = baudrate;

    return 0;
}

void xbee_ser_rx_isr( void)
{
    xbee_cbuf_putch( &xbee_buf.cbuf, xbee_ser.getc());
}

int xbee_ser_open( xbee_serial_t *serial, uint32_t baudrate)
{
    XBEE_SER_CHECK( serial);

    xbee_cbuf_init( &xbee_buf.cbuf, MY_BUF_SIZE);
    // need to attach a function pointer
    xbee_ser.attach( &xbee_ser_rx_isr);
    
    return xbee_ser_baudrate( serial, baudrate);
}


int xbee_ser_close( xbee_serial_t *serial)
{
    XBEE_SER_CHECK( serial);

    return 0;
}


int xbee_ser_break( xbee_serial_t *serial, bool_t enabled)
{
    XBEE_SER_CHECK( serial);

    return 0;
}


int xbee_ser_flowcontrol( xbee_serial_t *serial, bool_t enabled)
{
    XBEE_SER_CHECK( serial);

    return 0;
}

int xbee_ser_set_rts( xbee_serial_t *serial, bool_t asserted)
{
    XBEE_SER_CHECK( serial);

    return 0;
}


int xbee_ser_get_cts( xbee_serial_t *serial)
{
    XBEE_SER_CHECK( serial);
    
    return 1;
}


//@}
