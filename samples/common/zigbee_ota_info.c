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
/*
    Sample: ZCL OTA Info

    Displays information stored in the headers of XBee firmware images for
    over-the-air updates (.OTA and .OTB files) used for XBee3 802.15.4,
    DigiMesh and Zigbee modules.
*/

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "zigbee/zcl_ota_upgrade.h"

void dump_info(const char *filename)
{
    union {
        zcl_ota_file_header_t   ota;
        uint8_t                 raw_data[128];
    } header;

    printf("File '%s':\n", filename);

    FILE *f = fopen(filename, "rb");
    if (f == NULL) {
        printf("Error %u opening file\n", errno);
        return;
    }

    size_t bytes = fread(&header, 1, sizeof header, f);
    if (bytes != sizeof header) {
        printf("Error %u reading file header\n", errno);
        goto done;
    }

    uint32_t file_id = le32toh(header.ota.file_id_le);
    if (file_id != ZCL_OTA_FILE_ID) {
        printf("Not a valid OTA file (0x%08" PRIX32 " != 0x%08" PRIX32 "\n",
               file_id, ZCL_OTA_FILE_ID);
        goto done;
    }

    printf("Header version: 0x%04X (expected 0x%04X)\n",
           le16toh(header.ota.header_ver_le), ZCL_OTA_HEADER_VER);

    uint16_t expected_length = sizeof(zcl_ota_file_header_t);
    uint16_t field_control = le16toh(header.ota.field_control_le);
    if (field_control & ZCL_OTA_FIELD_CONTROL_SECURITY_CRED) {
        expected_length += 1;
    }
    if (field_control & ZCL_OTA_FIELD_CONTROL_DEVICE_SPECIFIC) {
        expected_length += 8;
    }
    if (field_control & ZCL_OTA_FIELD_CONTROL_HARDWARE_VERSIONS) {
        expected_length += 4;
    }

    uint16_t header_length = le16toh(header.ota.header_len_le);
    printf("Header length: %u (expected %u)\n", header_length, expected_length);

    printf("Field control: 0x%04X\n", field_control);
    printf("Manufacturer code: 0x%04X\n", le16toh(header.ota.id.mfg_code_le));
    printf("Image type: 0x%04X\n", le16toh(header.ota.id.image_type_le));
    printf("File version: 0x%08" PRIX32 "\n",
           le32toh(header.ota.id.file_version_le));

    uint16_t zigbee_stack_ver = le16toh(header.ota.zigbee_stack_ver_le);
    printf("Zigbee Stack version: 0x%04X (%s)\n", zigbee_stack_ver,
           zcl_ota_zigbee_stack_ver_str(zigbee_stack_ver));

    printf("OTA Header string: %.32s\n", header.ota.ota_header_string);

    uint32_t image_size = le32toh(header.ota.image_size_le);

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);

    printf("Image size: %" PRIu32 " (%s file size %lu)\n", image_size,
           image_size == file_size ? "matches" : "expected", file_size);

    // decode additional fields based on field_control
    uint8_t *p = &header.raw_data[sizeof(zcl_ota_file_header_t)];

    if (field_control & ZCL_OTA_FIELD_CONTROL_SECURITY_CRED) {
        printf("Security credential version: 0x%02X (%s)\n", *p,
               zcl_ota_security_credential_ver_str(*p));
        ++p;
    }

    if (field_control & ZCL_OTA_FIELD_CONTROL_DEVICE_SPECIFIC) {
        char file_dest[ADDR64_STRING_LENGTH];
        addr64 file_dest_be;

        // convert little-endian IEEE address to big-endian
        _swapcpy(&file_dest_be, p, 8);

        printf("Upgrade file destination: %s\n",
               addr64_format(file_dest, &file_dest_be));
        p += 8;
    }

    if (field_control & ZCL_OTA_FIELD_CONTROL_HARDWARE_VERSIONS) {
        printf("Minimum hardware version: 0x%04X\n", le16toh(*(uint16_t *)p));
        p += 2;
        printf("Maximum hardware version: 0x%04X\n", le16toh(*(uint16_t *)p));
        p += 2;
    }

    // Sub-elements follows
    zcl_ota_element_t element;
    long offset = p - &header.raw_data[0];

    while (offset + sizeof element < file_size) {
        fseek(f, offset, SEEK_SET);
        bytes = fread(&element, 1, sizeof element, f);
        if (bytes != sizeof element) {
            printf("Error %u reading sub-element header at %lu\n",
                   errno, offset);
            goto done;
        }
        uint16_t tag_id = le16toh(element.tag_id_le);
        uint32_t length = le32toh(element.length_le);
        uint32_t expected_len = zcl_ota_tag_identifier_length(length);

        printf("Sub-element @ offset %lu: tag 0x%04X (%s) length %" PRIu32 "\n",
               offset, tag_id, zcl_ota_tag_identifier_str(tag_id), length);
        if (expected_len && length != expected_len) {
            printf("ERROR: previous sub-element should be %" PRIu32 " bytes\n",
                   expected_len);
        }

        offset += sizeof element + length;
    }

    if (offset != file_size) {
        printf("ERROR: final offset (%lu) does not match file size (%lu)\n",
               offset, file_size);
    }

done:
    fclose(f);
    puts("");
}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        dump_info(argv[i]);
    }

    return 0;
}
