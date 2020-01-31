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
    @file xbee/bl_gen3.h
    Code to interface with the "Gen3" XBee bootloader.

    See src/xbee/xbee_bl_gen3.c for additional documentation.
*/

#ifndef XBEE_BL_GEN3_H
#define XBEE_BL_GEN3_H

#include "xbee/device.h"
#include "wpan/types.h"

XBEE_BEGIN_DECLS

typedef enum xbee_gen3_state {
    XBEE_GEN3_STATE_INIT,               // hold TX in break
    XBEE_GEN3_STATE_ENTER_BOOTLOADER,   // wait for bootloader (drops CTS)
    XBEE_GEN3_STATE_GET_BL_VERSION,     // wait until bootloader ready, send 'B'
    XBEE_GEN3_STATE_BL_VERSION,         // parse 'B' response, send 'L'
    XBEE_GEN3_STATE_BL_PROTOCOL,        // parse 'L' response, send 'R'/'X'
    XBEE_GEN3_STATE_BAUD_RATE,          // parse 'R'/'X', change baud, send 'V'
    XBEE_GEN3_STATE_EXT_VERSION,        // parse 'V' response, send 'I'
    XBEE_GEN3_STATE_START_TRANSFER,     // parse 'I' response, start transfer
    XBEE_GEN3_STATE_SEND_PAGE,          // send the current page
    XBEE_GEN3_STATE_LOAD_PAGE,          // on success, load next page or verify
    XBEE_GEN3_STATE_VERIFY,             // wait for response to 'C'

    XBEE_GEN3_STATE_SUCCESS,
    XBEE_GEN3_STATE_FAILURE
} xbee_gen3_state_t;

#define XBEE_GEN3_UPLOAD_PAGE_SIZE      512

// Response to a XBEE_GEN3_CMD_EXTENDED_VER command.
typedef XBEE_PACKED(xbee_gen3_extended_ver_t, {
    uint16_t            hw_version_le;          // ATHV
    uint16_t            hw_revision_le;         // AT%R
    uint8_t             hw_compatibility;       // AT%C
    uint8_t             unused;
    uint16_t            hw_series_le;           // ATHS
    addr64              mac_address_be;
    uint8_t             end_of_response;        // always 'U'
}) xbee_gen3_extended_ver_t;

void xbee_gen3_dump_extended_ver(const xbee_gen3_extended_ver_t *ver);

typedef struct xbee_gen3_update_t {
    uint32_t                    timer;
    xbee_gen3_state_t           state;
    xbee_gen3_state_t           next_state;
    uint16_t                    flags;
#define XBEE_GEN3_BL_FLAG_RETRY_MASK    (0xFF)  ///< count retries on upload
#define XBEE_GEN3_BL_FLAG_QUERY_ONLY    (1<<8)  ///< don't upload firmware
#define XBEE_GEN3_BL_FLAG_PROTO1        (1<<9)  ///< upload uses CRC16
#define XBEE_GEN3_BL_FLAG_EOF           (1<<10) ///< reached end of fw image

    uint16_t                    bl_version;     ///< parsed response from 'B' cmd
    uint16_t                    page_num;       ///< page we're sending to BL

    /// Current offset into buffer (0 - <n-1>); -1 if sending page header;
    /// <n> if sending checksum/CRC16 (where <n> = XBEE_GEN3_UPLOAD_PAGE_SIZE).
    int16_t                     page_offset;

    xbee_gen3_extended_ver_t    ext_ver;        ///< parsed response from 'V' cmd

    xbee_dev_t                  *xbee;

    /// buffer for upload page, must persist for duration of update
    char                        buffer[XBEE_GEN3_UPLOAD_PAGE_SIZE];

    /// Function to read firmware image contents.  Considered EOF if it returns
    /// a value less than <bytes>.  Values <0 are errors passed up from
    /// xbee_fw_install_ebin_tick().
    int (*read)(void *context, void *buffer, int16_t bytes);

    /// context passed to .read() function
    void *context;
} xbee_gen3_update_t;

#define XBEE_GEN3_FW_LOAD_TIMEOUT_MS    10000

/**
    @brief
    Prepare to install new firmware on an attached XBee module.

    @param[in]  xbee    XBee device to install firmware on.  Must have been
                        set up with xbee_dev_init().

    @param[in]  source  Structure with function pointer for reading contents
                        of new firmware image.

@code
        // Function prototype for function that will provide firmware
        // bytes when called by xbee_bl_gen3_install_tick().
        int my_firmware_read(void *context, void *buffer, int16_t bytes);

        xbee_gen3_update_t      fw;
        xbee_dev_t              xbee;

        xbee_dev_init(&xbee, ...);
        xbee_fw_install_init(&xbee, &fw);

        fw.read = &my_firmware_read;
        fw.context = NULL;
@endcode

    @retval     0           success
    @retval     -EINVAL     NULL parameter passed to function

*/
int xbee_bl_gen3_install_init(xbee_dev_t *xbee, xbee_gen3_update_t *source);

/**
    @brief
    Return a unique value identifying the install state.  Used by caller to
    identify state changes.

    @param[in]  source      Structure used to track install status.

    @return     Value identifying a unique install state.

    @sa xbee_bl_gen3_install_status
*/
uint16_t xbee_bl_gen3_install_state(xbee_gen3_update_t *source);

/**
    @brief
    Install the firmware image stored in \a source.

    You must call xbee_bl_gen3_install_init() on the source before calling this
    tick function.

    If successful, XBee module with boot into the new firmware image, which is
    typically at 9600 baud and "transparent" mode (ATAP=0).

    @param[in]  source      Structure used to track install status.

    @retval     0           Successfully installed new firmware.
    @retval     -EAGAIN     Firmware installation in progress (incomplete).
    @retval     -EINVAL     NULL \a source.
    @retval     -EBADMSG    Invalid state for firmware update process.
    @retval     -ECANCELED  Transfer aborted after three CRC failures on a page
                            or module reported a flash write error.
    @retval     -EILSEQ     Bootloader verification of firmware failed.
    @retval     -EIO        Couldn't establish communications with XBee module.

*/
int xbee_bl_gen3_install_tick(xbee_gen3_update_t *source);

/// Minimum size of buffer passed to xbee_bl_gen3_install_status()
#define XBEE_GEN3_STATUS_BUF_SIZE       60

/**
    @brief
    Return a string describing the current state of the firmware update.  Can
    either return a fixed string, or a dynamic string copied to \a buffer.

    @param[in]  source  State variable to generate status string for.
    @param[in]  buffer  Buffer (at least XBEE_GEN3_STATUS_BUF_SIZE bytes) to
                        receive dynamic status string.

    @return     Returns \a buffer or a pointer to a fixed status string.

    @sa xbee_bl_gen3_install_state
*/
const char *xbee_bl_gen3_install_status(xbee_gen3_update_t *source,
                                        char buffer[XBEE_GEN3_STATUS_BUF_SIZE]);

XBEE_END_DECLS

#endif // XBEE_BL_GEN3_H

///@}
