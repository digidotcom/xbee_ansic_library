/*
 * Copyright (c) 2024 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 9350 Excelsior Blvd #700, Hopkins, MN 55343
 * =======================================================================
 */

/*
 * Sample program demonstrating streaming of NMEA for XBee devices that support
 * GNSS.
 *
 * Usage: ./gnss_nmea [SERIAL...]
 *
 * The program will read lines from stdin, terminated by a platform-specific
 * sequence, which will be stripped. For each line, if the line starts with the
 * letters "AT", ignoring differences in case, then the line will be sent to the
 * XBee as an AT command. Upon execution of the program, the program will
 * issue a request to the device to begin streaming NMEA. The user may start
 * and stop the NMEA stream by typing 'start' or 'stop'.
 * You may also type 'quit' to exit this program.
 */
#include <stdio.h>
#include <string.h>

#include "xbee/atcmd.h"
#include "xbee/device.h"
#include "xbee/gnss.h"

// Common code
#include "_atinter.h"
#include "parse_serial_args.h"

// An instance required for the local XBee
xbee_dev_t my_xbee;


int gnss_start_stop_handler( xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context);

#define XBEE_FRAME_HANDLE_GNSS_START_STOP     \
    { XBEE_FRAME_GNSS_START_STOP_RESP, 0, gnss_start_stop_handler, NULL }

int nmea_handler( xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context);

#define XBEE_FRAME_HANDLE_GNSS_NMEA     \
    { XBEE_FRAME_GNSS_RAW_NMEA_RESP, 0, nmea_handler, NULL }


const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_HANDLE_GNSS_START_STOP,
    XBEE_FRAME_HANDLE_GNSS_NMEA,
    XBEE_FRAME_TABLE_END
};


// function that handles received GNSS start stop messages
int gnss_start_stop_handler(xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context)
{
    const xbee_frame_gnss_start_stop_resp_t FAR *gnss = raw;

    switch (gnss->request) {
    case XBEE_GNSS_REQUEST_START_RAW_NMEA:

        if (gnss->status != XBEE_GNSS_START_STOP_STATUS_SUCCESS) {
            printf("Got failure response for starting GNSS NMEA\n");
        } else {
            printf("Got success response for starting GNSS NMEA\n");
        }


        break;

    case XBEE_GNSS_REQUEST_STOP_RAW_NMEA:

        if (gnss->status != XBEE_GNSS_START_STOP_STATUS_SUCCESS) {
            printf("Got failure response for stopping GNSS NMEA\n");
        } else {
            printf("Got success response for stopping GNSS NMEA\n");
        }

        break;

    default:
        printf("Unexpected gnss start stop response type %u\n", gnss->request);
    }

    return 0;
}


// function that handles received NMEA messages
int nmea_handler(xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context)
{
    const xbee_frame_gnss_raw_nmea_resp_t FAR *gnss = raw;

    int16_t payload_len = length - offsetof(xbee_frame_gnss_raw_nmea_resp_t, payload);

    printf("%.*s\n", payload_len, gnss->payload);
    return 0;
}

int main( int argc, char *argv[])
{
    // Enough to get a useful error when the message is too long:
    char term_str[64];
    int status;
    xbee_serial_t xbee_ser;
    int update_nmea_state = 1;
    int want_nmea_active = 1;

    parse_serial_arguments(argc, argv, &xbee_ser);

    // Initialize the serial and device layer for this XBee device
    if (xbee_dev_init(&my_xbee, &xbee_ser, NULL, NULL))
    {
        printf( "Failed to initialize device.\n");
        return 0;
    }

    xbee_cmd_init_device(&my_xbee);
    printf( "Waiting for driver to query the XBee device...\n");
    do {
        xbee_dev_tick(&my_xbee);
        status = xbee_cmd_query_status(&my_xbee);
    } while (status == -EBUSY);
    if (status)
    {
        printf( "Error %d waiting for query to complete.\n", status);
    }

    // Report on the settings
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    puts("This sample displays GNSS NMEA stream");
    puts("You may stop the stream by typing 'stop'");
    puts("You may start the stream again by typing 'start'");
    puts("You may issue AT commands by entering a command prefaced by AT");
    puts("You may exit this program by typing 'quit'");
    puts("");

    while (1)
    {

        if (update_nmea_state) {

            update_nmea_state = 0;

            if (want_nmea_active) {

            	status = xbee_gnss_start_raw_nmea(&my_xbee, 0);
            	if (status < 0) {
            		printf("Error %d starting GNSS raw NMEA stream\n", status);
            	} else {
                    printf("Starting GNSS raw NMEA stream\n");
                }

            } else {
                status = xbee_gnss_stop_raw_nmea(&my_xbee, 0);
                if (status < 0) {
                    printf("Error %d stopping GNSS raw NMEA stream\n", status);
                } else {
                    printf("Stopping GNSS raw NMEA stream\n");
                }
            }
        }

        while ((status = xbee_readline(term_str, sizeof term_str)) == -EAGAIN) {
            status = xbee_dev_tick(&my_xbee);
            if (status < 0) {
               printf("Error %d from xbee_dev_tick().\n", status);
               return -1;
            }
            xbee_cmd_tick();
        }
        if (status == -ENODATA || ! strcmpi(term_str, "quit")){
            return 0;
        }
        else if (! strncmpi(term_str, "AT", 2)) {
            process_command(&my_xbee, term_str);
        }
        else if (term_str[0] == 0) {
            puts("");
        }
        else if (! strcmpi(term_str, "stop")) {
            update_nmea_state = 1;
            want_nmea_active = 0;
        }
        else if (! strcmpi(term_str, "start")) {
            update_nmea_state = 1;
            want_nmea_active = 1;
        }
    }
    return 0;
}
