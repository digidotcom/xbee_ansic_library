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
    Sample: Zigbee Register Device

    Tool used for registering devices to a Zigbee 3.0 network using
    XBEE_FRAME_REGISTER_DEVICE (0x24) frames sent to the trust center.

    Connect to centralized trust center (device with ATCE=1) with this sample,
    and use the `atinter` sample on a router (device with ATCE=0) not already
    on the network.  Sample provides methods of joining with a random link key,
    or an install code from the router.

    Make sure ATAO=0 if using the node discovery features.  To update this
    sample to work with ATAO != 0, copy the endpoint table from
    `samples/common/transparent_client.c`.

    Notes:
    * Make sure both devices have matching ATEE and ATEO settings.
    * Use ATSH and ATSL on the router to retrieve its serial number.
    * To test different settings/options, use ATNR on the router to have it
      leave the network.
    * The "dereg" command in this sample only removes entries that haven't
      been used to join a node to the network.  It does not kick nodes off
      of the network.

    Joining with an install key:
    * Set ATDC to 1 on the router.
    * Use ATI? on the router to retrieve its install code.
    * Enter "reg [ATSH value][ATSL value] [ATI? value]" in this sample's
      console to allow the router to join.
        example: reg 0013a200FFFFFFFF 0x09AE2F0353C7CD1E7C754DAAD650A026DE4E

    Joining with a link key:
    * Set ATDC to 0 on the router.
    * Use ATKY on the router to set a key (e.g., "ATKY 0x0123456789ABCDEF").
    * Enter "reg [ATSH value][ATSL value] [ATKY hex value]" in this sample's
      console to allow the router to join.
        example: reg 0013a200FFFFFFFF 0x1234567890ABCDEF

*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/register_device.h"
#include "wpan/types.h"

#include "_atinter.h"           // Common code for processing AT commands
#include "_nodetable.h"         // Common code for handling remote node lists
#include "sample_cli.h"         // Common code for parsing user-entered data
#include "parse_serial_args.h"


/**
    @brief
    Parse a hex string, with optional leading 0x, into an array of bytes.

    @param[out] dest    Location to store converted bytes.  Safe to re-use
                        \a src as \a dest.
    @param[in]  src     String consisting of isxdigit() characters.
    @param[in]  max_bytes       Maximum number of bytes to copy to dest.

    @retval     >= 0            Number of bytes stored in \a dest.
    @retval     -EINVAL         Invalid parameter passed to function.
    @retval     -E2BIG          \a src is too big to store in \a dest.

    @sa hexstrtobyte
*/
int xbee_parse_hexstr(void *dest, const char *src, size_t max_bytes)
{
    if (dest == NULL || src == NULL) {
        return -EINVAL;
    }

    // skip over optional leading 0x
    if (memcmp(src, "0x", 2) == 0) {
        src += 2;
    }

    // calculate number of hexadecimal digits (nibbles) in src
    int nibble = 0;
    while (isxdigit((uint8_t)src[nibble])) {
        ++nibble;
        if (nibble > max_bytes * 2) {
            return -E2BIG;
        }
    }

    uint8_t *p = dest;
    if (nibble & 1) {
        // first byte to dest will be a single nibble
        char first_byte[2];

        first_byte[0] = '0';
        first_byte[1] = *src++;
        *p++ = hexstrtobyte(first_byte);
        --nibble;
    }

    for (; nibble; src += 2, nibble -= 2) {
        *p++ = hexstrtobyte(src);
    }

    return p - (uint8_t *)dest;
}


const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,         // for processing AT command responses
    XBEE_FRAME_HANDLE_ATND_RESPONSE,    // for processing ATND responses
    XBEE_FRAME_HANDLE_AO0_NODEID,       // for processing NODEID messages
    { XBEE_FRAME_REGISTER_DEVICE_STATUS, 0,
      xbee_dump_register_device_status, NULL },
    XBEE_FRAME_TABLE_END
};


void node_discovered(xbee_dev_t *xbee, const xbee_node_id_t *rec)
{
    if (rec != NULL) {
        node_add(rec);
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

    puts("--- Register Joining Device ---");
    puts(" reg 0013a20012345678 <key>      Register device with link key/install code");
    puts(" dereg 0013a20012345678          Deregister device");

    puts("--- Other ---");

    print_cli_help_menu();

    puts(" quit                            Quit");
    puts("");
}


void handle_reg_cmd(xbee_dev_t *xbee, char *command)
{
    addr64 addr_be;
    const char *p = &command[3];        // past "reg"

    // buffers for address and key -- optional 0x followed by hexits and null
    char address[2 + 2 * 8 + 1];
    char key[2 + 2 * 18 + 1];
    char overflow;

    if (sscanf(p, "%18s %38s%c", &address[0], &key[0], &overflow) == 2) {
        int addr_len = xbee_parse_hexstr(&addr_be, address, 8);
        int key_len = xbee_parse_hexstr(key, key, 18);

        if (addr_len == 8 && key_len > 0) {
            int result = xbee_register_device(xbee, addr_be, key, key_len);
            if (result < 0) {
                printf("Error %d sending Register Joining Device frame\n",
                       result);
            } else {
                printf("Sent Register Joining Device frame id 0x%02X\n",
                       result);
            }
            return;
        }
    }

    puts("Error parsing address");
}


void handle_dereg_cmd(xbee_dev_t *xbee, char *command)
{
    addr64 addr_be;
    const char *p = &command[5];        // past "dereg"

    int result = addr64_parse(&addr_be, p);
    if (result < 0) {
        printf("Error %d parsing address\n", result);
    } else {
        int frame_id = xbee_register_device(xbee, addr_be, NULL, 0);
        if (frame_id < 0) {
            printf("Error %d registering device\n", frame_id);
        } else {
            char buffer[ADDR64_STRING_LENGTH];

            printf("Sent frame id %u to deregister %s\n", frame_id,
                   addr64_format(buffer, &addr_be));
        }
    }
}


const cmd_entry_t commands[] = {
    ATCMD_CLI_ENTRIES
    MENU_CLI_ENTRIES
    NODETABLE_CLI_ENTRIES

    { "reg ",           &handle_reg_cmd },
    { "dereg ",         &handle_dereg_cmd },

    { NULL, NULL }                      // end of command table
};

int main(int argc, char *argv[])
{
    xbee_dev_t my_xbee;
    char cmdstr[256];
    xbee_serial_t XBEE_SERPORT;
    int status;

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
    xbee_disc_add_node_id_handler( &my_xbee, &node_discovered);

    handle_menu_cmd(NULL, NULL);

    while (1) {
        int linelen;
        while ((linelen = xbee_readline(cmdstr, sizeof cmdstr)) == -EAGAIN) {
            status = xbee_dev_tick(&my_xbee);
            if (status < 0) {
               printf("Error %d from xbee_dev_tick().\n", status);
               return -1;
            }
        }

        if (linelen == -ENODATA || strcmpi(cmdstr, "quit") == 0) {
            break;
        }

        sample_cli_dispatch(&my_xbee, cmdstr, &commands[0]);
    }

    return 0;
}
