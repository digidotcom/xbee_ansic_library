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
    @file xbee_socket.c

    See xbee/socket.h for summary of API.
*/

#if 0
#include <stdio.h>
#define debug_printf(...)       fprintf(stderr, __VA_ARGS__)
#else
#define debug_printf(...)
#endif

#include <assert.h>

#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/ipv4.h"
#include "xbee/socket.h"

/*
    State machine for .type element of socket_t:

    AVAILABLE -> PENDING        user code calls xbee_sock_create()
    PENDING -> CREATED          handle_create_resp() frame handler
    CREATED -> ATTACH           user code calls xbee_sock_connect(),
                                xbee_sock_bind() or xbee_sock_listen()
    handle_attach_resp():
    ATTACH -> CONNECT           XBEE_FRAME_SOCK_CONNECT_RESP frame
    ATTACH -> BIND              XBEE_FRAME_SOCK_LISTEN_RESP frame (UDP)
    ATTACH -> LISTEN            XBEE_FRAME_SOCK_LISTEN_RESP frame (TCP/SSL)

    * -> AVAILABLE              xbee_sock_close(), xbee_sock_close_all(), or
                                xbee_sock_reset()
*/

// Private/opaque structure, intentionally not exposed in a header file.
typedef struct xbee_sock_socket {
    uint8_t     type;
#define XBEE_SOCK_SOCKET_TYPE_AVAILABLE 0x00    ///< slot in table available
#define XBEE_SOCK_SOCKET_TYPE_PENDING   0x01    ///< waiting for Create Response
#define XBEE_SOCK_SOCKET_TYPE_CREATED   0x02    ///< allocated but not established
#define XBEE_SOCK_SOCKET_TYPE_ATTACH    0x03    ///< waiting for Connect/Bind/Listen Resp
#define XBEE_SOCK_SOCKET_TYPE_CONNECT   0x04    ///< point-to-point connection
#define XBEE_SOCK_SOCKET_TYPE_BIND      0x05    ///< UDP socket accepting datagrams
#define XBEE_SOCK_SOCKET_TYPE_LISTEN    0x06    ///< TCP/SSL socket accepting connections
    uint8_t     create_frame_id;        ///< used to match Create Response frame
    uint8_t     tx_frame_id;            ///< used to match TX Status frame
    uint8_t     socket_id;
    uint8_t     protocol;
    xbee_dev_t  *xbee;                  ///< device hosting the socket

    /// Callback for Create, Connect, Listen, Close, State, & TX Status responses.
    xbee_sock_notify_fn                 notify_handler;

    /// Callback for most recent Get/Set Option request
    xbee_sock_option_resp_fn            option_handler;

    /// Callbacks based on the socket type.
    union {
        xbee_sock_receive_fn            receive;        ///< for TYPE_CONNECT
        xbee_sock_receive_from_fn       receive_from;   ///< for TYPE_BIND
        xbee_sock_ipv4_client_fn        ipv4_client;    ///< for TYPE_LISTEN
    } handler;
} socket_t;

static socket_t socket_table[XBEE_SOCK_SOCKET_COUNT];

/// Return the public ID that callers use to reference this socket.  Don't
/// call this function until after sending the Socket Create frame and setting
/// the create_frame_id of the structure.
///
/// Currently table_index in high byte and create_frame_id in low byte.
static xbee_sock_t _xbee_sock_t(socket_t *s)
{
    if (s == NULL) {
        return -ENOSPC;                 // unable to allocate socket
    }
    return ((s - &socket_table[0]) << 8) + s->create_frame_id;
}

/// Reverse of _xbee_sock_t() -- get socket_t * from an xbee_sock_t.
static socket_t *_lookup_sock_t(xbee_sock_t sock)
{
    unsigned index = sock >> 8;
    uint8_t create_frame_id = (uint8_t)sock;

    if (index < XBEE_SOCK_SOCKET_COUNT
        && socket_table[index].create_frame_id == create_frame_id)
    {
        return &socket_table[index];
    }

    debug_printf("%s: failed to find xbee_sock_t 0x%04X\n",
                 __FUNCTION__, sock);
    return NULL;
}

/// Helper used in frame handlers to match received frame to our socket entry.
static socket_t *_lookup_socket_id(xbee_dev_t *xbee, uint8_t socket_id)
{
    socket_t *s;

    // search socket table for in-use socket with matching ID
    for (s = &socket_table[0]; s < &socket_table[XBEE_SOCK_SOCKET_COUNT]; ++s) {
        if (s->type != XBEE_SOCK_SOCKET_TYPE_AVAILABLE
            && s->xbee == xbee
            && s->socket_id == socket_id)
        {
            return s;
        }
    }

    debug_printf("%s: failed to find socket_id 0x%02X\n",
                 __FUNCTION__, socket_id);
    return NULL;
}

/// Free an entry allocated from the socket table with _socket_alloc().
static void _socket_free(socket_t *s)
{
    memset(s, 0, sizeof(*s));
}

/// Returns a pointer to an open entry in the sockets table, with its state
/// updated to PENDING.
static socket_t *_socket_alloc(xbee_dev_t *xbee)
{
    socket_t *s;

    // search socket table for unused socket
    for (s = &socket_table[0]; s < &socket_table[XBEE_SOCK_SOCKET_COUNT]; ++s) {
        INTERRUPT_ENABLE;
        if (s->type == XBEE_SOCK_SOCKET_TYPE_AVAILABLE) {
            s->type = XBEE_SOCK_SOCKET_TYPE_PENDING;
            INTERRUPT_DISABLE;
            s->xbee = xbee;
            return s;
        }
        INTERRUPT_DISABLE;
    }

    debug_printf("%s: table is full\n", __FUNCTION__);
    return NULL;
}


// API documented in xbee/socket.h
xbee_sock_t xbee_sock_create(xbee_dev_t *xbee,
                             uint8_t protocol,
                             xbee_sock_notify_fn notify_handler)
{
    int retval;

    // Require a handler for notifications (status responses and state changes).
    if (notify_handler == NULL) {
        return -EINVAL;
    }

    // Note that we could include code to validate the protocol field, but
    // we can rely on the XBee module to do that validation.  In doing so,
    // we don't have to keep a protocol list up to date, and it's possible
    // a module wouldn't accept all known protocols (e.g., UDP-only or a
    // module without SSL sockets).

    // Make sure we have room in our socket table before sending a Socket
    // Create request to the XBee.
    socket_t *s = _socket_alloc(xbee);
    if (s == NULL) {
        return -ENOSPC;
    }

    xbee_frame_sock_create_t frame;
    frame.frame_type = XBEE_FRAME_SOCK_CREATE;
    frame.frame_id = xbee_next_frame_id(xbee);
    frame.protocol = protocol;
    retval = xbee_frame_write(xbee, &frame, sizeof(frame),
                              NULL, 0, XBEE_WRITE_FLAG_NONE);

    if (retval) {                       // Unable to send frame to XBee module.
        _socket_free(s);
        return retval;
    }

    s->notify_handler = notify_handler;
    s->create_frame_id = frame.frame_id;
    s->protocol = protocol;

    debug_printf("%s: created socket 0x%04X\n", __FUNCTION__, _xbee_sock_t(s));

    return _xbee_sock_t(s);
}


// API documented in xbee/socket.h
int xbee_sock_connect(xbee_sock_t socket, uint16_t remote_port,
                      uint32_t remote_addr, const char *remote_host,
                      xbee_sock_receive_fn receive_handler)
{
    if (receive_handler == NULL
        || remote_port == 0
        || (remote_addr == 0 && remote_host == NULL))
    {
        return -EINVAL;
    }

    socket_t *s = _lookup_sock_t(socket);
    if (s == NULL) {
        return -ENOENT;
    }

    if (s->type > XBEE_SOCK_SOCKET_TYPE_CREATED) {
        // socket attaching/attached (connected, bound or listening)
        return -EPERM;
    }

    xbee_header_sock_connect_t header;
    uint32_t remote_addr_be;
    const void *payload;
    uint16_t payload_len;

    header.frame_type = XBEE_FRAME_SOCK_CONNECT;
    header.frame_id = xbee_next_frame_id(s->xbee);
    header.socket_id = s->socket_id;
    header.remote_port_be = htobe16(remote_port);
    if (remote_addr == 0) {
        debug_printf("%s: sock 0x%04X to %s:%u\n", __FUNCTION__,
                     socket, remote_host, remote_port);
        header.addr_type = XBEE_SOCK_ADDR_TYPE_HOSTNAME;
        payload = remote_host;
        payload_len = strlen(remote_host);
    } else {
        debug_printf("%s: sock 0x%04X to 0x%08X:%u\n", __FUNCTION__,
                     socket, remote_addr, remote_port);
        header.addr_type = XBEE_SOCK_ADDR_TYPE_IPV4;
        remote_addr_be = htobe32(remote_addr);
        payload = &remote_addr_be;
        payload_len = sizeof(remote_addr_be);
    }

    int retval = xbee_frame_write(s->xbee, &header, sizeof(header),
                                  payload, payload_len, XBEE_WRITE_FLAG_NONE);

    debug_printf("%s: connect socket 0x%02X frame 0x%02X result %d\n", __FUNCTION__,
                 s->socket_id, header.frame_id, retval);

    if (retval == 0) {
        s->type = XBEE_SOCK_SOCKET_TYPE_ATTACH;
        s->handler.receive = receive_handler;
    }

    return retval;
}


/**
    @brief
    Common code shared by xbee_sock_bind() and xbee_sock_listen().

    @param[in]  socket          Socket ID returned from xbee_sock_create().
    @param[in]  local_port      Port to listen on.
    @param[in]  is_bind         TRUE for _bind(), FALSE for _listen().
    @param[out] sock            Allocated socket (if return value is 0).

    @retval     0               Sent Socket Bind/Listen frame to XBee module.
    @retval     -ENOENT         Invalid socket ID passed to function.
    @retval     -EPERM          Socket already connected/bound/listening, or
                                not a UDP socket.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).

    @sa xbee_sock_bind, xbee_sock_listen
*/
static int _common_bind_listen(xbee_sock_t socket,
                               uint16_t local_port,
                               int is_bind,
                               socket_t **sock)
{
    socket_t *s = _lookup_sock_t(socket);
    if (s == NULL) {
        return -ENOENT;
    }

    // can't bind/listen if already connected/bound/listening
    // can't bind non-UDP or listen on UDP
    if (s->type > XBEE_SOCK_SOCKET_TYPE_CREATED
        || is_bind != (s->protocol == XBEE_SOCK_PROTOCOL_UDP))
    {
        return -EPERM;
    }

    xbee_frame_sock_bind_listen_t frame;
    frame.frame_type = XBEE_FRAME_SOCK_BIND_LISTEN;
    frame.frame_id = xbee_next_frame_id(s->xbee);
    frame.socket_id = s->socket_id;
    frame.local_port_be = htobe16(local_port);

    int retval = xbee_frame_write(s->xbee, &frame, sizeof(frame),
                                  NULL, 0, XBEE_WRITE_FLAG_NONE);

    debug_printf("%s: socket 0x%02X frame 0x%02X result %d\n", __FUNCTION__,
                 s->socket_id, frame.frame_id, retval);

    if (retval == 0) {
        s->type = XBEE_SOCK_SOCKET_TYPE_ATTACH;
        *sock = s;
    }

    return retval;
}


// API documented in xbee/socket.h
int xbee_sock_bind(xbee_sock_t socket, uint16_t local_port,
                   xbee_sock_receive_from_fn receive_from_handler)
{
    if (receive_from_handler == NULL || local_port == 0) {
        return -EINVAL;
    }

    socket_t *s;
    int retval = _common_bind_listen(socket, local_port, TRUE, &s);

    if (retval == 0) {
        s->handler.receive_from = receive_from_handler;
    }

    return retval;
}


// API documented in xbee/socket.h
int xbee_sock_listen(xbee_sock_t socket, uint16_t local_port,
                     xbee_sock_ipv4_client_fn ipv4_client_handler)
{
    if (ipv4_client_handler == NULL || local_port == 0) {
        return -EINVAL;
    }

    socket_t *s;
    int retval = _common_bind_listen(socket, local_port, FALSE, &s);

    if (retval == 0) {
        s->handler.ipv4_client = ipv4_client_handler;
    }

    return retval;
}


// API documented in xbee/socket.h
int xbee_sock_send(xbee_sock_t socket, uint8_t tx_options,
                   const void *payload, size_t payload_len)
{
    socket_t *s = _lookup_sock_t(socket);
    if (s == NULL) {
        return -ENOENT;
    }

    if (s->type != XBEE_SOCK_SOCKET_TYPE_CONNECT) {
        // only allow sending for connected socket (not listening/bound)
        return -EPERM;
    }

    if (payload_len > XBEE_IPV4_MAX_PAYLOAD) {
        return -EMSGSIZE;
    }

    xbee_header_sock_send_t header;
    header.frame_type = XBEE_FRAME_SOCK_SEND;
    header.frame_id = xbee_next_frame_id(s->xbee);
    header.socket_id = s->socket_id;
    header.options = tx_options;

    int retval = xbee_frame_write(s->xbee, &header, sizeof(header),
                                  payload, payload_len, XBEE_WRITE_FLAG_NONE);

    debug_printf("%s: %u bytes in frame 0x%02X  sock 0x%02X  result %d\n",
                 __FUNCTION__, (unsigned)payload_len, header.frame_id,
                 s->socket_id, retval);

    if (retval == 0) {
        s->tx_frame_id = header.frame_id;
    }

    return retval;

}


// API documented in xbee/socket.h
int xbee_sock_sendto(xbee_sock_t socket, uint8_t tx_options,
                     uint32_t remote_addr, uint16_t remote_port,
                     const void *payload, size_t payload_len)
{
    socket_t *s = _lookup_sock_t(socket);
    if (s == NULL) {
        return -ENOENT;
    }

    if (s->type != XBEE_SOCK_SOCKET_TYPE_BIND) {
        // only allow sending for bound socket (not connected/listening)
        return -EPERM;
    }

    if (payload_len > XBEE_IPV4_MAX_PAYLOAD) {
        return -EMSGSIZE;
    }

    xbee_header_sock_sendto_t header;
    header.frame_type = XBEE_FRAME_SOCK_SENDTO;
    header.frame_id = xbee_next_frame_id(s->xbee);
    header.socket_id = s->socket_id;
    header.options = tx_options;
    header.remote_addr_be = htobe32(remote_addr);
    header.remote_port_be = htobe16(remote_port);

    int retval = xbee_frame_write(s->xbee, &header, sizeof(header),
                                  payload, payload_len, XBEE_WRITE_FLAG_NONE);

    debug_printf("%s: %u bytes in frame 0x%02X  sock 0x%02X  result %d\n",
                 __FUNCTION__, (unsigned)payload_len, header.frame_id,
                 s->socket_id, retval);

    if (retval == 0) {
        s->tx_frame_id = header.frame_id;
    }

    return retval;
}


// API documented in xbee/socket.h
int xbee_sock_option(xbee_sock_t socket, uint8_t option_id, const void *data,
                     size_t data_len, xbee_sock_option_resp_fn callback)
{
    socket_t *s = _lookup_sock_t(socket);
    if (s == NULL) {
        return -ENOENT;
    }

    if (s->type < XBEE_SOCK_SOCKET_TYPE_CREATED) {
        // still waiting for Create Response
        return -EPERM;
    }

    xbee_header_sock_option_req_t header;
    header.frame_type = XBEE_FRAME_SOCK_OPTION_REQ;
    // if caller doesn't provide a callback, no point in asking for a response
    header.frame_id = callback == NULL ? 0 : xbee_next_frame_id(s->xbee);
    header.socket_id = s->socket_id;
    header.option_id = option_id;

    int retval = xbee_frame_write(s->xbee, &header, sizeof(header),
                                  data, data_len, XBEE_WRITE_FLAG_NONE);

    debug_printf("%s: %u bytes in frame 0x%02X  sock 0x%02X  result %d\n",
                 __FUNCTION__, (unsigned)data_len, header.frame_id,
                 s->socket_id, retval);

    if (retval == 0) {
        s->option_handler = callback;
    }

    return retval;
}


/**
    @brief
    Shared helper for sending a Socket Close frame.

    @param[in]  xbee            XBee device for socket closure.
    @param[in]  socket_id       Socket ID to use in frame.

    @retval     0               Sent request.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
*/
static int _sock_close_frame(xbee_dev_t *xbee, uint8_t socket_id)
{
    xbee_frame_sock_close_t frame;
    frame.frame_type = XBEE_FRAME_SOCK_CLOSE;

    // There doesn't appear to be any value in asking for Socket Close Response
    // frames.  If the close fails, why do we care?  We can clear our structure
    // whether the close was successful (socket closed) or not (socket invalid).
    frame.frame_id = 0;
    frame.socket_id = socket_id;

    return xbee_frame_write(xbee, &frame, sizeof(frame),
                            NULL, 0, XBEE_WRITE_FLAG_NONE);
}


/**
    @brief
    Shared helper for sending a Socket Close frame and freeing the entry from
    the socket table.

    @param[in]  s               Socket to close and release.

    @retval     0               Sent request and freed socket entry.
    @retval     -ENOENT         \a s is NULL.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
*/
static int _sock_close(socket_t *s)
{
    if (s == NULL) {
        return -ENOENT;
    }

    int retval = _sock_close_frame(s->xbee, s->socket_id);

    if (retval == 0) {
        debug_printf("%s: closing 0x%02X\n", __FUNCTION__, s->socket_id);
        if (s->notify_handler != NULL) {
            // Provide notification of socket closure (even though user
            // initiated it).  TODO: is this necessary?  Useful for close_all?
            s->notify_handler(_xbee_sock_t(s), XBEE_FRAME_SOCK_STATE,
                              XBEE_SOCK_STATE_TRANSPORT_CLOSED);
        }
        _socket_free(s);
    }

    return retval;
}


// API documented in xbee/socket.h
int xbee_sock_reset(xbee_dev_t *xbee)
{
    // free any local sockets related to this XBee device
    socket_t *s;
    for (s = &socket_table[0]; s < &socket_table[XBEE_SOCK_SOCKET_COUNT]; ++s) {
        if (s->xbee == xbee) {
            _socket_free(s);
        }
    }

    // use the wildcard 0xFF socket ID to close all sockets on XBee device
    return _sock_close_frame(xbee, 0xFF);
}


// API documented in xbee/socket.h
int xbee_sock_close(xbee_sock_t socket)
{
    return _sock_close(_lookup_sock_t(socket));
}


// API documented in xbee/socket.h
int xbee_sock_close_all(xbee_dev_t *xbee)
{
    int retval;

    socket_t *s;
    for (s = &socket_table[0]; s < &socket_table[XBEE_SOCK_SOCKET_COUNT]; ++s) {
        // We can only close sockets that went from PENDING to CREATED.
        if (s->type >= XBEE_SOCK_SOCKET_TYPE_CREATED
            && (xbee == NULL || s->xbee == xbee))
        {
            retval = _sock_close(s);
            if (retval != 0) {
                return retval;
            }
        }
    }

    return 0;
}


/**
    @brief
    Frame handler for 0x89 (XBEE_FRAME_TX_STATUS) frames.

    Figures out which socket sent data matching this TX Status frame, and
    notifies the user with the XBEE_TX_DELIVERY_xxx message.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.

    @retval     0               Matched entry in socket table and called
                                the Notify handler.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
*/
static int handle_tx_status(xbee_dev_t *xbee,
                            const xbee_frame_tx_status_t FAR *frame)
{
    // Search socket table for matching socket and notify user.
    socket_t *s;
    for (s = &socket_table[0]; s < &socket_table[XBEE_SOCK_SOCKET_COUNT]; ++s) {
        if (s->xbee == xbee && s->tx_frame_id == frame->frame_id) {
            s->notify_handler(_xbee_sock_t(s), XBEE_FRAME_TX_STATUS,
                              frame->delivery);

            // After successfully matching frame_id, clear it so we don't
            // accidentally match another TX Status frame.
            s->tx_frame_id = 0;

            return 0;
        }
    }

    return -ENOENT;
}


/**
    @brief
    Frame handler for 0xC0 (XBEE_FRAME_SOCK_CREATE_RESP) frames.

    Updates entry in socket table and calls user's callback.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.

    @retval     0               Matched entry in socket table, updated status
                                and called the Notify handler.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
*/
static int handle_create_resp(xbee_dev_t *xbee,
                              const xbee_frame_sock_shared_resp_t FAR *frame)
{
    debug_printf("%s: type 0x%02X  frame 0x%02X  sock 0x%02X  status 0x%02X\n",
                 __FUNCTION__, frame->frame_type, frame->frame_id,
                 frame->socket_id, frame->status);

    // match response based on frame_id
    socket_t *s;
    for (s = &socket_table[0]; s < &socket_table[XBEE_SOCK_SOCKET_COUNT]; ++s) {
        if (s->type == XBEE_SOCK_SOCKET_TYPE_PENDING
            && s->xbee == xbee
            && s->create_frame_id == frame->frame_id)
        {
            if (frame->status == XBEE_SOCK_STATUS_SUCCESS) {
                s->socket_id = frame->socket_id;
                s->type = XBEE_SOCK_SOCKET_TYPE_CREATED;
            }

            s->notify_handler(_xbee_sock_t(s), XBEE_FRAME_SOCK_CREATE_RESP,
                              frame->status);

            if (frame->status != XBEE_SOCK_STATUS_SUCCESS) {
                _socket_free(s);
            }

            return 0;
        }
    }

    return -ENOENT;
}


/**
    @brief
    Frame handler for multiple response frames:
        0xC2 (XBEE_FRAME_SOCK_CONNECT_RESP)
        0xC6 (XBEE_FRAME_SOCK_LISTEN_RESP)

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.

    @retval     0               Matched entry in socket table, updated status
                                and called the Notify handler.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
*/
static int handle_attach_resp(xbee_dev_t *xbee,
                              const xbee_frame_sock_shared_resp_t FAR *frame)
{
    debug_printf("%s: type 0x%02X  frame 0x%02X  sock 0x%02X  status 0x%02X\n",
                 __FUNCTION__, frame->frame_type, frame->frame_id,
                 frame->socket_id, frame->status);

    socket_t *s = _lookup_socket_id(xbee, frame->socket_id);
    if (s == NULL) {
        debug_printf("%s: couldn't match socket_id 0x%02X\n",
                     __FUNCTION__, frame->socket_id);
        return -ENOENT;
    }

    if (s->type != XBEE_SOCK_SOCKET_TYPE_ATTACH) {
        debug_printf("%s: socket type 0x%02X not ATTACH\n",
                     __FUNCTION__, s->type);
        return -EPERM;
    }

    s->notify_handler(_xbee_sock_t(s), frame->frame_type, frame->status);

    // Update socket type based on response.

    if (frame->status != 0) {
        // connect/bind/listen failed, go back to CREATED state
        s->type = XBEE_SOCK_SOCKET_TYPE_CREATED;
    } else if (frame->frame_type == XBEE_FRAME_SOCK_CONNECT_RESP) {
        // connect() completed, safe to send()
        s->type = XBEE_SOCK_SOCKET_TYPE_CONNECT;
    } else if (s->protocol == XBEE_SOCK_PROTOCOL_UDP) {
        // bind() completed, safe to sendto()
        s->type = XBEE_SOCK_SOCKET_TYPE_BIND;
    } else {
        // listen() completed, expect ipv4_client notifications
        s->type = XBEE_SOCK_SOCKET_TYPE_LISTEN;
    }

    return 0;
}


/**
    @brief
    Frame handler for 0xC1 (XBEE_FRAME_SOCK_OPTION_RESP) frames.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.
    @param[in]  length          Length of frame (to calculate payload length).

    @retval     0               Matched entry in socket table and called the
                                Option handler.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
    @retval     -EINVAL         \a length was too small for this frame type.
*/
static int handle_option_resp(xbee_dev_t *xbee,
                              const xbee_frame_sock_option_resp_t FAR *frame,
                              uint16_t length)
{
    int16_t payload_len =
        length - offsetof(xbee_frame_sock_option_resp_t, option_data);

    debug_printf("%s: frame 0x%02X  sock 0x%02X  opt 0x%02X  status 0x%02X\n",
                 __FUNCTION__, frame->frame_id, frame->socket_id,
                 frame->option_id, frame->status);

    if (payload_len < 0) {
        return -EINVAL;
    }

    socket_t *s = _lookup_socket_id(xbee, frame->socket_id);
    if (s == NULL) {
        debug_printf("%s: couldn't match socket_id 0x%02X\n",
                     __FUNCTION__, frame->socket_id);
        return -ENOENT;
    }

    if (s->option_handler != NULL) {
        const void *option_data =
            payload_len ? &frame->option_data[0] : NULL;
        s->option_handler(_xbee_sock_t(s), frame->option_id, frame->status,
                          option_data, payload_len);
    }

    return 0;
}


/**
    @brief
    Frame handler for 0xCC (XBEE_FRAME_SOCK_IPV4_CLIENT) frames.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.

    @retval     0               Found Listening socket in socket table, created
                                new Connected socket, and called the IPv4 Client
                                handler.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
    @retval     -EPERM          Socket isn't a listening socket.
*/
static int handle_ipv4_client(xbee_dev_t *xbee,
                              const xbee_frame_sock_ipv4_client_t FAR *frame)
{
    debug_printf("%s: listen 0x%02X  new client 0x%02X from 0x%08X:%u\n",
                 __FUNCTION__, frame->sock_listen_id, frame->sock_client_id,
                 be32toh(frame->remote_addr_be),
                 be16toh(frame->remote_port_be));

    socket_t *listen = _lookup_socket_id(xbee, frame->sock_listen_id);
    if (listen == NULL) {
        debug_printf("%s: couldn't match socket_id 0x%02X\n",
                     __FUNCTION__, frame->sock_listen_id);
        return -ENOENT;
    }
    if (listen->type != XBEE_SOCK_SOCKET_TYPE_LISTEN) {
        debug_printf("%s: socket type 0x%02X not LISTEN\n",
                     __FUNCTION__, listen->type);
        return -EPERM;
    }

    debug_printf("%s: listen xbee_sock_t 0x%04X\n",
                 __FUNCTION__, _xbee_sock_t(listen));

    // Try to create a new socket entry to hold the new client socket.
    socket_t *client = _socket_alloc(xbee);
    if (client == NULL) {
        debug_printf("%s: cannot allocate sock for client 0x%02X\n",
                     __FUNCTION__, frame->sock_client_id);
    } else {
        client->socket_id = frame->sock_client_id;
        client->type = XBEE_SOCK_SOCKET_TYPE_CONNECT;
        client->protocol = listen->protocol;

        // client socket will use the same notification handler as listen socket
        client->notify_handler = listen->notify_handler;

        // need a non-zero create_frame_id to generate xbee_sock_t
        client->create_frame_id = listen->create_frame_id;

        debug_printf("%s: allocated sock 0x%04X for client 0x%02X\n",
                     __FUNCTION__, _xbee_sock_t(client), frame->sock_client_id);
    }

    // Call user's callback to notify them of new client socket.  If we were
    // unable to create a local socket entry, we'll pass -ENOSPC as the
    // <client_socket> parameter to their callback.
    xbee_sock_receive_fn client_rx_handler =
            listen->handler.ipv4_client(_xbee_sock_t(listen),
                                        _xbee_sock_t(client),
                                        be32toh(frame->remote_addr_be),
                                        be16toh(frame->remote_port_be));

    if (client != NULL && client_rx_handler != NULL) {
        // assign the receive handler provided by the ipv4_client() callback
        client->handler.receive = client_rx_handler;

        return 0;
    }

    // We're unable to accept this connection due to a full socket table
    // (client == NULL) or the ipv4_client() callback rejecting it (maybe
    // it limits connections by IP address?).

    // release the client socket, if we allocated one
    if (client != NULL) {
        _socket_free(client);
    }

    // Ask the XBee to close this new client socket on its end.
    _sock_close_frame(xbee, frame->sock_client_id);

    return 0;
}


/**
    @brief
    Shared code for frame handlers processing XBEE_FRAME_SOCK_RECEIVE and
    XBEE_FRAME_SOCK_RECEIVE_FROM frames.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  payload_len     Length of payload (up to 1500 bytes).
    @param[in]  type_check      XBEE_SOCK_SOCKET_TYPE_CONNECT for
                                XBEE_FRAME_SOCK_RECEIVE or
                                XBEE_SOCK_SOCKET_TYPE_BIND for
                                XBEE_FRAME_SOCK_RECEIVE_FROM.
    @param[in]  socket_id       Socket ID field from received frame.
    @param[out] socket          Socket table entry (if return value is 0).

    @retval     0               Found socket in table with correct type.
    @retval     -EINVAL         Invalid (negative) \a payload_len.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
    @retval     -EPERM          Socket type doesn't match \a type_check.
*/
static int _common_receive(xbee_dev_t *xbee,
                           int16_t payload_len,
                           uint8_t type_check,
                           uint8_t socket_id,
                           socket_t **socket)
{
    debug_printf("%s: socket 0x%02X:  %d bytes\n",
                 __FUNCTION__, socket_id, payload_len);

    if (payload_len < 0) {
        return -EINVAL;
    }

    socket_t *s = _lookup_socket_id(xbee, socket_id);
    if (s == NULL) {
        debug_printf("%s: couldn't match socket_id 0x%02X\n",
                     __FUNCTION__, socket_id);
        return -ENOENT;
    }
    debug_printf("%s: xbee_sock_t 0x%04X\n", __FUNCTION__, _xbee_sock_t(s));

    if (s->type != type_check) {
        debug_printf("%s: socket type 0x%02X != 0x%02X\n",
                     __FUNCTION__, s->type, type_check);
        return -EPERM;
    }

    *socket = s;
    return 0;
}

/**
    @brief
    Frame handler for 0xCD (XBEE_FRAME_SOCK_RECEIVE) frames.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.
    @param[in]  length          Length of frame (to calculate payload length).

    @retval     0               Found socket in table and passed payload to
                                the Receive handler.
    @retval     -EINVAL         \a length was too small for this frame type.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
    @retval     -EPERM          Socket type isn't XBEE_SOCK_SOCKET_TYPE_CONNECT.
*/
static int handle_receive(xbee_dev_t *xbee,
                          const xbee_frame_sock_receive_t FAR *frame,
                          uint16_t length)
{
    int16_t payload_len =
        length - offsetof(xbee_frame_sock_receive_t, payload);

    socket_t *s;
    int retval = _common_receive(xbee, payload_len,
                                 XBEE_SOCK_SOCKET_TYPE_CONNECT,
                                 frame->socket_id, &s);

    if (retval == 0) {
        s->handler.receive(_xbee_sock_t(s), frame->status,
                           frame->payload, payload_len);
    }

    return retval;
}


/**
    @brief
    Frame handler for 0xCE (XBEE_FRAME_SOCK_RECEIVE_FROM) frames.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.
    @param[in]  length          Length of frame (to calculate payload length).

    @retval     0               Found socket in table and passed payload to
                                the Receive From handler.
    @retval     -EINVAL         \a length was too small for this frame type.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
    @retval     -EPERM          Socket type isn't XBEE_SOCK_SOCKET_TYPE_BIND.
*/
static int handle_receive_from(xbee_dev_t *xbee,
                               const xbee_frame_sock_receive_from_t FAR *frame,
                               uint16_t length)
{
    int16_t payload_len =
        length - offsetof(xbee_frame_sock_receive_from_t, payload);

    socket_t *s;
    int retval = _common_receive(xbee, payload_len,
                                 XBEE_SOCK_SOCKET_TYPE_BIND,
                                 frame->socket_id, &s);

    if (retval == 0) {
        s->handler.receive_from(_xbee_sock_t(s), frame->status,
                                be32toh(frame->remote_addr_be),
                                be16toh(frame->remote_port_be),
                                frame->payload, payload_len);
    }

    return retval;
}


/**
    @brief
    Frame handler for 0xCF (XBEE_FRAME_SOCK_STATE) frames.

    @param[in]  xbee            XBee device that received the frame.
    @param[in]  frame           Received frame.

    @retval     0               Called Notify handler and released socket entry
                                if frame reported an error.
    @retval     -ENOENT         Couldn't find matching entry in socket table.
*/
static int handle_sock_state(xbee_dev_t *xbee,
                             const xbee_frame_sock_state_t FAR *frame)
{
    debug_printf("%s: socket 0x%02X  state 0x%02x\n", __FUNCTION__,
                 frame->socket_id, frame->state);

    socket_t *s = _lookup_socket_id(xbee, frame->socket_id);
    if (s == NULL) {
        debug_printf("%s: couldn't match socket_id 0x%02X\n",
                     __FUNCTION__, frame->socket_id);
        return -ENOENT;
    }

    debug_printf("%s: xbee_sock_t 0x%04X\n", __FUNCTION__, _xbee_sock_t(s));
    s->notify_handler(_xbee_sock_t(s), XBEE_FRAME_SOCK_STATE, frame->state);

    if (frame->state != 0) {
        // fatal error, our socket is no longer valid
        _socket_free(s);
    }

    return 0;
}

/*
    Single handler for all frames, reduces number of entries in frame handler
    table and can use a simpler API for each frame type (length only necessary
    for variable-length frames, drop unused context, cast the void pointer
    to the correct frame type).

    API documented in xbee/socket.h.
*/
int xbee_sock_frame_handler(xbee_dev_t *xbee, const void FAR *rawframe,
                            uint16_t length, void FAR *context)
{
    // Switch on frame_type, always teh first byte of the frame.
    switch (*(const uint8_t *)rawframe)
    {
    case XBEE_FRAME_TX_STATUS:
        return handle_tx_status(xbee, rawframe);

    case XBEE_FRAME_SOCK_CREATE_RESP:
        return handle_create_resp(xbee, rawframe);

    case XBEE_FRAME_SOCK_CONNECT_RESP:
    case XBEE_FRAME_SOCK_LISTEN_RESP:
        return handle_attach_resp(xbee, rawframe);

    case XBEE_FRAME_SOCK_OPTION_RESP:
        return handle_option_resp(xbee, rawframe, length);

    case XBEE_FRAME_SOCK_IPV4_CLIENT:
        return handle_ipv4_client(xbee, rawframe);

    case XBEE_FRAME_SOCK_RECEIVE:
        return handle_receive(xbee, rawframe, length);

    case XBEE_FRAME_SOCK_RECEIVE_FROM:
        return handle_receive_from(xbee, rawframe, length);

    case XBEE_FRAME_SOCK_STATE:
        return handle_sock_state(xbee, rawframe);
    }

    // Because we use a 0 for frame_type in out xbee_dispatch_table_entry_t,
    // the library calls our handler for ALL frame types.  So just return if
    // this is a frame we aren't interested in.
    return 0;
}

///@}
