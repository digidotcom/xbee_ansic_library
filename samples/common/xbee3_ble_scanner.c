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
    Sample: XBee 3 BLE Scanner

    Tool used for manually performing BLE GAP scans to explore the
    xbee/xbee_ble.c APIs.  For use with XBee 3 BLU.
*/

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/ble.h"

#include "_xbee_term.h"  


typedef enum ble_scan_state_e {
    SCAN_STATE_INIT,
    SCAN_STATE_AWAIT_START,
    SCAN_STATE_RUNNING,
    SCAN_STATE_STOPPED,
    SCAN_STATE_ERROR,
} ble_scan_state_t;

static ble_scan_state_t scan_state;

#define XBEE_BLE_FRAME_HANDLERS \
    { XBEE_FRAME_BLE_SCAN_STATUS, 0, scan_status_handler, NULL}, \
    { XBEE_FRAME_BLE_SCAN_LEGACY_ADVERT_RESP, 0, xbee_ble_scan_legacy_advert_dump, NULL}, \
    { XBEE_FRAME_BLE_SCAN_EXT_ADVERT_RESP, 0, xbee_ble_scan_ext_advert_dump, NULL}

#define BLE_SCAN_INTERVAL_DEFAULT_USEC (0x138800) // 1.28 seconds
#define BLE_SCAN_WINDOW_DEFAULT_USEC   (0x2BF2)   // 11.25 milliseconds

static int scan_status_handler(xbee_dev_t *xbee, const void FAR *raw,
                               uint16_t length, void FAR *context)
{
    const xbee_frame_ble_scan_status_t FAR *status = raw;

    switch (status->scan_status) {
    case XBEE_BLE_SCAN_STATUS_SUCCESS:
        /* Intentional fall-through */
    case XBEE_BLE_SCAN_STATUS_RUNNING:
        scan_state = SCAN_STATE_RUNNING;
        fprintf(stderr, "Scan is running\n\n");
        break;

    case XBEE_BLE_SCAN_STATUS_STOPPED:
        scan_state = SCAN_STATE_STOPPED;
        fprintf(stderr, "Scan has stopped\n");
        break;

    case XBEE_BLE_SCAN_STATUS_ERROR:
        fprintf(stderr, "The scan failed to start\n");
        scan_state = SCAN_STATE_ERROR;
        break;
    case XBEE_BLE_SCAN_STATUS_INVALID_PARAMS:
        fprintf(stderr, "The scan failed to start due to invalid parameters\n");
        scan_state = SCAN_STATE_ERROR;
        break;
    default:
        fprintf(stderr, "An unknown error occoured: %" PRIu8"\n",
                status->scan_status);
        scan_state = SCAN_STATE_ERROR;
        break;
   };

    return 0;
}

int ble_scanner_sample(xbee_dev_t *xbee, uint16_t scan_duration,
                       uint32_t scan_window, uint32_t scan_interval,
                       const char *filter_local_name)
{
    int result;
    const uint8_t filter_type = filter_local_name ? XBEE_BLE_SCAN_FILTER_NAME : 0;
    const size_t filter_len = filter_local_name ? strlen(filter_local_name) : 0;
    scan_state = SCAN_STATE_INIT;

    while (scan_state < SCAN_STATE_STOPPED) {
        result = xbee_dev_tick(xbee);
        if (result < 0) {
            fprintf(stderr, "xbee_dev_tick failed with %d\n", result);
            return EXIT_FAILURE;
        }

        if (scan_state == SCAN_STATE_INIT) {
            // Send scan initiation
            fprintf(stderr, "Initiating scan\n");
            result = xbee_ble_scan_start(xbee, scan_duration, scan_window,
                        scan_interval, filter_type,
                        filter_local_name, filter_len);
            if (result < 0) {
                fprintf(stderr, "scan start failed with %d\n", result);
                return EXIT_FAILURE;
            }
            fprintf(stderr, "Scan started with result %d\n", result);
            scan_state = SCAN_STATE_AWAIT_START;
        }
    }

    if (scan_state == SCAN_STATE_ERROR) {
        printf("There was a failure during the scan\n");
        result = EXIT_FAILURE;
    }

    return result;
}


void print_help(void)
{
    puts("XBee BLE Scanner");
    puts("Usage: xbee3_ble_scan [options] [duration]");
    puts("");
    puts("  -b baudrate           Baudrate for XBee interface (default 115200)");
    puts("  -x serialport         Serial port for XBee interface");
    puts("                        ('COMxx' for Win32, '/dev/ttySxx' for POSIX)");
    puts("  -f filter_name        Local name to filter advertisements by");
    puts("  -i scan_interval      The scan interval in microseconds");
    puts("  -w scan_window        The scan window in microseconds");
    puts("  -h                    Display this help screen");
    puts("");
}

// returns ULONG_MAX if unable to completely parse <str> or <str> is empty
unsigned long parse_num(const char *str)
{
    char *endptr;
    unsigned long parsed_num;

    if (!str || *str == '\0') {
        return ULONG_MAX;
    }
    parsed_num = strtoul(str, &endptr, 0);

    return *endptr != '\0' ? ULONG_MAX : parsed_num;
}

int main(int argc, char *argv[])
{
    xbee_dev_t xbee_dev;
    int c;
    unsigned long param;
    char *filter_name = NULL;
    uint32_t scan_window = BLE_SCAN_WINDOW_DEFAULT_USEC;
    uint32_t scan_interval = BLE_SCAN_INTERVAL_DEFAULT_USEC;
    uint16_t scan_duration = 0;

    // parsed command-line options with default values
    xbee_serial_t xbee_serial = { .baudrate = 115200 };
    int parse_error = 0;

    while ( (c = getopt(argc, argv, "b:hf:x:i:w:")) != -1) {
        switch (c) {
        case 'b': {
            unsigned long param = parse_num(optarg);
            if (param > 0 && param <= 1000000) {
                xbee_serial.baudrate = (uint32_t)param;
            } else {
                fprintf(stderr, "ERROR: failed to parse baudrate '%s'\n",
                        optarg);
                ++parse_error;
            }
            break;
        }

        case '?':
            ++parse_error;
            // fall through to printing help
        case 'h':
            print_help();
            break;

        case 'x':
#ifdef POSIX
            snprintf(xbee_serial.device, sizeof xbee_serial.device,
                    "%s", optarg);
#else // assume Win32
            if (memcmp(optarg, "COM", 3) == 0) {
                unsigned long param = parse_num(&optarg[3]);
                if (param > 0 && param < 256) {
                    xbee_serial.comport = (int)param;
                    break;
                }
           
            }
            fprintf(stderr,
                    "ERROR: expected 'COM<n>' (1 to 255) for serial port\n");
            ++parse_error;
#endif
            break;
        case 'f':
            filter_name = optarg;
            break;

        case 'i':
            param = parse_num(optarg);
            if (param >= XBEE_BLE_SCAN_VALUES_MIN_USEC && 
                param <= XBEE_BLE_SCAN_VALUES_MAX_USEC) {
                scan_interval = (uint32_t)param;
            } else {
                fprintf(stderr, "ERROR: failed to parse scan interval '%s'\n",
                        optarg);
                ++parse_error;
            }
            break;

        case 'w':
            param = parse_num(optarg);
            if (param >= XBEE_BLE_SCAN_VALUES_MIN_USEC && 
                param <= XBEE_BLE_SCAN_VALUES_MAX_USEC) {
                scan_window = (uint32_t)param;
            } else {
                fprintf(stderr, "ERROR: failed to parse scan window '%s'\n",
                        optarg);
                ++parse_error;
            }
            break;

        default:
            fprintf(stderr, "ERROR: unknown option '%c'\n", c);
            ++parse_error;
        }
    }

    if (optind + 1 > argc) {
        fprintf(stderr, "ERROR: insufficient arguments\n");
        ++parse_error;
    } else {
        param = parse_num(argv[optind]);

        if (param >= 0 && param <= UINT16_MAX) {
            scan_duration = (uint16_t) param;
        } else {
            fprintf(stderr,
                    "ERROR: failed to parse scan duration '%s'\n",
                    argv[optind]);
            ++parse_error;
        }
        ++optind;
    }

    if (!parse_error && optind != argc) {
        fprintf(stderr, "ERROR: %u unexpected argument(s)\n", argc - optind);
        ++parse_error;
    }

    if (parse_error) {
       return EXIT_FAILURE;
    }

#ifdef POSIX
    fprintf(stderr, "Opening %s at %u baud...\n",
            xbee_serial.device, xbee_serial.baudrate);
#else
    fprintf(stderr, "Opening COM%u at %u baud...\n",
            xbee_serial.comport, xbee_serial.baudrate);
#endif

    if (xbee_dev_init(&xbee_dev, &xbee_serial, NULL, NULL)) {
        fprintf(stderr, "ERROR: Failed to initialize serial connection.\n");
        return EXIT_FAILURE;
    }

    xbee_term_console_init();
    xbee_term_set_color(SOURCE_STATUS);

    int result = ble_scanner_sample(&xbee_dev, scan_duration, scan_window,
                                    scan_interval,  filter_name);

    xbee_term_console_restore();

    fprintf(stderr, "Closing serial port...\n");
    xbee_ser_close(&xbee_serial);

    return result;
}

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_BLE_FRAME_HANDLERS,
    XBEE_FRAME_TABLE_END
};

