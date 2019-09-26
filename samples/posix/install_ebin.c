/*
 * Copyright (c) 2010-2019 Digi International Inc.,
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
    Install firmware updates on XBee modules that use .EBIN firmware files
    and the "Gen3" bootloader.

    ./install_ebin /dev/ttyS7 firmware.ebin     # install firmware image
    ./install_ebin /dev/ttyS31 --query          # query BL & report hw info
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/bl_gen3.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"         // for XBEE_FRAME_HANDLE_LOCAL_AT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_TABLE_END
};

#include "parse_serial_args.h"

xbee_dev_t my_xbee;

int fw_read(void *context, void *buffer, int16_t bytes)
{
    return fread(buffer, 1, bytes, context);
}

int main(int argc, char *argv[])
{
    xbee_gen3_update_t fw = { 0 };
    char *firmware;
    FILE *file = NULL;
    char buffer[80];
    uint32_t t;
    int result;
    int query_only = FALSE;
    xbee_gen3_state_t last_state = 0, new_state;
    xbee_serial_t XBEE_SERPORT;

    parse_serial_arguments(argc, argv, &XBEE_SERPORT);

    // treat last argument as firmware filename
    if (argc > 1) {
        if (strcmp(argv[argc - 1], "--query") == 0) {
            query_only = TRUE;
        } else {
            firmware = argv[argc - 1];
            file = fopen(firmware, "rb");
        }
    }
    if (!query_only && !file) {
        if (firmware == NULL) {
            printf("Error: pass path to .EBIN or --query as last parameter\n");
        } else {
            printf("Error: couldn't open %s\n", firmware);
        }
        exit(-1);
    }

    if (xbee_dev_init(&my_xbee, &XBEE_SERPORT, NULL, NULL)) {
        printf("Failed to initialize device.\n");
        return 0;
    }

    if (query_only) {
        printf("Querying bootloader for extended version info...\n");
    } else {
        fw.context = file;
        fw.read = fw_read;
        printf("Installing %s...\n", firmware);
    }
    xbee_bl_gen3_install_init(&my_xbee, &fw);

    do {
        t = xbee_millisecond_timer();
        result = xbee_bl_gen3_install_tick(&fw);
        t = xbee_millisecond_timer() - t;
        new_state = xbee_bl_gen3_install_state(&fw);
#ifdef BLOCKING_WARNING
        if (t > 50) {
            printf("!!! blocked for %ums in state 0x%04X (now state 0x%04X)\n",
                t, last_state, new_state);
        }
#endif
        if (last_state != new_state) {
            last_state = new_state;
            printf(" Status: %s                          \r",
                xbee_bl_gen3_install_status(&fw, buffer));
            fflush(stdout);
        }
    } while (result == -EAGAIN);

    if (result) {
        printf("\nFAILED with error %d\n", result);
    } else {
        printf("\nSUCCESS\n");
        xbee_gen3_dump_extended_ver(&fw.ext_ver);
    }

    if (file) {
        fclose(file);
    }

    xbee_ser_close(&my_xbee.serport);

    // Note that some firmware updates may revert to default settings,
    // including baud rate, flow control and even API mode.

    return 0;
}

