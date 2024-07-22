/*
 * Copyright (c) 2024 Digi International Inc.,
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
    @defgroup xbee_ble Frames: BLE (0x34, 0xB4, 0xB5 and 0xB7)
    @ingroup xbee_frame
    @{
    @file xbee/ble.h

    Because this framework dispatches received frames to frame handlers, this
    API layer for BLE makes use of callbacks to pass advertisement responses
    back to the calling code.

    Calling code uses xbee_ble_scan_request() to initiate a scan. The caller
    is expected to have receive handlers in the xbee_frame_handlers[] that
    can handle the frame responses. See below for the callback descriptions
    and definitions.

    For use with the XBee 3 BLU.
*/

#ifndef XBEE_BLE_H
#define XBEE_BLE_H

#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/ble_frames.h"

XBEE_BEGIN_DECLS

/**
    @brief
    Callback handler for status updates from a scan request.

    \a status contains one of the XBEE_BLE_SCAN_STATUS_xxx values:
        - XBEE_FRAME_BLE_SCAN_STATUS_SUCCESS
        - XBEE_FRAME_BLE_SCAN_STATUS_RUNNING
        - XBEE_FRAME_BLE_SCAN_STATUS_STOPPED
        - XBEE_FRAME_BLE_SCAN_STATUS_ERROR
        - XBEE_FRAME_BLE_SCAN_STATUS_INVALID_PARAMS

    @param[in]  frame_type  XBee frame responsible for the status update.
    @param[in]  status      Status delivered. See above for possibilities.
*/
typedef void (*xbee_ble_scan_status_fn)(uint8_t frame_type,
                                        uint8_t status);

/**
    @brief Initiate a scan request. A BLE Scan Status frame will be returned
           when the scan request has been processed.

    @param[in]  xbee            XBee device sending the message
    @param[in]  scan_duration   Scan duration in seconds. Can be set to zero
                                for an indefinite scan. Scans can be stopped
                                early with xbee_ble_scan_stop().
    @param[in]  scan_window     Scan window in microseconds. This parameter
                                must not be bigger than scan duration.
    @param[in]  scan_interval   Scan interval in microseconds. This parameter
                                must not be bigger than the scan window.
    @param[in]  filter_type     Bitfield where only one option may be selected:
                                - XBEE_BLE_SCAN_FILTER_NAME
    @param[in]  filter          Filter data, its form being dictated by
                                /a filter_type. The data passed, even if it is
                                a string, does not need to be null-terminated.
    @param[in]  filter_length   Length of the /a filter parameter in bytes.

    @retval  0                  Successfully queued the request frame.
    @retval  -EINVAL            Invalid parameter passed to function.
    @retval  -E2BIG             The /a filter_length is invalid given the
                                /a filter_type.

    @sa xbee_ble_scan_stop()
*/
int xbee_ble_scan_start(xbee_dev_t *xbee, uint16_t scan_duration,
                        uint32_t scan_window, uint32_t scan_interval,
                        uint8_t filter_type, const void *filter,
                        size_t filter_len);

/**
    @brief Stop an in-progress scan. A BLE Scan Status frame will be returned
           when the scan request has been processed.

    @param[in]  xbee            XBee device sending the message

    @retval  0                  Successfully queued the request frame.
    @retval  -EINVAL            Invalid parameter passed to function.

    @sa xbee_ble_scan_start()
*/
int xbee_ble_scan_stop(xbee_dev_t *xbee);

/**
    @brief Dump received BLE scan status  message to stdout.

    This is a sample frame handler that simply prints received BLE messages
    to the screen.  Include it in your frame handler table as:

    { XBEE_FRAME_BLE_SCAN_STATUS, 0, xbee_ble_scan_status_dump, NULL }

    See the function help for xbee_frame_handler_fn() for full
    documentation on this function's API.
*/
int xbee_ble_scan_status_dump(xbee_dev_t *xbee, const void FAR *raw,
                              uint16_t length, void FAR *context);

/**
    @brief Dump received BLE legacy advertisement message to stdout.

    This is a sample frame handler that simply prints received BLE messages
    to the screen.  Include it in your frame handler table as:

    { XBEE_FRAME_BLE_SCAN_LEGACY_ADVERT_RESP, 0, xbee_ble_scan_legacy_advert_dump, NULL }

    See the function help for xbee_frame_handler_fn() for full
    documentation on this function's API.
*/
int xbee_ble_scan_legacy_advert_dump(xbee_dev_t *xbee, const void FAR *raw,
                                     uint16_t length, void FAR *context);

/**
    @brief Dump received BLE extended advertisement message to stdout.

    This is a sample frame handler that simply prints received BLE messages
    to the screen.  Include it in your frame handler table as:

    { XBEE_FRAME_BLE_SCAN_EXT_ADVERT_RESP, 0, xbee_ble_scan_ext_advert_dump, NULL }

    See the function help for xbee_frame_handler_fn() for full
    documentation on this function's API.
*/
int xbee_ble_scan_ext_advert_dump(xbee_dev_t *xbee, const void FAR *raw,
                                  uint16_t length, void FAR *context);

XBEE_END_DECLS

#endif // XBEE_BLE_H

///@}
