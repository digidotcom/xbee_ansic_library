/*
 * Copyright (c) 2007-2019 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/discovery.h"

#include "_atinter.h"           // Common code for processing AT commands
#include "_nodetable.h"         // Common code for handling remote node lists
#include "sample_cli.h"         // Common code for parsing user-entered data
#include "parse_serial_args.h"

xbee_dev_t my_xbee;

/*
    main

    Initiate communication with the XBee module, then accept AT commands from
    STDIO, pass them to the XBee module and print the result.
*/


const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,         // AT command responses
    XBEE_FRAME_HANDLE_REMOTE_AT,        // remote AT command responses
    XBEE_FRAME_HANDLE_ATND_RESPONSE,    // ATND responses

    XBEE_FRAME_MODEM_STATUS_DEBUG,      // modem status frames

    XBEE_FRAME_TABLE_END
};


// Global to signal main input routine to redraw input prompt after we erase
// it in the AT Command callback routine.
int redraw_prompt = 1;
void clear_input(void)
{
    // erase current line & signal input routine to redraw prompt
    printf("\r                                                            \r");
    redraw_prompt = 1;
}


// Callback registered with xbee_disc_add_node_id_handler to update node list.
void node_discovered(xbee_dev_t *xbee, const xbee_node_id_t *rec)
{
    if (rec != NULL) {
        clear_input();

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

    puts(" target                          Send AT commands to local device");
    puts(" target <n>                      Send AT commands to node <n>");

    puts("--- Other ---");

    print_cli_help_menu();

    puts(" quit                            Quit");
    puts("");
}


const xbee_node_id_t *target = NULL;

// target <n>
void handle_target_cmd(xbee_dev_t *xbee, char *command)
{
    const char *p = &command[6];        // point beyond "target"
    while (isspace((uint8_t)*p)) {
        ++p;
    }
    if (*p == '\0') {
        puts("Targeting local XBee module");
        target = NULL;
        return;
    }

    char *next_param;
    unsigned n = (unsigned)strtoul(p, &next_param, 0);

    if (next_param == NULL || next_param == p) {
        puts("Error: must specify node index");
    } else {
        target = node_by_index(n);
        if (target == NULL) {
            printf("Error: invalid node index %u\n", n);
        }
    }
}

static int _at_cmd_callback(const xbee_cmd_response_t FAR *response)
{
    clear_input();
    return xbee_cmd_callback(response);
}

void handle_remote_at_cmd(xbee_dev_t *xbee, char *cmdstr)
{
   int request = process_command_remote(xbee, cmdstr,
                                        target ? &target->ieee_addr_be : NULL);
   if (request >= 0) {
       // override the default response handler with our own
       xbee_cmd_set_callback(request, _at_cmd_callback, NULL);
   }
}


const cmd_entry_t commands[] = {
    // use our own AT command handler to target remote nodes
    { "at",             &handle_remote_at_cmd },

    MENU_CLI_ENTRIES
    NODETABLE_CLI_ENTRIES

    { "target",         &handle_target_cmd },

    { NULL, NULL }                      // end of command table
};

int main(int argc, char *argv[])
{
    char cmdstr[256] = { '\0' };
    int status, frame_count;
    xbee_serial_t XBEE_SERPORT;

    parse_serial_arguments(argc, argv, &XBEE_SERPORT);

    // initialize the serial and device layer for this XBee device
    if (xbee_dev_init(&my_xbee, &XBEE_SERPORT, NULL, NULL)) {
        printf("Failed to initialize device.\n");
        return 0;
    }

    // Initialize the AT Command layer for this XBee device and have the
    // driver query it for basic information (hardware version, firmware version,
    // serial number, IEEE address, etc.)
    xbee_cmd_init_device( &my_xbee);
    printf( "Waiting for driver to query the XBee device...\n");
    do {
        xbee_dev_tick( &my_xbee);
        status = xbee_cmd_query_status( &my_xbee);
    } while (status == -EBUSY);
    if (status) {
        printf( "Error %d waiting for query to complete.\n", status);
    }

    // report on the settings
    xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    // receive node discovery notifications
    xbee_disc_add_node_id_handler(&my_xbee, &node_discovered);

    handle_menu_cmd(NULL, NULL);

    // automatically initiate node discovery
    handle_nd_cmd(&my_xbee, "nd");

    while (1) {
        int linelen;
        redraw_prompt = 1;
        *cmdstr = '\0';
        do {
            if (redraw_prompt) {
                redraw_prompt = 0;
                if (target) {
                    printf("[%08" PRIx32 "-%08" PRIx32 "]> %s",
                           be32toh(target->ieee_addr_be.l[0]),
                           be32toh(target->ieee_addr_be.l[1]),
                           cmdstr);
                } else {
                    printf("[LOCAL]> %s", cmdstr);
                }
            }
            linelen = xbee_readline(cmdstr, sizeof cmdstr);
            xbee_cmd_tick();
            frame_count = xbee_dev_tick(&my_xbee);
        } while (linelen == -EAGAIN && frame_count >= 0);

        if (frame_count < 0) {
            printf("Error %d calling xbee_dev_tick().\n", frame_count);
            break;
        }

        if (linelen == -ENODATA || strcmpi(cmdstr, "quit") == 0) {
            break;
        }

        sample_cli_dispatch(&my_xbee, cmdstr, &commands[0]);
    }

    return frame_count < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
