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
    @file zigbee/zcl_ota_upgrade.c

    Support for ZCL Over-the-Air Upgrade Cluster.  Based on ZigBee document
    095264r23.

    This is a limited implementation and may not be fully Zibee compliant.
    XBee3 802.15.4, DigiMesh, and Zigbee products use a modified version of
    this protocol for firmware and file system updates on endpoint 0xE8 with
    Profile ID 0xC105.
*/

#include "zigbee/zcl_ota_upgrade.h"

const char *zcl_ota_zigbee_stack_ver_str(uint16_t v)
{
    switch (v) {
    case ZCL_OTA_ZIGBEE_STACK_2006:     return "ZigBee 2006";
    case ZCL_OTA_ZIGBEE_STACK_2007:     return "ZigBee 2007";
    case ZCL_OTA_ZIGBEE_STACK_PRO:      return "ZigBee Pro";
    case ZCL_OTA_ZIGBEE_STACK_IP:       return "ZigBee IP";
    default:                            return "Reserved";
    }
}


const char *zcl_ota_security_credential_ver_str(uint8_t v)
{
    switch (v) {
    case ZCL_OTA_SECURITY_CRED_SE_10:   return "SE 1.0";
    case ZCL_OTA_SECURITY_CRED_SE_11:   return "SE 1.1";
    case ZCL_OTA_SECURITY_CRED_SE_20:   return "SE 2.0";
    default:                            return "Reserved";
    }
}


const char *zcl_ota_tag_identifier_str(uint16_t id)
{
    if (id > 0xF000) {
        return "Mfg. Specific";
    }
    switch (id) {
    case ZCL_OTA_TAG_ID_UPGRADE_IMAGE:          return "Upgrade Image";
    case ZCL_OTA_TAG_ID_ECDSA_SIGNATURE:        return "ECDSA Signature";
    case ZCL_OTA_TAG_ID_ECDSA_CERTIFICATE:      return "ECDSA Signing Cert";
    case ZCL_OTA_TAG_ID_IMAGE_INTEGRITY_CODE:   return "Image Integrity Code";
    default:                                    return "Reserved";
    }
}


/**
    @brief
    Return expected length for a given sub-element Tag Identifier, or 0 if
    variable-length.

    @param[in]  id              Tag Identifier from Sub-element in OTA file.

    @retval     0               Unknown or variable size.
    @retval     >0              Expected value of sub-element's length field.
*/
uint32_t zcl_ota_tag_identifier_length(uint16_t id)
{
    switch (id) {
    case ZCL_OTA_TAG_ID_ECDSA_SIGNATURE:        return 0x32;
    case ZCL_OTA_TAG_ID_ECDSA_CERTIFICATE:      return 0x30;
    case ZCL_OTA_TAG_ID_IMAGE_INTEGRITY_CODE:   return 0x10;
    default:                                    return 0;
    }
}

///@}
