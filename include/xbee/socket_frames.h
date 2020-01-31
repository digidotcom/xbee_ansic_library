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
    @file xbee/socket_frames.h

    Frame definitions and support functions for Extended Socket frames
    (0x40-0x4F and 0xC0-0xCF).
*/

#ifndef XBEE_SOCKET_FRAMES_H
#define XBEE_SOCKET_FRAMES_H

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

/// Status codes used by multiple socket response frame types.
#define XBEE_SOCK_STATUS_SUCCESS        0x00    ///< operation successful
#define XBEE_SOCK_STATUS_INVALID_PARAM  0x01    ///< invalid parameters
#define XBEE_SOCK_STATUS_FAILED_TO_READ 0x02    ///< failed to retrieve option
#define XBEE_SOCK_STATUS_IN_PROGRESS    0x03    ///< connect already in progress
#define XBEE_SOCK_STATUS_CONNECTED      0x04    ///< connected/bound/listening
#define XBEE_SOCK_STATUS_UNKNOWN        0x05    ///< unknown error
#define XBEE_SOCK_STATUS_BAD_SOCKET     0x20    ///< invalid socket ID
#define XBEE_SOCK_STATUS_OFFLINE        0x22    ///< not registered to network
#define XBEE_SOCK_STATUS_INTERNAL_ERR   0x31    ///< internal error
#define XBEE_SOCK_STATUS_RESOURCE_ERR   0x32    ///< resource error, try later
#define XBEE_SOCK_STATUS_BAD_PROTOCOL   0x7B    ///< invalid protocol
// ^^^ Be sure to update xbee_sock_status_str() when adding to this macro list.

/// The 8-bit Socket ID is never 0xFF, except in Socket Create Response when
/// socket creation wasn't successful.
#define XBEE_SOCK_SOCKET_ID_INVALID     0xFF

/// Frame Type: Extended Socket Create
#define XBEE_FRAME_SOCK_CREATE          0x40
/// Format of XBee API frame type 0x40 (#XBEE_FRAME_SOCK_CREATE).
/// Sent from host to XBee, which responds with XBEE_FRAME_SOCK_CREATE_RESP.
typedef XBEE_PACKED(xbee_frame_sock_create_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_CREATE (0x40)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     protocol;
#define XBEE_SOCK_PROTOCOL_UDP          0       ///< UDP
#define XBEE_SOCK_PROTOCOL_TCP          1       ///< TCP
#define XBEE_SOCK_PROTOCOL_SSL          4       ///< SSL over TCP
}) xbee_frame_sock_create_t;


/// Frame Type: Extended Socket Option Request
#define XBEE_FRAME_SOCK_OPTION_REQ      0x41
/// Header of XBee API frame type 0x41 (#XBEE_FRAME_SOCK_OPTION_REQ);
/// sent from host to XBee.  Followed by variable-length data (to set)
/// or nothing (to query).  XBee responds with XBEE_FRAME_SOCK_OPTION_RESP.
typedef XBEE_PACKED(xbee_header_sock_option_req_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_OPTION_REQ (0x41)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     socket_id;      ///< socket to query/modify
    uint8_t     option_id;      ///< option to query/modify
#define XBEE_SOCK_OPT_ID_TLS_PROFILE    0x00
}) xbee_header_sock_option_req_t;

/// Frame Type: Extended Socket Option Response
#define XBEE_FRAME_SOCK_OPTION_RESP     0xC1
/// Format of XBee API frame type 0xC1 (#XBEE_FRAME_SOCK_OPTION_RESP);
/// sent from XBee to host.
typedef XBEE_PACKED(xbee_frame_sock_option_resp_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_OPTION_RESP (0xC1)
    uint8_t     frame_id;       ///< identifier from request
    uint8_t     socket_id;      ///< socket queried/modified
    uint8_t     option_id;      ///< option queried/modified
    uint8_t     status;         ///< request status
    uint8_t     option_data[1]; ///< variable-length value of option queried
}) xbee_frame_sock_option_resp_t;


/// Frame Type: Extended Socket Connect
#define XBEE_FRAME_SOCK_CONNECT         0x42
/// Header of XBee API frame type 0x42 (#XBEE_FRAME_SOCK_CONNECT);
/// sent from host to XBee.  Followed by variable-length destination address.
/// XBee responds with XBEE_FRAME_SOCK_CONNECT_RESP immediately to confirm
/// the request and XBEE_FRAME_SOCK_STATUS with the connection status.
typedef XBEE_PACKED(xbee_header_sock_connect_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_CREATE (0x40)
    uint8_t     frame_id;       ///< identifier to match response frames
    uint8_t     socket_id;      ///< socket used for connection
    uint16_t    remote_port_be; ///< destination port
    uint8_t     addr_type;      ///< format of variable-length destination addr
#define XBEE_SOCK_ADDR_TYPE_IPV4        0   ///< 4-byte binary IPv4 address
#define XBEE_SOCK_ADDR_TYPE_HOSTNAME    1   ///< hostname or dotted quad (text)
}) xbee_header_sock_connect_t;


/// Frame Type: Extended Socket Close
#define XBEE_FRAME_SOCK_CLOSE           0x43
/// Format of XBee API frame type 0x43 (#XBEE_FRAME_SOCK_CLOSE).
/// Sent from host to XBee, which responds with XBEE_FRAME_SOCK_CLOSE_RESP.
typedef XBEE_PACKED(xbee_frame_sock_close_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_CLOSE (0x43)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     socket_id;      ///< socket used for connection or 0xFF for all
}) xbee_frame_sock_close_t;


/// Frame Type: Extended Socket Send
#define XBEE_FRAME_SOCK_SEND            0x44
/// Header of XBee API frame type 0x44 (#XBEE_FRAME_SOCK_SEND);
/// sent from host to XBee.  Followed by variable-length payload.
/// XBee responds with XBEE_FRAME_TX_STATUS (0x89) for a non-zero frame_id.
typedef XBEE_PACKED(xbee_header_sock_send_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_SEND (0x44)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     socket_id;      ///< socket used for connection
    uint8_t     options;        ///< reserved for transmit options (set to 0)
}) xbee_header_sock_send_t;


/// Frame Type: Extended Socket Sendto
#define XBEE_FRAME_SOCK_SENDTO          0x45
/// Header of XBee API frame type 0x45 (#XBEE_FRAME_SOCK_SENDTO);
/// sent from host to XBee.  Followed by variable-length payload.
/// XBee responds with XBEE_FRAME_TX_STATUS (0x89) for a non-zero frame_id.
typedef XBEE_PACKED(xbee_header_sock_sendto_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_SENDTO (0x45)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     socket_id;      ///< socket used for connection
    uint32_t    remote_addr_be; ///< 4-byte destination IPv4 address
    uint16_t    remote_port_be; ///< 16-bit destination port
    uint8_t     options;        ///< reserved for transmit options (set to 0)
}) xbee_header_sock_sendto_t;


/// Frame Type: Extended Socket Bind/Listen
#define XBEE_FRAME_SOCK_BIND_LISTEN     0x46
/// Format of XBee API frame type 0x46 (#XBEE_FRAME_SOCK_BIND_LISTEN).
/// Sent from host to XBee, which responds with XBEE_FRAME_SOCK_LISTEN_RESP.
/// New TCP/SSL connections result in XBee sending an
/// XBEE_FRAME_SOCK_IPV4_CLIENT frame.  For UDP sockets, XBee sends an
/// XBEE_FRAME_SOCK_RECEIVE_FROM frame for every inbound packet.
typedef XBEE_PACKED(xbee_frame_sock_bind_listen_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_BIND_LISTEN (0x46)
    uint8_t     frame_id;       ///< identifier to match response frame
    uint8_t     socket_id;      ///< socket used for connection
    uint16_t    local_port_be;  ///< port to listen on
}) xbee_frame_sock_bind_listen_t;


/// Frame Type: Extended Socket Create Response
#define XBEE_FRAME_SOCK_CREATE_RESP     0xC0

/// Frame Type: Extended Socket Connect Response
#define XBEE_FRAME_SOCK_CONNECT_RESP    0xC2

/// Frame Type: Extended Socket Close Response
#define XBEE_FRAME_SOCK_CLOSE_RESP      0xC3

/// Frame Type: Extended Socket Bind/Listen Response
#define XBEE_FRAME_SOCK_LISTEN_RESP     0xC6

/// Format of multiple XBee API frame types sent from XBee to host:
///     0xC0 (#XBEE_FRAME_SOCK_CREATE_RESP)
///     0xC2 (#XBEE_FRAME_SOCK_CONNECT_RESP)
///     0xC3 (#XBEE_FRAME_SOCK_CLOSE_RESP)
///     0xC6 (#XBEE_FRAME_SOCK_LISTEN_RESP)
typedef XBEE_PACKED(xbee_frame_sock_shared_resp_t, {
    /// XBEE_FRAME_SOCK_CREATE_RESP (0xC0), XBEE_FRAME_SOCK_CONNECT_RESP (0xC2),
    /// XBEE_FRAME_SOCK_CLOSE_RESP (0xC3), or XBEE_FRAME_SOCK_LISTEN_RESP (0xC6)
    uint8_t     frame_type;
    uint8_t     frame_id;       ///< identifier from request
    uint8_t     socket_id;      ///< socket closed
    uint8_t     status;         ///< request status, see XBEE_SOCK_STATUS_xxx
}) xbee_frame_sock_shared_resp_t;


/// Frame Type: Extended Socket IPv4 Client
#define XBEE_FRAME_SOCK_IPV4_CLIENT 0xCC
/// Format of XBee API frame type 0xCC (#XBEE_FRAME_SOCK_IPV4_CLIENT);
/// sent from XBee to host for new connections on a listening socket.
typedef XBEE_PACKED(xbee_frame_sock_ipv4_client_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_IPV4_CLIENT (0xCC)
    uint8_t     sock_listen_id; ///< listening socket's ID
    uint8_t     sock_client_id; ///< new socket for this client connection
    uint32_t    remote_addr_be; ///< remote IPv4 address
    uint16_t    remote_port_be; ///< remote port
}) xbee_frame_sock_ipv4_client_t;


/// Frame Type: Extended Socket Receive
#define XBEE_FRAME_SOCK_RECEIVE         0xCD
/// Format of XBee API frame type 0xCD (#XBEE_FRAME_SOCK_RECEIVE);
/// sent from XBee to host when data arrives on a connected socket.
typedef XBEE_PACKED(xbee_frame_sock_receive_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_RECEIVE (0xCD)
    uint8_t     frame_id;       ///< identifier from request
    uint8_t     socket_id;      ///< socket that received data
    uint8_t     status;         ///< reserved for status information (set to 0)
    uint8_t     payload[1];     ///< variable-length payload
}) xbee_frame_sock_receive_t;


/// Frame Type: Extended Socket Receive From
#define XBEE_FRAME_SOCK_RECEIVE_FROM    0xCE
/// Format of XBee API frame type 0xCE (#XBEE_FRAME_SOCK_RECEIVE_FROM);
/// sent from XBee to host when a datagram arrives on a bound UDP socket.
typedef XBEE_PACKED(xbee_frame_sock_receive_from_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_RECEIVE_FROM (0xCE)
    uint8_t     frame_id;       ///< identifier from request
    uint8_t     socket_id;      ///< socket that received data
    uint32_t    remote_addr_be; ///< remote IPv4 address
    uint16_t    remote_port_be; ///< remote port
    uint8_t     status;         ///< reserved for status information (set to 0)
    uint8_t     payload[1];     ///< variable-length payload
}) xbee_frame_sock_receive_from_t;


/// Frame Type: Extended Socket State
#define XBEE_FRAME_SOCK_STATE           0xCF
/// Format of XBee API frame type 0xCF (#XBEE_FRAME_SOCK_STATE);
/// sent from XBee to host when a socket's state changes.  All values other
/// than 0x00 (CONNECTED) are fatal, resulting in a closed socket and
/// invalidated socket_id.
typedef XBEE_PACKED(xbee_frame_sock_state_t, {
    uint8_t     frame_type;     ///< XBEE_FRAME_SOCK_STATE (0xCF)
    uint8_t     socket_id;      ///< socket with updated state
    uint8_t     state;          ///< new socket state
#define XBEE_SOCK_STATE_CONNECTED               0x00
#define XBEE_SOCK_STATE_DNS_FAILED              0x01
#define XBEE_SOCK_STATE_CONNECTION_REFUSED      0x02
#define XBEE_SOCK_STATE_TRANSPORT_CLOSED        0x03
#define XBEE_SOCK_STATE_TIMED_OUT               0x04
#define XBEE_SOCK_STATE_INTERNAL_ERR            0x05
#define XBEE_SOCK_STATE_HOST_UNREACHABLE        0x06
#define XBEE_SOCK_STATE_CONNECTION_LOST         0x07    ///< remote closed socket
#define XBEE_SOCK_STATE_UNKNOWN_ERR             0x08
#define XBEE_SOCK_STATE_UNKNOWN_SERVER          0x09
#define XBEE_SOCK_STATE_RESOURCE_ERR            0x0A
#define XBEE_SOCK_STATE_LISTENER_CLOSED         0x0B
// ^^^ Be sure to update xbee_sock_state_str() when adding to this macro list.

}) xbee_frame_sock_state_t;

/// Buffer size used for xbee_sock_status_str() and xbee_sock_state_str().
#define XBEE_SOCK_STR_BUF_SIZE          40

/// Get a description of an XBEE_SOCK_STATUS_xxx value (returned in multiple
/// socket response frame types) for error messages.
const char *xbee_sock_status_str(uint8_t status,
                                 char buffer[XBEE_SOCK_STR_BUF_SIZE]);

/// Get a description of an XBEE_SOCK_STATE_xxx value (returned in an
/// XBEE_FRAME_SOCK_STATE) for error messages.
const char *xbee_sock_state_str(uint8_t state,
                                char buffer[XBEE_SOCK_STR_BUF_SIZE]);

XBEE_END_DECLS

#endif // XBEE_SOCKET_FRAMES_H

///@}
