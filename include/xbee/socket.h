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
    @file xbee/socket.h

    Support code for Extended Socket frames (0x40-0x4F and 0xC0-0xCF).

    TODO: lots of Doxygen-style docs for everything in here.
*/

#ifndef XBEE_SOCKET_H
#define XBEE_SOCKET_H

#include "xbee/platform.h"
#include "xbee/socket_frames.h"
#include "xbee/tx_status.h"

XBEE_BEGIN_DECLS

/// Unique identifier for sockets created in this API layer.  Valid IDs are
/// greater than 0, and APIs returning an xbee_sock_t use -Exxx macros to
/// signal errors.
typedef int xbee_sock_t;

#ifndef XBEE_SOCK_SOCKET_COUNT
#define XBEE_SOCK_SOCKET_COUNT          10
#endif

/**
    @brief
    Callback handler for notification of various socket events.

    \a message contains an XBEE_SOCK_STATUS_xxx value for \a frame_type values:
        - XBEE_FRAME_SOCK_CREATE_RESP
        - XBEE_FRAME_SOCK_CONNECT_RESP
        - XBEE_FRAME_SOCK_CLOSE_RESP
        - XBEE_FRAME_SOCK_LISTEN_RESP

    \a message contains an XBEE_TX_DELIVERY_xxx value for \a frame_type
    XBEE_FRAME_TX_STATUS

    \a message contains an XBEE_SOCK_STATE_xxx value for \a frame_type
    XBEE_FRAME_SOCK_STATE

    @param[in]  socket          Socket receiving notification.
    @param[in]  frame_type      XBee frame responsible for the notification.
    @param[in]  message         Status/state delivered.  0x00 always indicates
                                success; meaning of non-zero values varies by
                                value of \a frame_type.
*/
typedef void (*xbee_sock_notify_fn)(xbee_sock_t socket,
                                    uint8_t frame_type,
                                    uint8_t message);

/**
    @brief
    Callback for data received on sockets created with xbee_sock_connect().

    For UDP sockets, this is a single datagram.  For TCP/SSL sockets this is
    data from the stream.  Note that the status bitfield is currently unused.

    @param[in]  socket          Socket that received the data.
    @param[in]  status          Status bitfield from Socket Receive frame.
    @param[in]  payload         Received data.
    @param[in]  payload_length  Length of received data.

    @sa xbee_sock_receive_from_fn, xbee_sock_connect
*/
typedef void (*xbee_sock_receive_fn)(xbee_sock_t socket,
                                     uint8_t status,
                                     const void *payload,
                                     size_t payload_length);

/**
    @brief
    Callback for datagrams received on UDP sockets created with
    xbee_sock_bind().

    Note that the status bitfield is currently unused.

    @param[in]  socket          Socket that received the data.
    @param[in]  status          Status bitfield from Socket Receive From frame.
    @param[in]  remote_addr     Remote device's IPv4 address.
    @param[in]  remote_port     Remote device's UDP port.
    @param[in]  datagram        Datagram payload.
    @param[in]  datagram_length Length of datagram.

    @sa xbee_sock_receive_fn, xbee_sock_bind
*/
typedef void (*xbee_sock_receive_from_fn)(xbee_sock_t socket,
                                          uint8_t status,
                                          uint32_t remote_addr,
                                          uint16_t remote_port,
                                          const void *datagram,
                                          size_t datagram_length);

/**
    @brief
    Callback for TCP/SSL sockets created with xbee_sock_listen().

    @param[in]  listening_socket    Socket that received the connection.
    @param[in]  client_socket       New socket created for the client connection.
    @param[in]  remote_addr         Client's IPv4 address.
    @param[in]  remote_port         Client's TCP/SSL port.

    @return     Handler to use for data received on the new client socket, or
                NULL to reject the connection and close the socket.
*/
typedef xbee_sock_receive_fn
    (*xbee_sock_ipv4_client_fn)(xbee_sock_t listening_socket,
                                xbee_sock_t client_socket,
                                uint32_t remote_addr,
                                uint16_t remote_port);

/**
    @brief
    Callback for responses to Get/Set Option requests sent with
    xbee_sock_option().

    @param[in]  socket          Socket of original request.
    @param[in]  option_id       Option ID of original request.
    @param[in]  status          Status of the request.  0x00 indicates SUCCESS,
                                see XBEE_SOCK_STATUS_xxx for other values.
    @param[in]  data            Value of Option ID (for GET) or NULL (for SET).
    @param[in]  data_length     Data length (for GET) or 0 (for SET).

    @sa xbee_sock_option
*/
typedef void (*xbee_sock_option_resp_fn)(xbee_sock_t socket,
                                         uint8_t option_id,
                                         uint8_t status,
                                         const void *data,
                                         size_t data_length);


/**
    @brief
    Allocate a socket from the local socket table, and send a Socket Create
    frame to the XBee module to allocate space in its socket table.  Socket
    is PENDING until XBee returns a Socket Create Response.

    TODO: Should there be a handler for Socket Option Response handling if the
    caller plans to use that after the socket's established?

    @param[in]  protocol        one of XBEE_SOCK_PROTOCOL_UDP, _TCP or _SSL
    @param[in]  notify_handler  Callback to receive socket notifications from
                                multiple frame types.

    @retval     -EINVAL         invalid protocol or NULL created_handler
    @retval     -ENOSPC         active socket table is full
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
    @retval     >0              socket identifier of newly allocated socket

    @sa xbee_sock_notify_fn
*/
xbee_sock_t xbee_sock_create(xbee_dev_t *xbee,
                             uint8_t protocol,
                             xbee_sock_notify_fn notify_handler);

/**
    @brief
    Connect a socket created with xbee_sock_create() to a remote host.

    Uses \a remote_host when \a remote_addr is 0.
*/
int xbee_sock_connect(xbee_sock_t socket, uint16_t remote_port,
                      uint32_t remote_addr, const char *remote_host,
                      xbee_sock_receive_fn receive_handler);

int xbee_sock_bind(xbee_sock_t socket, uint16_t local_port,
                   xbee_sock_receive_from_fn receive_from_handler);

int xbee_sock_listen(xbee_sock_t socket, uint16_t local_port,
                     xbee_sock_ipv4_client_fn ipv4_client_handler);

// TODO: maybe make tx_options an int or uint16_t -- lower byte passed in frame,
// upper byte used for API options (like DISABLE_TX_STATUS).
int xbee_sock_send(xbee_sock_t socket, uint8_t tx_options,
                   const void *payload, size_t payload_len);

int xbee_sock_sendto(xbee_sock_t socket, uint8_t tx_options,
                     uint32_t remote_addr, uint16_t remote_port,
                     const void *payload, size_t payload_len);

/**
    @brief
    Get or set a socket option.

    See XBEE_SOCK_OPT_ID_xxx macros for values of \a option_id.

    @param[in]  socket          Socket to get or set the option on.
    @param[in]  option_id       Option to get or set.
    @param[in]  data            Value to set or NULL to get the current value.
    @param[in]  data_length     Length of \a data, or 0 if \a data is NULL.
    @param[in]  callback        Function called with data in Option Response.
                                Only used if retval is 0.  Can be NULL to
                                ignore the response.

    @retval     -ENOENT         \a socket isn't a valid ID
    @retval     -EPERM          Waiting for Create Response from XBee module.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
    @retval     0               Sent request to get/set the socket option.
*/
int xbee_sock_option(xbee_sock_t socket, uint8_t option_id, const void *data,
                     size_t data_length, xbee_sock_option_resp_fn callback);

/**
    @brief
    Send command to close all open sockets on device \a xbee, even if they
    aren't tracked in the local sockets table.  Useful if a previous program
    left sockets open on the device.

    @param[in]  xbee            XBee device for socket closures.

    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
    @retval     0               Sent request to close all sockets.
*/
int xbee_sock_reset(xbee_dev_t *xbee);


/**
    @brief
    Close and release a socket.

    @retval     -ENOENT         \a socket isn't a valid ID
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
    @retval     0               Sent request to close the socket. \a socket
                                is no longer valid.
*/
int xbee_sock_close(xbee_sock_t socket);


/**
    @brief
    Close all created sockets for a given XBee device.

    @param[in]  xbee            XBee device for socket closures, or NULL for
                                all sockets.

    @retval     0               Socket Close sent for all allocated sockets.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
*/
int xbee_sock_close_all(xbee_dev_t *xbee);


/// Single handler for all frames, reduces number of entries in frame handler
/// table and can use a simplier API for each frame type (length only necessary
/// for variable-length frames, drop unused context, cast the void pointer
/// to the correct frame type).
int xbee_sock_frame_handler(xbee_dev_t *xbee, const void FAR *rawframe,
                            uint16_t length, void FAR *context);

#define XBEE_SOCK_FRAME_HANDLERS        { 0, 0, xbee_sock_frame_handler, NULL }

XBEE_END_DECLS

#endif // XBEE_SOCKET_H

