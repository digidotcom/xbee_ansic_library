/*
 * Copyright (c) 2017 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/**
    @addtogroup xbee_delivery_status
    @{
    @file xbee/delivery_status.h
 */

#ifndef __XBEE_DELIVERY_STATUS_H
#define __XBEE_DELIVERY_STATUS_H

XBEE_BEGIN_DECLS

/** @name
    Values for \c delivery member of xbee_frame_transmit_status_t and
    xbee_frame_tx_status_t.
    @{
 */
        /// XBee Transmit Delivery Status: Success [Wi-Fi, Cellular]
        #define XBEE_TX_DELIVERY_SUCCESS                    0x00

        /// XBee Transmit Delivery Status: Transmission purged; stack not ready
        /// [Wi-Fi]
        #define XBEE_TX_DELIVERY_STACK_NOT_READY            0x03

        /// XBee Transmit Delivery Status: Physical error with transceiver
        /// [Wi-Fi]
        #define XBEE_TX_DELIVERY_PHYSICAL_ERROR             0x04

        /// XBee Transmit Delivery Status: Network ACK Failure [Wi-Fi, Cellular]
        #define XBEE_TX_DELIVERY_NET_ACK_FAIL               0x21

        /// XBee Transmit Delivery Status: Not Joined to Network
        /// [Wifi, Cellular]
        #define XBEE_TX_DELIVERY_NOT_JOINED                 0x22

        /// XBee Transmit Delivery Status: Invalid Endpoint [Cellular]
        #define XBEE_TX_DELIVERY_INVALID_EP                 0x2C

        /// XBee Transmit Delivery Status: Internal error [Cellular]
        #define XBEE_TX_DELIVERY_INTERNAL_ERROR             0x31

        /// XBee Transmit Delivery Status: Resource error (lack of buffers,
        /// timers, etc.) [Wi-Fi, Cellular]
        #define XBEE_TX_DELIVERY_RESOURCE_ERROR             0x32

        /// XBee Transmit Delivery Status: Data payload too large
        /// [Wi-Fi, Cellular]
        #define XBEE_TX_DELIVERY_PAYLOAD_TOO_BIG            0x74

        /// XBee Transmit Delivery Status: Failure creating client socket
        /// [Wi-Fi]
        #define XBEE_TX_DELIVERY_SOCKET_CREATION_FAILED     0x76

        /// XBee Transmit Delivery Status: Connection does not exist
        /// [Wi-Fi, Cellular?]
        #define XBEE_TX_DELIVERY_CONNECTION_DNE             0x77

        /// XBee Transmit Delivery Status: Invalid UDP port [Wi-Fi, Cellular]
        #define XBEE_TX_DELIVERY_INVALID_UDP_PORT           0x78

        /// XBee Transmit Delivery Status: Invalid TCP port [Cellular]
        #define XBEE_TX_DELIVERY_INVALID_TCP_PORT           0x79

        /// XBee Transmit Delivery Status: Invalid host address [Cellular]
        #define XBEE_TX_DELIVERY_INVALID_HOST_ADDR          0x7A

        /// XBee Transmit Delivery Status: Invalid data mode [Cellular]
        #define XBEE_TX_DELIVERY_INVALID_DATA_MODE          0x7B

        /// XBee Transmit Delivery Status: Connection refused [Cellular]
        #define XBEE_TX_DELIVERY_CONNECTION_REFUSED         0x80

        /// XBee Transmit Delivery Status: Socket connection lost [Cellular]
        #define XBEE_TX_DELIVERY_CONNECTION_LOST            0x81

        /// XBee Transmit Delivery Status: No server [Cellular]
        #define XBEE_TX_DELIVERY_NO_SERVER                  0x82

        /// XBee Transmit Delivery Status: Socket closed [Cellular]
        #define XBEE_TX_DELIVERY_SOCKET_CLOSED              0x83

        /// XBee Transmit Delivery Status: Unknown server [Cellular]
        #define XBEE_TX_DELIVERY_UNKNOWN_SERVER             0x84

        /// XBee Transmit Delivery Status: Unknown error [Cellular]
        #define XBEE_TX_DELIVERY_UNKNOWN_ERROR              0x85
//@}

XBEE_END_DECLS

//@}

#endif // __XBEE_DELIVERY_STATUS_H
