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
    Sample: xbee3_verifier

    Tool for setting and retrieving the Secure Remote Password (SRP) salt and
    verifier used to access BLE on XBee3 and SecureSessions on XBee3
    802.15.4/Digimesh/Zigbee.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/random.h"
#include "util/srp.h"

#include "_atinter.h"           // Common code for processing AT commands
#include "sample_cli.h"         // Common code for parsing user-entered data
#include "parse_serial_args.h"

typedef struct xbee_verifier_settings_t {
    const char *description;
    const char *username;
    const char *atcmd[5];
} xbee_verifier_settings_t;

const xbee_verifier_settings_t ble_verifier = {
    "BLE",
    "apiservice",
    { "$S", "$V", "$W", "$X", "$Y" }
};

const xbee_verifier_settings_t secure_session_verifier = {
    "SecureSession",
    "xbee",
    { "*S", "*V", "*W", "*X", "*Y" }
};

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,         // for processing AT command responses
    XBEE_FRAME_TABLE_END
};


// Structure used to hold SRP salt and verifier for BLE or SecureSession.
typedef struct verifier_data_t {
    uint8_t salt[4];
    uint8_t verifier[4][32];
} verifier_data_t;

verifier_data_t verifier_data;

// set_verifier_regs() updates the .command and .type (XBEE_CLT_COPY when
// reading or XBEE_CLT_SET when writing) entries of this structure, and then
// execute_verifier_regs() uses xbee_cmd_list_execute() to either read data
// into or write data out to verifier_data.
xbee_atcmd_reg_t verifier_regs[] = {
    XBEE_ATCMD_REG('$', 'S', XBEE_CLT_COPY, verifier_data_t, salt),
    XBEE_ATCMD_REG('$', 'V', XBEE_CLT_COPY, verifier_data_t, verifier[0]),
    XBEE_ATCMD_REG('$', 'W', XBEE_CLT_COPY, verifier_data_t, verifier[1]),
    XBEE_ATCMD_REG('$', 'X', XBEE_CLT_COPY, verifier_data_t, verifier[2]),
    XBEE_ATCMD_REG('$', 'Y', XBEE_CLT_COPY, verifier_data_t, verifier[3]),

    XBEE_ATCMD_REG_END       // Mark list end
};


void set_verifier_regs(const xbee_verifier_settings_t *verifier,
                       bool_t is_write)
{
    for (int i = 0; i < 5; ++i) {
        // copy AT commands to verifier_regs[] and set .type based on is_write
        memcpy(verifier_regs[i].command.str, verifier->atcmd[i], 2);
        verifier_regs[i].type = is_write ? XBEE_CLT_SET : XBEE_CLT_COPY;
    }
}

void execute_verifier_regs(xbee_dev_t *xbee)
{
    xbee_command_list_context_t clc;
    int status;

    // Execute pre-defined list of commands and save results to buffer
    xbee_cmd_list_execute(xbee, &clc, verifier_regs, &verifier_data, NULL);
    while ((status = xbee_cmd_list_status(&clc)) == XBEE_COMMAND_LIST_RUNNING) {
        xbee_dev_tick(xbee);
    }

    if (status == XBEE_COMMAND_LIST_TIMEOUT) {
        puts("Error: XBee command list timed out.");
    } else if (status == XBEE_COMMAND_LIST_ERROR) {
        puts("Error: XBee command list completed with errors.");
    } else {
        for (int i = 0; i < 5; ++i) {
            printf("AT%.2s 0x", verifier_regs[i].command.str);
            const uint8_t *p = verifier_data.salt + verifier_regs[i].offset;
            for (int b = 0; b < verifier_regs[i].bytes; ++b) {
                printf("%02X", *p++);
            }
            puts("");
        }
    }
    puts("");
}

void generate_verifier(xbee_dev_t *xbee,
                       const xbee_verifier_settings_t *verifier,
                       const char *password)
{
    const uint8_t *bytes_s;
    const uint8_t *bytes_v;
    int len_s, len_v;

    printf("Generating %s verifier with user '%s' and pass '%s':\n",
           verifier->description, verifier->username, password);

    int ret = srp_create_salted_verification_key(verifier->username,
                                                 (const uint8_t *)password,
                                                 strlen(password),
                                                 &bytes_s, &len_s,
                                                 &bytes_v, &len_v);

    if (ret) {
        printf("Error %d generating verification key.\n", ret);
        return;
    }

    // prepare to write salt and verifier
    set_verifier_regs(verifier, TRUE);
    memcpy(verifier_data.salt, bytes_s, 4);
    for (int i = 0; i < 4; ++i) {
        memcpy(verifier_data.verifier[i], &bytes_v[i * 32], 32);
    }

    // release memory allocated by srp_create_salted_verification_key()
    free((void *)bytes_s);
    free((void *)bytes_v);

    execute_verifier_regs(xbee);

    puts(">>> Remember to ATWR to save these values! <<<");
    puts("");
}


void read_verifier(xbee_dev_t *xbee, const xbee_verifier_settings_t *verifier)
{
    printf("Reading %s verifier:\n", verifier->description);

    set_verifier_regs(verifier, FALSE);

    execute_verifier_regs(xbee);
}

void handle_verifier(xbee_dev_t *xbee, const char *param,
                     const xbee_verifier_settings_t *verifier)
{
    // skip over whitespace
    while (isspace((uint8_t)*param)) {
        ++param;
    }

    if (*param == '\0') {
        // user is querying settings for this verifier
        read_verifier(xbee, verifier);
    } else {
        // user wants to set verifier using <param> as the password
        generate_verifier(xbee, verifier, param);
    }
}

void handle_ble_cmd(xbee_dev_t *xbee, char *command)
{
    handle_verifier(xbee, &command[3], &ble_verifier);
}

void handle_ss_cmd(xbee_dev_t *xbee, char *command)
{
    handle_verifier(xbee, &command[2], &secure_session_verifier);
}

void handle_menu_cmd(xbee_dev_t *xbee, char *command)
{
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(command);

    puts("");

    print_cli_help_atcmd();

    puts("--- Get/Set Salt/Verifier ---");
    puts(" ble                             Report values for BLE verifier");
    puts(" ble <new_password>              Generate new BLE verifier");
    puts(" ss                              Report values for SecureSession verifier");
    puts(" ss <new_password>               Generate new SecureSession verifier");

    puts("--- Other ---");

    print_cli_help_menu();

    puts(" quit                            Quit");
    puts("");
}


const cmd_entry_t commands[] = {
    ATCMD_CLI_ENTRIES
    MENU_CLI_ENTRIES

    { "ble",            &handle_ble_cmd },
    { "ss",             &handle_ss_cmd },

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

    handle_menu_cmd(NULL, NULL);

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
