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
    @file xbee_user_data.c
*/

#include "xbee/user_data.h"

/**
    @brief
    Send a User Data Relay frame (XBEE_FRAME_USER_DATA_TX, type 0x2D).

    @param[in]  xbee        XBee device to receive the frame.
    @param[in]  dest        One of the XBEE_USER_DATA_IF_xxx macros.
    @param[in]  payload     Payload to send.
    @param[in]  length      Length of \a payload.

    @retval     >0          Frame ID of User Data Relay frame sent to \a xbee.
                            Used to match up an XBEE_FRAME_TX_STATUS response.
    @retval     -EINVAL     \a xbee is \c NULL or invalid flags passed
    @retval     -EBUSY      Transmit serial buffer is full, or XBee is not
                            accepting serial data (deasserting /CTS signal).
    @retval     -EMSGSIZE   Serial buffer can't ever send a frame this large.
*/
int xbee_user_data_relay_tx(xbee_dev_t *xbee, uint8_t dest,
                            const void *payload, uint16_t length)
{
    xbee_header_user_data_tx_t header;
    int retval;

    header.frame_type = XBEE_FRAME_USER_DATA_TX;
    header.frame_id = xbee_next_frame_id(xbee);
    header.destination = dest;

    retval = xbee_frame_write(xbee, &header, sizeof(header),
                              payload, length, XBEE_WRITE_FLAG_NONE);

    return retval < 0 ? retval : header.frame_id;
}


/**
    @brief
    Return a string description for an XBEE_USER_DATA_IF_xxx value.

    @param[in]  iface   A valid User Data Relay frame interface.

    @retval     A string description for that interface or "[invalid]".
*/
const char *xbee_user_data_interface(uint8_t iface)
{
    switch (iface) {
        case XBEE_USER_DATA_IF_SERIAL:      return "serial";
        case XBEE_USER_DATA_IF_BLE:         return "Bluetooth";
        case XBEE_USER_DATA_IF_MICROPYTHON: return "MicroPython";
    }

    return "[invalid]";
}

///@}
