
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
    @addtogroup xbee_ext_modem_status
    @{
    @file xbee_ext_modem_status.c
*/

#include <stdio.h>

#include "xbee/ext_modem_status.h"

int xbee_frame_dump_ext_modem_status(xbee_dev_t *xbee,
                                     const void FAR *payload,
                                     uint16_t length,
                                     void FAR *context)
{
    const xbee_frame_xms_ss_header_t FAR *header = payload;

    // Standard frame handler callback API, but we don't use all parameters.
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(context);

    if (payload == NULL || length < sizeof *header) {
        return -EINVAL;
    }

    if (header->frame_type != XBEE_FRAME_EXT_MODEM_STATUS) {
        return -EBADMSG;
    }

    length -= sizeof *header;

    printf("%s: status_code=0x%02X, %d bytes of data:\n",
           __FUNCTION__, header->status_code, length);

    hex_dump(header + 1, length, HEX_DUMP_FLAG_OFFSET);

    return 0;
}

/** @} */
