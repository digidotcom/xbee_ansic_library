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

/**
    @addtogroup xbee_firmware
    @{
    @file xbee_bl_gen3.c

    Support code for bootloader used on "Gen3" products (including S3B, S6,
    S6B, XLR, Cellular, SX, SX868, S8).  Firmware updates use .ebin files.

    For Cellular products, all XBee Cellular modules (Cat 1 Verizon and 3G
    Global) use this bootloader.  XBee3 Cellular modules (Cat 1 AT&T and
    LTE-M/NB-IoT) with firmware versions (ATVR values) ending in 0C or lower
    do as well.

    After installing firmware *0F (1140F or 3100F) on an XBee3 Cellular module,
    it will update the hardware to use the "Gecko bootloader" and .gbl files
    for further firmware updates.  See xbee/firmware.h and xbee_firmware.c
    for .gbl and Gecko bootloader support.

    Define XBEE_BL_GEN3_VERBOSE for debugging messages.
*/

#include <stdio.h>
#include <stdlib.h>

#include "xbee/bl_gen3.h"
#include "xbee/byteorder.h"
#include "util/crc16buypass.h"

// Commands sent from host to bootloader.
#define XBEE_GEN3_CMD_BL_VERSION        'B'
#define XBEE_GEN3_CMD_EXTENDED_VER      'V'
#define XBEE_GEN3_CMD_REGION_CODE       'N'
#define XBEE_GEN3_CMD_INITIALIZE        'I'
#define XBEE_GEN3_CMD_PROTOCOL_VER      'L'
#define XBEE_GEN3_CMD_MAX_BAUDRATE      'X'
#define XBEE_GEN3_CMD_NEW_BAUDRATE      'R'
#define XBEE_GEN3_CMD_UPLOAD_PAGE       'P'
#define XBEE_GEN3_CMD_UPLOAD_FINISHED   'F'
#define XBEE_GEN3_CMD_VERIFY_FW         'C'

/// Successful responses from the bootloader end with XBEE_GEN3_RSP_SUCCESS.
#define XBEE_GEN3_RSP_SUCCESS           'U'

/// Generic failure response from bootloader.
#define XBEE_GEN3_RSP_FAILURE           '\x11'

// single-character response to sending an XBEE_GEN3_CMD_UPLOAD_PAGE
#define XBEE_GEN3_RSP_UPLOAD_CRC_ERR    '\x12'  ///< Checksum/CRC invalid
#define XBEE_GEN3_RSP_UPLOAD_VERIFY_ERR '\x13'  ///< flash write failed

// Parameter to XBEE_GEN3_CMD_UPLOAD_PAGE when bootloader protocol version
// (returned from 'L' command) returns 'U'.  Pad last page with 0xFF bytes.
typedef XBEE_PACKED(xbee_gen3_page_ver0_t, {
    uint8_t     command;                ///< XBEE_GEN3_CMD_UPLOAD_PAGE
    uint8_t     page_num;
    uint8_t     page_data[XBEE_GEN3_UPLOAD_PAGE_SIZE];
    uint8_t     checksum;
}) xbee_gen3_page_ver0_t;

// Parameter to XBEE_GEN3_CMD_UPLOAD_PAGE when bootloader protocol version
// (returned from 'L' command) returns '1U'.  On upload, skip any pages
// that contain only 0xFF.  Pad last page with 0xFF bytes.
typedef XBEE_PACKED(xbee_gen3_page_ver1_t, {
    uint8_t     command;                ///< XBEE_GEN3_CMD_UPLOAD_PAGE
    uint16_t    page_num_le;
    uint8_t     page_data[XBEE_GEN3_UPLOAD_PAGE_SIZE];
    uint16_t    crc_le;                 ///< CRC16 with 0x8005 polynomial
}) xbee_gen3_page_ver1_t;


void xbee_gen3_dump_extended_ver(const xbee_gen3_extended_ver_t *ver)
{
    char mac[ADDR64_STRING_LENGTH];

    printf("ATHV:%04X  AT%%R:%04X  AT%%C:%02X  ATHS:%04X  MAC:%s\n",
           le16toh(ver->hw_version_le),
           le16toh(ver->hw_revision_le),
           ver->hw_compatibility,
           le16toh(ver->hw_series_le),
           addr64_format(mac, &ver->mac_address_be));
}


int xbee_bl_gen3_install_init(xbee_dev_t *xbee, xbee_gen3_update_t *source)
{
    if (! (xbee && source)) {
        return -EINVAL;
    }

    // initialize struct elements used in bootloader handshaking
    memset(source, 0, offsetof(xbee_gen3_update_t, xbee));

    source->xbee = xbee;
    source->state = XBEE_GEN3_STATE_INIT;

    return 0;
}


uint16_t xbee_bl_gen3_install_state(xbee_gen3_update_t *source)
{
    return (source->page_num << 6) | source->state;
}


// Helper function to advance to next state and reset timeout timer
static void _next_state(xbee_gen3_update_t *source)
{
    source->timer = xbee_millisecond_timer();
    source->state++;
}


// Helper function to read the next page of data into source->buffer.
// Returns number of bytes read or an error code < 0.
// Sets XBEE_GEN3_BL_FLAG_EOF for incomplete reads.
static int _load_page(xbee_gen3_update_t *source)
{
    int bytes_read;

    while (1) {
        if (source->flags & XBEE_GEN3_BL_FLAG_EOF) {
            return 0;
        }

        bytes_read = source->read(source->context, source->buffer,
                                  XBEE_GEN3_UPLOAD_PAGE_SIZE);

        if (bytes_read < XBEE_GEN3_UPLOAD_PAGE_SIZE) {
            // short read indicates that we're at the end of the file
            source->flags |= XBEE_GEN3_BL_FLAG_EOF;
        }

        if (bytes_read <= 0) {          // error on read or EOF
            break;
        }

        if (memcheck(source->buffer, 0xFF, bytes_read) == 0) {
            // page filled with 0xFF, advance to the next page number
#ifdef XBEE_BL_GEN3_VERBOSE
            printf("%s: skipping page %u, all 0xFF\n", __FUNCTION__,
                   source->page_num);
#endif
            ++source->page_num;
            continue;
        }

        if (source->flags & XBEE_GEN3_BL_FLAG_EOF) {
            // fill remaining bytes of page with 0xFF
            memset(&source->buffer[bytes_read], 0xFF,
                   XBEE_GEN3_UPLOAD_PAGE_SIZE - bytes_read);
        }
        // page contains data to upload, break out of outer loop
        break;
    }

    return bytes_read;
}


static void _send_page(xbee_gen3_update_t *source)
{
    xbee_serial_t *serport = &source->xbee->serport;
    int tx_free = xbee_ser_tx_free(serport);
    if (tx_free == 0) {
        return;
    }

    if (source->page_offset < 0 && tx_free >= 3) {
#ifdef XBEE_BL_GEN3_VERBOSE
        printf("%s: sending page %u\n", __FUNCTION__, source->page_num);
#endif
        xbee_ser_putchar(serport, XBEE_GEN3_CMD_UPLOAD_PAGE);
        xbee_ser_putchar(serport, (uint8_t)source->page_num);
        if (source->flags & XBEE_GEN3_BL_FLAG_PROTO1) {
            // Proto 1 includes high bytes of page num
            xbee_ser_putchar(serport, (uint8_t)(source->page_num >> 8));
        }
        source->page_offset = 0;
    } else if (source->page_offset < XBEE_GEN3_UPLOAD_PAGE_SIZE) {
        int to_send = XBEE_GEN3_UPLOAD_PAGE_SIZE - source->page_offset;
        if (to_send > tx_free) {
            to_send = tx_free;
        }
        xbee_ser_write(serport,
                       &source->buffer[source->page_offset],
                       to_send);
        source->page_offset += to_send;
    } else if (tx_free >= 2) {
        // make sure we don't have any buffered characters
        xbee_ser_rx_flush(serport);

        if (source->flags & XBEE_GEN3_BL_FLAG_PROTO1) {         // CRC16
            uint16_t crc16 = crc16buypass_byte(0, source->buffer,
                                               XBEE_GEN3_UPLOAD_PAGE_SIZE);
            xbee_ser_putchar(serport, (uint8_t)crc16);
            xbee_ser_putchar(serport, (uint8_t)(crc16 >> 8));
        } else {                                                // checksum
            int i;
            uint8_t checksum = 0xFF;

            for (i = 0; i < XBEE_GEN3_UPLOAD_PAGE_SIZE; ++i) {
                checksum -= source->buffer[i];
            }
            xbee_ser_putchar(serport, checksum);
        }
        _next_state(source);
    }
}


#define _TIME_ELAPSED(x)  (xbee_millisecond_timer() - source->timer > (x))
int xbee_bl_gen3_install_tick(xbee_gen3_update_t *source)
{
    int result, ch;
    xbee_dev_t *xbee;
    xbee_serial_t *serport;

    if (! source) {
        return -EINVAL;
    }

    xbee = source->xbee;
    serport = &xbee->serport;

    switch (source->state) {
    case XBEE_GEN3_STATE_INIT:
#ifdef XBEE_BL_GEN3_VERBOSE
        printf("%s: open serial port\n", __FUNCTION__);
#endif

        // Open serial port to bootloader's baud rate.
        xbee_ser_open(serport, 38400);

        // clear RTS
#ifdef XBEE_BL_GEN3_VERBOSE
        printf("%s: RTS off\n", __FUNCTION__);
#endif
        xbee_ser_set_rts(serport, 0);

        // Begin serial break condition by setting Tx line low (space) while
        // resetting XBee module.
#ifdef XBEE_BL_GEN3_VERBOSE
        printf("%s: start serial break\n", __FUNCTION__);
        printf("%s: CTS is %u\n", __FUNCTION__, xbee_ser_get_cts(serport));
#endif
        xbee_ser_break(serport, 1);

        _next_state(source);
        break;

    case XBEE_GEN3_STATE_ENTER_BOOTLOADER:
        // keep break condition for 6000ms, then re-open serial port
        // Watch CTS for indication that we've entered the bootloader.
        if (_TIME_ELAPSED(6000) || xbee_ser_get_cts(serport) == 0) {
#ifdef XBEE_BL_GEN3_VERBOSE
            printf("%s: CTS is %u\n", __FUNCTION__,
                   xbee_ser_get_cts(serport));
#endif

            _next_state(source);
        }
        break;

    case XBEE_GEN3_STATE_GET_BL_VERSION:
        // wait for 100ms from reset for bootloader to be ready
        if (_TIME_ELAPSED(100)) {
#ifdef XBEE_BL_GEN3_VERBOSE
            printf("%s: clear break\n", __FUNCTION__);
#endif
            xbee_ser_break(serport, 0);

            // Flush any buffered data from serial port
            xbee_ser_rx_flush(serport);

            // Request bootloader version
            xbee_ser_putchar(serport, XBEE_GEN3_CMD_BL_VERSION);
            _next_state(source);
        }
        break;

    case XBEE_GEN3_STATE_BL_VERSION:
        if (xbee_ser_rx_used(serport) >= 5) {
            char bl_ver[5];

            xbee_ser_read(serport, bl_ver, 5);
            if (bl_ver[4] == XBEE_GEN3_RSP_SUCCESS) {
                bl_ver[4] = '\0';
                source->bl_version = (uint16_t)strtoul(bl_ver, NULL, 16);
#ifdef XBEE_BL_GEN3_VERBOSE
                printf("%s: bl version 0x%04X\n", __FUNCTION__,
                       source->bl_version);
#endif
            }

            xbee_ser_putchar(serport, XBEE_GEN3_CMD_PROTOCOL_VER);
            _next_state(source);
        }
        break;

    case XBEE_GEN3_STATE_BL_PROTOCOL:
        ch = xbee_ser_getchar(serport);
        if (ch == '1') {
            source->flags |= XBEE_GEN3_BL_FLAG_PROTO1;
        } else if (ch == XBEE_GEN3_RSP_SUCCESS) {
#if XBEE_SERIAL_MAX_BAUDRATE >= 921600
            // only send MAX_BAUDRATE command if supported and we can handle a
            // possible 921600 response
            if (source->flags & XBEE_GEN3_BL_FLAG_PROTO1) {
#ifdef XBEE_BL_GEN3_VERBOSE
                printf("%s: requesting %s\n", __FUNCTION__, "max baudrate");
#endif
                // Request maximum baud rate (up to 921600)
                xbee_ser_putchar(serport, XBEE_GEN3_CMD_MAX_BAUDRATE);
            } else
#endif // XBEE_SERIAL_MAX_BAUDRATE >= 921600
            {
#ifdef XBEE_BL_GEN3_VERBOSE
                printf("%s: requesting %s\n", __FUNCTION__, "115200");
#endif
                // Request increase in baud rate (from 38400 to 115200)
                xbee_ser_putchar(serport, XBEE_GEN3_CMD_NEW_BAUDRATE);
            }
            source->page_offset = 0;
            _next_state(source);
        }
        break;

    case XBEE_GEN3_STATE_BAUD_RATE:
        // Bootloader responses: 38400U, 115200U, 230400U, 460800U or 921600U.
        ch = xbee_ser_getchar(serport);
        if (ch == XBEE_GEN3_RSP_SUCCESS) {
            // source->buffer should contain the new baudrate
            source->buffer[source->page_offset] = '\0';
#ifdef XBEE_BL_GEN3_VERBOSE
            printf("%s: increase baud rate to %s\n", __FUNCTION__,
                   source->buffer);
#endif
            xbee_ser_baudrate(serport, (uint32_t)strtoul(source->buffer,
                                                         NULL, 10));
            xbee_ser_putchar(serport, XBEE_GEN3_CMD_EXTENDED_VER);
            _next_state(source);
        } else if (ch != -ENODATA && source->page_offset < 10) {
            source->buffer[source->page_offset++] = ch;
        }
        break;

    case XBEE_GEN3_STATE_EXT_VERSION:
        if (xbee_ser_rx_used(serport) >= sizeof(xbee_gen3_extended_ver_t)) {
            xbee_ser_read(serport, &source->ext_ver,
                          sizeof(xbee_gen3_extended_ver_t));
#ifdef XBEE_BL_GEN3_VERBOSE
            xbee_gen3_dump_extended_ver(&source->ext_ver);
#endif
            if ((source->flags & XBEE_GEN3_BL_FLAG_QUERY_ONLY)
                || source->read == NULL)
            {
                // Caller just wanted to enter bootloader and query it.  Some
                // bootloader versions will time out and automatically launch
                // the application, others are stuck unless we send a command
                // to run the app.
                xbee_ser_putchar(serport, XBEE_GEN3_CMD_VERIFY_FW);

                source->state = XBEE_GEN3_STATE_SUCCESS;
                return 0;
            }
            xbee_ser_putchar(serport, XBEE_GEN3_CMD_INITIALIZE);
            _next_state(source);
        }
        break;

    case XBEE_GEN3_STATE_START_TRANSFER:
        if (xbee_ser_getchar(serport) == XBEE_GEN3_RSP_SUCCESS) {
            result = _load_page(source);        // load first page
            if (result < 0) {
                source->state = XBEE_GEN3_STATE_FAILURE;
                return result;
            }
            source->page_offset = -1;           // send header
            _next_state(source);
        }
        break;

    case XBEE_GEN3_STATE_SEND_PAGE:
        _send_page(source);
        break;

    case XBEE_GEN3_STATE_LOAD_PAGE:
        switch (xbee_ser_getchar(serport)) {
        case -ENODATA:                  // still waiting on response
        default:                        // some unexpected character
            break;
        case XBEE_GEN3_RSP_SUCCESS:
            // load the next page and send it
            source->flags &= ~XBEE_GEN3_BL_FLAG_RETRY_MASK;
            ++source->page_num;
            result = _load_page(source);
            if (result < 0) {
#ifdef XBEE_BL_GEN3_VERBOSE
                printf("%s: abort upload, read() returned %d\n",
                       __FUNCTION__, result);
#endif
                source->state = XBEE_GEN3_STATE_FAILURE;
                return result;
            }
            if (result == 0) {
#ifdef XBEE_BL_GEN3_VERBOSE
                printf("%s: upload complete, start verify\n", __FUNCTION__);
#endif
                xbee_ser_putchar(serport, XBEE_GEN3_CMD_VERIFY_FW);
                _next_state(source);
                return -EAGAIN;
            }
            // fall through to common code for [re-]sending loaded page
        case XBEE_GEN3_RSP_UPLOAD_CRC_ERR:
            if ((++source->flags & XBEE_GEN3_BL_FLAG_RETRY_MASK) > 3) {
#ifdef XBEE_BL_GEN3_VERBOSE
                printf("%s: abort transfer on page %u\n", __FUNCTION__,
                       source->page_num);
#endif
                source->state = XBEE_GEN3_STATE_FAILURE;
                return -ECANCELED;
            }
            source->page_offset = -1;           // send header
            source->state = XBEE_GEN3_STATE_SEND_PAGE;
            source->timer = xbee_millisecond_timer();
            break;

        case XBEE_GEN3_RSP_UPLOAD_VERIFY_ERR:
            // flash write failed; abort the upload (need to reinitialize flash)
            source->state = XBEE_GEN3_STATE_FAILURE;
            return -ECANCELED;
        }
        break;

    case XBEE_GEN3_STATE_VERIFY:        // waiting on response to 'C'
        ch = xbee_ser_getchar(serport);
        if (ch == XBEE_GEN3_RSP_FAILURE) {
#ifdef XBEE_BL_GEN3_VERBOSE
            printf("%s: verification failed\n", __FUNCTION__);
#endif
            source->state = XBEE_GEN3_STATE_FAILURE;
            return -EILSEQ;
        }
        if (ch != XBEE_GEN3_RSP_SUCCESS) {
            break;
        }
#ifdef XBEE_BL_GEN3_VERBOSE
        printf("%s: upload successful\n", __FUNCTION__);
#endif
        source->state = XBEE_GEN3_STATE_SUCCESS;
        // fall through to SUCCESS state
    case XBEE_GEN3_STATE_SUCCESS:
        return 0;

    case XBEE_GEN3_STATE_FAILURE:
        return -EIO;

    default:
#ifdef XBEE_BL_GEN3_VERBOSE
        printf( "%s: invalid state %u\n", __FUNCTION__, source->state);
#endif
        return -EBADMSG;
    }

    // any state that hasn't already returned uses this shared timeout feature
    if (_TIME_ELAPSED(XBEE_GEN3_FW_LOAD_TIMEOUT_MS)) {
#ifdef XBEE_BL_GEN3_VERBOSE
        printf( "%s: timeout\n", __FUNCTION__);
#endif

        source->state = XBEE_GEN3_STATE_FAILURE;
        return -ETIMEDOUT;
    }

    return -EAGAIN;
}
#undef _TIME_ELAPSED


const char *xbee_bl_gen3_install_status(xbee_gen3_update_t *source,
                                        char buffer[XBEE_GEN3_STATUS_BUF_SIZE])
{
    if (! source || ! source->xbee) {
        return "Invalid source.";
    }

    switch (source->state) {
    case XBEE_GEN3_STATE_INIT:               // hold TX in break
        return "Starting update.";

    case XBEE_GEN3_STATE_ENTER_BOOTLOADER:   // wait for bootloader (drops CTS)
        return "Generating serial break on TX.";

    case XBEE_GEN3_STATE_GET_BL_VERSION:     // wait until bootloader ready, send 'B'
        return "Entered bootloader.";

    case XBEE_GEN3_STATE_BL_VERSION:         // parse 'B' response, send 'L'
        return "Getting bootloader version.";

    case XBEE_GEN3_STATE_BL_PROTOCOL:        // parse 'L' response, send 'R'/'X'
        sprintf(buffer, "BL ver 0x%04X.", source->bl_version);
        return buffer;

    case XBEE_GEN3_STATE_BAUD_RATE:          // parse 'R'/'X', change baud, send 'V'
        sprintf(buffer, "BL protocol %u.",
                (source->flags & XBEE_GEN3_BL_FLAG_PROTO1) ? 1 : 0);
        return buffer;

    case XBEE_GEN3_STATE_EXT_VERSION:        // parse 'V' response, send 'I'
        sprintf(buffer, "Switched to %" PRIu32 " baud.",
                source->xbee->serport.baudrate);
        return buffer;

    case XBEE_GEN3_STATE_START_TRANSFER:     // parse 'I' response, start transfer
        return "Retrieved extended version.";

    case XBEE_GEN3_STATE_SEND_PAGE:          // send the current page
        sprintf(buffer, "Sending page %u.", source->page_num);
        return buffer;

    case XBEE_GEN3_STATE_LOAD_PAGE:          // on success, load next page or verify
        sprintf(buffer, "Loading page %u.", source->page_num);
        return buffer;

    case XBEE_GEN3_STATE_VERIFY:             // wait for response to 'C'
        return "Waiting for BL to verify firmware.";

    case XBEE_GEN3_STATE_SUCCESS:
        return "Install successful.";

    case XBEE_GEN3_STATE_FAILURE:
        return "Install failed.";
    }

    sprintf(buffer, "ERROR: unknown state %u.", source->state);
    return buffer;
}

///@}
