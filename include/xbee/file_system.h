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
    @defgroup xbee_file_system Frames: File System (0x3B, 0x3C, 0xBB, 0xBC)

    Support of Local/Remote File System Request/Response frames on
    XBee/XBee 3 Cellular (firmware x16 and later) and XBee 3 
    802.15.4/DigiMesh/Zigbee (firmware x00D and later).

    @ingroup xbee_frame
    @{
    @file xbee/file_system.h
*/

#ifndef XBEE_FILE_SYSTEM_H
#define XBEE_FILE_SYSTEM_H

#include "xbee/platform.h"
#include "xbee/device.h"
#include "wpan/types.h"

XBEE_BEGIN_DECLS

/// File System Request frame type
#define XBEE_FRAME_FILE_SYSTEM_REQ              0x3B

/** Frame format for the header of File System Request frame. */
typedef XBEE_PACKED(xbee_header_file_system_req_t, {
    /** Frame type, set to #XBEE_FRAME_FILE_SYSTEM_REQ (0x3B) */
    uint8_t     frame_type;

    uint8_t     frame_id;       ///< identifier to match response frame
}) xbee_header_file_system_req_t;

/// Remote File System Request frame type (on XBee3 802.15.4/DigiMesh/Zigbee)
#define XBEE_FRAME_REMOTE_FS_REQ                0x3C

/** Frame format for the header of Remote File System Request frame. */
typedef XBEE_PACKED(xbee_header_remote_fs_req_t, {
    /** Frame type, set to #XBEE_FRAME_REMOTE_FS_REQ (0x3C) */
    uint8_t     frame_type;

    uint8_t     frame_id;       ///< identifier to match response frame

    addr64      remote_addr_be; ///< 64-bit address of remote device

    uint8_t     options;        ///< combination of XBEE_TX_OPT_* macros
}) xbee_header_remote_fs_req_t;


#define XBEE_FS_CMD_ID_FILE_OPEN                0x01
#define XBEE_FS_CMD_ID_FILE_CLOSE               0x02
#define XBEE_FS_CMD_ID_FILE_READ                0x03
#define XBEE_FS_CMD_ID_FILE_WRITE               0x04
#define XBEE_FS_CMD_ID_FILE_HASH                0x08

#define XBEE_FS_CMD_ID_DIR_CREATE               0x10
#define XBEE_FS_CMD_ID_DIR_OPEN                 0x11
#define XBEE_FS_CMD_ID_DIR_CLOSE                0x12
#define XBEE_FS_CMD_ID_DIR_READ                 0x13
#define XBEE_FS_CMD_ID_GET_PATH_ID              0x1C

#define XBEE_FS_CMD_ID_RENAME                   0x21
#define XBEE_FS_CMD_ID_DELETE                   0x2F

#define XBEE_FS_CMD_ID_VOL_STAT                 0x40
#define XBEE_FS_CMD_ID_VOL_FORMAT               0x4F

#define XBEE_FS_OPT_CREATE         (1<<0)  // create if file/dir doesn't exist
#define XBEE_FS_OPT_EXCLUSIVE      (1<<1)  // error out if file/dir exists
#define XBEE_FS_OPT_READ           (1<<2)  // open file for reading
#define XBEE_FS_OPT_WRITE          (1<<3)  // open file for writing
#define XBEE_FS_OPT_TRUNCATE       (1<<4)  // truncate file to 0 bytes
#define XBEE_FS_OPT_APPEND         (1<<5)  // append to end of file
#define XBEE_FS_OPT_SECURE         (1<<7)  // create a secure file


/** Format for the payload of File System Request with path_id and name:
    - XBEE_FS_CMD_ID_DELETE
    - XBEE_FS_CMD_ID_FILE_HASH
    - XBEE_FS_CMD_ID_DIR_CREATE
    - XBEE_FS_CMD_ID_DIR_OPEN
    - XBEE_FS_CMD_ID_GET_PATH_ID
*/
typedef XBEE_PACKED(xbee_fs_req_path_and_name_t, {
    uint8_t     fs_cmd_id;      ///< XBEE_FS_CMD_ID_xxx

    uint16_t    path_id_be;     ///< path_id or 0x0000 for "/"

    // followed by variable-length file/directory name
}) xbee_fs_req_path_and_name_t;


/** Format for the payload of "File Open" File System Request. */
typedef XBEE_PACKED(xbee_fs_req_file_open_t, {
    uint8_t     fs_cmd_id;      ///< XBEE_FS_CMD_ID_FILE_OPEN

    uint16_t    path_id_be;     ///< path_id or 0x0000 for "/"

    uint8_t     options;        ///< combination of XBEE_FS_OPT_xxx bitmasks

    // followed by variable-length file/directory name
}) xbee_fs_req_file_open_t;


/// Value for .offset_be field of File Read/Write to use remote file's current
/// offset.
#define XBEE_FS_OFFSET_CURRENT  0xFFFFFFFF

/// Value for .byte_count_be field of File Read to read maximum number of bytes.
#define XBEE_FS_BYTE_COUNT_MAX  0xFFFF

/** Format for the payload of "File Read" File System Request. */
typedef XBEE_PACKED(xbee_fs_req_file_read_t, {
    uint8_t     fs_cmd_id;      ///< XBEE_FS_CMD_ID_FILE_READ

    uint16_t    file_id_be;     ///< handle to file on XBee module

    uint32_t    offset_be;      ///< offset into file or XBEE_FS_OFFSET_CURRENT

    uint16_t    byte_count_be;  ///< number of bytes to read
}) xbee_fs_req_file_read_t;


/** Format for the payload of "File Write" File System Request. */
typedef XBEE_PACKED(xbee_fs_req_file_write_t, {
    uint8_t     fs_cmd_id;      ///< XBEE_FS_CMD_ID_FILE_WRITE

    uint16_t    file_id_be;     ///< handle to file on XBee module

    uint32_t    offset_be;      ///< offset into file or XBEE_FS_OFFSET_CURRENT

    // followed by variable-length data to write
}) xbee_fs_req_file_write_t;


/** Format for the payload of "File Close" File System Request. */
typedef XBEE_PACKED(xbee_fs_req_file_close_t, {
    uint8_t     fs_cmd_id;      ///< XBEE_FS_CMD_ID_FILE_CLOSE

    uint16_t    file_id_be;     ///< handle to file on XBee module
}) xbee_fs_req_file_close_t;


/** Format for the payload of "Directory Read/Close" File System Requests. */
typedef XBEE_PACKED(xbee_fs_req_dir_read_t, {
    uint8_t     fs_cmd_id;      ///< XBEE_FS_CMD_ID_DIR_READ or _DIR_CLOSE

    /// handle returned in XBEE_FS_CMD_ID_DIR_OPEN response
    uint16_t    dir_handle_be;
}) xbee_fs_req_dir_read_t;


/// File System Response frame type
#define XBEE_FRAME_FILE_SYSTEM_RESP             0xBB

/** Frame format for the header of File System Response frame. */
typedef XBEE_PACKED(xbee_header_file_system_resp_t, {
    /** Frame type, set to #XBEE_FRAME_FILE_SYSTEM_RESP (0xBB) */
    uint8_t     frame_type;

    uint8_t     frame_id;       ///< identifier from request

    uint8_t     fs_cmd_id;      ///< file system command

    uint8_t     status;         ///< success (0x00) or error from executing cmd

    // payload following header depends on value of <fs_cmd_id>
}) xbee_header_file_system_resp_t;

/// Remote File System Response frame type (on XBee3 802.15.4/DigiMesh/Zigbee)
#define XBEE_FRAME_REMOTE_FS_RESP               0xBC

/** Frame format for the header of Remote File System Response frame. */
typedef XBEE_PACKED(xbee_header_remote_fs_resp_t, {
    /** Frame type, set to #XBEE_FRAME_REMOTE_FS_RESP (0xBC) */
    uint8_t     frame_type;

    uint8_t     frame_id;       ///< identifier from request

    addr64      remote_addr_be; ///< 64-bit address of remote device

    uint8_t     options;        ///< combination of XBEE_RX_OPT_* macros

    uint8_t     fs_cmd_id;      ///< file system command

    uint8_t     status;         ///< success (0x00) or error from executing cmd

    // payload following header depends on value of <fs_cmd_id>
}) xbee_header_remote_fs_resp_t;


// values for status of xbee_header_file_system_resp_t
#define XBEE_FS_STATUS_SUCCESS                  0x00
#define XBEE_FS_STATUS_UNKNOWN_ERROR            0x01
#define XBEE_FS_STATUS_INVALID_COMMAND          0x02
#define XBEE_FS_STATUS_INVALID_PARAM            0x03
#define XBEE_FS_STATUS_ACCESS_DENIED            0x50
#define XBEE_FS_STATUS_ALREADY_EXISTS           0x51
#define XBEE_FS_STATUS_DOES_NOT_EXIST           0x52
#define XBEE_FS_STATUS_INVALID_NAME             0x53
#define XBEE_FS_STATUS_IS_DIRECTORY             0x54
#define XBEE_FS_STATUS_DIR_NOT_EMPTY            0x55
#define XBEE_FS_STATUS_EOF                      0x56
#define XBEE_FS_STATUS_HW_FAILURE               0x57
#define XBEE_FS_STATUS_NO_DEVICE                0x58
#define XBEE_FS_STATUS_VOLUME_FULL              0x59
#define XBEE_FS_STATUS_TIMED_OUT                0x5A
#define XBEE_FS_STATUS_BUSY                     0x5B
#define XBEE_FS_STATUS_RESOURCE_FAILURE         0x5C


/** Format for a directory entry in a Directory Read Response. */
typedef XBEE_PACKED(xbee_fs_dir_entry_t, {
    /// File size in lower 24 bits, and entry flags (XBEE_FS_DIR_ENTRY_xxx).
    uint32_t    size_and_flags_be;

    /// Variable-length name
    char        name[1];
}) xbee_payload_fs_dir_entry_t;

#define XBEE_FS_DIR_ENTRY_IS_DIR        (UINT32_C(1)<<31)
#define XBEE_FS_DIR_ENTRY_IS_SECURE     (UINT32_C(1)<<30)
#define XBEE_FS_DIR_ENTRY_IS_LAST       (UINT32_C(1)<<24)
#define XBEE_FS_DIR_ENTRY_FLAG_MASK     UINT32_C(0xFF000000)

/** Format for the payload of File System Response: File Open. */
typedef XBEE_PACKED(xbee_payload_fs_file_open_resp_t, {
    /// identifier used for followup commands (READ, WRITE and CLOSE)
    uint16_t    file_handle_be;

    uint32_t    file_size_be;   ///< current size of file
}) xbee_payload_fs_file_open_resp_t;


/** Format for the payload of File System Response: File Read/File Write. */
typedef XBEE_PACKED(xbee_payload_fs_file_rw_resp_t, {
    uint16_t    file_handle_be; ///< identifier from request

    uint32_t    offset_be;      ///< current offset into file

    // variable-length file contents follow for FILE_READ
    uint8_t     data[1];        ///< data read from file
}) xbee_payload_fs_file_rw_resp_t;


/** Format for the payload of File System Response: Volume Stat/Format. */
typedef XBEE_PACKED(xbee_payload_fs_volume_resp_t, {
    uint32_t    bytes_used_be;  ///< used bytes on volume

    uint32_t    bytes_free_be;  ///< free bytes on volume

    uint32_t    bytes_bad_be;   ///< bad bytes on volume
}) xbee_payload_fs_volume_resp_t;


// Maximum size of a file/directory name (excluding null terminator).
#define XBEE_FS_MAX_PATH_ELEMENT_LEN            64

// XBee file system directory entry extracted from DIR_READ response.
typedef struct {
    uint32_t    filesize;
    uint8_t     name_len;
    uint8_t     is_dir:1;
    uint8_t     is_secure:1;
    uint8_t     is_last_entry:1;
    char        name[XBEE_FS_MAX_PATH_ELEMENT_LEN + 1];
} xbee_fs_dir_entry_t;

// <payload> points to file_size field of response
// Returns number of bytes extracted (<= <length>) or an error (-Exxx)
// if packet is malformed.
int xbee_fs_extract_dir_entry(xbee_fs_dir_entry_t *entry,
                              const uint8_t FAR *payload,
                              int length);

#define xbee_fs_req_send(xbee, header_data, header_len, target) \
    xbee_fs_req_send_data(xbee, NULL, 0, header_data, header_len, target)

#define xbee_fs_req_send_str(xbee, header_data, header_len, var_str, target) \
    xbee_fs_req_send_data(xbee, header_data, header_len, \
                          var_str, strlen(var_str), target)

int xbee_fs_req_send_data(xbee_dev_t *xbee,
                          const void *header_data, uint16_t header_len,
                          const void *payload, uint16_t payload_len,
                          const addr64 FAR *target_ieee);
XBEE_END_DECLS

#endif // XBEE_FILE_SYSTEM_H

///@}
