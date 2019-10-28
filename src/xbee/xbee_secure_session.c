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
    @addtogroup xbee_secure_session
    @{
    @file xbee_secure_session.c
*/

#include <stdio.h>

#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/ext_modem_status.h"
#include "xbee/secure_session.h"

// API documented in xbee/secure_session.h.
int xbee_secure_session_request(xbee_dev_t *xbee,
                                const addr64 *dest_be,
                                uint8_t options,
                                uint16_t timeout_ds,
                                const char *password)
{
    xbee_header_secure_session_req_t header;

    if (dest_be == NULL) {
        return -EINVAL;
    }

    header.frame_type = XBEE_FRAME_SECURE_SESSION_REQ;
    header.remote_addr_be = *dest_be;
    header.options = options;
    header.timeout_be = htobe16(timeout_ds);

    size_t pw_len = password == NULL ? 0 : strlen(password);

    return xbee_frame_write(xbee, &header, sizeof(header),
                            password, pw_len, XBEE_WRITE_FLAG_NONE);
}


/**
    @brief
    Dump a parsed Extended Modem Status frame for a Secure Session code.

    View the documentation of xbee_frame_handler_fn() for this function's
    parameters and return value.

    @retval -EINVAL     Error in parameters passed to function.
    @retval -EBADMSG    This isn't an Extended Modem Status frame.
    @retval -ENOENT     This isn't a Secure Session extended modem status.
    @retval 0           Status message printed to stdout.

    @see xbee_frame_handler_fn()
*/
int xbee_frame_dump_ext_mod_status_ss(xbee_dev_t *xbee,
                                      const void FAR *payload,
                                      uint16_t length,
                                      void FAR *context)
{
    const xbee_frame_xms_ss_header_t FAR *header = payload;
    char buffer[ADDR64_STRING_LENGTH];

    // Standard frame handler callback API, but we don't use all parameters.
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(length);
    XBEE_UNUSED_PARAMETER(context);

    // stack will never call us with a NULL frame, but user code might
    if (payload == NULL) {
        return -EINVAL;
    }

    if (header->frame_type != XBEE_FRAME_EXT_MODEM_STATUS) {
        return -EBADMSG;
    }

    switch (header->status_code) {
    case XBEE_MODEM_STATUS_SS_ESTABLISHED:
        {
            const xbee_frame_xms_ss_established_t FAR *frame = payload;

            printf("%s %s ESTABLISHED options=0x%02X timeout=0x%04X\n",
                   "Secure Session",
                   addr64_format(buffer, &frame->remote_addr_be),
                   frame->options, be16toh(frame->timeout_be));
        }
        break;

    case XBEE_MODEM_STATUS_SS_ENDED:
        {
            const xbee_frame_xms_ss_ended_t FAR *frame = payload;

            printf("%s %s ENDED reason=0x%02X\n", "Secure Session",
                   addr64_format(buffer, &frame->remote_addr_be),
                   frame->reason);
        }
        break;

    case XBEE_MODEM_STATUS_SS_AUTH_FAILED:
        {
            const xbee_frame_xms_ss_auth_failed_t FAR *frame = payload;

            printf("%s %s AUTH_FAILED error=0x%02X\n", "Secure Session",
                   addr64_format(buffer, &frame->remote_addr_be),
                   frame->error);
        }
        break;

    default:
        return -ENOENT;                 // this isn't a Secure Session status
    }

    return 0;
}


/**
    @brief
    Dump a parsed Secure Session Response frame.

    View the documentation of xbee_frame_handler_fn() for this function's
    parameters and return value.

    @see XBEE_FRAME_DUMP_SS_RESP, xbee_frame_handler_fn()

    @retval -EINVAL     Error in parameters passed to function.
    @retval -EBADMSG    This isn't a Secure Session Response frame.
    @retval -ENOENT     This isn't a Secure Session extended modem status.
    @retval 0           Status message printed to stdout.
*/
int xbee_frame_dump_secure_session_resp(xbee_dev_t *xbee,
                                        const void FAR *payload,
                                        uint16_t length,
                                        void FAR *context)
{
    const xbee_frame_secure_session_resp_t FAR *frame = payload;
    char buffer[ADDR64_STRING_LENGTH];

    // Standard frame handler callback API, but we don't use all parameters.
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(length);
    XBEE_UNUSED_PARAMETER(context);

    // stack will never call us with a NULL frame, but user code might
    if (payload == NULL) {
        return -EINVAL;
    }

    if (frame->frame_type != XBEE_FRAME_SECURE_SESSION_RESP
        || length != sizeof *frame)
    {
        return -EBADMSG;
    }

    printf("%s %s RESPONSE type=0x%02X status=0x%02X\n", "Secure Session",
           addr64_format(buffer, &frame->remote_addr_be),
           frame->type, frame->status);

    return 0;
}

/** @} */
