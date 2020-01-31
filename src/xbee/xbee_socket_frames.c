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
    @addtogroup xbee_socket
    @{
    @file xbee_socket_frames.c

    Support functions for Extended Socket frames (0x40-0x4F and 0xC0-0xCF).
*/

#include <stdio.h>

#include "xbee/socket_frames.h"

/// Get a description of an XBEE_SOCK_STATUS_xxx value (returned in multiple
/// socket response frame types) for error messages.
const char *xbee_sock_status_str(uint8_t status,
                                 char buffer[XBEE_SOCK_STR_BUF_SIZE])
{
    switch (status) {
    case XBEE_SOCK_STATUS_SUCCESS:        return "operation successful";
    case XBEE_SOCK_STATUS_INVALID_PARAM:  return "invalid parameters";
    case XBEE_SOCK_STATUS_FAILED_TO_READ: return "failed to retrieve option";
    case XBEE_SOCK_STATUS_IN_PROGRESS:    return "connect already in progress";
    case XBEE_SOCK_STATUS_CONNECTED:      return "connected/bound/listening";
    case XBEE_SOCK_STATUS_UNKNOWN:        return "unknown error";
    case XBEE_SOCK_STATUS_BAD_SOCKET:     return "invalid socket ID";
    case XBEE_SOCK_STATUS_OFFLINE:        return "not registered to network";
    case XBEE_SOCK_STATUS_INTERNAL_ERR:   return "internal error";
    case XBEE_SOCK_STATUS_RESOURCE_ERR:   return "resource error, try later";
    case XBEE_SOCK_STATUS_BAD_PROTOCOL:   return "invalid protocol";

    default:
        sprintf(buffer, "unknown status 0x%02X", status);
        return buffer;
    }
}

/// Get a description of an XBEE_SOCK_STATE_xxx value (returned in an
/// XBEE_FRAME_SOCK_STATE) for error messages.
const char *xbee_sock_state_str(uint8_t state,
                                char buffer[XBEE_SOCK_STR_BUF_SIZE])
{
    switch (state) {
    case XBEE_SOCK_STATE_CONNECTED:             return "connected";
    case XBEE_SOCK_STATE_DNS_FAILED:            return "DNS failed";
    case XBEE_SOCK_STATE_CONNECTION_REFUSED:    return "connection refused";
    case XBEE_SOCK_STATE_TRANSPORT_CLOSED:      return "transport closed";
    case XBEE_SOCK_STATE_TIMED_OUT:             return "timed out";
    case XBEE_SOCK_STATE_INTERNAL_ERR:          return "internal error";
    case XBEE_SOCK_STATE_HOST_UNREACHABLE:      return "host unreachable";
    case XBEE_SOCK_STATE_CONNECTION_LOST:       return "connection lost";
    case XBEE_SOCK_STATE_UNKNOWN_ERR:           return "unknown error";
    case XBEE_SOCK_STATE_UNKNOWN_SERVER:        return "unknown server";
    case XBEE_SOCK_STATE_RESOURCE_ERR:          return "resource error";
    case XBEE_SOCK_STATE_LISTENER_CLOSED:       return "listener closed";

    default:
        sprintf(buffer, "unknown state 0x%02X", state);
        return buffer;
    }
}

///@}
