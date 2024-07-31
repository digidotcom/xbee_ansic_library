/*
 * Copyright (c) 2024 Digi International Inc.,
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
    @addtogroup xbee_ble
    @{
    @file xbee/ble_frames.h

    Frame definitions and support functions for BLE frames
    (0x34, 0xB4, 0xB5 and 0xB7).

    For use with the XBee 3 BLU
*/

#ifndef XBEE_BLE_FRAMES_H
#define XBEE_BLE_FRAMES_H

#include "xbee/platform.h"

XBEE_BEGIN_DECLS

#define XBEE_BLE_ADDRESS_TYPE_PUBLIC (0)
#define XBEE_BLE_ADDRESS_TYPE_RANDOM (1)

/// Flag used in advertisement responses, in the 0xB4 and 0xB7 frame types
#define XBEE_BLE_ADVERT_FLAG_CONNECTABLE  ( 1 << 0 ) // Advertisement is connectable

// Max and min values for the scan window and the scan interval in microseconds.
#define XBEE_BLE_SCAN_VALUES_MAX_USEC       (0xFFFF * 625)
#define XBEE_BLE_SCAN_VALUES_MIN_USEC       (2500)

/// Frame Type: BLE Scan Request
#define XBEE_FRAME_BLE_SCAN_REQUEST             0x34
/// Format of the XBee API frame type 0x34 (#XBEE_FRAME_BLE_SCAN_REQUEST)
/// Sent from host to XBee, which responds with XBEE_FRAME_BLE_SCAN_STATUS
typedef XBEE_PACKED(xbee_frame_ble_scan_req_t, {
    uint8_t    frame_type;          ///< XBEE_FRAME_BLE_SCAN_REQUEST (0x34)
    uint8_t    start_cmd;           ///< start/stop command, start=1 & stop=0
    uint16_t   scan_duration_be;    ///< scan duration in seconds
    uint32_t   scan_window_be;      ///< scanning window in microseconds
    uint32_t   scan_interval_be;    ///< scanning interval in microseconds
    uint8_t    filter_type;         ///< type of filters to apply to scan
#define XBEE_BLE_SCAN_FILTER_NONE         ( 0 )
#define XBEE_BLE_SCAN_FILTER_NAME         ( 1 << 0 )   // Filter on local name. 
    uint8_t    filter[1];           ///< filter value, max 22 bytes
}) xbee_frame_ble_scan_req_t;

typedef XBEE_PACKED(xbee_header_ble_scan_req_t, {
    uint8_t    frame_type;          ///< XBEE_FRAME_BLE_SCAN_REQUEST (0x34)
    uint8_t    start_cmd;           ///< start/stop command, start=1 & stop=0
    uint16_t   scan_duration_be;    ///< scan duration in seconds
    uint32_t   scan_window_be;      ///< scanning window in microseconds
    uint32_t   scan_interval_be;    ///< scanning interval in microseconds
    uint8_t    filter_type;         ///< type of filters to apply to scan
}) xbee_header_ble_scan_req_t;

/// Frame Type: BLE Scan Status
#define XBEE_FRAME_BLE_SCAN_STATUS              0xB5
/// Format of the XBee API frame type 0xB5 (#XBEE_FRAME_BLE_SCAN_STATUS)
/// Sent from XBee to host.
typedef XBEE_PACKED(xbee_frame_ble_scan_status_t, {
    uint8_t    frame_type;         ///< XBEE_FRAME_BLE_SCAN_STATUS (0xB5)
    uint8_t    scan_status;        ///< request status
#define XBEE_BLE_SCAN_STATUS_SUCCESS            0x00
#define XBEE_BLE_SCAN_STATUS_RUNNING            0x01
#define XBEE_BLE_SCAN_STATUS_STOPPED            0x02
#define XBEE_BLE_SCAN_STATUS_ERROR              0x03
#define XBEE_BLE_SCAN_STATUS_INVALID_PARAMS     0x04
}) xbee_frame_ble_scan_status_t;

/// Frame Type: BLE Scan Legacy Advertisement Response
#define XBEE_FRAME_BLE_SCAN_LEGACY_ADVERT_RESP  0xB4
/// Sent from XBee to host. Sent out when an advertisement is found.
typedef XBEE_PACKED(xbee_frame_ble_scan_legacy_advert_resp_t, {
    uint8_t    frame_type;         ///< XBEE_FRAME_BLE_SCAN_LEGACY_ADVERT_RESP (0xB4)
    uint8_t    address[6];         ///< BLE MAC address of received advertisment
    uint8_t    address_type;       ///< indicates the address type (XBEE_BLE_ADDRESS_TYPE_xxx)
    uint8_t    advert_flags;       ///< advertisement flags (XBEE_BLE_ADVERT_FLAG_xxx)
    int8_t     rssi;               ///< receive signal strength, in dBm
    uint8_t    reserved;           ///< reserved for future use
    uint8_t    payload_len;        ///< length of the advertisement payload
    uint8_t    payload[1];         ///< payload of the advertisement
}) xbee_frame_ble_scan_legacy_advert_resp_t;

/// Frame Type: BLE Scan Extended Advertisement Response
#define XBEE_FRAME_BLE_SCAN_EXT_ADVERT_RESP     0xB7
/// Sent from XBee to host. Sent out when an advertisement is found.
typedef XBEE_PACKED(xbee_frame_ble_scan_ext_advert_resp_t, {
    uint8_t    frame_type;              ///< XBEE_FRAME_BLE_SCAN_EXT_ADVERT_RESP (0xB7)
    uint8_t    address[6];              ///< BLE MAC address of received advertisment
    uint8_t    address_type;            ///< indicates the address type, public or random
    uint8_t    advert_flags;            ///< advertisement flags
    int8_t     rssi;                    ///< receieve signal strength, in dBm
    uint8_t    reserved;                ///< reserved for future use
    uint8_t    advert_ident;            ///< advertisement set idenitifier
    uint8_t    primary_phy;             ///< primary PHY, used for a connection
    uint8_t    secondary_phy;           ///< secondary PHY, used for a connection
#define XBEE_BLE_PHY_FLAG_1M    ( 1 << 0 )
#define XBEE_BLE_PHY_FLAG_2M    ( 1 << 1 )
#define XBEE_BLE_PHY_FLAG_125K  ( 1 << 2 )
#define XBEE_BLE_PHY_FLAG_500K  ( 1 << 3 )
#define XBEE_BLE_PHY_FLAG_ALL   ( 0xFF )
    int8_t     tx_power;                ///< transmission power of received advert, in dBm
    uint16_t   periodic_interval_be;    ///< interval for periodic advertising
    uint8_t    data_complete;           ///< data completion flags
    uint8_t    payload_len;             ///< length of the advertisement payload
    uint8_t    payload[1];              ///< payload of the advertisement
}) xbee_frame_ble_scan_ext_advert_resp_t;

XBEE_END_DECLS

#endif // XBEE_BLE_FRAMES_H

///@}
