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
 * Sample program demonstrating communication via TCP over IPv4
 *
 * Usage: ./ipv4_client IP:PORT [SERIAL...]
 * IP is the IPv4 address to connect to in dotted decimal format. PORT is the
 * TCP port number to connect to, expressed in decimal.
 *
 * The program will read lines from stdin, terminated by a platform-specific
 * sequence, which will be stripped. For each line, if the line starts with the
 * letters "AT", ignoring differences in case, then the line will be sent to the
 * XBee as an AT command. Otherwise, the line, followed by CRLF, will be sent to
 * the remote host. Any data received from the remote host will be printed on
 * stdout.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <signal.h>
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/ipv4.h"
#include "xbee/tx_status.h"
#include "parse_serial_args.h"
#include "_xbee_term.h"
#include "_atinter.h"

static volatile sig_atomic_t termflag = 0;
static void sigterm(int sig);

static int receive_handler(xbee_dev_t *xbee, const void FAR *frame,
    uint16_t length, void FAR *context);
static int tx_status_handler(xbee_dev_t *xbee, const void FAR *frame,
    uint16_t length, void FAR *context);

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    {XBEE_FRAME_RECEIVE_IPV4, 0, receive_handler, NULL},
    {XBEE_FRAME_TX_STATUS, 0, tx_status_handler, NULL},
    XBEE_FRAME_TABLE_END
};

static int parseaddr(xbee_ipv4_envelope_t *env, int argc, char **argv)
{
    if (argc < 2)
        return -EINVAL;

    char *arg = argv[1];
    char *colon = strchr(arg, ':');
    if (!colon)
        return -EINVAL;
    *colon = '\0';

    if (xbee_ipv4_aton(arg, &env->remote_addr_be))
        return -EINVAL;

    char *port_end;
    long port = strtol(colon + 1, &port_end, 10);
    if (port < 1 || port > UINT16_MAX || *port_end)
        return -EINVAL;
    env->remote_port = port;

    return 0;
}

int main(int argc, char **argv)
{
    int err;

    xbee_serial_t serial;
    xbee_dev_t xbee;
    xbee_ipv4_envelope_t env;
    char payload[XBEE_IPV4_MAX_PAYLOAD + 1]; // Null terminated

    err = parseaddr(&env, argc, argv);
    if (err) {
        printf("Usage: %s IP:PORT [SERIAL...]\n",
            argc ? argv[0] : "ipv4_client");
        printf("IP is the IPv4 address to connect to in dotted decimal format. PORT is the TCP\n");
        printf("port number to connect to, expressed in decimal.\n");
        printf("\nCould not parse address and port number.\n");
        return EXIT_FAILURE;
    }
    env.xbee = &xbee;
    env.local_port = 0;
    env.protocol = XBEE_IPV4_PROTOCOL_TCP;
    env.options = 0;
    env.payload = payload;

    parse_serial_arguments(argc - 1, argv + 1, &serial);
    err = xbee_dev_init(&xbee, &serial, NULL, NULL);
    if (err) {
        printf("Error initializing device: %"PRIsFAR"\n", strerror(-err));
        return EXIT_FAILURE;
    }

    err = xbee_cmd_init_device(&xbee);
    if (!err)
        err = -EBUSY;
    while (err == -EBUSY) {
        xbee_dev_tick(&xbee);
        err = xbee_cmd_query_status(&xbee);
    }
    if (err) {
        printf("Error initializing AT command layer: %"PRIsFAR"\n",
            strerror(-err));
        return EXIT_FAILURE;
    }

    xbee_dev_dump_settings(&xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    // The signal handler allows us to exit gracefully upon receiving SIGTERM
    // or SIGINT, resetting the terminal color and completing any in-progress
    // transmissions.
    if (signal(SIGTERM, sigterm) == SIG_ERR
            || signal(SIGINT, sigterm) == SIG_ERR) {
        printf("Error setting signal handler\n");
        return EXIT_FAILURE;
    }
    for (;;) {
        do {
            err = xbee_dev_tick(&xbee);
            if (err >= 0) {
                xbee_term_set_color(SOURCE_KEYBOARD);
                err = xbee_readline(payload, sizeof(payload));
                xbee_term_set_color(SOURCE_UNKNOWN);
            }
        } while (!termflag && err == -EAGAIN);
        if (termflag || err == -ENODATA)
            return EXIT_SUCCESS;
        if (err < 0) {
            printf("Error reading input: %"PRIsFAR"\n", strerror(-err));
            return EXIT_FAILURE;
        }

        if (strncmpi(payload, "AT", 2) == 0) {
            process_command(&xbee, payload);
            continue;
        }

        size_t len = strlen(payload);
        if (len > XBEE_IPV4_MAX_PAYLOAD - 2) {
            // No room for CRLF.
            printf("Line too long, ignoring\n");
            continue;
        }
        strcpy(payload + len, "\r\n");
        env.length = len + 2;

        err = xbee_ipv4_envelope_send(&env);
        if (err < 0) {
            printf("Error sending packet: %"PRIsFAR"\n", strerror(-err));
            return EXIT_FAILURE;
        }
    }
}

static int receive_handler(xbee_dev_t *xbee, const void FAR *frame_in,
    uint16_t frame_len, void FAR *context)
{
    const xbee_frame_receive_ipv4_t FAR *frame = frame_in;
    size_t len = frame_len - offsetof(xbee_frame_receive_ipv4_t, payload);
    xbee_term_set_color(SOURCE_SERIAL);
    if (fwrite(frame->payload, len, 1, stdout) != 1) {
        printf("Error writing received data\n");
        exit(EXIT_FAILURE);
    }
    xbee_term_set_color(SOURCE_UNKNOWN);
    return 0;
}

static int tx_status_handler(xbee_dev_t *xbee, const void FAR *frame_in,
    uint16_t frame_len, void FAR *context)
{
    const xbee_frame_tx_status_t FAR *frame = frame_in;
    if (frame->delivery != XBEE_TX_DELIVERY_SUCCESS) {
        printf("Error delivering packet, delivery status 0x%.2x\n",
            frame->delivery);
        exit(EXIT_FAILURE);
    }
    return 0;
}

static void sigterm(int sig)
{
    signal(sig, sigterm);
    termflag = 1;
}
