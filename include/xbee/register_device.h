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
    @addtogroup xbee_zigbee Frames: Register Device (0x24, 0xA4)
    @ingroup xbee_frame
    @{
    @file xbee/register_device.h

    Frame definitions and support functions for Register Joining Device (0x24)
    and Register Device Status (0xA4) frames used on XBee3 Zigbee modules.
*/

#ifndef XBEE_REGISTER_DEVICE_H
#define XBEE_REGISTER_DEVICE_H

#include "xbee/device.h"
#include "wpan/types.h"

XBEE_BEGIN_DECLS

#define XBEE_ZIGBEE_INSTALL_KEY_LEN     18      ///< length of an install code
#define XBEE_ZIGBEE_LINK_KEY_MAXLEN     16      ///< maximum length of a link key

#define XBEE_FRAME_REGISTER_DEVICE              0x24
typedef XBEE_PACKED(xbee_header_register_device_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_REGISTER_DEVICE (0x24)
    uint8_t     frame_id;       ///< ID to match REGISTER_DEVICE_STATUS frame
    addr64      device_addr_be; ///< 64-bit address of destination devices
    uint16_t    reserved_be;    ///< set to WPAN_NET_ADDR_UNDEFINED (0xFFFE)
    uint8_t     options;        ///< options for join, XBEE_REG_DEV_OPT_xxx
#define XBEE_REG_DEV_OPT_LINK_KEY       0x00
#define XBEE_REG_DEV_OPT_INSTALL_CODE   0x01
    // header followed by optional key (1-16 bytes for link key, 18 bytes for
    // install code, 0 bytes to deregister a device)
}) xbee_header_register_device_t;

#define XBEE_FRAME_REGISTER_DEVICE_STATUS       0xA4
typedef XBEE_PACKED(xbee_frame_register_device_status_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_REGISTER_DEVICE_STATUS (0xA4)
    uint8_t     frame_id;       ///< ID of REGISTER_DEVICE frame
    uint8_t     status;         ///< result of operation
#define XBEE_REG_DEV_STATUS_SUCCESS             0x00
#define XBEE_REG_DEV_STATUS_KEY_TOO_LONG        0x01
#define XBEE_REG_DEV_STATUS_ADDR_NOT_FOUND      0xB1
#define XBEE_REG_DEV_STATUS_KEY_INVALID         0xB2
#define XBEE_REG_DEV_STATUS_ADDR_INVALID        0xB3
#define XBEE_REG_DEV_STATUS_KEY_TABLE_FULL      0xB4
#define XBEE_REG_DEV_STATUS_BAD_INSTALL_CRC     0xBD
#define XBEE_REG_DEV_STATUS_KEY_NOT_FOUND       0xFF
}) xbee_frame_register_device_status_t;

/**
    @brief
    Send a Register Joining Device frame to a local XBee3 Zigbee module.

    @param[in]  xbee            Trust Center to register device with.
    @param[in]  device_addr_be  Address of device to register.
    @param[in]  key             Device key or NULL to deregister a device.
    @param[in]  key_len         0 to deregister, 18 if \a key is an install
                                code, 1 to 16 if \a key is a link key.

    @retval     >0              ID of frame sent to XBee module.
    @retval     -EINVAL         Invalid parameter.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
*/
int xbee_register_device(xbee_dev_t *xbee, addr64 device_addr_be,
                         const void *key, unsigned key_len);

/// Buffer size used for xbee_register_device_status_str().
#define XBEE_REGISTER_DEVICE_STR_BUF_SIZE       40

/// Get a description of an XBEE_REG_DEV_STATUS_xxx value (returned in an
/// XBEE_FRAME_REGISTER_DEVICE_STATUS) for error messages.
const char *xbee_register_device_status_str(
    uint8_t status, char buffer[XBEE_REGISTER_DEVICE_STR_BUF_SIZE]);

/**
    @brief
    Debugging frame handler to print Register Device Status (0xA4) frames.

    View the documentation of xbee_frame_handler_fn() for this function's
    parameters and return value.

    @sa xbee_frame_handler_fn
*/
int xbee_dump_register_device_status(xbee_dev_t *xbee, const void FAR *rawframe,
                                     uint16_t length, void FAR *context);

XBEE_END_DECLS

#endif // XBEE_REGISTER_DEVICE_H

///@}
