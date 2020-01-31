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
    @defgroup xbee_delivery_status Frames: Delivery Status (0x89, 0x8B)
    @ingroup xbee_frame
    @{
    @file xbee/delivery_status.h
*/

#ifndef __XBEE_DELIVERY_STATUS_H
#define __XBEE_DELIVERY_STATUS_H

XBEE_BEGIN_DECLS

/** @name
    Values for \c delivery member of xbee_frame_tx_status_t (type 0x89) and
    xbee_frame_transmit_status_t (type 0x8B).
    @{
*/
    /// XBee Transmit Delivery Status: Success [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_SUCCESS                    0x00

    /// XBee Transmit Delivery Status: An expected MAC acknowledgement never
    /// occurred [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_MAC_ACK_FAIL               0x01

    /// XBee Transmit Delivery Status: CCA failure [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_CCA_FAIL                   0x02

    /// XBee Transmit Delivery Status: Transmission purged; stack not ready
    /// [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_STACK_NOT_READY            0x03

    /// XBee Transmit Delivery Status: Physical error with transceiver
    /// [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_PHYSICAL_ERROR             0x04

    /// XBee Transmit Delivery Status: No buffers [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_NO_BUFFERS                 0x18

    /// XBee Transmit Delivery Status: Network ACK Failure [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_NET_ACK_FAIL               0x21

    /// XBee Transmit Delivery Status: Not Joined to Network [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_NOT_JOINED                 0x22

    /// XBee Transmit Delivery Status: Invalid Endpoint [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_INVALID_EP                 0x2C

    /// XBee Transmit Delivery Status: Internal error [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_INTERNAL_ERROR             0x31

    /// XBee Transmit Delivery Status: Resource error (lack of buffers,
    /// timers, etc.) [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_RESOURCE_ERROR             0x32

    /// XBee Transmit Delivery Status: No Secure Session Connection
    #define XBEE_TX_DELIVERY_NO_SECURE_SESSION          0x34

    /// XBee Transmit Delivery Status: No Secure Session Connection
    #define XBEE_TX_DELIVERY_ENCRYPTION_FAILURE         0x35

    /// XBee Transmit Delivery Status: Data payload too large [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_PAYLOAD_TOO_BIG            0x74

    /// XBee Transmit Delivery Status: Indirect message unrequested [0x89 only]
    #define XBEE_TX_DELIVERY_UNREQ_INDIRECT_MSG         0x75

    /// XBee Transmit Delivery Status: Failure creating client socket
    /// [0x89, 0x8B]
    #define XBEE_TX_DELIVERY_SOCKET_CREATION_FAIL       0x76

    /// XBee Transmit Delivery Status: Connection does not exist
    /// [0x89 only]
    #define XBEE_TX_DELIVERY_CONNECTION_DNE             0x77

    /// XBee Transmit Delivery Status: Invalid UDP port [0x89 only]
    #define XBEE_TX_DELIVERY_INVALID_UDP_PORT           0x78

    /// XBee Transmit Delivery Status: Invalid TCP port [0x89 only]
    #define XBEE_TX_DELIVERY_INVALID_TCP_PORT           0x79

    /// XBee Transmit Delivery Status: Invalid host address [0x89 only]
    #define XBEE_TX_DELIVERY_INVALID_HOST_ADDR          0x7A

    /// XBee Transmit Delivery Status: Invalid data mode [0x89 only]
    #define XBEE_TX_DELIVERY_INVALID_DATA_MODE          0x7B

    /// XBee Transmit Delivery Status: Invalid interface [0x89 only]
    #define XBEE_TX_DELIVERY_INVALID_INTERFACE          0x7C

    /// XBee Transmit Delivery Status: Interface blocked [0x89 only]
    #define XBEE_TX_DELIVERY_INTERFACE_BLOCKED          0x7D

    /// XBee Transmit Delivery Status: Connection refused [0x89 only]
    #define XBEE_TX_DELIVERY_CONNECTION_REFUSED         0x80

    /// XBee Transmit Delivery Status: Socket connection lost [0x89 only]
    #define XBEE_TX_DELIVERY_CONNECTION_LOST            0x81

    /// XBee Transmit Delivery Status: No server [0x89 only]
    #define XBEE_TX_DELIVERY_NO_SERVER                  0x82

    /// XBee Transmit Delivery Status: Socket closed [0x89 only]
    #define XBEE_TX_DELIVERY_SOCKET_CLOSED              0x83

    /// XBee Transmit Delivery Status: Unknown server [0x89 only]
    #define XBEE_TX_DELIVERY_UNKNOWN_SERVER             0x84

    /// XBee Transmit Delivery Status: Unknown error [0x89 only]
    #define XBEE_TX_DELIVERY_UNKNOWN_ERROR              0x85

    /// XBee Transmit Delivery Status: Invalid TLS configuration [0x89 only]
    #define XBEE_TX_DELIVERY_INVALID_TLS_CONFIG         0x86

    /// XBee Transmit Delivery Status: Key not authorized [0x89, 08B]
    #define XBEE_TX_DELIVERY_KEY_NOT_AUTHORIZED         0xBB

///@}

/// Buffer size used for xbee_tx_delivery_str().
#define XBEE_TX_DELIVERY_STR_BUF_SIZE   40

const char *xbee_tx_delivery_str(uint8_t status,
                                 char buffer[XBEE_TX_DELIVERY_STR_BUF_SIZE]);

XBEE_END_DECLS

///@}

#endif // __XBEE_DELIVERY_STATUS_H
