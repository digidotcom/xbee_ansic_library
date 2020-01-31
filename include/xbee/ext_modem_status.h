/*
 * Copyright (c) 2019 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

/**
    @defgroup xbee_ext_modem_status Frames: Extended Modem Status (0x98)

    @ingroup xbee_frame
    @{
    @file xbee/ext_modem_status.h
*/

#ifndef XBEE_EXT_MODEM_STATUS_H
#define XBEE_EXT_MODEM_STATUS_H

#include "xbee/platform.h"
#include "xbee/device.h"

XBEE_BEGIN_DECLS

/// Extended Modem Status frame type
#define XBEE_FRAME_EXT_MODEM_STATUS             0x98

/** Header for Extended Modem Status */
typedef XBEE_PACKED(xbee_frame_xms_ss_header_t, {
    /** Frame type, set to #XBEE_FRAME_EXT_MODEM_STATUS (0x98). */
    uint8_t     frame_type;

    /** Reported status, which determines format of data to follow header. */
    uint8_t     status_code;
}) xbee_frame_xms_ss_header_t;

int xbee_frame_dump_ext_modem_status(xbee_dev_t *xbee,
                                     const void FAR *payload,
                                     uint16_t length,
                                     void FAR *context);

/**
    @brief
    Add this macro to the list of XBee frame handlers to dump Extended Modem
    Status messages to STDOUT.
*/
#define XBEE_FRAME_EXT_MODEM_STATUS_DEBUG  \
    { XBEE_FRAME_EXT_MODEM_STATUS, 0, xbee_frame_dump_ext_modem_status, NULL }

XBEE_END_DECLS

#endif // XBEE_SECURE_SESSION_H

///@}
