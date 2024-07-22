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
    @file xbee_ble.c

    See xbee/ble.h for summary of API.
*/

#include <stdio.h>
#include <string.h>

#include "xbee/device.h"
#include "xbee/platform.h"
#include "xbee/ble.h"


int xbee_ble_scan_start(xbee_dev_t *xbee, uint16_t scan_duration,
                        uint32_t scan_window, uint32_t scan_interval,
                        uint8_t filter_type,  const void *filter,
                        size_t filter_len)
{
    if (!xbee || (!filter && filter_len)) {
        return -EINVAL;
    }

    // Bounds check scan window and interval
    if (scan_window > scan_interval) {
        return -EINVAL;
    }

    // Bounds check filter
    switch (filter_type) {
    case XBEE_BLE_SCAN_FILTER_NAME:
        if (filter_len > 22) {
            return -E2BIG;
        }
        break;

    default:
        // No filter
        break;
    };

    xbee_header_ble_scan_req_t header;

    header.frame_type = XBEE_FRAME_BLE_SCAN_REQUEST;
    header.start_cmd = 1;
    header.scan_duration_be = htobe16(scan_duration);
    header.scan_window_be = htobe32(scan_window);
    header.scan_interval_be = htobe32(scan_interval);
    header.filter_type = filter_type;

    return  xbee_frame_write(xbee, &header, sizeof(header),
                             filter, filter_len, 0);
}

int xbee_ble_scan_stop(xbee_dev_t *xbee)
{
    if (!xbee) {
        return -EINVAL;
    }
    xbee_header_ble_scan_req_t header;

    // Clear other unused fields
    memset(&header, 0, sizeof(header));

    header.frame_type = XBEE_FRAME_BLE_SCAN_REQUEST;
    header.start_cmd = 0;

    return xbee_frame_write( xbee, &header, sizeof(header), NULL, 0, 0);
}

static const char *xbee_ble_scan_status_str(uint8_t status)
{
    switch( status ) {
    case XBEE_BLE_SCAN_STATUS_SUCCESS:           return "started";
    case XBEE_BLE_SCAN_STATUS_RUNNING:           return "running";
    case XBEE_BLE_SCAN_STATUS_STOPPED:           return "stopped";
    case XBEE_BLE_SCAN_STATUS_ERROR:             return "error";
    case XBEE_BLE_SCAN_STATUS_INVALID_PARAMS:    return "invalid parameters";
    default:                                     return "unknown";
    };
}

int xbee_ble_scan_status_dump(xbee_dev_t *xbee, const void FAR *raw,
                              uint16_t length, void FAR *context)
{
    const xbee_frame_ble_scan_status_t FAR *status = raw;
    const char *status_str = xbee_ble_scan_status_str(status->scan_status);

    printf("BLE scan status: %s\n", status_str);
    return 0;
}

#define BLE_MAC_ADDR_LENGTH 19
static int xbee_ble_scan_mac_str(char *buf, const uint8_t mac[6])
{
    return sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
                   mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int xbee_ble_scan_legacy_advert_dump(xbee_dev_t *xbee, const void FAR *raw,
                                     uint16_t length, void FAR *context)
{
    const xbee_frame_ble_scan_legacy_advert_resp_t FAR *advert = raw;
    char mac_addr_buf[BLE_MAC_ADDR_LENGTH];

    xbee_ble_scan_mac_str(mac_addr_buf, advert->address);
    bool_t connectable = advert->advert_flags & XBEE_BLE_ADVERT_FLAG_CONNECTABLE;

    printf("Legacy advertisement from: %s\n RSSI: %d dBm, Connectable: %s\n",
           mac_addr_buf, -advert->rssi, connectable ? "true" : "false");
    hex_dump(advert->payload, advert->payload_len, HEX_DUMP_FLAG_TAB);

   return 0;
}

int xbee_ble_scan_ext_advert_dump(xbee_dev_t *xbee, const void FAR *raw,
                                  uint16_t length, void FAR *context)
{
    const xbee_frame_ble_scan_ext_advert_resp_t FAR *advert = raw;
    char mac_addr_buf[BLE_MAC_ADDR_LENGTH];

    xbee_ble_scan_mac_str(mac_addr_buf, advert->address);
    bool_t connectable = advert->advert_flags & XBEE_BLE_ADVERT_FLAG_CONNECTABLE;

    printf("Extended advertisement from: %s\n RSSI: %d, Connectable: %s\n",
           mac_addr_buf, -advert->rssi, connectable ? "true" : "false");
    hex_dump(advert->payload, advert->payload_len, HEX_DUMP_FLAG_TAB);

    return 0;
}

