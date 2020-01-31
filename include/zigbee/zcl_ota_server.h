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
    @file zigbee/zcl_ota_server.h

    Support for a ZCL OTA Upgrade Cluster Server.
*/

#ifndef ZCL_OTA_SERVER_H
#define ZCL_OTA_SERVER_H

#include "zigbee/zcl_ota_upgrade.h"
#include "zigbee/zcl.h"

XBEE_BEGIN_DECLS

/// Handler for an OTA Upgrade Server's wpan_cluster_table_entry_t.
int zcl_ota_upgrade_cluster_handler(const wpan_envelope_t FAR *envelope,
                                    void FAR *context);

/// Macro to include an OTA Upgrade Server in an endpoint's cluster table.
#define ZCL_OTA_UPGRADE_CLUSTER_ENTRY \
    { ZCL_CLUST_OTA_UPGRADE, zcl_ota_upgrade_cluster_handler, \
      NULL, WPAN_CLUST_FLAG_SERVER }

// forward declaration to define zcl_ota_upgrade_read_fn
struct zcl_ota_upgrade_source_t;

/**
    @brief
    Callback for ZCL OTA Upgrade Server to supply data to Image Block Requests.

    @param[in]  context         Context field of zcl_ota_upgrade_source_t.
    @param[out] buffer          Location for requested data.
    @param[in]  offset          Offset into image for reading data.
    @param[in]  bytes           Number of bytes to store in \a buffer.
    @param[in]  client_ieee_be  Client's IEEE address (if necessary to customize
                                OTA image content).

    @return     Number of bytes actually read.
*/
typedef int (*zcl_ota_upgrade_read_fn)
    (const struct zcl_ota_upgrade_source_t *source,
     void *buffer, uint32_t offset, size_t bytes, const addr64 *client_ieee_be);

/// Datatype for providing an OTA Upgrade Image to a client.
typedef struct zcl_ota_upgrade_source_t {
    zcl_ota_image_id_t          id;             ///< ID fields for image
    uint32_t                    image_size;     ///< size of image

    zcl_ota_upgrade_read_fn     read_handler;   ///< callback to read image
    void                        *context;       ///< location to hold user data
} zcl_ota_upgrade_source_t;

/**
    @brief
    Send at OTA Upgrade Image Notify command.

    @param[in]  wpan_dev        Device used to send.
    @param[in]  image_id        Image ID for Mfg. Code, Image Type and File
                                Version.
    @param[in]  src_ep          Source endpoint structure to send from.
    @param[in]  dest_ep         Target device's endpoint.
    @param[in]  ieee_addr_be    Target device's 64-bit IEEE address.
    @param[in]  network_addr    Target device's 16-bit network address.

    @retval     0               Request Sent
    @retval     <0              Error returned from wpan_dev->endpoint_send().
*/
int zcl_ota_upgrade_image_notify(wpan_dev_t *wpan_dev,
                                 const zcl_ota_image_id_t *image_id,
                                 const wpan_endpoint_table_entry_t *src_ep,
                                 uint8_t dest_ep,
                                 const addr64 *ieee_addr_be,
                                 uint16_t network_addr);

/**
    @brief
    Application-provided callback from ZCL OTA Upgrade cluster handler to
    retrieve a structure with details and callbacks/content for responses.

    @param[in]  client_ieee_be  Client's 64-bit IEEE address (big-endian).
    @param[in]  id              Key fields (Mfg Code, Image Type, File Version)
                                from request frame to match against available
                                updates.
    @param[in]  zcl_command     ZCL Command (ZCL_OTA_CMD_xxx)

    @retval     NULL            Respond to client with NO_IMAGE_AVAILABLE.
    @retval     non-NULL        Source with image to send to client.
*/
const zcl_ota_upgrade_source_t
    *zcl_ota_get_upgrade_source(const addr64 *client_ieee_be,
                                const zcl_ota_image_id_t *id,
                                uint8_t zcl_command);

/**
    @brief
    Application-provided callback from ZCL OTA Upgrade cluster handler to
    retrieve the upgrade time to respond to client's Upgrade End Request.

    @param[in]  client_ieee_be  Client's 64-bit IEEE address (big-endian).
    @param[in]  source          Source of OTA Update Image (as returned from
                                zcl_ota_get_upgrade_source).

    @retval     NULL            Respond to client with NO_IMAGE_AVAILABLE.
    @retval     non-NULL        Source with image to send to client.
*/
// TODO: If we support using a current_time field, pass that to the callback.
uint32_t zcl_ota_get_upgrade_time(const addr64 *client_ieee_be,
                                  const zcl_ota_upgrade_source_t *source);

XBEE_END_DECLS

#endif // ZCL_OTA_SERVER_H

///@}
