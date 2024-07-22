/*
 * Copyright (c) 2024 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 9350 Excelsior Blvd #700, Hopkins, MN 55343
 * =======================================================================
 */

/**
    @addtogroup xbee_gnss
    @{
    @file
*/

#include <stdio.h>
#include <string.h>
#include "xbee/platform.h"
#include "xbee/gnss.h"
#include "xbee/byteorder.h"

// API documented in xbee/gnss.h
int xbee_gnss_start_one_shot(xbee_dev_t *xbee, uint16_t timeout, uint16_t options)
{
    if (!xbee) {
        return -EINVAL;
    }

    xbee_frame_gnss_start_stop_t header;
    

	memset(&header, 0, sizeof(header));
    header.request = XBEE_GNSS_REQUEST_START_ONE_SHOT;
    header.timeout_be = htobe16(timeout);
    header.frame_type = XBEE_FRAME_GNSS_START_STOP;


    if (!(options & XBEE_GNSS_START_STOP_OPT_NO_RESP)) {
        header.frame_id = xbee_next_frame_id(xbee);
    }

    return xbee_frame_write( xbee, &header, sizeof(header), NULL, 0, 0);
}

// API documented in xbee/gnss.h
int xbee_gnss_stop_one_shot(xbee_dev_t *xbee, uint16_t options)
{
    if (!xbee) {
        return -EINVAL;
    }

    xbee_frame_gnss_start_stop_t header;
    
    memset(&header, 0, sizeof(header));
    header.request = XBEE_GNSS_REQUEST_STOP_ONE_SHOT;
    header.frame_type = XBEE_FRAME_GNSS_START_STOP;


    if (options & XBEE_GNSS_START_STOP_OPT_NO_RESP) {
        header.frame_id = 0;
    } else {
        header.frame_id = xbee_next_frame_id(xbee);
    }

    return xbee_frame_write( xbee, &header, sizeof(header), NULL, 0, 0);
}

// API documented in xbee/gnss.h
int xbee_gnss_start_raw_nmea(xbee_dev_t *xbee, uint16_t options)
{
    if (!xbee) {
        return -EINVAL;
    }

    xbee_frame_gnss_start_stop_t header;
    

	memset(&header, 0, sizeof(header));
    header.request = XBEE_GNSS_REQUEST_START_RAW_NMEA;
    header.frame_type = XBEE_FRAME_GNSS_START_STOP;


    if (options & XBEE_GNSS_START_STOP_OPT_NO_RESP) {
        header.frame_id = 0;
    } else {
        header.frame_id = xbee_next_frame_id(xbee);
    }

    return xbee_frame_write( xbee, &header, sizeof(header), NULL, 0, 0);
}

// API documented in xbee/gnss.h
int xbee_gnss_stop_raw_nmea(xbee_dev_t *xbee, uint16_t options)
{
    if (!xbee) {
        return -EINVAL;
    }

    xbee_frame_gnss_start_stop_t header;
    
    memset(&header, 0, sizeof(header));
    header.request = XBEE_GNSS_REQUEST_STOP_RAW_NMEA;
    header.frame_type = XBEE_FRAME_GNSS_START_STOP;


    if (options & XBEE_GNSS_START_STOP_OPT_NO_RESP) {
        header.frame_id = 0;
    } else {
        header.frame_id = xbee_next_frame_id(xbee);
    }

    return xbee_frame_write( xbee, &header, sizeof(header), NULL, 0, 0);
}
