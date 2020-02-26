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
    @addtogroup xbee3_ota_update
    @{
    @file xbee3_ota_tool.c

    Sample for performing over-the-air updates on XBee3 802.15.4, DigiMesh and
    Zigbee modules using .OTA (firmware) and .OTB (file system) images.

    Note that this sample requires setting ATAO=1 to enable the Explicit Rx
    Indicator Frame (0x91) as explained in the "Over-the-air firmware/file
    system upgrade process" section of the XBee3 documentation.

    Usage (assumes XBee set to ATAP=1, BD=7, AO=1):

    Windows: xbee3_ota_tool COMxx path\to\file.ota

    POSIX: ./xbee3_ota_tool /dev/ttySxx path/to/file.ota
*/

#include <stdio.h>
#include <stdlib.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/wpan.h"

#include "zigbee/zcl_ota_server.h"
#include "zigbee/zdo.h"

#include "_atinter.h"           // Common code for processing AT commands
#include "_nodetable.h"         // Common code for handling remote node lists
#include "sample_cli.h"         // Common code for parsing user-entered data
#include "parse_serial_args.h"

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,         // for processing AT command responses
    XBEE_FRAME_HANDLE_ATND_RESPONSE,    // for processing ATND responses

    // this entry is for when ATAO is not 0
    XBEE_FRAME_HANDLE_RX_EXPLICIT,      // rx messages via endpoint table

    XBEE_FRAME_TABLE_END
};

zcl_ota_upgrade_source_t ota_update;

// See zigbee/zcl_ota_server.h for documentation of this callback.
const zcl_ota_upgrade_source_t
    *zcl_ota_get_upgrade_source(const addr64 *client_ieee_be,
                                const zcl_ota_image_id_t *id,
                                uint8_t zcl_command)
{
    char buffer[ADDR64_STRING_LENGTH];

    bool_t match = (id->mfg_code_le == ota_update.id.mfg_code_le
                    && id->image_type_le == ota_update.id.image_type_le);
    if (zcl_command == ZCL_OTA_CMD_QUERY_NEXT_IMAGE_REQ) {
        printf("ota to %s: responding to Query Next Image Request\n",
               addr64_format(buffer, client_ieee_be));
    } else if (match) {
        // Only match file_version if this isn't a QUERY_NEXT_IMAGE_REQ where
        // the client is telling us what their current version is.
        match = id->file_version_le == ota_update.id.file_version_le;
    }

    return match ? &ota_update : NULL;
}

// See zigbee/zcl_ota_server.h for documentation of this callback.
uint32_t zcl_ota_get_upgrade_time(const addr64 *client_ieee_be,
                                  const zcl_ota_upgrade_source_t *source)
{
    char buffer[ADDR64_STRING_LENGTH];

    XBEE_UNUSED_PARAMETER(source);

    printf("ota to %s: update complete\n",
           addr64_format(buffer, client_ieee_be));

    // number of seconds for client to wait before installing upgrade
    return 0;
}

// Cluster table for WPAN_ENDPOINT_DIGI_DATA.  Must be ordered by cluster ID.
const wpan_cluster_table_entry_t digi_data_clusters[] =
{
    // Zigbee OTA Upgrade Cluster (cluster 0x0019)
    ZCL_OTA_UPGRADE_CLUSTER_ENTRY,

    // handle join notifications (cluster 0x0095) when ATAO is not 0
    XBEE_DISC_DIGI_DATA_CLUSTER_ENTRY,

    WPAN_CLUST_ENTRY_LIST_END
};

/// Used to track ZDO transactions in order to match responses to requests
/// (#ZDO_MATCH_DESC_RESP).
wpan_ep_state_t zdo_ep_state = { 0 };

/// Used to track ZCL Upgrade Image Notify command sequence numbers
wpan_ep_state_t digi_data_ep_state = { 0 };

const wpan_endpoint_table_entry_t sample_endpoints[] = {
    ZDO_ENDPOINT(zdo_ep_state),

    // Endpoint/cluster for OTA Upgrade Cluster and join notifications
    {   WPAN_ENDPOINT_DIGI_DATA,        // endpoint
        WPAN_PROFILE_DIGI,              // profile ID
        NULL,                           // endpoint handler
        &digi_data_ep_state,            // ep_state
        0x0000,                         // device ID
        0x00,                           // version
        digi_data_clusters              // clusters
    },

    { WPAN_ENDPOINT_END_OF_LIST }
};


// Callback registered with xbee_disc_add_node_id_handler to update node list.
void node_discovered(xbee_dev_t *xbee, const xbee_node_id_t *rec)
{
    if (rec != NULL) {
        int index = node_add(rec);
        if (index == -ENOSPC) {
            printf("Node table full.  Unable to add node:\n    ");
        } else {
            printf("%2u: ", index);
        }
        xbee_disc_node_id_dump(rec);
    }
}


void handle_menu_cmd(xbee_dev_t *xbee, char *command)
{
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(command);

    puts("");

    print_cli_help_atcmd();

    print_cli_help_nodetable();

    puts("--- OTA Update ---");
    puts(" notify <n>                      Notify a node of upgrade");
    puts(" notify all                      Broadcast Notification to all nodes");

    puts("--- Other ---");

    print_cli_help_menu();

    puts(" quit                            Quit");
    puts("");
}


void handle_notify_cmd(xbee_dev_t *xbee, char *command)
{
    const addr64 *ieee_addr_be;
    uint16_t network_addr;
    const xbee_node_id_t *target = NULL;

    if (strcmpi(command, "notify all") == 0) {
        ieee_addr_be = WPAN_IEEE_ADDR_BROADCAST;
        network_addr = WPAN_NET_ADDR_UNDEFINED;
    } else {
        char *p;
        unsigned n = (unsigned)strtoul(&command[6], &p, 0);

        if (p == NULL || p == &command[6]) {
            puts("Error: must specify node index");
            return;
        }

        target = node_by_index(n);
        if (target == NULL) {
            printf("Error: invalid node index %u\n", n);
            return;
        }
        ieee_addr_be = &target->ieee_addr_be;
        network_addr = target->network_addr;
    }

    int result = zcl_ota_upgrade_image_notify(&xbee->wpan_dev,
                                              &ota_update.id,
                                              &sample_endpoints[1],
                                              WPAN_ENDPOINT_DIGI_DATA,
                                              ieee_addr_be,
                                              network_addr);

    if (target) {
        if (result) {
            printf("Error %d sending notify to:\n", result);
        } else {
            puts("Sent notify to:");
        }
        xbee_disc_node_id_dump(target);
    } else {
        if (result) {
            printf("Error %d sending broadcast notify.\n", result);
        } else {
            puts("Sent broadcast notify.");
        }
    }
}


const cmd_entry_t commands[] = {
    ATCMD_CLI_ENTRIES
    MENU_CLI_ENTRIES
    NODETABLE_CLI_ENTRIES

    { "notify",         &handle_notify_cmd },

    { NULL, NULL }                      // end of command table
};


/// Structure used as .context element of zcl_ota_upgrade_source_t.
typedef struct {
    FILE        *file;
    size_t      data_offset;            // offset to GBL content of OTA file
} ota_context_t;

/// Callback used as .read_handler element of zcl_ota_upgrade_source_t.
int ota_read(const zcl_ota_upgrade_source_t *source, void *dest,
             uint32_t offset, size_t bytes, const addr64 *client_ieee_be)
{
    char buffer[ADDR64_STRING_LENGTH];
    ota_context_t *c = source->context;

    int err = fseek(c->file, offset + c->data_offset, SEEK_SET);
    if (err == 0) {
        err = fread(dest, 1, bytes, c->file);
    }

    printf("ota to %s: %d bytes from offset %" PRIu32 "/%" PRIu32 " (%u%%)\n",
           addr64_format(buffer, client_ieee_be),
           err, offset, source->image_size, offset * 100 / source->image_size);

    return err;
}

/// File to send.
ota_context_t ota_context = { NULL, 0 };

/// Load an OTA file for updates.
int load_ota_file(const char *filename)
{
    if (ota_context.file != NULL) {
        puts("Error: file previously loaded for updates");
        return -EPERM;
    }

    int retval;
    zcl_ota_file_header_t   ota_header;
    FILE *f = fopen(filename, "rb");

    if (f == NULL) {
        retval = -errno;
        fprintf(stderr, "Error %d opening '%s'\n", retval, filename);

        return retval;
    }

    int bytes = fread(&ota_header, 1, sizeof ota_header, f);
    if (bytes != sizeof ota_header) {
        retval = ferror(f);
        fprintf(stderr, "Error %d reading OTA header of '%s'\n",
                retval, filename);
        fclose(f);

        return retval;
    }

    uint32_t file_id = le32toh(ota_header.file_id_le);
    if (file_id != ZCL_OTA_FILE_ID) {
        fprintf(stderr, "Invalid OTA file (0x%08" PRIX32 " != 0x%08" PRIX32 "\n",
               file_id, ZCL_OTA_FILE_ID);
        return -EILSEQ;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);

    ota_context.file = f;

    ota_update.id = ota_header.id;

    // XBee3 OTA updates only send the first sub-element of the OTA file.
    ota_context.data_offset =
        le16toh(ota_header.header_len_le) + sizeof(zcl_ota_element_t);

    ota_update.image_size = (uint32_t)(file_size - ota_context.data_offset);

    ota_update.read_handler = &ota_read;
    ota_update.context = &ota_context;

    printf("Loaded '%s':\n", filename);
    printf("  OTA Header: %.32s\n", ota_header.ota_header_string);
    printf("  Mfg 0x%04X  Type 0x%04X  Ver 0x%08" PRIX32 "  Length %u\n",
           le16toh(ota_update.id.mfg_code_le),
           le16toh(ota_update.id.image_type_le),
           le32toh(ota_update.id.file_version_le),
           ota_update.image_size);

    return 0;
}


int main(int argc, char *argv[])
{
    xbee_dev_t my_xbee;
    char cmdstr[256];
    xbee_serial_t XBEE_SERPORT;
    int status, frame_count;

    const char *filename = argv[argc - 1];
    int name_len = strlen(filename);
    if (name_len < 4
        || (strcmpi(&filename[name_len - 4], ".ota") != 0
            && strcmpi(&filename[name_len - 4], ".otb") != 0))
    {
        puts("Error: specify .ota or .otb filename as last parameter");
        return EXIT_FAILURE;
    }

    if (load_ota_file(filename) != 0) {
        return EXIT_FAILURE;
    }

    parse_serial_arguments(argc, argv, &XBEE_SERPORT);

    // initialize the serial and device layer for this XBee device
    if (xbee_dev_init(&my_xbee, &XBEE_SERPORT, NULL, NULL)) {
        printf("Failed to initialize device.\n");
        return 0;
    }

    // Initialize the AT Command layer for this XBee device and have the
    // driver query it for basic information (hardware version, firmware version,
    // serial number, IEEE address, etc.)
    xbee_cmd_init_device(&my_xbee);
    printf("Waiting for driver to query the XBee device...\n");
    while ((status = xbee_cmd_query_status(&my_xbee)) == -EBUSY) {
        xbee_dev_tick(&my_xbee);
    }
    if (status) {
        printf("Error %d waiting for query to complete.\n", status);
    }

    // report on the settings
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    // Initialize the WPAN layer of the XBee device driver.  This layer enables
    // endpoints and clusters, and is required for all ZigBee layers.
    xbee_wpan_init(&my_xbee, sample_endpoints);

    // receive node discovery notifications
    xbee_disc_add_node_id_handler(&my_xbee, &node_discovered);

    handle_menu_cmd(NULL, NULL);

    // automatically initiate node discovery
    handle_nd_cmd(&my_xbee, "nd");

    while (1) {
        int linelen;
        do {
            linelen = xbee_readline(cmdstr, sizeof cmdstr);
            xbee_cmd_tick();
            frame_count = wpan_tick(&my_xbee.wpan_dev);
        } while (linelen == -EAGAIN && frame_count >= 0);

        if (frame_count < 0) {
            printf("Error %d calling wpan_tick().\n", frame_count);
            break;
        }

        if (linelen == -ENODATA || strcmpi(cmdstr, "quit") == 0) {
            break;
        }

        sample_cli_dispatch(&my_xbee, cmdstr, &commands[0]);
    }

    return frame_count < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
