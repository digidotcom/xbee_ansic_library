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
    @addtogroup zcl_ota_upgrade
    @{
    @file zigbee/zcl_ota_server.c

    Support for a ZCL OTA Upgrade Cluster Server.
*/

// define ZCL_OTA_VERBOSE to get verbose output to aid in debugging
//#define ZCL_OTA_VERBOSE

#ifdef ZCL_OTA_VERBOSE
#define debug_printf(...)       fprintf(stderr, __VA_ARGS__)
#else
#define debug_printf(...)
#endif

#include <stdio.h>

#include "zigbee/zcl_ota_server.h"

// See zigbee/zcl_ota_server.h for this API's documentation.
int zcl_ota_upgrade_image_notify(wpan_dev_t *wpan_dev,
                                 const zcl_ota_image_id_t *image_id,
                                 const wpan_endpoint_table_entry_t *src_ep,
                                 uint8_t dest_ep,
                                 const addr64 *ieee_addr_be,
                                 uint16_t network_addr)
{
    wpan_envelope_t env;
    XBEE_PACKED(, {
        zcl_header_nomfg_t      header;
        zcl_ota_image_notify_t  notify;
    }) zcl;

    zcl.header.frame_control =
        ZCL_FRAME_SERVER_TO_CLIENT | ZCL_FRAME_TYPE_CLUSTER;
    zcl.header.sequence = wpan_endpoint_next_trans(src_ep);
    zcl.header.command = ZCL_OTA_CMD_IMAGE_NOTIFY;

    zcl.notify.payload_type = ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_INC_FILE_VERSION;
    zcl.notify.query_jitter = 100;
    zcl.notify.id = *image_id;

    wpan_envelope_create(&env, wpan_dev, ieee_addr_be, network_addr);
    env.profile_id = src_ep->profile_id;
    env.source_endpoint = src_ep->endpoint;
    env.dest_endpoint = dest_ep;
    env.cluster_id = ZCL_CLUST_OTA_UPGRADE;
    env.payload = &zcl;
    env.length = sizeof zcl;

#ifdef ZCL_OTA_VERBOSE
    puts("Sending Image Notify:");
    zcl_envelope_payload_dump(&env);
#endif

    return wpan_envelope_send(&env);
}


// Keep direction & type bits from request (old_frame_control & mask),
// toggle direction (1 ^ old_direction)
// and set disable default response (1 ^ 0) bit.
// direction is opposite of command's direction, use xor to toggle
static void _set_response_header(zcl_header_nomfg_t *resp, zcl_command_t *req)
{
    resp->frame_control =
        (ZCL_FRAME_DISABLE_DEF_RESP | ZCL_FRAME_DIRECTION)
        ^ (req->frame_control & (ZCL_FRAME_DIRECTION | ZCL_FRAME_TYPE_MASK));
    resp->sequence = req->sequence;
}


/// Respond to an OTA Upgrade Next Image Request.
int zcl_ota_next_image_resp(zcl_command_t *zcl_req)
{
    const zcl_ota_query_next_image_req_t *request = zcl_req->zcl_payload;
    XBEE_PACKED(, {
        zcl_header_nomfg_t              header;
        zcl_ota_query_next_image_resp_t response;
    }) frame;

    frame.header.command = ZCL_OTA_CMD_QUERY_NEXT_IMAGE_RESP;
    _set_response_header(&frame.header, zcl_req);

    const zcl_ota_upgrade_source_t *ota = NULL;

    if ((request->field_control == ZCL_OTA_QUERY_NEXT_IMAGE_FIELD_HARDWARE_VER
         && zcl_req->length == sizeof *request)
        || (request->field_control == 0
            && zcl_req->length == sizeof *request - 2))
    {
        ota = zcl_ota_get_upgrade_source(&zcl_req->envelope->ieee_address,
                                         &request->id, zcl_req->command);
        frame.response.status = ota == NULL ? ZCL_STATUS_NO_IMAGE_AVAILABLE
                                            : ZCL_STATUS_SUCCESS;
    } else {
        frame.response.status = ZCL_STATUS_MALFORMED_COMMAND;
    }

    if (frame.response.status != ZCL_STATUS_SUCCESS) {
        // on error, response is 4 bytes
        return zcl_send_response(zcl_req, &frame, sizeof frame.header + 1);
    }

    frame.response.id = ota->id;
    frame.response.image_size_le = htole32(ota->image_size);

#ifdef ZCL_OTA_VERBOSE
    puts("ota: sending next_image_resp");
#endif

    return zcl_send_response(zcl_req, &frame, sizeof frame);
}


// Platforms with limited resources might restrict the block size further,
// but a typical OTA server will have a stack large enough for 256 bytes.
#ifndef ZCL_OTA_SERVER_MAX_BLOCK_SIZE
#define ZCL_OTA_SERVER_MAX_BLOCK_SIZE   256
#endif

/// Respond to an OTA Upgrade Image Block Request.
int zcl_ota_image_block_resp(zcl_command_t *zcl_req)
{
    const zcl_ota_image_block_req_t *request = zcl_req->zcl_payload;
    XBEE_PACKED(, {
        zcl_header_nomfg_t              header;
        zcl_ota_image_block_resp_t      response;
        uint8_t                         data[ZCL_OTA_SERVER_MAX_BLOCK_SIZE];
    }) frame;

    frame.header.command = ZCL_OTA_CMD_IMAGE_BLOCK_RESP;
    _set_response_header(&frame.header, zcl_req);

    const zcl_ota_upgrade_source_t *ota = NULL;

    uint32_t file_offset = le32toh(request->file_offset_le);
    // we only expect requests without additional fields
    if (request->field_control == 0
        && zcl_req->length == sizeof *request - 10)
    {
        ota = zcl_ota_get_upgrade_source(&zcl_req->envelope->ieee_address,
                                         &request->id, zcl_req->command);
        if (ota == NULL) {
            frame.response.u.status = ZCL_STATUS_NO_IMAGE_AVAILABLE;
        } else if (file_offset >= ota->image_size) {
            frame.response.u.status = ZCL_STATUS_INVALID_VALUE;
        } else {
            frame.response.u.status = ZCL_STATUS_SUCCESS;
        }
    } else {
        frame.response.u.status = ZCL_STATUS_MALFORMED_COMMAND;
    }

    if (frame.response.u.status != ZCL_STATUS_SUCCESS) {
        // on error, response is a 4 bytes
        return zcl_send_response(zcl_req, &frame,  sizeof frame.header + 1);
    }

    unsigned data_size = request->max_data_size;
    if (data_size > ZCL_OTA_SERVER_MAX_BLOCK_SIZE) {
        data_size = ZCL_OTA_SERVER_MAX_BLOCK_SIZE;
    }
    int bytes_read = ota->read_handler(ota, &frame.data[0],
                                       file_offset, data_size,
                                       &zcl_req->envelope->ieee_address);

    if (bytes_read < data_size) {
        data_size = bytes_read;
    }

    frame.response.u.success.id = ota->id;
    frame.response.u.success.file_offset_le = request->file_offset_le;
    frame.response.u.success.data_size = data_size;

    debug_printf("ota: sending %u bytes from offset %" PRIu32 "/%" PRIu32 "\n",
                 data_size, file_offset, ota->image_size);

    return zcl_send_response(zcl_req, &frame,
                             sizeof frame
                                 - ZCL_OTA_SERVER_MAX_BLOCK_SIZE
                                 + data_size);
}


/// Respond to an OTA Upgrade End Request.
int zcl_ota_upgrade_end_resp(zcl_command_t *zcl_req)
{
    const zcl_ota_upgrade_end_req_t *request = zcl_req->zcl_payload;
    XBEE_PACKED(, {
        zcl_header_nomfg_t              header;
        zcl_ota_upgrade_end_resp_t      response;
    }) frame;

#ifdef ZCL_OTA_VERBOSE
    debug_printf("ota: Upgrade End Response 0x%02X\n", request->status);
#endif

    if (request->status != ZCL_STATUS_SUCCESS) {
        // nothing to do if client is telling us update failed
        return 0;
    }

    frame.header.command = ZCL_OTA_CMD_UPGRADE_END_RESP;
    _set_response_header(&frame.header, zcl_req);

    uint8_t status;
    const zcl_ota_upgrade_source_t *ota = NULL;

    if (zcl_req->length == sizeof *request) {
        ota = zcl_ota_get_upgrade_source(&zcl_req->envelope->ieee_address,
                                         &request->id, zcl_req->command);
        if (ota == NULL) {
            status = ZCL_STATUS_NO_IMAGE_AVAILABLE;
        } else {
            status = ZCL_STATUS_SUCCESS;
        }
    } else {
        status = ZCL_STATUS_MALFORMED_COMMAND;
    }

    if (status != ZCL_STATUS_SUCCESS) {
        debug_printf("ota: Upgrade End error 0x%02X parsing request\n", status);
        // TODO: send a Default Response?
        return 0;
    }

    frame.response.id = ota->id;

    // default to not bothering to send current time
    frame.response.current_time_le = htole32(0);

    frame.response.upgrade_time_le =
        htole32(zcl_ota_get_upgrade_time(&zcl_req->envelope->ieee_address,
                                         ota));

    return zcl_send_response(zcl_req, &frame, sizeof frame);
}


// See zigbee/zcl_ota_upgrade.h for this API's documentation.
//
// Cluster handler for a Zigbee OTA Upgrade Server
int zcl_ota_upgrade_cluster_handler(const wpan_envelope_t FAR *envelope,
                                    void FAR *context)
{
    zcl_command_t       zcl;

    int err = zcl_command_build(&zcl, envelope, context);
    if (err) {
#ifdef ZCL_OTA_VERBOSE
        debug_printf("\tzcl_command_build() returned %d for %u-byte payload\n",
                     err, envelope->length);
        hex_dump(envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
#endif
        return 0;
    }

    if (ZCL_CMD_MATCH(&zcl.frame_control,
                      GENERAL, CLIENT_TO_SERVER, CLUSTER))
    {
#ifdef ZCL_OTA_VERBOSE
        puts("OTA Upgrade Server Request:");
        zcl_envelope_payload_dump(envelope);
#endif

        switch (zcl.command) {
        case ZCL_OTA_CMD_QUERY_NEXT_IMAGE_REQ:
            return zcl_ota_next_image_resp(&zcl);

        case ZCL_OTA_CMD_IMAGE_BLOCK_REQ:
            return zcl_ota_image_block_resp(&zcl);

        case ZCL_OTA_CMD_UPGRADE_END_REQ:
            return zcl_ota_upgrade_end_resp(&zcl);
        }
    }

#ifdef ZCL_OTA_VERBOSE
    puts("Unhandled payload:");
    zcl_envelope_payload_dump(envelope);
#endif

    // let the ZCL General Command handler process all unhandled frames
    return zcl_general_command(envelope, context);
}

///@}
