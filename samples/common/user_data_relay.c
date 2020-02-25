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
    Sample: User Data Relay

    Demonstrates sending frame type 0x2D (User Data Relay) and parsing 0xAD
    (User Data Relay Output) responses.

    Connect to the serial port of an XBee module in API mode (ATAP=1).

    You can send a loopback test by selecting "Serial" as the target,
    then typing a message and hitting return.

    For testing with the BLE interface, first use the xbee3_srp_verifier
    sample to set a password and enable the BLE interface (ATBT=1).  Then
    use the Digi XBee Mobile app[1] to connect to the BLE interface and
    use its "Relay Console" (in "Options") to send and receive data.

    For testing with the MicroPython interface, you can use samples from
    the xbee-micropython project[2].

    [1]: https://www.digi.com/products/embedded-systems/digi-xbee/digi-xbee-tools/digi-xbee-mobile-app
    [2]: https://github.com/digidotcom/xbee-micropython/
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/tx_status.h"
#include "xbee/user_data.h"

#include "parse_serial_args.h"


// function that handles received User Data frames
int user_data_rx(xbee_dev_t *xbee, const void FAR *raw,
                 uint16_t length, void FAR *context)
{
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(context);

    const xbee_frame_user_data_rx_t FAR *data = raw;
    int payload_length = length - offsetof(xbee_frame_user_data_rx_t,
                                           payload);

    printf("Message from %s interface:\n",
        xbee_user_data_interface(data->source));

    // If all characters of message are printable, just print it as a string
    // with printf().  Otherwise use hex_dump() for non-printable messages.
    int printable = TRUE;
    for (size_t i = 0; printable && i < payload_length; ++i) {
        if (!isprint(data->payload[i])) {
            printable = FALSE;
        }
    }

    if (printable) {
        printf("%.*s\n", payload_length, data->payload);
    } else {
        hex_dump(data->payload, payload_length, HEX_DUMP_FLAG_OFFSET);
    }

    return 0;
}


int dump_tx_status(xbee_dev_t *xbee, const void FAR *frame,
                   uint16_t length, void FAR *context)
{
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(length);
    XBEE_UNUSED_PARAMETER(context);

    const xbee_frame_tx_status_t FAR *tx_status = frame;
    char buffer[40];
    const char *status = NULL;

    // Provide descriptive strings for the only two errors we expect
    // from sending User Data Relay frames.
    switch (tx_status->delivery) {
        case XBEE_TX_DELIVERY_INVALID_INTERFACE:
            status = "invalid interface";
            break;
        case XBEE_TX_DELIVERY_INTERFACE_BLOCKED:
            status = "interface blocked";
            break;
        default:
            sprintf(buffer, "unknown status 0x%X", tx_status->delivery);
            status = buffer;
    }

    printf("Error on message id 0x%02X: %s\n", tx_status->frame_id, status);

    return 0;
}


const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
    XBEE_FRAME_HANDLE_LOCAL_AT,
    { XBEE_FRAME_USER_DATA_RX, 0, user_data_rx, NULL },
    { XBEE_FRAME_TX_STATUS, 0, dump_tx_status, NULL },
    XBEE_FRAME_TABLE_END
};


xbee_dev_t my_xbee;

// TODO: add a command-line parameter to select the starting interface?
int main(int argc, char *argv[])
{
    char cmdstr[256];
    xbee_serial_t XBEE_SERPORT;
    // default to loopback messages
    int iface = XBEE_USER_DATA_IF_SERIAL;
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
    printf( "Waiting for driver to query the XBee device...\n");
    do {
        xbee_dev_tick(&my_xbee);
        status = xbee_cmd_query_status(&my_xbee);
    } while (status == -EBUSY);
    if (status) {
        printf( "Error %d waiting for query to complete.\n", status);
    }

    // report on the settings
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    printf("Enter a blank line to change interface.  CTRL-D to exit.\n");
    printf("Enter messages for %s interface:\n",
           xbee_user_data_interface(iface));

    while (1) {
        int linelen;
        do {
            linelen = xbee_readline(cmdstr, sizeof cmdstr);
            if (linelen == -ENODATA) {
                return 0;
            }
            status = xbee_dev_tick(&my_xbee);
            if (status < 0) {
               printf("Error %d from xbee_dev_tick().\n", status);
               return -1;
            }
        } while (linelen == -EAGAIN);

        // blank line changes to next interface
        if (*cmdstr == '\0') {
            if (++iface > XBEE_USER_DATA_IF_MICROPYTHON) {
                iface = XBEE_USER_DATA_IF_SERIAL;
            }
            printf("Enter messages for %s interface:\n",
                   xbee_user_data_interface(iface));
            continue;
        }

        status = xbee_user_data_relay_tx(&my_xbee, iface,
                                         cmdstr, strlen(cmdstr));
        if (status < 0) {
            printf("error %d sending data\n", status);
        } else {
            printf("sent message id 0x%02X\n", status);
        }
    }

    return 0;
}
