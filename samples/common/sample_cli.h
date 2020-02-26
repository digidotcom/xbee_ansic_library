/*
 * Copyright (c) 2020 Digi International Inc.,
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
    Shared code used by samples to process user-entered commands using a
    table to match patterns and dispatch to specific functions for each
    command.
*/

#ifndef SAMPLE_CLI_H
#define SAMPLE_CLI_H

#include "xbee/device.h"

typedef void (*command_fn)(xbee_dev_t *xbee, char *command);
typedef struct cmd_entry_t {
    const char *command;
    command_fn handler;
} cmd_entry_t;

int sample_cli_dispatch(xbee_dev_t *xbee, char *cmdstr,
                        const cmd_entry_t *cmd_table);

// Split cmdstr into a maximum of *argc null-terminated strings, stored in
// argv[].  Returns the number of values assigned to argv[] or a negative
// error code.
int sample_cli_split(char *cmdstr, char *argv[], int argc);

// Declaration for function provided by each sample program
void handle_menu_cmd(xbee_dev_t *xbee, char *command);

void print_cli_help_menu(void);

#define MENU_CLI_ENTRIES \
    { "?",              &handle_menu_cmd },  \
    { "help",           &handle_menu_cmd },  \
    { "menu",           &handle_menu_cmd },  \


#endif // SAMPLE_CLI_H
