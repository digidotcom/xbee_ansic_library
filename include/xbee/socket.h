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
    @defgroup xbee_socket Frames: Extended Sockets (0x40-0x4F and 0xC0-0xCF)
    @ingroup xbee_frame
    @{
    @file xbee/socket.h

    Because this framework dispatches received frames to frame handlers, this
    API layer for sockets makes use of callbacks to pass received data back to
    the calling code.

    Calling code uses xbee_sock_create() to create a new local socket entry and
    allocate a socket on the XBee module.  It also registers a callback for
    notifications related to that socket.  Those notifications include Create,
    Connect and Bind/Listen status events, delivery status of payloads sent
    with xbee_sock_send()/xbee_sock_sendto(), and socket state changes.

    xbee_sock_create() returns an xbee_sock_t used to identify the socket in
    all other APIs.

    Once created, the caller can attach the socket in three different ways:

    API to initiate     Callback Prototype         Protos   Description
    -----------------   -------------------------  -------  -------------------
    xbee_sock_connect   xbee_sock_receive_fn         All    Single destination
    xbee_sock_bind      xbee_sock_receive_from_fn    UDP    Mult. destinations
    xbee_sock_listen    xbee_sock_ipv4_client_fn   TCP/SSL  Inbound connections

    - A Connected socket sends and receives data from a single destination.  Use
      xbee_sock_send() to send data.

    - A Bound UDP socket receives datagrams from and can send datagrams to any
      destination.  Use xbee_sock_sendto() to send data.

    - A Listening socket spawns new Connected sockets from inbound connections.
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
/// Number of sockets to track on the host.  Base on number of simultaneous
/// sockets while considering XBee Cellular device's limits.
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

    @sa xbee_sock_create
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
    @param[in]  payload_len     Length of received data, up to 1500 bytes.

    @sa xbee_sock_connect, xbee_sock_send, xbee_sock_receive_from_fn
*/
typedef void (*xbee_sock_receive_fn)(xbee_sock_t socket,
                                     uint8_t status,
                                     const void *payload,
                                     size_t payload_len);

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
    @param[in]  datagram_len    Length of datagram, up to 1500 bytes.

    @sa xbee_sock_bind, xbee_sock_sendto, xbee_sock_receive_fn
*/
typedef void (*xbee_sock_receive_from_fn)(xbee_sock_t socket,
                                          uint8_t status,
                                          uint32_t remote_addr,
                                          uint16_t remote_port,
                                          const void *datagram,
                                          size_t datagram_len);

/**
    @brief
    Callback for TCP/SSL sockets created with xbee_sock_listen().

    @param[in]  listening_socket    Socket that received the connection.
    @param[in]  client_socket       New socket created for the client connection
                                    (acts like an xbee_socket_connect() socket).
    @param[in]  remote_addr         Client's IPv4 address.
    @param[in]  remote_port         Client's TCP/SSL port.

    @return     Handler to use for data received on the new client socket, or
                NULL to reject the connection and close the socket.

    @sa xbee_sock_listen, xbee_sock_receive_fn
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
    @param[in]  data_len        Data length (for GET) or 0 (for SET).

    @sa xbee_sock_option
*/
typedef void (*xbee_sock_option_resp_fn)(xbee_sock_t socket,
                                         uint8_t option_id,
                                         uint8_t status,
                                         const void *data,
                                         size_t data_len);


/**
    @brief
    Allocate a socket from the local socket table, and send a Socket Create
    frame to the XBee module to allocate space in its socket table.  Socket
    is PENDING until XBee returns a Socket Create Response.

    @param[in]  xbee            XBee device to create socket on
    @param[in]  protocol        one of XBEE_SOCK_PROTOCOL_UDP, _TCP or _SSL
    @param[in]  notify_handler  Callback to receive socket notifications from
                                multiple frame types.  Must be non-NULL.

    @retval     >0              Socket identifier of newly allocated socket.
    @retval     -EINVAL         Invalid protocol or NULL notify_handler.
    @retval     -ENOSPC         Active socket table is full.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_notify_fn
*/
xbee_sock_t xbee_sock_create(xbee_dev_t *xbee,
                             uint8_t protocol,
                             xbee_sock_notify_fn notify_handler);

/**
    @brief
    Connect a socket created with xbee_sock_create() to a remote host.
    Caller must wait for xbee_sock_notify_fn callback with a \a frame_type of
    XBEE_FRAME_SOCK_CONNECT_RESP and \a message of 0 before sending.

    @param[in]  socket          Socket ID returned from xbee_sock_create().
    @param[in]  remote_port     Remote device's port.
    @param[in]  remote_addr     Remote device's address as a 32-bit value
                                (0x01020304 = 1.2.3.4) or zero to make use of
                                /a remote_host.
    @param[in]  remote_host     Remote hostname or a "dotted quad" IPv4 address
                                as a null-terminated string.
    @param[in]  receive_handler         Callback for data received on socket
                                        once established.  Must be non-NULL.

    @retval     0               Sent Socket Connect frame to XBee module.
    @retval     -EINVAL         Invalid parameter passed to function.
    @retval     -ENOENT         Invalid socket ID passed to function.
    @retval     -EPERM          Socket already connected/bound/listening.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_create, xbee_sock_send, xbee_sock_receive_fn
*/
int xbee_sock_connect(xbee_sock_t socket, uint16_t remote_port,
                      uint32_t remote_addr, const char *remote_host,
                      xbee_sock_receive_fn receive_handler);

/**
    @brief
    Bind a UDP socket to a local port to accept datagrams from multiple hosts.
    Caller must wait for xbee_sock_notify_fn callback with a \a frame_type of
    XBEE_FRAME_SOCK_LISTEN_RESP and \a message of 0 before sending.

    @param[in]  socket          Socket ID returned from xbee_sock_create().
    @param[in]  local_port      Port to bind to.
    @param[in]  receive_from_handler    Callback for data received on socket
                                        once established.  Must be non-NULL.

    @retval     0               Sent Socket Bind/Listen frame to XBee module.
    @retval     -EINVAL         Invalid parameter passed to function.
    @retval     -ENOENT         Invalid socket ID passed to function.
    @retval     -EPERM          Socket already connected/bound/listening, or
                                not a UDP socket.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_create, xbee_sock_sendto, xbee_sock_receive_from_fn
*/
int xbee_sock_bind(xbee_sock_t socket, uint16_t local_port,
                   xbee_sock_receive_from_fn receive_from_handler);

/**
    @brief
    Listen on a local port for inbound TCP/SSL connections from multiple hosts.

    @param[in]  socket          Socket ID returned from xbee_sock_create().
    @param[in]  local_port      Port to listen on.
    @param[in]  ipv4_client_handler     Callback for client notifications from
                                        listening socket.  Must be non-NULL.

    @retval     0               Sent Socket Bind/Listen frame to XBee module.
    @retval     -EINVAL         Invalid parameter passed to function.
    @retval     -ENOENT         Invalid socket ID passed to function.
    @retval     -EPERM          Socket already connected/bound/listening, or
                                not a UDP socket.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_create, xbee_sock_ipv4_client_fn
*/
int xbee_sock_listen(xbee_sock_t socket, uint16_t local_port,
                     xbee_sock_ipv4_client_fn ipv4_client_handler);

// Future: maybe make tx_options an int or uint16_t -- lower byte passed in
// frame, upper byte used for API options (like DISABLE_TX_STATUS).
/**
    @brief
    Send data on a Connected socket, or an IPv4 Client socket spawned from
    a Listening socket.

    This function has a hard limit of 1500 bytes for the payload, and returns
    -EMSGSIZE for values above 1500.  An XBee Cellular module may have a lower
    limit in some scenarios (e.g., UDP sockets on XBee3 Cellular LTE-M/NBIoT)
    and will respond with an XBEE_TX_DELIVERY_PAYLOAD_TOO_BIG delivery status
    (passed to the socket's Notify handler.

    @param[in]  socket          Socket ID returned from xbee_sock_create().
    @param[in]  tx_options      Reserved field of Socket Send frame; set to 0.
    @param[in]  payload         Data to send.
    @param[in]  payload_len     Length of data to send (maximum of 1500 bytes).

    @retval     0               Sent Socket Send frame to XBee module.
    @retval     -EINVAL         Invalid parameter passed to function.
    @retval     -ENOENT         Invalid socket ID passed to function.
    @retval     -EPERM          Not a Connected socket.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
    @retval     -EMSGSIZE       Serial buffer can't hold a frame this large,
                                or \a payload_len is over 1500 bytes.

    @sa xbee_sock_connect, xbee_sock_receive_fn, xbee_sock_sendto
*/
int xbee_sock_send(xbee_sock_t socket, uint8_t tx_options,
                   const void *payload, size_t payload_len);

/**
    @brief
    Send a datagram on a Bound UDP socket.

    This function has a hard limit of 1500 bytes for the payload, and returns
    -EMSGSIZE for values above 1500.  An XBee Cellular module may have a lower
    limit in some scenarios (e.g., UDP sockets on XBee3 Cellular LTE-M/NBIoT)
    and will respond with an XBEE_TX_DELIVERY_PAYLOAD_TOO_BIG delivery status
    (passed to the socket's Notify handler).

    @param[in]  socket          Socket ID returned from xbee_sock_create().
    @param[in]  tx_options      Reserved field of Socket SendTo frame; set to 0.
    @param[in]  remote_addr     Remote device's address as a 32-bit value
                                (0x01020304 = 1.2.3.4).
    @param[in]  remote_port     Remote device's port.
    @param[in]  payload         Data to send.
    @param[in]  payload_len     Length of data to send (maximum of 1500 bytes).

    @retval     0               Sent Socket Send frame to XBee module.
    @retval     -EINVAL         Invalid parameter passed to function.
    @retval     -ENOENT         Invalid socket ID passed to function.
    @retval     -EPERM          Not a Bound socket.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
    @retval     -EMSGSIZE       Serial buffer can't hold a frame this large,
                                or \a payload_len is over 1500 bytes.

    @sa xbee_sock_bind, xbee_sock_receive_from_fn, xbee_sock_send
*/
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
    @param[in]  data_len        Length of \a data, or 0 if \a data is NULL.
    @param[in]  callback        Function called with data in Option Response.
                                Only used if retval is 0.  Can be NULL to
                                ignore the response.

    @retval     0               Sent request to get/set the socket option.
    @retval     -ENOENT         \a socket isn't a valid ID
    @retval     -EPERM          Waiting for Create Response from XBee module.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_option_resp_fn
*/
int xbee_sock_option(xbee_sock_t socket, uint8_t option_id, const void *data,
                     size_t data_len, xbee_sock_option_resp_fn callback);

/**
    @brief
    Send command to close all open sockets on device \a xbee, even if they
    aren't tracked in the local sockets table.  Useful if a previous program
    left sockets open on the device.

    @param[in]  xbee            XBee device for socket closures.

    @retval     0               Sent request to close all sockets.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_close, xbee_sock_close_all
*/
int xbee_sock_reset(xbee_dev_t *xbee);


/**
    @brief
    Close and release a socket.

    @param[in]  socket          Socket to close.

    @retval     0               Sent request to close the socket. \a socket
                                is no longer valid.
    @retval     -ENOENT         \a socket isn't a valid ID
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_reset, xbee_sock_close_all
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

    @sa xbee_sock_reset, xbee_sock_close
*/
int xbee_sock_close_all(xbee_dev_t *xbee);


/**
    @brief
    Frame handler for all response frames.

    View the documentation of xbee_frame_handler_fn() for this function's
    parameters and return value.

    @sa xbee_frame_handler_fn
*/
int xbee_sock_frame_handler(xbee_dev_t *xbee, const void FAR *rawframe,
                            uint16_t length, void FAR *context);


/// Macro for xbee_frame_handlers table of xbee_dispatch_table_entry_t structs.
#define XBEE_SOCK_FRAME_HANDLERS        { 0, 0, xbee_sock_frame_handler, NULL }

XBEE_END_DECLS

#endif // XBEE_SOCKET_H

///@}
