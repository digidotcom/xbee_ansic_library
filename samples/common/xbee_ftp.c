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

/*
    Sample: xbee_ftp

    Tool to access the file system on local and remote (XBee3 802.15.4,
    DigiMesh, and Zigbee) XBee modules.  Requires modules with ATFS commands
    and updated firmware supporting frame types 0x3B, 0x3C, 0xBB and 0xBC.

    Supported Firmware:
        XBee 3 Zigbee   version 100D and later
        XBee 3 802.15.4 version 200D and later
        XBee 3 DigiMesh version 300D and later
        XBee Cellular   version x16 and later
        XBee 3 Cellular version x16 and later

    On the local node, set:
        ATBD 7          Default baud rate for all samples (115,200).
        ATAP 1          Use regular API mode (non-escaped).

    For non-Cellular products (Zigbee/802.15.4/DigiMesh), also include:
        ATAO 0          For Node Discovery responses in 0x95 frames.

    Windows usage:
        xbee_ftp COMxx
    POSIX usage:
        xbee_ftp /dev/ttySxx
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/file_system.h"
#include "xbee/wpan.h"

#include "_atinter.h"           // Common code for processing AT commands
#include "_nodetable.h"         // Common code for handling remote node lists
#include "sample_cli.h"         // Common code for parsing user-entered data
#include "parse_serial_args.h"

// ID of the current working directory on the device, starts at 0 for "/".
// Updated via the "cd" command.
uint16_t path_id = 0;

struct {
    uint16_t file_handle;       // ID of the current open file
    uint32_t file_size;         // total bytes in file
    uint32_t current_offset;    // current offset to read/write
    FILE *local_file;           // FILE pointer to local file
    bool_t is_upload;           // TRUE if sending to XBee, FALSE if receiving
} xfer_state;

const xbee_node_id_t *target = NULL;
#define TARGET_IEEE (target ? &target->ieee_addr_be : NULL)


// return -EAGAIN if there are more entries, 0 if payload held last entry
// otherwise <0 for error
int print_dir_entries(const uint8_t FAR *payload, uint16_t length)
{
    // at least a dir_handle (u16), size_and_flags (u32)
    // an empty directory will have is_last_entry set and a 0-byte filename
    if (length < 6) {
        printf("unexpected length %u\n", length);
        return -EBADMSG;
    }

    // skip over dir_handle
    payload += 2;
    length -= 2;

    // then dump one or more entries
    xbee_fs_dir_entry_t entry;
    int bytes_parsed;

    do {
        bytes_parsed = xbee_fs_extract_dir_entry(&entry, payload, length);
        if (bytes_parsed < 0) {
            printf("error %d parsing entry\n", bytes_parsed);
            return 0;
        }
        if (entry.name_len > 0) {
            if (entry.is_dir) {
                printf("%*s %s/\n", 7, "<DIR>", entry.name);
            } else {
                printf("%*" PRIu32 " %s%c\n", 7, entry.filesize, entry.name,
                       entry.is_secure ? '#' : ' ');
            }
        }
        payload += bytes_parsed;
        length -= bytes_parsed;
    } while (length != 0 && !entry.is_last_entry);

    return entry.is_last_entry ? 0 : -EAGAIN;
}


int send_file_close(xbee_dev_t *xbee, uint16_t file_id)
{
    xbee_fs_req_file_close_t req;

    fclose(xfer_state.local_file);
    xfer_state.local_file = NULL;

    req.fs_cmd_id = XBEE_FS_CMD_ID_FILE_CLOSE;
    req.file_id_be = htobe16(file_id);

    printf("Closing 0x%04X\n", file_id);
    return xbee_fs_req_send(xbee, &req, sizeof(req), TARGET_IEEE);
}


int send_file_read(xbee_dev_t *xbee)
{
    xbee_fs_req_file_read_t req;

    req.fs_cmd_id = XBEE_FS_CMD_ID_FILE_READ;
    req.file_id_be = htobe16(xfer_state.file_handle);
    req.offset_be = htobe32(XBEE_FS_OFFSET_CURRENT);
    req.byte_count_be = htobe16(XBEE_FS_BYTE_COUNT_MAX);

    return xbee_fs_req_send(xbee, &req, sizeof(req), TARGET_IEEE);
}


int send_file_write(xbee_dev_t *xbee)
{
    if (xfer_state.current_offset == xfer_state.file_size) {
        printf("Upload complete\n");
        return send_file_close(xbee, xfer_state.file_handle);
    }

    xbee_fs_req_file_write_t req;

    req.fs_cmd_id = XBEE_FS_CMD_ID_FILE_WRITE;
    req.file_id_be = htobe16(xfer_state.file_handle);
    req.offset_be = htobe32(XBEE_FS_OFFSET_CURRENT);

    uint8_t buffer[1491];	// match maximum read_size used below
    uint16_t read_size = 0;
    uint16_t bytes_read;

    // Set hardcoded payload limits derived from testing and FILE_READ payloads.

    if ((xbee->hardware_series & XBEE_HW_SERIES_MASK)
        == XBEE_HW_SERIES_CELLULAR)
    {
        read_size = 1491;
    } else {
        switch (xbee->hardware_version & XBEE_HARDWARE_MASK) {
        case XBEE_HARDWARE_XB3_MICRO:
        case XBEE_HARDWARE_XB3_TH:
            switch (xbee->firmware_version & XBEE_PROTOCOL_MASK) {
            case XBEE_PROTOCOL_XB3_ZIGBEE:
                // FILE_READ uses 75 bytes, but fragmentation supports 200
                read_size = target == NULL ? 246 : 200;
                break;
            case XBEE_PROTOCOL_XB3_802_15_4:
                read_size = target == NULL ? 231 : 80;
                break;
            case XBEE_PROTOCOL_XB3_DIGIMESH:
                read_size = target == NULL ? 173 : 64;
                break;
            }
        }
    }

    if (read_size == 0) {
        printf("ERROR: unrecognized ATHS/HV/VR combination (0x%X/0x%X/0x%"
               PRIX32 ")\n", xbee->hardware_series, xbee->hardware_version,
               xbee->firmware_version);
        return -ENOTSUP;
    }

    bytes_read = fread(buffer, 1, read_size, xfer_state.local_file);
    xfer_state.current_offset += bytes_read;

    if (bytes_read == 0) {
        printf("Upload complete? cur=%u  size=%u\n",
               xfer_state.current_offset, xfer_state.file_size);
        return send_file_close(xbee, xfer_state.file_handle);
    }

    return xbee_fs_req_send_data(xbee, &req, sizeof(req),
                                 buffer, bytes_read, TARGET_IEEE);
}


int process_file_open(xbee_dev_t *xbee,
                      const void FAR *payload, uint16_t length)
{
    const xbee_payload_fs_file_open_resp_t FAR *response = payload;

    if (length != sizeof(*response)) {
        printf("  (data size %u != %u)\n", length, sizeof(*response));
        return -EINVAL;
    }

    xfer_state.file_handle = be16toh(response->file_handle_be);
    if (xfer_state.is_upload) {
        return send_file_write(xbee);
    } else {
        xfer_state.file_size = be32toh(response->file_size_be);
        return send_file_read(xbee);
    }
}


int process_file_read(xbee_dev_t *xbee,
                      const void FAR *payload, uint16_t length)
{
    const xbee_payload_fs_file_rw_resp_t FAR *response = payload;

    if (xfer_state.file_handle != be16toh(response->file_handle_be)) {
        printf("Error: FILE_READ response for wrong handle 0x%04X != 0x%04X\n",
               be16toh(response->file_handle_be), xfer_state.file_handle);
    } else {
        int to_write = length - offsetof(xbee_payload_fs_file_rw_resp_t, data);

        if (xfer_state.current_offset != be32toh(response->offset_be)) {
            printf("Warning: unexpected FILE_READ offset (%" PRIu32
                   " != %" PRIu32 ")\n",
                   be32toh(response->offset_be), xfer_state.current_offset);
        } else {
            printf("Read %d at offset %u\n", to_write,
                   xfer_state.current_offset);
        }

        if (to_write > 0) {
            size_t written = fwrite(response->data, 1, to_write,
                                    xfer_state.local_file);
            xfer_state.current_offset += written;

            if (written != to_write) {
                printf("ERROR: failed to complete write (%u bytes missing)\n",
                       (unsigned)(to_write - written));
            } else if (xfer_state.current_offset == xfer_state.file_size) {
                printf("Download complete.\n");
                // fall through to common case of sending FILE_CLOSE
            } else {
                // read more data from file
                return send_file_read(xbee);
            }
        } else {
            // we're at EOF because the read returned 0 bytes
            fclose(xfer_state.local_file);
            xfer_state.local_file = NULL;
        }
    }

    // in error cases or file complete, send FILE_CLOSE
    return send_file_close(xbee, be16toh(response->file_handle_be));
}


int process_file_write(xbee_dev_t *xbee,
                       const void FAR *payload, uint16_t length)
{
    const xbee_payload_fs_file_rw_resp_t FAR *response = payload;

    if (xfer_state.file_handle != be16toh(response->file_handle_be)) {
        printf("Error: FILE_WRITE response for wrong handle 0x%04X != 0x%04X\n",
               be16toh(response->file_handle_be), xfer_state.file_handle);
    } else {
        if (xfer_state.current_offset != be32toh(response->offset_be)) {
            printf("Warning: unexpected FILE_WRITE offset (%" PRIu32
                   " != %" PRIu32 ")\n",
                   be32toh(response->offset_be), xfer_state.current_offset);
        } else {
            printf("Remote file offset is %u\n", xfer_state.current_offset);
        }
        return send_file_write(xbee);
    }

    // in error case, send FILE_CLOSE
    return send_file_close(xbee, be16toh(response->file_handle_be));
}


void print_vol_stat(const void FAR *payload, uint16_t length)
{
    const xbee_payload_fs_volume_resp_t FAR *response = payload;

    if (length != sizeof(*response)) {
        printf("  (data size %u != %u)\n", length, sizeof(*response));
    } else {
        uint32_t bytes[3];

        bytes[0] = be32toh(response->bytes_used_be);
        bytes[1] = be32toh(response->bytes_free_be);
        bytes[2] = be32toh(response->bytes_bad_be);

        printf("%7u used\n%7u free\n%7u bad\n%7u total\n",
               bytes[0], bytes[1], bytes[2],
               bytes[0] + bytes[1] + bytes[2]);
    }
}

int xbee_frame_file_system_resp(xbee_dev_t *xbee, const void FAR *payload,
                                uint16_t length, void FAR *context)
{
    // strip header off of command-specific payload
    const xbee_header_file_system_resp_t FAR *local = payload;
    const xbee_header_remote_fs_resp_t FAR *remote = payload;
    const addr64 FAR *target_ieee = NULL;
    char sender[ADDR64_STRING_LENGTH];
    const uint8_t FAR *response;

    if (local->frame_type == XBEE_FRAME_FILE_SYSTEM_RESP) {
        payload = &local[1];
        length -= sizeof(*local);
        response = &local->fs_cmd_id;
        strcpy(sender, "local");
    } else if (remote->frame_type == XBEE_FRAME_REMOTE_FS_RESP) {
        payload = &remote[1];
        length -= sizeof(*remote);
        response = &remote->fs_cmd_id;
        addr64_format(sender, &remote->remote_addr_be);
    } else {
        printf("ERROR: This handler only for file system responses\n");
        return -EINVAL;
    }

    printf("RESPONSE id:0x%02X cmd:0x%02X status:0x%02X (%s)\n",
           local->frame_id, response[0], response[1], sender);

    if (response[1] != XBEE_FS_STATUS_SUCCESS) {
        if (response[0] == XBEE_FS_CMD_ID_FILE_OPEN) {
            // Open failed; close our local file.  Because we don't store
            // the local filename, we are unable to delete canceled downloads.
            fclose(xfer_state.local_file);
            xfer_state.local_file = NULL;
        }
        return 0;
    }

    switch (response[0]) {
    case XBEE_FS_CMD_ID_FILE_OPEN:
        process_file_open(xbee, payload, length);
        break;

    case XBEE_FS_CMD_ID_FILE_READ:
        process_file_read(xbee, payload, length);
        break;

    case XBEE_FS_CMD_ID_FILE_WRITE:
        process_file_write(xbee, payload, length);
        break;

    case XBEE_FS_CMD_ID_FILE_HASH:
        if (length != 32) {
            printf("Error: FILE_HASH should be 32 bytes (not %u)\n", length);
        } else {
            const uint8_t *p = payload;
            printf("sha256 ");
            while (length--) {
                printf("%02x", *p++);
            }
            puts("");
        }
        break;

    case XBEE_FS_CMD_ID_DIR_OPEN:
    case XBEE_FS_CMD_ID_DIR_READ:
        {
            xbee_fs_req_dir_read_t req;
            req.dir_handle_be = xbee_get_unaligned16(payload);

            if (print_dir_entries(payload, length) == -EAGAIN) {
                req.fs_cmd_id = XBEE_FS_CMD_ID_DIR_READ;
                xbee_fs_req_send(xbee, &req, sizeof(req), target_ieee);
            } else {
                // Either an error or printed last entry.  Print a blank line
                // to signal the end of the directory listing.
                puts("");
            }
        }
        break;

    case XBEE_FS_CMD_ID_GET_PATH_ID:
        path_id = be16toh(xbee_get_unaligned16(payload));
        printf("Path ID 0x%04X: %.*s\n", path_id, length - 2,
               (uint8_t *)payload + 2);
        break;

    case XBEE_FS_CMD_ID_DIR_CREATE:
    case XBEE_FS_CMD_ID_DIR_CLOSE:
    case XBEE_FS_CMD_ID_FILE_CLOSE:
    case XBEE_FS_CMD_ID_DELETE:
    case XBEE_FS_CMD_ID_RENAME:
        // no payload, just the status byte
        break;

    case XBEE_FS_CMD_ID_VOL_STAT:
    case XBEE_FS_CMD_ID_VOL_FORMAT:
        print_vol_stat(payload, length);
        break;

    default:
        printf("  (no parser for cmd 0x%02X)\n", response[0]);
    }

    return 0;
}


const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,         // for processing AT command responses
    XBEE_FRAME_HANDLE_ATND_RESPONSE,    // for processing ATND responses
    XBEE_FRAME_TRANSMIT_STATUS_DEBUG,   // for processing 0x8B responses

    { XBEE_FRAME_FILE_SYSTEM_RESP, 0, xbee_frame_file_system_resp, NULL },
    { XBEE_FRAME_REMOTE_FS_RESP, 0, xbee_frame_file_system_resp, NULL },

    XBEE_FRAME_TABLE_END
};


// Callback registered with xbee_disc_add_node_id_handler to update node list.
void node_discovered(xbee_dev_t *xbee, const xbee_node_id_t *rec)
{
    if (rec != NULL) {
        int index = node_add(rec);
        if (index == -ENOSPC) {
            printf("Node table full.  Unable to add node:\n    ");
        } else {
            printf("%2u: ", index);
        }
        xbee_disc_node_id_dump(rec);
    }
}


void handle_menu_cmd(xbee_dev_t *xbee, char *command)
{
    XBEE_UNUSED_PARAMETER(xbee);
    XBEE_UNUSED_PARAMETER(command);

    puts("");

    print_cli_help_atcmd();

    print_cli_help_nodetable();
    puts(" target                          Send FS commands to local device");
    puts(" target <n>                      Send FS commands to node <n>");

    puts("--- File System ---");
    puts(" ls <dirname>                    List files in directory <dirname>");
    puts(" md <dirname>                    Make directory <dirname>");
    puts(" cd <dirname>                    Change current working directory");
    puts("");
    puts(" get <remote> <local>            Download remote file to file named <local>");
    puts(" put <local> <remote>            Upload local file to file named <remote>");
    puts(" xput <local> <remote>           Upload to secure (read-only) file");
    puts(" hash <filename>                 Get 32-byte hash of file");
    puts("");
    puts(" rm <name>                       Delete file or empty directory");
    puts(" mv <orig> <new>                 Rename file or directory");
    puts("");
    puts(" vol_stat /flash                 Print space used/available in file system");
    puts(" format /flash                   Format the file system");

    puts("--- Other ---");

    print_cli_help_menu();

    puts(" quit                            Quit");
    puts("");
}


static const xbee_node_id_t broadcast_target = {
    { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF } }
};


bool_t error_on_broadcast_target(void)
{
    if (target == &broadcast_target) {
        puts("ERROR: command not valid for broadcast");
        return TRUE;
    }

    return FALSE;
}


void set_target(const xbee_node_id_t *new_target)
{
    target = new_target;
    path_id = 0;    // reset working directory to "/"

    if (target == NULL) {
        puts("Targeting local XBee module; working directory is '/'.");
    } else if (target == &broadcast_target) {
        puts("Sending broadcast requests; working directory is '/'.");
    } else {
        char address[ADDR64_STRING_LENGTH];
        printf("Targeting remote %s; working directory is '/'.\n",
               addr64_format(address, &target->ieee_addr_be));
    }
}


// target [<n>]
void handle_target_cmd(xbee_dev_t *xbee, char *command)
{
    const char *p = &command[6];        // point beyond "target"
    while (isspace((uint8_t)*p)) {
        ++p;
    }
    if (*p == '\0') {
        set_target(NULL);
        return;
    }

    char *next_param;
    unsigned n = (unsigned)strtoul(p, &next_param, 0);

    if (next_param == NULL || next_param == p) {
        puts("Error: must specify node index");
    } else {
        const xbee_node_id_t *t = node_by_index(n);
        if (t == NULL) {
            printf("Error: invalid node index %u\n", n);
        } else {
            set_target(t);
        }
    }
}


// broadcast
void handle_broadcast_cmd(xbee_dev_t *xbee, char *command)
{
    set_target(&broadcast_target);
}


// Helper routine for commands with a path ID and name.
void handle_oneparam_cmd(xbee_dev_t *xbee, uint8_t fs_cmd_id, char *name)
{
    xbee_fs_req_path_and_name_t req;

    // skip over whitespace between command and parameter/volume name
    while (isspace(*name)) {
        ++name;
    }

    req.fs_cmd_id = fs_cmd_id;
    req.path_id_be = htobe16(path_id);

    int result = xbee_fs_req_send_str(xbee, &req, sizeof(req), name, TARGET_IEEE);

    if (result < 0) {
        printf("error %d sending request\n", result);
    } else {
        printf("sent frame id 0x%02X\n", result);
    }
}

// md <dirname>
void handle_md_cmd(xbee_dev_t *xbee, char *command)
{
    handle_oneparam_cmd(xbee, XBEE_FS_CMD_ID_DIR_CREATE, &command[2]);
}

// ls <dirname>
void handle_ls_cmd(xbee_dev_t *xbee, char *command)
{
    if (!error_on_broadcast_target()) {
        handle_oneparam_cmd(xbee, XBEE_FS_CMD_ID_DIR_OPEN, &command[2]);
    }
}

// rm <name>
void handle_rm_cmd(xbee_dev_t *xbee, char *command)
{
    handle_oneparam_cmd(xbee, XBEE_FS_CMD_ID_DELETE, &command[2]);
}

// cd <dirname>
void handle_cd_cmd(xbee_dev_t *xbee, char *command)
{
    if (!error_on_broadcast_target()) {
        handle_oneparam_cmd(xbee, XBEE_FS_CMD_ID_GET_PATH_ID, &command[2]);
    }
}

// hash <filename>
void handle_hash_cmd(xbee_dev_t *xbee, char *command)
{
    handle_oneparam_cmd(xbee, XBEE_FS_CMD_ID_FILE_HASH, &command[4]);
}


// mv <source> <dest>
void handle_mv_cmd(xbee_dev_t *xbee, char *command)
{
    char *params = &command[2];
    while (isspace(*params)) {
        ++params;
    }

    if (*params != '\0') {
        // convert space separating parameters to a comma
        char *sep = strchr(params, ' ');
        if (sep != NULL) {
            *sep = ',';
            printf("param is '%s'\n", params);
            handle_oneparam_cmd(xbee, XBEE_FS_CMD_ID_RENAME, params);
            return;
        }
    }

    printf("Error parsing src_name and dst_name from command\n");
}


// get <remote_file> <local_file>
void handle_get_cmd(xbee_dev_t *xbee, char *command)
{
    if (error_on_broadcast_target()) {
        return;
    }

    char *remote = &command[3];
    while (isspace(*remote)) {
        ++remote;
    }
    // skip over remote file to get local filename
    char *local = strchr(remote, ' ');
    if (local == NULL) {
        printf("ERROR: must specify local filename\n");
        return;
    }

    // null-terminate remote and advance local to the next character
    *local++ = '\0';
    while (isspace(*local)) {
        ++local;
    }

    memset(&xfer_state, 0, sizeof xfer_state);
    xfer_state.local_file = fopen(local, "wb");
    if (xfer_state.local_file == NULL) {
        printf("Error %d opening '%s'.\n", errno, local);
        return;
    }
    xfer_state.is_upload = FALSE;

    xbee_fs_req_file_open_t req;

    req.fs_cmd_id = XBEE_FS_CMD_ID_FILE_OPEN;
    req.path_id_be = htobe16(path_id);
    req.options = XBEE_FS_OPT_READ;

    int result = xbee_fs_req_send_str(xbee, &req, sizeof(req), remote, TARGET_IEEE);

    if (result < 0) {
        printf("error %d sending request\n", result);
    } else {
        printf("sent frame id 0x%02X\n", result);
    }
}


// put <local_file> <remote_file>
void handle_put_cmd(xbee_dev_t *xbee, char *command)
{
    if (error_on_broadcast_target()) {
        return;
    }

    bool_t is_secure = toupper(*command) == 'X';

    char *local = &command[is_secure ? 4 : 3];
    while (isspace(*local)) {
        ++local;
    }
    // skip over local file to get remote filename
    char *remote = strchr(local, ' ');
    if (remote == NULL) {
        printf("ERROR: must specify remote filename\n");
        return;
    }

    // null-terminate local and advance remote to the next character
    *remote++ = '\0';
    while (isspace(*remote)) {
        ++remote;
    }

    memset(&xfer_state, 0, sizeof xfer_state);
    xfer_state.local_file = fopen(local, "rb");
    if (xfer_state.local_file == NULL) {
        printf("Error %d opening '%s'.\n", errno, local);
        return;
    }
    xfer_state.is_upload = TRUE;
    fseek(xfer_state.local_file, 0L, SEEK_END);
    xfer_state.file_size = ftell(xfer_state.local_file);
    rewind(xfer_state.local_file);

    printf("Uploading %" PRIu32 "-byte file...\n", xfer_state.file_size);

    xbee_fs_req_file_open_t req;

    req.fs_cmd_id = XBEE_FS_CMD_ID_FILE_OPEN;
    req.path_id_be = htobe16(path_id);
    req.options = XBEE_FS_OPT_WRITE | XBEE_FS_OPT_CREATE | XBEE_FS_OPT_TRUNCATE | XBEE_FS_OPT_EXCLUSIVE;
    if (is_secure) {
        req.options |= XBEE_FS_OPT_SECURE;
    }

    int result = xbee_fs_req_send_str(xbee, &req, sizeof(req), remote, TARGET_IEEE);

    if (result < 0) {
        printf("error %d sending request\n", result);
    } else {
        printf("sent frame id 0x%02X\n", result);
    }
}


// {vol_stat|format} <vol_name>
void handle_volume_cmd(xbee_dev_t *xbee, char *command)
{
    uint8_t fs_cmd_id;

    // parameter starts immediately after command
    const char *vol;

    switch (tolower(*command)) {
    case 'f':   // format
        fs_cmd_id = XBEE_FS_CMD_ID_VOL_FORMAT;
        vol = &command[6];
        break;
    case 'v':   // vol_stat
        fs_cmd_id = XBEE_FS_CMD_ID_VOL_STAT;
        vol = &command[8];
        break;
    }

    // skip over whitespace between command and parameter/volume name
    while (isspace(*vol)) {
        ++vol;
    }

    int result = xbee_fs_req_send_str(xbee, &fs_cmd_id, sizeof(fs_cmd_id),
                                  vol, TARGET_IEEE);

    if (result < 0) {
        printf("error %d sending request\n", result);
    } else {
        printf("sent frame id 0x%02X\n", result);
    }
}


const cmd_entry_t commands[] = {
    ATCMD_CLI_ENTRIES
    MENU_CLI_ENTRIES
    NODETABLE_CLI_ENTRIES

    { "target",         &handle_target_cmd },
    { "broadcast",      &handle_broadcast_cmd },

    { "md",             &handle_md_cmd },
    { "ls",             &handle_ls_cmd },
    { "cd",             &handle_cd_cmd },
    { "get",            &handle_get_cmd },
    { "put",            &handle_put_cmd },
    { "xput",           &handle_put_cmd },
    { "hash",           &handle_hash_cmd },

    { "rm",             &handle_rm_cmd },
    { "mv",             &handle_mv_cmd },

    { "vol_stat",       &handle_volume_cmd },
    { "format",         &handle_volume_cmd },

    { NULL, NULL }                      // end of command table
};


int main(int argc, char *argv[])
{
    xbee_dev_t my_xbee;
    char cmdstr[256];
    xbee_serial_t XBEE_SERPORT;
    int status, frame_count;

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
    printf("Waiting for driver to query the XBee device...\n");
    while ((status = xbee_cmd_query_status(&my_xbee)) == -EBUSY) {
        xbee_dev_tick(&my_xbee);
    }
    if (status) {
        printf("Error %d waiting for query to complete.\n", status);
    }

    // report on the settings
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    // receive node discovery notifications
    xbee_disc_add_node_id_handler(&my_xbee, &node_discovered);

    handle_menu_cmd(NULL, NULL);

    // automatically initiate node discovery
    handle_nd_cmd(&my_xbee, "nd");

    while (1) {
        int linelen;
        do {
            linelen = xbee_readline(cmdstr, sizeof cmdstr);
            xbee_cmd_tick();
            frame_count = xbee_dev_tick(&my_xbee);
        } while (linelen == -EAGAIN && frame_count >= 0);

        if (frame_count < 0) {
            printf("Error %d calling xbee_dev_tick().\n", frame_count);
            break;
        }

        if (linelen == -ENODATA || strcmpi(cmdstr, "quit") == 0) {
            break;
        }

        sample_cli_dispatch(&my_xbee, cmdstr, &commands[0]);
    }

    return frame_count < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
