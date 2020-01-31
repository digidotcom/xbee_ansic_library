/*
 * Copyright (c) 2017-2019 Digi International Inc.,
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
    @addtogroup xbee_delivery_status
    @{
    @file xbee_delivery_status.c
*/

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/delivery_status.h"

/// Get a description of an XBEE_TX_DELIVERY_xxx value (returned in
/// xbee_frame_tx_status_t and xbee_frame_transmit_status_t frame types) for
/// error messages.
const char *xbee_tx_delivery_str(uint8_t status,
                                 char buffer[XBEE_TX_DELIVERY_STR_BUF_SIZE])
{
    switch (status) {
    case XBEE_TX_DELIVERY_SUCCESS:              return "success";
    case XBEE_TX_DELIVERY_MAC_ACK_FAIL:         return "MAC ACK failure";
    case XBEE_TX_DELIVERY_CCA_FAIL:             return "CCA failure";
    case XBEE_TX_DELIVERY_STACK_NOT_READY:      return "stack not ready";
    case XBEE_TX_DELIVERY_PHYSICAL_ERROR:       return "physical error";
    case XBEE_TX_DELIVERY_NO_BUFFERS:           return "no buffers";
    case XBEE_TX_DELIVERY_NET_ACK_FAIL:         return "network ACK failure";
    case XBEE_TX_DELIVERY_NOT_JOINED:           return "not joined";
    case XBEE_TX_DELIVERY_INVALID_EP:           return "invalid endpoint";
    case XBEE_TX_DELIVERY_INTERNAL_ERROR:       return "internal error";
    case XBEE_TX_DELIVERY_RESOURCE_ERROR:       return "resource error";
    case XBEE_TX_DELIVERY_NO_SECURE_SESSION:    return "no secure session";
    case XBEE_TX_DELIVERY_ENCRYPTION_FAILURE:   return "encryption failure";
    case XBEE_TX_DELIVERY_PAYLOAD_TOO_BIG:      return "payload too large";
    case XBEE_TX_DELIVERY_UNREQ_INDIRECT_MSG:   return "unrequested indirect msg";
    case XBEE_TX_DELIVERY_SOCKET_CREATION_FAIL: return "socket creation failed";
    case XBEE_TX_DELIVERY_CONNECTION_DNE:       return "connection doesn't exist";
    case XBEE_TX_DELIVERY_INVALID_UDP_PORT:     return "invalid UDP port";
    case XBEE_TX_DELIVERY_INVALID_TCP_PORT:     return "invalid TCP port";
    case XBEE_TX_DELIVERY_INVALID_HOST_ADDR:    return "invalid host address";
    case XBEE_TX_DELIVERY_INVALID_DATA_MODE:    return "invalid data mode";
    case XBEE_TX_DELIVERY_INVALID_INTERFACE:    return "invalid interface";
    case XBEE_TX_DELIVERY_INTERFACE_BLOCKED:    return "interface blocked";
    case XBEE_TX_DELIVERY_CONNECTION_REFUSED:   return "connection refused";
    case XBEE_TX_DELIVERY_CONNECTION_LOST:      return "connection lost";
    case XBEE_TX_DELIVERY_NO_SERVER:            return "no server";
    case XBEE_TX_DELIVERY_SOCKET_CLOSED:        return "socket closed";
    case XBEE_TX_DELIVERY_UNKNOWN_SERVER:       return "unknown server";
    case XBEE_TX_DELIVERY_UNKNOWN_ERROR:        return "unknown error";
    case XBEE_TX_DELIVERY_INVALID_TLS_CONFIG:   return "invalid TLS config";
    case XBEE_TX_DELIVERY_KEY_NOT_AUTHORIZED:   return "key not authorized";

    default:
        sprintf(buffer, "unknown status 0x%02X", status);
        return buffer;
    }
}

///@}
