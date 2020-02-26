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

#include <stdio.h>
#include <string.h>

#include "sample_cli.h"

void print_cli_help_menu(void)
{
    puts(" <menu|help|?>                   Print this menu");
}

int sample_cli_dispatch(xbee_dev_t *xbee, char *cmdstr,
                        const cmd_entry_t *cmd_table)
{
    if (cmdstr == NULL || cmd_table == NULL) {
        return -EINVAL;
    }

    // Match command to an entry in the command table.
    const cmd_entry_t *cmd;
    for (cmd = cmd_table; cmd->command != NULL; ++cmd) {
        if (strncmpi(cmdstr, cmd->command, strlen(cmd->command)) == 0) {
            cmd->handler(xbee, cmdstr);
            return 0;
        }
    }

    printf("Error: unknown command '%s'\n", cmdstr);
    return -ENOTSUP;
}
