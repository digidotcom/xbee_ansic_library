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
    Sample: xbee3_secure_session

    Tool to demonstrate establishing a Secure Session to a remote node on the
    network.

    On all nodes, set:
        ATBD 7          Default baud rate for all samples (115,200).
        ATAP 1          Use regular API mode (non-escaped).
        ATAO 0          For Node Discovery responses in 0x95 frames.

    Optional:
        ATAZ 8          Enable Extended Modem Status (0x98) frames instead of
                        regular Modem Status (0x8A).

    Use the `xbee3_srp_verifier` sample to set the Secure Session password on
    each node.

    Windows usage:
        xbee3_secure_session COMxx
    POSIX usage:
        xbee3_secure_session /dev/ttySxx
*/

/**** Configuration Options for Sample ****/

// XBEE_SS_REQ_OPT_TIMEOUT_FIXED or XBEE_SS_REQ_OPT_TIMEOUT_INTER_PACKET
#define TIMEOUT_OPT             XBEE_SS_REQ_OPT_TIMEOUT_INTER_PACKET

// Duration in units of 0.1s, or XBEE_SS_REQ_TIMEOUT_YIELDING
#define TIMEOUT_DURATION        (60 * XBEE_SS_REQ_TIMEOUT_UNITS_PER_SEC)

/**** End of Configuration Options ****/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/random.h"
#include "xbee/ext_modem_status.h"
#include "xbee/secure_session.h"
#include "util/srp.h"

#include "_atinter.h"           // Common code for processing AT commands
#include "_nodetable.h"         // Common code for handling remote node lists
#include "sample_cli.h"         // Common code for parsing user-entered data
#include "parse_serial_args.h"

int dump_ext_modem_status(xbee_dev_t *xbee,
                          const void FAR *payload,
                          uint16_t length,
                          void FAR *context)
{
    int ret;

    // default/common case: a Secure Session Extended Modem Status
    ret = xbee_frame_dump_ext_mod_status_ss(xbee, payload, length, context);

    if (ret == -ENOENT) {
        // use a standard hex dump for all other Extended Modem Status frames
        ret = xbee_frame_dump_ext_modem_status(xbee, payload, length, context);
    }

    return ret;
}

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,         // for processing AT command responses
    XBEE_FRAME_HANDLE_ATND_RESPONSE,    // for processing ATND responses

    // Dump Modem Status frames (if bit 3 of ATAZ is clear)
    XBEE_FRAME_MODEM_STATUS_DEBUG,

    // Our handler for Extended Modem Status frames (if bit 3 of ATAZ set)
    { XBEE_FRAME_EXT_MODEM_STATUS, 0, dump_ext_modem_status, NULL },

    XBEE_FRAME_DUMP_SS_RESP,            // Dump Secure Session Response frames

    XBEE_FRAME_TABLE_END
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

    puts("--- Secure Session ---");
    puts(" login <n> <password>            Initiate client-side session to node <n>");
    puts(" logout <n>                      Logout client-side session with node <n>");
    puts(" logout all                      Broadcast logout of all client-side sessions");
    puts(" close <n>                       Close server-side session with node <n>");
    puts(" close all                       Broadcast close of all server-side sessions");

    puts("--- Other ---");

    print_cli_help_menu();

    puts(" quit                            Quit");
    puts("");
}


const xbee_node_id_t *get_target(const char *num, char **next_param)
{
    const xbee_node_id_t *target = NULL;
    unsigned n = (unsigned)strtoul(num, next_param, 0);

    if (*next_param == NULL || *next_param == num) {
        puts("Error: must specify node index");
    } else {
        target = node_by_index(n);
        if (target == NULL) {
            printf("Error: invalid node index %u\n", n);
        }
    }

    return target;
}


void handle_login_cmd(xbee_dev_t *xbee, char *command)
{
    char *next_param;
    const xbee_node_id_t *target = get_target(&command[5], &next_param);

    if (target == NULL) {
        // error parsing command
        return;
    }

    // advance <next_param> to password
    while (isspace((uint8_t)*next_param)) {
        ++next_param;
    }

    if (*next_param == '\0') {
        puts("Error: must specify password after node number");
        return;
    }

    int result = xbee_secure_session_request(xbee, &target->ieee_addr_be,
                                             XBEE_SS_REQ_OPT_CLIENT_LOGIN
                                                 | TIMEOUT_OPT,
                                             TIMEOUT_DURATION, next_param);

    if (result) {
        printf("Error %d sending %s to:\n", result, "LOGIN");
    } else {
        printf("Sent %s to:\n", "LOGIN");
    }
    xbee_disc_node_id_dump(target);
}


void handle_logout_or_close_cmd(xbee_dev_t *xbee, char *command)
{
    bool_t is_logout = (strncmpi(command, "logout", 6) == 0);
    const char *cmd = is_logout ? "LOGOUT" : "CLOSE";
    const char *params = &command[is_logout ? 6 : 5];
    const addr64 *ieee_addr_be;
    const xbee_node_id_t *target = NULL;
    char *next_param;

    if (strcmpi(params, " all") == 0) {
        ieee_addr_be = WPAN_IEEE_ADDR_BROADCAST;
    } else {
        target = get_target(params, &next_param);
        if (target == NULL) {
            return;
        }
        ieee_addr_be = &target->ieee_addr_be;
    }

    int result = xbee_secure_session_request(
                     xbee, ieee_addr_be,
                     is_logout ? XBEE_SS_REQ_OPT_CLIENT_LOGOUT
                               : XBEE_SS_REQ_OPT_SERVER_TERMINATION,
                     0, NULL);

    if (target) {
        if (result) {
            printf("Error %d sending %s to:\n", result, cmd);
        } else {
            printf("Sent %s to:\n", cmd);
        }
        xbee_disc_node_id_dump(target);
    } else {
        if (result) {
            printf("Error %d sending broadcast %s.\n", result, cmd);
        } else {
            printf("Sent broadcast %s.\n", cmd);
        }
    }
}


const cmd_entry_t commands[] = {
    ATCMD_CLI_ENTRIES
    MENU_CLI_ENTRIES
    NODETABLE_CLI_ENTRIES

    { "login",          &handle_login_cmd },
    { "logout",         &handle_logout_or_close_cmd },
    { "close",          &handle_logout_or_close_cmd },

    { NULL, NULL }                      // end of command table
};


int main(int argc, char *argv[])
{
    xbee_dev_t my_xbee;
    char cmdstr[256];
    xbee_serial_t XBEE_SERPORT;
    int status, frame_count;

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
