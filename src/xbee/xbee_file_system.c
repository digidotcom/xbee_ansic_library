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

/**
    @addtogroup xbee_file_system
    @{
    @file xbee_file_system.c
*/

#include "xbee/byteorder.h"
#include "xbee/file_system.h"

/**
    @brief
    Send a File System Request to the local or a remote XBee module.

    @param[in]  xbee            Local device receiving the frame.
    @param[in]  header_data     Header starting with fs_cmd_id field.
    @param[in]  header_len      Number of bytes in \a header_data.
    @param[in]  payload         Payload bytes following header.
    @param[in]  payload_len     Number of bytes in \a payload.
    @param[in]  target_ieee     Address of remote device or NULL for local.

    @retval     >0      frame_id assigned to the request
    @retval     <0      error trying to send
*/
int xbee_fs_req_send_data(xbee_dev_t *xbee,
                          const void *header_data, uint16_t header_len,
                          const void *payload, uint16_t payload_len,
                          const addr64 FAR *target_ieee)
{
    uint8_t buffer[256];
    uint8_t *p = buffer;

    // Build up local or remote request in <buffer>.

    *p++ = target_ieee ? XBEE_FRAME_REMOTE_FS_REQ
                       : XBEE_FRAME_FILE_SYSTEM_REQ;
    *p++ = xbee_next_frame_id(xbee);
    if (target_ieee) {
        memcpy(p, target_ieee, 8);
        p += 8;
        *p++ = 0;       // TX Options
    }

    if (header_len > 0) {
        // make sure there's enough room for the fixed fields
        if (header_len > &buffer[sizeof(buffer)] - p) {
            return -EMSGSIZE;
        }
        memcpy(p, header_data, header_len);
        p += header_len;
    }

    int result = xbee_frame_write(xbee, buffer, p - buffer,
                                  payload, payload_len,
                                  XBEE_WRITE_FLAG_NONE);

    // return Frame ID on success, or negative error otherwise
    return result == 0 ? buffer[1] : result;
}


/**
    @brief
    Send a File System Request to the local or a remote XBee module.

    @param[out] entry           Structure to hold the parsed entry.
    @param[in]  payload         Remaining payload to parse.
    @param[in]  length          Number of bytes remaining in the payload.

    @retval     >0      number of bytes extracted from payload
    @retval     <0      error trying to send
*/
int xbee_fs_extract_dir_entry(xbee_fs_dir_entry_t *entry,
                              const uint8_t FAR *payload,
                              int length)
{
    if (entry == NULL || payload == NULL) {
        return -EINVAL;
    }
    if (length < 4) {
        return -EBADMSG;
    }

    memset(entry, 0, sizeof *entry);

    uint32_t size_and_flags = be32toh(xbee_get_unaligned32(payload));
    payload += 4;
    length -= 4;
    entry->filesize = size_and_flags & ~XBEE_FS_DIR_ENTRY_FLAG_MASK;
    entry->is_dir = (size_and_flags & XBEE_FS_DIR_ENTRY_IS_DIR) != 0;
    entry->is_secure = (size_and_flags & XBEE_FS_DIR_ENTRY_IS_SECURE) != 0;
    entry->is_last_entry = (size_and_flags & XBEE_FS_DIR_ENTRY_IS_LAST) != 0;

    // look for a null terminator before end of payload
    const uint8_t FAR *null_term = memchr(payload, '\0', length);
    if (null_term == NULL) {
        entry->name_len = length;
    } else {
        entry->name_len = null_term - payload;
        // Include the null terminator in the count of parsed bytes, so the
        // caller will skip over it before parsing the next payload entry.
        length = entry->name_len + 1;
    }
    memcpy(entry->name, payload, entry->name_len);

    return 4 + length;      // parsed <size_and_flags> & a <length>-byte name
}


/** @} */
