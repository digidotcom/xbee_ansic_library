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
    Sample: Socket Test

    Tool used for manually/interactively exploring and testing
    xbee/xbee_socket.c APIs.
*/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/ipv4.h"
#include "xbee/socket.h"
#include "xbee/delivery_status.h"

#include "_atinter.h"
#include "parse_serial_args.h"


const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_SOCK_FRAME_HANDLERS,
    XBEE_FRAME_TABLE_END
};


static void notify_callback(xbee_sock_t socket,
                            uint8_t frame_type, uint8_t message)
{
#if XBEE_TX_DELIVERY_STR_BUF_SIZE > XBEE_SOCK_STR_BUF_SIZE
    char buffer[XBEE_TX_DELIVERY_STR_BUF_SIZE];
#else
    char buffer[XBEE_SOCK_STR_BUF_SIZE];
#endif

    switch (frame_type) {
    case XBEE_FRAME_TX_STATUS:          // message is an XBEE_TX_DELIVERY_xxx
        if (message != 0) {
            printf("Socket 0x%04X: XBEE_TX_DELIVERY=%s\n",
                   socket, xbee_tx_delivery_str(message, buffer));
        }
        break;

    case XBEE_FRAME_SOCK_STATE:         // message is an XBEE_SOCK_STATE_xxx
        printf("Socket 0x%04X: XBEE_SOCK_STATE=%s\n",
               socket, xbee_sock_state_str(message, buffer));
        break;

    case XBEE_FRAME_SOCK_CREATE_RESP:
    case XBEE_FRAME_SOCK_CONNECT_RESP:
    case XBEE_FRAME_SOCK_CLOSE_RESP:
    case XBEE_FRAME_SOCK_LISTEN_RESP:   // message is an XBEE_SOCK_STATUS_xxx
        if (message != 0) {
            printf("Socket 0x%04X: XBEE_SOCK_STATUS=%s\n",
                   socket, xbee_sock_status_str(message, buffer));
        }
        break;

    default:
        printf("Socket 0x%04X: unexpected frame_type 0x%02X, message 0x%02X\n",
               socket, frame_type, message);
    }
}

static void receive_callback(xbee_sock_t socket, uint8_t status,
                             const void *payload, size_t payload_length)
{
    // Since payload isn't null-terminated, use ".*s" to limit length.
    printf("Socket 0x%04X: received %u bytes\n%.*s\n",
           socket, (unsigned)payload_length,
           (int)payload_length, (const char *)payload);
}

static void receive_from_callback(xbee_sock_t socket, uint8_t status,
                                  uint32_t remote_addr, uint16_t remote_port,
                                  const void *datagram, size_t datagram_length)
{
    // Since datagram isn't null-terminated, use ".*s" to limit length.
    printf("Socket 0x%04X: received %u bytes (from 0x%08X:%u)\n%.*s\n",
           socket, (unsigned)datagram_length, remote_addr, remote_port,
           (int)datagram_length, (const char *)datagram);
}

static xbee_sock_receive_fn ipv4_client_callback(xbee_sock_t listening_socket,
                                                 xbee_sock_t client_socket,
                                                 uint32_t remote_addr,
                                                 uint16_t remote_port)
{
    if (client_socket < 0) {
        printf("Socket 0x%04X: error %d for client 0x%08X:%u\n",
               listening_socket, client_socket, remote_addr, remote_port);

        return NULL;
    }

    printf("Socket 0x%04X: created socket 0x%04X for client 0x%08X:%u\n",
           listening_socket, client_socket, remote_addr, remote_port);

    return &receive_callback;
}

static void option_callback(xbee_sock_t socket, uint8_t option_id,
                            uint8_t status, const void *data, size_t data_len)
{
    char buffer[XBEE_SOCK_STR_BUF_SIZE];

    if (status) {
        printf("Socket 0x%04X: option 0x%02X status 0x%02X (%s)\n",
               socket, option_id, status, xbee_sock_status_str(status, buffer));
    } else if (data_len) {
        printf("Socket 0x%04X: option 0x%02X is\n", socket, option_id);
        hex_dump(data, data_len, HEX_DUMP_FLAG_TAB);
    } else {
        printf("Socket 0x%04X: option 0x%02X set\n", socket, option_id);
    }
}

void print_menu(void)
{
    puts("");
    puts("--- AT Commands ---");
    puts("Valid command formats (CC is command):");
    puts(" ATCC 0xXXXXXX (where XXXXXX is an even number of " \
           "hexadecimal characters)");
    puts(" ATCC YYYY (where YYYY is an integer, up to 32 bits)");
    puts(" ATCC \"Node ID String\" (where quotes contain string data)");
    puts("--- Create Socket ---");
    puts(" create [udp|tcp|ssl]            Create socket (returns ID for other cmds)");
    puts("--- Option Request ---");
    puts(" option <s> <opt_id> <data>      Get/Set option on socket (1-byte data)");
    puts("--- Listen/Connect/Send Socket ---");
    puts(" listen <s> <port>               Listen on <port> of TCP/SSL socket <s>");
    puts(" connect <s> <hostname> <port>   Connect socket <s> to hostname:port");
    puts(" send <s> <string data>          Send <string data> to socket <s>");
    puts(" sendline <s> <string data>      Send <string data> + CRLF to socket <s>");
    puts(" sendcr <s>                      Send bare CR to socket <s>");
    puts(" sendcrlf <s>                    Send CR/LF sequence to socket <s>");
    puts("--- Bind/Sendto Socket ---");
    puts(" bind <s> <port>                 Bind UDP socket <s> to <port>");
    puts(" sendto <s> <addr> <p> <string>  Send <string> to <addr>:<p>");
    puts("--- Close Socket ---");
    puts(" close <s>                       Close socket id <s>");
    puts(" close all                       Close all sockets");
    puts("--- Other ---");
    puts(" ip <0x00000000|12.34.56.78>     Convert IP between 32-bit & dotted quad");
    puts(" <menu|help|?>                   Print this menu");
    puts(" quit                            Quit");
    puts("");
}

int main(int argc, char *argv[])
{
    xbee_dev_t my_xbee;
    char cmdstr[256];
    xbee_serial_t XBEE_SERPORT;
    int status;

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
    do {
        xbee_dev_tick(&my_xbee);
        status = xbee_cmd_query_status(&my_xbee);
    } while (status == -EBUSY);
    if (status) {
        printf("Error %d waiting for query to complete.\n", status);
    }

    // report on the settings
    xbee_dev_dump_settings(&my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

    print_menu();
    xbee_sock_reset(&my_xbee);          // close all open sockets

    while (1) {
        int linelen;
        do {
            linelen = xbee_readline(cmdstr, sizeof cmdstr);
            xbee_dev_tick(&my_xbee);
        } while (linelen == -EAGAIN);

        if (linelen == -ENODATA || strcmpi(cmdstr, "quit") == 0) {
            break;
        } else if (strcmpi(cmdstr, "menu") == 0
                   || strcmpi(cmdstr, "help") == 0
                   || strcmp(cmdstr, "?") == 0)
        {
            print_menu();
        } else if (strncmpi(cmdstr, "at", 2) == 0) {
            process_command(&my_xbee, cmdstr);
        } else if (strncmpi(cmdstr, "ip ", 3) == 0) {
            uint32_t ip_be;
            char buffer[16];
            if (strchr(cmdstr, '.')) {
                // convert dotted quad to uint32_t
                if (xbee_ipv4_aton(&cmdstr[3], &ip_be) == 0) {
                    printf("%s = 0x%08X\n", &cmdstr[3], be32toh(ip_be));
                    continue;
                }
            } else {
                // convert uint32_t to dotted quad
                ip_be = htobe32((uint32_t)strtoul(&cmdstr[3], NULL, 0));
                if (xbee_ipv4_ntoa(buffer, ip_be) == 0) {
                    printf("%s = %s\n", &cmdstr[3], buffer);
                    continue;
                }
            }
            printf("Error parsing command\n");
        } else if (strncmpi(cmdstr, "create ", 7) == 0) {
            int proto = -1;
            if (strcmpi(&cmdstr[7], "udp") == 0) {
                proto = XBEE_SOCK_PROTOCOL_UDP;
            } else if (strcmpi(&cmdstr[7], "tcp") == 0) {
                proto = XBEE_SOCK_PROTOCOL_TCP;
            } else if (strcmpi(&cmdstr[7], "ssl") == 0) {
                proto = XBEE_SOCK_PROTOCOL_SSL;
            } else {
                printf("Error: invalid protocol '%s'\n", &cmdstr[7]);
            }
            if (proto != -1) {
                xbee_sock_t s = xbee_sock_create(&my_xbee, proto,
                                                 &notify_callback);
                if (s < 0) {
                    printf("Error %d\n", s);
                } else {
                    printf("Created socket 0x%04X\n", s);
                }
            }
        } else if (strncmpi(cmdstr, "option ", 7) == 0) {
            xbee_sock_t s;
            int option_id, value;

            int params = sscanf(&cmdstr[7], "%i %i %i", &s, &option_id, &value);
            if (params == 2) {          // get option
                xbee_sock_option(s, option_id, NULL, 0, option_callback);
            } else if (params == 3) {   // set option
                uint8_t data = (uint8_t)value;
                xbee_sock_option(s, option_id, &data, 1, option_callback);
            }
        } else if (strncmpi(cmdstr, "connect ", 8) == 0) {
            xbee_sock_t s;
            char hostname[80];
            unsigned port;

            if (sscanf(&cmdstr[8], "%i %80s %u", &s, &hostname[0], &port) < 3) {
                printf("Error parsing command\n");
            } else {
                int result;
                if (strchr(hostname, '.') == NULL) {
                    // treat hostname as 32-bit numeric address
                    uint32_t address = (uint32_t)strtoul(hostname, NULL, 0);
                    result = xbee_sock_connect(s, (uint16_t)port, address,
                                               NULL, receive_callback);
                } else {
                    result = xbee_sock_connect(s, (uint16_t)port, 0,
                                               hostname, receive_callback);
                }
                if (result) {
                    printf("Error %d\n", result);
                }
            }
        } else if (strncmpi(cmdstr, "bind ", 5) == 0) {
            xbee_sock_t s;
            unsigned port;

            if (sscanf(&cmdstr[5], "%i %u", &s, &port) < 2) {
                printf("Error parsing command\n");
            } else {
                int result = xbee_sock_bind(s, (uint16_t)port,
                                            receive_from_callback);
                if (result) {
                    printf("Error %d\n", result);
                } else {
                    printf("Socket 0x%04X: bound to port %u\n", s, port);
                }
            }
        } else if (strncmpi(cmdstr, "listen ", 7) == 0) {
            xbee_sock_t s;
            unsigned port;

            if (sscanf(&cmdstr[7], "%i %u", &s, &port) < 2) {
                printf("Error parsing command\n");
            } else {
                printf("sock 0x%04X listening to port %u\n", s, port);
                int result = xbee_sock_listen(s, (uint16_t)port,
                                              ipv4_client_callback);
                if (result) {
                    printf("Error %d\n", result);
                } else {
                    printf("Socket 0x%04X: listening on port %u\n", s, port);
                }
            }
        } else if (strncmpi(cmdstr, "sendto ", 7) == 0) {
            xbee_sock_t s;
            unsigned addr, port;
            char message[80];

            memset(message, '\0', sizeof(message));
            int parsed = sscanf(&cmdstr[7], "%i %i %u %80c",
                                &s, &addr, &port, &message[0]);
            if (parsed < 4) {
                printf("Error parsing command\n");
            } else {
                xbee_sock_sendto(s, 0, (uint32_t)addr, (uint16_t)port,
                                 message, strlen(message));
            }
        } else if (strncmpi(cmdstr, "send", 4) == 0) {
            xbee_sock_t s;
            char *space = strchr(cmdstr, ' ');
            if (space) {
                s = (xbee_sock_t)strtoul(space + 1, &space, 0);
                if (strncmpi(cmdstr, "sendcrlf ", 9) == 0) {
                    xbee_sock_send(s, 0, "\r\n", 2);
                } else if (strncmpi(cmdstr, "sendcr ", 7) == 0) {
                    xbee_sock_send(s, 0, "\r", 1);
                } else if (space) {
                    ++space;
                    size_t len = strlen(space);
                    if (strncmpi(cmdstr, "sendline ", 9) == 0) {
                        strcpy(&space[len], "\r\n");
                        len += 2;
                    }
                    xbee_sock_send(s, 0, space, len);
                } else {
                    printf("Error parsing command\n");
                }
            }
        } else if (strncmpi(cmdstr, "close ", 6) == 0) {
            int err;
            if (strcmpi(&cmdstr[6], "all") == 0) {
                err = xbee_sock_close_all(&my_xbee);
            } else {
                xbee_sock_t s = (xbee_sock_t)strtoul(&cmdstr[6], NULL, 0);
                err = xbee_sock_close(s);
            }
            if (err) {
                printf("Error %d\n", err);
            }
        } else {
            printf("Error: unknown command '%s'\n", cmdstr);
        }
    }

    xbee_sock_close_all(NULL);
    // wait for all Socket Close frames to actually send
    while (xbee_ser_tx_used(&XBEE_SERPORT) > 0) {
        xbee_dev_tick(&my_xbee);
    }

    return 0;
}
