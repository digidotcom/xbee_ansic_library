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
    @addtogroup zcl_ota_upgrade ZCL Over-the-Air Upgrade Cluster
    @ingroup zcl_clusters
    @{
    @file zigbee/zcl_ota_upgrade.h

    Based on ZigBee document 095264r23.

    XBee3 802.15.4, DigiMesh, and Zigbee products use a modified version of
    this protocol for firmware and file system updates on endpoint 0xE8 with
    Profile ID 0xC105.  Current firmware versions use fields from the OTA
    File Header, but only send the first sub-element's content (the GBL
    firmware image, or a file system image created from XCTU).
*/

#ifndef ZCL_OTA_UPGRADE_H
#define ZCL_OTA_UPGRADE_H

#include "xbee/platform.h"
#include "wpan/types.h"
#include "zigbee/zcl_types.h"

XBEE_BEGIN_DECLS

#define ZCL_CLUST_OTA_UPGRADE           0x0019

/// Common sequence of fields appearing in multiple frame formats.
typedef XBEE_PACKED(zcl_ota_image_id_t, {
    uint16_t    mfg_code_le;            ///< Manufacturer code
    uint16_t    image_type_le;          ///< Image type
    uint32_t    file_version_le;        ///< File version
}) zcl_ota_image_id_t;

/// OTA Upgrade File Header
typedef XBEE_PACKED(zcl_ota_file_header_t, {
    /// OTA upgrade file identifier; should be ZCL_OTA_FILE_ID.
    uint32_t    file_id_le;
    /// OTA Header version; should be ZCL_OTA_HEADER_VER.
    uint16_t    header_ver_le;
    /// OTA Header length; offset from start of file to first sub-element.
    uint16_t    header_len_le;
    /// OTA Header Field control; see ZCL_OTA_FIELD_CONTROL_xxx macros.
    uint16_t    field_control_le;
    /// Image Identification (Manufacturer Code, Image Type, File Version).
    zcl_ota_image_id_t  id;
    /// ZigBee Stack version; see ZCL_OTA_ZIGBEE_STACK_xxx macros.
    uint16_t    zigbee_stack_ver_le;
    /// OTA Header string; description of this image.
    char        ota_header_string[32];
    /// Total OTA Upgrade Image size (including this header).
    uint32_t    image_size_le;
}) zcl_ota_file_header_t;

/*
// Optional fields after zcl_ota_file_header_t, based on field_control_le bits:
    uint8_t     security_cred_ver;      ///< Security credential version
    addr64      upgrade_dest_le;        ///< Upgrade file destination
    uint16_t    min_hardware_ver_le;    ///< Minimum hardware version
    uint16_t    max_hardware_ver_le;    ///< Maximum hardware version
*/

/// value for file_id_le element of zcl_ota_file_header_t
#define ZCL_OTA_FILE_ID                 0x0BEEF11E

/// value for header_ver_le element of zcl_ota_file_header_t
#define ZCL_OTA_HEADER_VER              0x0100

// bitfields for field_control_le element of zcl_ota_file_header_t
#define ZCL_OTA_FIELD_CONTROL_SECURITY_CRED     (1<<0)
#define ZCL_OTA_FIELD_CONTROL_DEVICE_SPECIFIC   (1<<1)
#define ZCL_OTA_FIELD_CONTROL_HARDWARE_VERSIONS (1<<2)

// values for image_type_le element of zcl_ota_file_header_t
#define ZCL_OTA_IMAGE_TYPE_CLIENT_SECURITY_CREDS        0xFFC0
#define ZCL_OTA_IMAGE_TYPE_CLIENT_CONFIG                0xFFC1
#define ZCL_OTA_IMAGE_TYPE_SERVER_LOG                   0xFFC2
#define ZCL_OTA_IMAGE_TYPE_WILDCARD                     0xFFFF

// values for zigbee_stack_ver_le element of zcl_ota_file_header_t
#define ZCL_OTA_ZIGBEE_STACK_2006                       0x0000
#define ZCL_OTA_ZIGBEE_STACK_2007                       0x0001
#define ZCL_OTA_ZIGBEE_STACK_PRO                        0x0002
#define ZCL_OTA_ZIGBEE_STACK_IP                         0x0003

// values for optional security_cred_ver
#define ZCL_OTA_SECURITY_CRED_SE_10                     0x00
#define ZCL_OTA_SECURITY_CRED_SE_11                     0x01
#define ZCL_OTA_SECURITY_CRED_SE_20                     0x02

/// Header of sub-elements in OTA Upgrade File following zcl_ota_file_header_t
typedef XBEE_PACKED(zcl_ota_element_t, {
    /// Tag ID; see ZCL_OTA_TAG_ID_UPGRADE_xxx macros.
    uint16_t    tag_id_le;
    /// Length Field; number of bytes of data following this header.
    uint32_t    length_le;
    // followed by <length_le> bytes of data
}) zcl_ota_element_t;

#define ZCL_OTA_TAG_ID_UPGRADE_IMAGE                    0x0000
#define ZCL_OTA_TAG_ID_ECDSA_SIGNATURE                  0x0001
#define ZCL_OTA_TAG_ID_ECDSA_CERTIFICATE                0x0002
#define ZCL_OTA_TAG_ID_IMAGE_INTEGRITY_CODE             0x0003

/// Sub-element with signature for the entire file, must be last in the file.
typedef XBEE_PACKED(zcl_ota_element_ecdsa_signature_t, {
    uint16_t    tag_id_le;      ///< Tag ID (ZCL_OTA_TAG_ID_ECDSA_SIGNATURE)
    uint32_t    length_le;      ///< Length Field (always 0x00000032)
    addr64      signer_addr_le; ///< Signer IEEE Address
    uint8_t     signature[42];  ///< Signature Data
}) zcl_ota_element_ecdsa_signature_t;

/// Sub-element with the certificate used to generate the OTA file's signature.
typedef XBEE_PACKED(zcl_ota_element_ecdsa_cert_t, {
    uint16_t    tag_id_le;      ///< Tag ID (ZCL_OTA_TAG_ID_ECDSA_CERTIFICATE)
    uint32_t    length_le;      ///< Length Field (always 0x00000030)
    uint8_t     cert[48];       ///< ECDSA Certificate
}) zcl_ota_element_ecdsa_cert_t;

/// Sub-element with a hash value to verify the integrity of the OTA file.
typedef XBEE_PACKED(zcl_ota_element_integrity_code_t, {
    uint16_t    tag_id_le;      ///< Tag ID (ZCL_OTA_TAG_ID_IMAGE_INTEGRITY_CODE)
    uint32_t    length_le;      ///< Length Field (always 0x00000010)
    uint8_t     hash[16];       ///< Hash Value
}) zcl_ota_element_integrity_code_t;

// OTA Cluster Attributes (client side)
#define ZCL_OTA_ATTR_UPGRADE_SERVER_ID                  0x0000  // IEEE Address
#define ZCL_OTA_ATTR_FILE_OFFSET                        0x0001  // UINT32
#define ZCL_OTA_ATTR_CURRENT_FILE_VER                   0x0002  // UINT32
#define ZCL_OTA_ATTR_CURRENT_ZIGBEE_STACK_VER           0x0003  // UINT16
#define ZCL_OTA_ATTR_DOWNLOAD_FILE_VER                  0x0004  // UINT32
#define ZCL_OTA_ATTR_DOWNLOAD_ZIGBEE_STACK_VER          0x0005  // UINT16
#define ZCL_OTA_ATTR_IMAGE_UPGRADE_STATUS               0x0006  // ENUM8
#define ZCL_OTA_ATTR_MANUFACTURER_ID                    0x0007  // UINT16
#define ZCL_OTA_ATTR_IMAGE_TYPE_ID                      0x0008  // UINT16
#define ZCL_OTA_ATTR_MIN_BLOCK_REQUEST_DELAY            0x0009  // UINT16
#define ZCL_OTA_ATTR_IMAGE_STAMP                        0x000A  // UINT32

// Server-generated commands
#define ZCL_OTA_CMD_IMAGE_NOTIFY                        0x00
#define ZCL_OTA_CMD_QUERY_NEXT_IMAGE_RESP               0x02
#define ZCL_OTA_CMD_IMAGE_BLOCK_RESP                    0x05
#define ZCL_OTA_CMD_UPGRADE_END_RESP                    0x07
#define ZCL_OTA_CMD_QUERY_SPECIFIC_FILE_RESP            0x09

// Client-generated commands
#define ZCL_OTA_CMD_QUERY_NEXT_IMAGE_REQ                0x01
#define ZCL_OTA_CMD_IMAGE_BLOCK_REQ                     0x03
#define ZCL_OTA_CMD_IMAGE_PAGE_REQ                      0x04
#define ZCL_OTA_CMD_UPGRADE_END_REQ                     0x06
#define ZCL_OTA_CMD_QUERY_SPECIFIC_FILE_REQ             0x08

typedef XBEE_PACKED(zcl_ota_image_notify_t, {
    uint8_t     payload_type;
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_JITTER_ONLY        0x00    // ends at .query_jitter
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_INC_MFG_CODE       0x01    // ends at .id.mfg_code_le
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_INC_IMAGE_TYPE     0x02    // ends at .id.image_type_le
#define ZCL_OTA_IMAGE_NOTIFY_PAYLOAD_INC_FILE_VERSION   0x03    // ends at .id.file_version_le
    uint8_t     query_jitter;
    zcl_ota_image_id_t  id;             ///< new image identification
}) zcl_ota_image_notify_t;

typedef XBEE_PACKED(zcl_ota_query_next_image_req_t, {
    uint8_t     field_control;
#define ZCL_OTA_QUERY_NEXT_IMAGE_FIELD_HARDWARE_VER     (1<<0)
    zcl_ota_image_id_t
                id;                     ///< current image identification
    uint16_t    hardware_ver_le;
}) zcl_ota_query_next_image_req_t;

typedef XBEE_PACKED(zcl_ota_query_next_image_resp_t, {
    uint8_t     status;
    zcl_ota_image_id_t  id;             ///< new image identification
    uint32_t    image_size_le;
}) zcl_ota_query_next_image_resp_t;

typedef XBEE_PACKED(zcl_ota_image_block_req_t, {
    uint8_t     field_control;
#define ZCL_OTA_IMAGE_BLOCK_FIELD_NODE_ADDR             (1<<0)
#define ZCL_OTA_IMAGE_BLOCK_FIELD_BLOCK_DELAY           (1<<1)
    zcl_ota_image_id_t  id;             ///< upgrade image identification
    uint32_t    file_offset_le;
    uint8_t     max_data_size;
    addr64      request_node_addr_le;
    uint16_t    block_request_delay_le; // in milliseconds
}) zcl_ota_image_block_req_t;

typedef XBEE_PACKED(zcl_ota_image_page_req_t, {
    uint8_t     field_control;
#define ZCL_OTA_IMAGE_PAGE_FIELD_NODE_ADDR             (1<<0)
    zcl_ota_image_id_t  id;             ///< upgrade image identification
    uint32_t    file_offset_le;
    uint8_t     max_data_size;
    uint16_t    page_size_le;
    uint16_t    response_spacing_le;
    addr64      request_node_addr_le;
}) zcl_ota_image_page_req_t;

typedef XBEE_PACKED(zcl_ota_image_block_resp_t, {
    union {
        uint8_t status;
        XBEE_PACKED(, {
            uint8_t     status;         // set to ZCL_STATUS_SUCCESS
            zcl_ota_image_id_t  id;     ///< upgrade image identification
            uint32_t    file_offset_le;
            uint8_t     data_size;
            // followed by <data_size> bytes of image data
        }) success;
        XBEE_PACKED(, {
            uint8_t         status;     // set to ZCL_STATUS_WAIT_FOR_DATA
            zcl_utctime_t   current_time_le;
            zcl_utctime_t   request_time_le;
            uint16_t        block_request_delay_le;
        }) wait_for_data;
        XBEE_PACKED(, {
            uint8_t     status;         // set to ZCL_STATUS_ABORT
        }) abort;
    } u;
}) zcl_ota_image_block_resp_t;

typedef XBEE_PACKED(zcl_ota_upgrade_end_req_t, {
    uint8_t     status;
    zcl_ota_image_id_t  id;             ///< upgrade image identification
}) zcl_ota_upgrade_end_req_t;

typedef XBEE_PACKED(zcl_ota_upgrade_end_resp_t, {
    zcl_ota_image_id_t  id;             ///< upgrade image identification
    zcl_utctime_t   current_time_le;
    zcl_utctime_t   upgrade_time_le;
}) zcl_ota_upgrade_end_resp_t;


/// Return a description for an OTA header's Zigbee Stack Version.
const char *zcl_ota_zigbee_stack_ver_str(uint16_t v);

/// Return a description for an OTA header's Security Credential Version.
const char *zcl_ota_security_credential_ver_str(uint8_t v);

/// Return a description for an OTA header's Sub-Element Tag.
const char *zcl_ota_tag_identifier_str(uint16_t id);

/// The expected length (or 0 for variable) of an OTA file's Sub-Element Tag.
uint32_t zcl_ota_tag_identifier_length(uint16_t id);

XBEE_END_DECLS

#endif // ZCL_OTA_UPGRADE_H defined

///@}
