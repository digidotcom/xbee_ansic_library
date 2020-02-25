/*
 * Copyright (c) 2017 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/*
 * Sample program demonstrating communication with the xbee via sms
 *
 * Usage: ./sms_client [SERIAL...]
 *
 * The program will read lines from stdin, terminated by a platform-specific
 * sequence, which will be stripped. For each line, if the line starts with the
 * letters "AT", ignoring differences in case, then the line will be sent to the
 * XBee as an AT command. The user must first send an sms to the XBee to link
 * a destination phone number in order to send lines from stdin to a phone.
 * You may also type 'quit' to exit this program.
 */
#include <stdio.h>
#include <string.h>

#include "xbee/atcmd.h"
#include "xbee/device.h"
#include "xbee/sms.h"

// Common code
#include "_atinter.h"
#include "parse_serial_args.h"

// An instance required for the local XBee
xbee_dev_t my_xbee;
char destination_pn[XBEE_SMS_MAX_PHONE_LENGTH + 1] = {}; // Null terminated


int receive_handler( xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context);

#define XBEE_SMS_RECEIVE_HANDLE     \
    { XBEE_FRAME_RECEIVE_SMS, 0, receive_handler, NULL }

const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_SMS_RECEIVE_HANDLE,
    XBEE_FRAME_TABLE_END
};


// function that handles received sms messages
int receive_handler(xbee_dev_t *xbee, const void FAR *raw,
    uint16_t length, void FAR *context)
{
    const xbee_frame_receive_sms_t FAR *sms = raw;

    printf("Message from %.*s:\n%.*s\n",
        XBEE_SMS_MAX_PHONE_LENGTH,
        sms->phone,
        (int) (length - offsetof(xbee_frame_receive_sms_t, message)),
        sms->message);

    // copy the phone number as new destination
    memcpy(destination_pn, sms->phone, XBEE_SMS_MAX_PHONE_LENGTH);
    destination_pn[XBEE_SMS_MAX_PHONE_LENGTH] = '\0';
    return 0;
}


int main( int argc, char *argv[])
{
    // Enough to get a useful error when the message is too long:
    char term_str[XBEE_SMS_MAX_MESSAGE_LENGTH + 2];
    int status;
    xbee_serial_t xbee_ser;

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

    puts("This sample waits for a text message to be sent to the XBee and displays it");
    puts("You may respond to a received message by typing a response");
    puts("You may issue AT commands by entering a command prefaced by AT");
    puts("You may exit this program by typing 'quit'");


    while (1)
    {
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
        else if (destination_pn[0] == 0) {
            puts("Still waiting to receive a text message to reply to...");
        }
        else if (term_str[0] == 0) {
            puts("");
        }
        else {

            printf("Sending text to %s:\n%s\n", destination_pn, term_str);
            status = xbee_sms_send(&my_xbee, destination_pn, term_str, 0);

            if (status == -E2BIG) {
                puts("That message was too long\n");
            }
            else if (status == 0) {
                puts("Message sent successfully\n");
            }
        }


    }
}
