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
    @defgroup xbee_secure_session Frames: Secure Session (0x2E, 0xAE)

    Support of XBee3 802.15.4/DigiMesh/Zigbee Secure Sessions.
    @ingroup xbee_frame
    @{
    @file xbee/secure_session.h
*/

#ifndef XBEE_SECURE_SESSION_H
#define XBEE_SECURE_SESSION_H

#include "xbee/platform.h"
#include "xbee/device.h"
#include "wpan/types.h"

XBEE_BEGIN_DECLS

/**
    @name ATSA (Secure Access)
    ATSA bit values for services that only work over a Secure Session.
    @{
*/
/// Remote AT Commands
#define XBEE_CMD_ATSA_REMOTE_AT                 (1<<1)

/// Serial Data
#define XBEE_CMD_ATSA_SERIAL_DATA               (1<<2)
/** @} */


/// Secure Session Control frame type
#define XBEE_FRAME_SECURE_SESSION_REQ           0x2E

/** Frame format for the header of Secure Session Control frame. */
typedef XBEE_PACKED(xbee_header_secure_session_req_t, {
    /** Frame type, set to #XBEE_FRAME_SECURE_SESSION_REQ (0x2E) */
    uint8_t     frame_type;

    /** 64-bit address of remote device (client or server) */
    addr64      remote_addr_be;

    /** Combination of XBEE_SS_REQ_OPT_xxx macros */
    uint8_t     options;

    /** Timeout in 0.1s increments or #XBEE_SS_REQ_TIMEOUT_YIELDING;
        see also XBEE_SS_REQ_TIMEOUT_xxx. */
    uint16_t    timeout_be;

    // header followed by variable-length password
}) xbee_header_secure_session_req_t;

/**
    @name XBEE_SS_REQ_OPT_xxx
    Values for \a options field of xbee_header_secure_session_req_t.
    @{
*/
/// Local device initiating SRP authentication with target server.
#define XBEE_SS_REQ_OPT_CLIENT_LOGIN            (0)

/// Local device logging out of SRP session with target server.
#define XBEE_SS_REQ_OPT_CLIENT_LOGOUT           (1<<0)

/// Server terminating a specific client session (address in \a device_addr_be
/// field) or all client sessions (using #WPAN_IEEE_ADDR_BROADCAST).
#define XBEE_SS_REQ_OPT_SERVER_TERMINATION      (1<<1)

/// Session terminates after timeout period has elapsed from start of session.
#define XBEE_SS_REQ_OPT_TIMEOUT_FIXED           (0)

/// Session terminates after timeout period has elapsed from last transmission.
#define XBEE_SS_REQ_OPT_TIMEOUT_INTER_PACKET    (1<<2)
/** @} */

/**
    @name XBEE_SS_REQ_TIMEOUT_xxx
    Values related to \a timeout_be field of xbee_header_secure_session_req_t.
    @{
*/
/// Scaler for converting timeout to seconds.
#define XBEE_SS_REQ_TIMEOUT_UNITS_PER_SEC       10

/// Maximum timeout value (30 minutes).
#define XBEE_SS_REQ_TIMEOUT_MAX (30 * 60 * XBEE_SS_REQ_TIMEOUT_UNITS_PER_SEC)

/// Value for a yielding session.
#define XBEE_SS_REQ_TIMEOUT_YIELDING            0x0000
/** @} */


/**
    @brief
    Send a Secure Session Request (0x2A) frame.

    @param[in]  xbee            Device to send the frame on.
    @param[in]  dest_be         Target node for request, or
                                WPAN_IEEE_ADDR_BROADCAST to logout of all nodes.
    @param[in]  options         Combination of XBEE_SS_REQ_OPT_xxx macros.
                                    - #XBEE_SS_REQ_OPT_CLIENT_LOGIN
                                    - #XBEE_SS_REQ_OPT_CLIENT_LOGOUT
                                    - #XBEE_SS_REQ_OPT_SERVER_TERMINATION
                                    - #XBEE_SS_REQ_OPT_TIMEOUT_FIXED
                                    - #XBEE_SS_REQ_OPT_TIMEOUT_INTER_PACKET
    @param[in]  timeout_ds      Timeout in deciseconds (0.1s) or
                                XBEE_SS_REQ_TIMEOUT_YIELDING to close the idlest
                                session when session list is full.
    @param[in]  password        Password to use for XBEE_SS_REQ_OPT_CLIENT_LOGIN
                                or NULL for XBEE_SS_REQ_OPT_CLIENT_LOGOUT.

    @retval     0               Frame sent.
    @retval     -EINVAL         Invalid parameter.
    @retval     -EBUSY          Transmit serial buffer is full, or XBee is not
                                accepting serial data (deasserting /CTS signal).
*/
int xbee_secure_session_request(xbee_dev_t *xbee,
                                const addr64 *dest_be,
                                uint8_t options,
                                uint16_t timeout_ds,
                                const char *password);


/// Secure Session Response frame type
#define XBEE_FRAME_SECURE_SESSION_RESP          0xAE

/** Frame format for Secure Session Response */
typedef XBEE_PACKED(xbee_frame_secure_session_resp_t, {
    /** Frame type, set to #XBEE_FRAME_SECURE_SESSION_RESP (0xAE). */
    uint8_t     frame_type;

    /** Either XBEE_SS_RESP_TYPE_LOGIN or XBEE_SS_RESP_TYPE_LOGOUT. */
    uint8_t     type;

    /** 64-bit address of responding device. */
    addr64      remote_addr_be;

    /** One of the XBEE_SS_RESP_STATUS_xxx values. */
    uint8_t     status;

}) xbee_frame_secure_session_resp_t;

/**
    @name XBEE_SS_RESP_TYPE_xxx
    Values for \a type field of xbee_frame_secure_session_resp_t.
    @{
*/
/// Login response
#define XBEE_SS_RESP_TYPE_LOGIN                 0x00

/// Logout response
#define XBEE_SS_RESP_TYPE_LOGOUT                0x01
/** @} */

/**
    @name XBEE_SS_RESP_STATUS_xxx
    Values for \a status field of xbee_frame_secure_session_resp_t.  Typical
    statuses range from 0x00 to 0x7F; manual SRP statuses from 0x80 to 0xFF.
    @{
*/
/// Operation was successful
#define XBEE_SS_RESP_STATUS_SUCCESS             0x00

/// Invalid password
#define XBEE_SS_RESP_STATUS_INVALID_PW          0x01

/// Too many active sessions on server
#define XBEE_SS_RESP_STATUS_BUSY                0x02

/// Session options or timeout are invalid
#define XBEE_SS_RESP_STATUS_INVALID_ARG         0x03

/// Requested session does not exist
#define XBEE_SS_RESP_STATUS_NO_SESSION          0x04

/// Timeout waiting for response
#define XBEE_SS_RESP_STATUS_TIMEOUT             0x05

/// Memory allocation failed
#define XBEE_SS_RESP_STATUS_NO_MEMORY           0x06

/// A request to terminate a session in progress was made
#define XBEE_SS_RESP_STATUS_IN_PROGRESS         0x07

/// No password set on server
#define XBEE_SS_RESP_STATUS_NO_PASSWORD         0x08

/// No response from server
#define XBEE_SS_RESP_STATUS_NO_RESPONSE         0x09

/// Malformed server response
#define XBEE_SS_RESP_STATUS_BAD_FRAME           0x0A

/// Server received a client packet or vice versa
#define XBEE_SS_RESP_STATUS_SRP_WRONG_WAY       0x80

/// Received unexpected SRP packet
#define XBEE_SS_RESP_STATUS_SRP_UNEXPECTED      0x81

/// Offset for split value (A/B) came out of order
#define XBEE_SS_RESP_STATUS_SRP_MISORDERED      0x82

/// Unrecognized or invalid SRP frame type
#define XBEE_SS_RESP_STATUS_SRP_BAD_FRAME       0x83

/// Authentication protocol version is not supported
#define XBEE_SS_RESP_STATUS_SRP_BAD_PROTOCOL    0x84

/// Undefined error occurred
#define XBEE_SS_RESP_STATUS_UNDEFINED           0xFF
/** @} */


int xbee_frame_dump_secure_session_resp(xbee_dev_t *xbee,
                                        const void FAR *payload,
                                        uint16_t length,
                                        void FAR *context);

#define XBEE_FRAME_DUMP_SS_RESP \
    { XBEE_FRAME_SECURE_SESSION_RESP, 0, \
      xbee_frame_dump_secure_session_resp, NULL }

/// Identify whether an extended modem status code is Secure Session related.
#define XBEE_MODEM_STATUS_IS_SS(code)         \
    (code >= XBEE_MODEM_STATUS_SS_ESTABLISHED \
     && code <= XBEE_MODEM_STATUS_SS_AUTH_FAILED)

/** Frame format for Extended Modem Status: Secure Session Established */
typedef XBEE_PACKED(xbee_frame_xms_ss_established_t, {
    /** Frame type, set to #XBEE_FRAME_EXT_MODEM_STATUS (0x98). */
    uint8_t     frame_type;

    /** #XBEE_MODEM_STATUS_SS_ESTABLISHED (0x3B). */
    uint8_t     status_code;

    /** 64-bit address of client. */
    addr64      remote_addr_be;

    /** Options set by client; combination of XBEE_SS_REQ_OPT_xxx macros. */
    uint8_t     options;

    /** Timeout set by client; see XBEE_SS_REQ_TIMEOUT_xxx. */
    uint16_t    timeout_be;
}) xbee_frame_xms_ss_established_t;

/** Frame format for Extended Modem Status: Secure Session Ended */
typedef XBEE_PACKED(xbee_frame_xms_ss_ended_t, {
    /** Frame type, set to #XBEE_FRAME_EXT_MODEM_STATUS (0x98). */
    uint8_t     frame_type;

    /** #XBEE_MODEM_STATUS_SS_ENDED (0x3C). */
    uint8_t     status_code;

    /** 64-bit address of the remote node in this session. */
    addr64      remote_addr_be;

    /** Reason session ended; see XBEE_SS_END_REASON_xxx macros. */
    uint8_t     reason;
}) xbee_frame_xms_ss_ended_t;

/**
    @name XBEE_SS_END_REASON_xxx
    Values for \a status_code field of an Extended Modem Status (0x98) frame,
    when reporting Secure Session status changes.
    @{
*/
/// Terminated by remote node.
#define XBEE_SS_END_REASON_REMOTE_TERM          0x00
/// Timed out.
#define XBEE_SS_END_REASON_TIMED_OUT            0x01
/// Received invalid encryption counter.
#define XBEE_SS_END_REASON_BAD_ENCRYPT_CTR      0x02
/// Encryption counter overflow: maximum number of transmissions for a
/// single session reached.
#define XBEE_SS_END_REASON_ENCRYPT_CTR_OVERFLOW 0x03
/// Remote node out of memory.
#define XBEE_SS_END_REASON_REMOTE_NO_MEMORY     0x04
/** @} */

/** Frame format for Extended Modem Status: Secure Session Auth Failed */
typedef XBEE_PACKED(xbee_frame_xms_ss_auth_failed_t, {
    /** Frame type, set to #XBEE_FRAME_EXT_MODEM_STATUS (0x98). */
    uint8_t     frame_type;

    /** #XBEE_MODEM_STATUS_SS_AUTH_FAILED (0x3D). */
    uint8_t     status_code;

    /** 64-bit address of the remote node in this session. */
    addr64      remote_addr_be;

    /** Error that caused failure; see XBEE_SS_RESP_STATUS_xxx macros. */
    uint8_t     error;
}) xbee_frame_xms_ss_auth_failed_t;

int xbee_frame_dump_ext_mod_status_ss(xbee_dev_t *xbee,
                                      const void FAR *payload,
                                      uint16_t length,
                                      void FAR *context);

XBEE_END_DECLS

#endif // XBEE_SECURE_SESSION_H

///@}
