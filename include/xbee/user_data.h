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
    @addtogroup xbee_user_data
    @{
    @file xbee/user_data.h

    Support code for User Data Relay Frames (type 0x2D and 0xAD).
*/

#ifndef XBEE_USER_DATA_H
#define XBEE_USER_DATA_H

#include "xbee/platform.h"
#include "xbee/device.h"

XBEE_BEGIN_DECLS

/// Send data to MicroPython or BLE interface
#define XBEE_FRAME_USER_DATA_TX         0x2D

/// Data received from MicroPython or BLE interface
#define XBEE_FRAME_USER_DATA_RX         0xAD

// Interface identifiers used with USER_DATA_TX and USER_DATA_RX frames.
#define XBEE_USER_DATA_IF_SERIAL        0x00    ///< SPI/UART when in API mode
#define XBEE_USER_DATA_IF_BLE           0x01    ///< Bluetooth
#define XBEE_USER_DATA_IF_MICROPYTHON   0x02    ///< XBee-hosted MicroPython

/// Header of XBee API frame type 0x2D (#XBEE_FRAME_USER_DATA_RELAY_TX);
/// sent from host to XBee.
typedef PACKED_STRUCT xbee_header_user_data_tx_t {
    uint8_t     frame_type;             ///< XBEE_FRAME_USER_DATA_TX (0x2D)
    uint8_t     frame_id;               ///< in TX Status frame on error
    uint8_t     destination;            ///< see XBEE_USER_DATA_IF_xxx
} xbee_header_user_data_tx_t;

/// Format of XBee API frame type 0xAD (#XBEE_FRAME_USER_DATA_RELAY_RX);
/// received from XBee by host.
typedef PACKED_STRUCT xbee_frame_user_data_rx_t {
    uint8_t     frame_type;             ///< XBEE_FRAME_USER_DATA_RX (0xAD)
    uint8_t     source;                 ///< see XBEE_USER_DATA_IF_xxx
    uint8_t     payload[1];             ///< multi-byte payload
} xbee_frame_user_data_rx_t;

// See src/xbee/xbee_user_data.c for function documentation.

int xbee_user_data_relay_tx(xbee_dev_t *xbee, uint8_t dest,
                            const void *payload, uint16_t length);

const char *xbee_user_data_interface(uint8_t iface);

XBEE_END_DECLS

#endif // XBEE_USER_DATA_H
