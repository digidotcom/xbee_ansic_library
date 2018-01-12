/*
 * Copyright (c) 2013 Digi International Inc.,
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
    @file xbee/platform_kl25.h
    Header for Freescale KL25 platform using mbed.org.

    This file is automatically included by xbee/platform.h.
*/

#ifndef __XBEE_PLATFORM_KL25_MBED
#define __XBEE_PLATFORM_KL25_MBED
    #define XBEE_DEVICE_ENABLE_ATMODE
        
    #define strcmpi         strcasecmp
    #define strncmpi        strncasecmp
    
    // macro used to declare a packed structure (no alignment of elements)
    #define PACKED_STRUCT        struct __attribute__ ((__packed__))

    #define _f_memcpy        memcpy
    #define _f_memset        memset

    // stdint.h for int8_t, uint8_t, int16_t, etc. types
    #include <stdint.h>

    typedef int bool_t;

    // inttypes.h for PRIx16, PRIx32, etc. macros
    #include <inttypes.h>
    
    // the "FAR" modifier is not used
    #define FAR

    typedef struct xbee_serial_t
    {
        uint32_t                    baudrate;
    } xbee_serial_t;

    // We'll use 1/1/2000 as the epoch, to match ZigBee.
    #define ZCL_TIME_EPOCH_DELTA    0

    #define XBEE_MS_TIMER_RESOLUTION 1

    XBEE_BEGIN_DECLS

    uint16_t _xbee_get_unaligned16( const void FAR *p);
    uint32_t _xbee_get_unaligned32( const void FAR *p);
    void _xbee_set_unaligned16( void FAR *p, uint16_t value);
    void _xbee_set_unaligned32( void FAR *p, uint32_t value);

    #define xbee_get_unaligned16( p)    _xbee_get_unaligned16( p)
    #define xbee_get_unaligned32( p)    _xbee_get_unaligned32( p)
    #define xbee_set_unaligned16( p, v) _xbee_set_unaligned16( p, v)
    #define xbee_set_unaligned32( p, v) _xbee_set_unaligned32( p, v)

    void xbee_platform_init( void);
    #define XBEE_PLATFORM_INIT() xbee_platform_init()

    XBEE_END_DECLS
#endif
