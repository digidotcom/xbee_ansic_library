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
 * Sample program demonstrating finding the current location for XBee devices
 * that support GNSS.
 *
 * Usage: ./gnss_locate [SERIAL...]
 *
 * The program will read lines from stdin, terminated by a platform-specific
 * sequence, which will be stripped. For each line, if the line starts with the
 * letters "AT", ignoring differences in case, then the line will be sent to the
 * XBee as an AT command. Upon execution of the program, the program will
 * issue a request to the device to get the location. The user may start a new
 * request or stop the current request by typing 'start' or 'stop'.
 * You may also type 'quit' to exit this program.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xbee/atcmd.h"
#include "xbee/device.h"
#include "xbee/byteorder.h"
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

int one_shot_handler( xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context);

#define XBEE_FRAME_HANDLE_GNSS_ONE_SHOT \
    { XBEE_FRAME_GNSS_ONE_SHOT_RESP, 0, one_shot_handler, NULL }


const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_FRAME_HANDLE_GNSS_START_STOP,
    XBEE_FRAME_HANDLE_GNSS_ONE_SHOT,
    XBEE_FRAME_TABLE_END
};


// function that handles received GNSS start stop messages
int gnss_start_stop_handler(xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context)
{
    const xbee_frame_gnss_start_stop_resp_t FAR *gnss = raw;

    switch (gnss->request) {
    case XBEE_GNSS_REQUEST_START_ONE_SHOT:

    	if (gnss->status != XBEE_GNSS_START_STOP_STATUS_SUCCESS) {
    		printf("Got failure response for starting GNSS one shot\n");
    	} else {
    		printf("Got success response for starting GNSS one shot\n");
    	}

    	break;

    case XBEE_GNSS_REQUEST_STOP_ONE_SHOT:

    	if (gnss->status != XBEE_GNSS_START_STOP_STATUS_SUCCESS) {
    		printf("Got failure response for stopping GNSS one shot\n");
    	} else {
    		printf("Got success response for stopping GNSS one shot\n");
    	}

    	break;

    default:
    	printf("Unexpected gnss start stop response type %u\n", gnss->request);
    }

    return 0;
}


// function that handles received one shot messages
int one_shot_handler(xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context)
{
    const xbee_frame_gnss_one_shot_resp_t FAR *gnss = raw;


    switch (gnss->status) {
    case XBEE_GNSS_ONE_SHOT_STATUS_VALID:
    {
        union {
            int32_t i;
            uint32_t u;
        } latitude, longitude;

        printf("\nReceived one shot location:\n");
        latitude.u =  be32toh(gnss->latitude_be);
        longitude.u = be32toh(gnss->longitude_be);
        printf("latitude %i.%06u  longitude %i.%06u\n", latitude.i / 10000000, abs(latitude.i) % 10000000, longitude.i / 10000000, abs(longitude.i) % 10000000);
        unsigned int altitude = be32toh(gnss->altitude_be);
        printf("altitude %u.%03u meters\n", altitude / 1000, altitude % 1000);
        printf("satellites: %u\n", gnss->satellites);
        break;
    }

    case XBEE_GNSS_ONE_SHOT_STATUS_INVALID:
        printf("Received one shot status invalid\n");
        break;

    case XBEE_GNSS_ONE_SHOT_STATUS_TIMEOUT:
        printf("Received one shot status timeout\n");
        break;

    case XBEE_GNSS_ONE_SHOT_STATUS_CANCELED:
        printf("Received one shot status canceled\n");
        break;

    default:
        printf("Received unexpected one shot status %d\n", gnss->status);

        break;
    }

    return 0;
}


int main( int argc, char *argv[])
{
    char term_str[64];
    int status;
    xbee_serial_t xbee_ser;
    int update_one_shot_state = 1;
    int want_one_shot_active = 1;

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

    puts("This sample waits for a GNSS location and displays it");
    puts("You may stop the location request by typing 'stop'");
    puts("You may start a new location request by typing 'start'");
    puts("You may issue AT commands by entering a command prefaced by AT");
    puts("You may exit this program by typing 'quit'");
    puts("");


    while (1)
    {

        if (update_one_shot_state) {

            update_one_shot_state = 0;

            if (want_one_shot_active) {

                status = xbee_gnss_start_one_shot(&my_xbee, 300, 0);
                if (status < 0) {
                    printf("Error %d starting one shot location request\n", status);
                } else {
                    printf("Starting one shot location request\n");
                }

            } else {
                status = xbee_gnss_stop_one_shot(&my_xbee, 0);
                if (status < 0) {
                    printf("Error %d stopping one shot location request\n", status);
                } else {
                    printf("Stopping one shot location request\n");
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
            update_one_shot_state = 1;
            want_one_shot_active = 0;
        }
        else if (! strcmpi(term_str, "start")) {
            update_one_shot_state = 1;
            want_one_shot_active = 1;
        }

    }
}
