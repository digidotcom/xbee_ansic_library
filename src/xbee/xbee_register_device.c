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
    @addtogroup xbee_zigbee
    @{
    @file xbee_register_device.c

    Frame definitions and support functions for Register Joining Device (0x24)
    and Register Device Status (0xA4) frames used on XBee3 Zigbee modules.
*/

#include <stdio.h>

#include "xbee/register_device.h"
#include "xbee/byteorder.h"

int xbee_register_device(xbee_dev_t *xbee, addr64 device_addr_be,
                         const void *key, unsigned key_len)
{
    xbee_header_register_device_t frame;

    // check parameters, valid key_len is 0-16 or 18
    if (xbee == NULL
        || (key_len > XBEE_ZIGBEE_LINK_KEY_MAXLEN
            && key_len != XBEE_ZIGBEE_INSTALL_KEY_LEN)
        || (key == NULL && key_len > 0) )
    {
        return -EINVAL;
    }

    frame.frame_type = XBEE_FRAME_REGISTER_DEVICE;
    frame.frame_id = xbee_next_frame_id(xbee);
    memcpy(&frame.device_addr_be, &device_addr_be, 8);
    frame.reserved_be = htobe16(WPAN_NET_ADDR_UNDEFINED);
    frame.options = key_len == 18 ? XBEE_REG_DEV_OPT_INSTALL_CODE
                                  : XBEE_REG_DEV_OPT_LINK_KEY;

    int retval = xbee_frame_write(xbee, &frame, sizeof(frame),
                                  key, key_len, XBEE_WRITE_FLAG_NONE);

    return retval < 0 ? retval : frame.frame_id;
}

const char *xbee_register_device_status_str(
    uint8_t status, char buffer[XBEE_REGISTER_DEVICE_STR_BUF_SIZE])
{
    switch (status) {
    case XBEE_REG_DEV_STATUS_SUCCESS:           return "success";
    case XBEE_REG_DEV_STATUS_KEY_TOO_LONG:      return "key too long";
    case XBEE_REG_DEV_STATUS_ADDR_NOT_FOUND:    return "address not found";
    case XBEE_REG_DEV_STATUS_KEY_INVALID:       return "key invalid";
    case XBEE_REG_DEV_STATUS_ADDR_INVALID:      return "address invalid";
    case XBEE_REG_DEV_STATUS_KEY_TABLE_FULL:    return "key table full";
    case XBEE_REG_DEV_STATUS_BAD_INSTALL_CRC:   return "bad install code crc";
    case XBEE_REG_DEV_STATUS_KEY_NOT_FOUND:     return "key not found";
    }

    snprintf(buffer, XBEE_REGISTER_DEVICE_STR_BUF_SIZE, "unknown 0x%02X",
             status);
    return buffer;
}

int xbee_dump_register_device_status(xbee_dev_t *xbee, const void FAR *rawframe,
                                     uint16_t length, void FAR *context)
{
    const xbee_frame_register_device_status_t *frame = rawframe;
    char buffer[XBEE_REGISTER_DEVICE_STR_BUF_SIZE];

    printf("Register Device Status: frame 0x%02X status '%s'\n",
           frame->frame_id,
           xbee_register_device_status_str(frame->status, buffer));

    return 0;
}

///@}
