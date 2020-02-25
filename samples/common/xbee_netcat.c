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
    Sample: XBee Netcat

    A version of the netcat (nc) tool that uses the XBee sockets API to open a
    TCP or UDP socket and pass its data to/from stdout/stdin.
*/

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/ipv4.h"
#include "xbee/socket.h"

#include "_xbee_term.h"         // for xbee_term_set_color()

typedef enum netcat_state {
    NETCAT_STATE_INIT,
    NETCAT_STATE_SEND_ATAI,
    NETCAT_STATE_WAIT_FOR_ATAI,
    NETCAT_STATE_GOT_ATAI,
    NETCAT_STATE_SEND_ATMY,
    NETCAT_STATE_WAIT_FOR_ATMY,
    NETCAT_STATE_GOT_ATMY,
    NETCAT_STATE_WAIT_FOR_CREATE,
    NETCAT_STATE_CREATED,
    NETCAT_STATE_WAIT_FOR_ATTACH,
    NETCAT_STATE_WAIT_FOR_CONNECT,
    NETCAT_STATE_CONNECTED,
    NETCAT_STATE_INTERACTIVE,
    NETCAT_STATE_WAIT_FOR_CLOSE,
    NETCAT_STATE_DONE,
    NETCAT_STATE_ERROR
} netcat_state_t;

static netcat_state_t netcat_state;
uint8_t netcat_atai = 0;
uint32_t netcat_atmy = 0;
xbee_sock_t netcat_listen = 0;
xbee_sock_t netcat_socket = 0;

static void netcat_sock_notify_cb(xbee_sock_t socket,
                                  uint8_t frame_type, uint8_t message)
{
#if XBEE_TX_DELIVERY_STR_BUF_SIZE > XBEE_SOCK_STR_BUF_SIZE
    char buffer[XBEE_TX_DELIVERY_STR_BUF_SIZE];
#else
    char buffer[XBEE_SOCK_STR_BUF_SIZE];
#endif

    // make sure it's a status for our sockets
    if (socket != netcat_socket && socket != netcat_listen) {
        return;
    }

    xbee_term_set_color(SOURCE_STATUS);

    switch (frame_type) {
    case XBEE_FRAME_TX_STATUS:          // message is an XBEE_TX_DELIVERY_xxx
        if (message != 0) {
            fprintf(stderr, "%s: %s\n", "XBEE_TX_DELIVERY",
                   xbee_tx_delivery_str(message, buffer));
        }
        break;

    case XBEE_FRAME_SOCK_STATE:         // message is an XBEE_SOCK_STATE_xxx
        if (netcat_state == NETCAT_STATE_WAIT_FOR_CONNECT && message == 0) {
            netcat_state = NETCAT_STATE_CONNECTED;
        } else if (message != 0) {
            // ignore errors on listening socket
            if (socket == netcat_socket) {
                netcat_state = message == XBEE_SOCK_STATE_TRANSPORT_CLOSED
                    ? NETCAT_STATE_DONE : NETCAT_STATE_ERROR;
            }
            fprintf(stderr, "%s: %s\n", "XBEE_SOCK_STATE",
                   xbee_sock_state_str(message, buffer));
        }
        break;

    case XBEE_FRAME_SOCK_CREATE_RESP:
        if (message == 0) {
            if (netcat_state == NETCAT_STATE_WAIT_FOR_CREATE) {
                netcat_state = NETCAT_STATE_CREATED;
            }
        } else {
            fprintf(stderr, "%s: %s\n", "XBEE_SOCK_CREATE",
                   xbee_sock_status_str(message, buffer));
            netcat_state = NETCAT_STATE_ERROR;
        }
        break;

    case XBEE_FRAME_SOCK_CONNECT_RESP:
    case XBEE_FRAME_SOCK_LISTEN_RESP:
        if (message == 0) {
            if (netcat_state == NETCAT_STATE_WAIT_FOR_ATTACH) {
                netcat_state = NETCAT_STATE_WAIT_FOR_CONNECT;
            }
        } else {
            fprintf(stderr, "%s: %s\n", "XBEE_SOCK_CONNECT/LISTEN",
                    xbee_sock_status_str(message, buffer));
            netcat_state = NETCAT_STATE_ERROR;
        }
        break;

    default:
        // we don't care about other possible notifications
        break;
    }

    xbee_term_set_color(SOURCE_KEYBOARD);
}


static void netcat_sock_receive_cb(xbee_sock_t socket, uint8_t status,
                                   const void *payload, size_t payload_length)
{
    xbee_term_set_color(SOURCE_SERIAL);

    // Since payload isn't null-terminated, use ".*s" to limit length.
    printf("%.*s", (int)payload_length, (const char *)payload);

    xbee_term_set_color(SOURCE_KEYBOARD);
}


xbee_sock_receive_fn netcat_sock_ipv4_client_cb(xbee_sock_t listening_socket,
                                                xbee_sock_t client_socket,
                                                uint32_t remote_addr,
                                                uint16_t remote_port)
{
    char ipbuf[16];

    xbee_ipv4_ntoa(ipbuf, htobe32(remote_addr));
    fprintf(stderr, "Inbound connection from %s:%u...\n", ipbuf, remote_port);

    netcat_socket = client_socket;

    if (netcat_state == NETCAT_STATE_WAIT_FOR_CONNECT) {
        netcat_state = NETCAT_STATE_INTERACTIVE;
    }

    return &netcat_sock_receive_cb;
}

static struct {
    uint32_t addr;
    uint16_t port;
} netcat_last_datagram = { 0, 0 };

static void netcat_sock_receive_from_cb(xbee_sock_t socket, uint8_t status,
                                        uint32_t remote_addr,
                                        uint16_t remote_port,
                                        const void *datagram,
                                        size_t datagram_length)
{
    char ipbuf[16];

    xbee_term_set_color(SOURCE_SERIAL);

    netcat_last_datagram.addr = remote_addr;
    netcat_last_datagram.port = remote_port;

    xbee_ipv4_ntoa(ipbuf, htobe32(remote_addr));
    // Since payload isn't null-terminated, use ".*s" to limit length.
    printf("Datagram from %s:%u:\n%.*s",
           ipbuf, remote_port, (int)datagram_length, (const char *)datagram);

    xbee_term_set_color(SOURCE_KEYBOARD);

    if (netcat_state == NETCAT_STATE_WAIT_FOR_CONNECT) {
        netcat_state = NETCAT_STATE_INTERACTIVE;
    }
}


int netcat_atcmd_callback( const xbee_cmd_response_t FAR *response)
{
    if ((response->flags & XBEE_CMD_RESP_MASK_STATUS) != XBEE_AT_RESP_SUCCESS) {
        fprintf(stderr, "ERROR: sending AT%.2" PRIsFAR "\n",
                response->command.str);
        return XBEE_ATCMD_DONE;
    }

    switch (netcat_state) {
    case NETCAT_STATE_WAIT_FOR_ATAI:
        if (memcmp(response->command.str, "AI", 2) == 0) {
            netcat_atai = (uint8_t)response->value;
            netcat_state = NETCAT_STATE_GOT_ATAI;
        }
        break;
    case NETCAT_STATE_WAIT_FOR_ATMY:
        if (memcmp(response->command.str, "MY", 2) == 0) {
            netcat_atmy = response->value;
            netcat_state = NETCAT_STATE_GOT_ATMY;
        }
        break;
    default:
        // not our AT command, ignore result
        break;
    }

    return XBEE_ATCMD_DONE;
}

int netcat_atcmd(xbee_dev_t *xbee, const char *cmd)
{
    int request;

    request = xbee_cmd_create(xbee, cmd);
    if (request < 0) {
        fprintf(stderr, "ERROR: couldn't create AT%s command (%d)\n",
                cmd, request);
    } else {
        xbee_cmd_set_callback(request, netcat_atcmd_callback, NULL);
        xbee_cmd_send(request);
    }

    return request;
}

/// Bitfields for \a flags parameter to netcat_simple()
#define NETCAT_FLAG_CRLF        (1<<0)  ///< send CR/LF for EOL sequence
#define NETCAT_FLAG_LISTEN      (1<<1)  ///< listen for incoming connections
#define NETCAT_FLAG_UDP         (1<<2)  ///< use UDP instead of TCP
/**
    @brief
    Pass data between a socket and stdout/stdin.

    @param[in]  xbee            XBee device for connection
    @param[in]  flags           One or more of the following:
        NETCAT_FLAG_CRLF        send CR/LF for EOL sequence
        NETCAT_FLAG_LISTEN      listen for incoming connection
        NETCAT_FLAG_UDP         use UDP instead of TCP
    @param[in]  remote_host     Remote host for connection, if not listening.
    @param[in]  remote_port     Port of remote host, if not listening.

    @retval     EXIT_SUCCCESS   clean exit from function
    @retval     EXIT_FAILURE    exited due to some failure
*/
int netcat_sample(xbee_dev_t *xbee, unsigned flags, uint16_t source_port,
                  const char *remote_host, uint16_t remote_port)
{
    int result;
    int last_ai = -1;
    char linebuf[256];

    xbee_term_console_init();
    xbee_term_set_color(SOURCE_STATUS);

    netcat_state = NETCAT_STATE_INIT;
    fprintf(stderr, "Waiting for Internet connection...\n");
    while (netcat_state < NETCAT_STATE_DONE) {
        result = xbee_dev_tick(xbee);
        if (result < 0) {
            printf("Error %d calling xbee_dev_tick().\n", result);
            return EXIT_FAILURE;
        }
        switch (netcat_state) {
        case NETCAT_STATE_INIT:
            // fall through to sending ATAI to check for online status
        case NETCAT_STATE_SEND_ATAI:
            if (netcat_atcmd(xbee, "AI") >= 0) {
                netcat_state = NETCAT_STATE_WAIT_FOR_ATAI;
            }
            break;

        case NETCAT_STATE_WAIT_FOR_ATAI:
            break;

        case NETCAT_STATE_GOT_ATAI:
            // default action is to send ATAI until we get a 0 response
            netcat_state = NETCAT_STATE_SEND_ATAI;
            if (last_ai != netcat_atai) {
                if (netcat_atai == 0) {
                    fprintf(stderr, "Cellular module connected...\n");
                    netcat_state = NETCAT_STATE_SEND_ATMY;
                } else {
                    fprintf(stderr, "ATAI change from 0x%02X to 0x%02X\n",
                            last_ai, netcat_atai);
                    last_ai = netcat_atai;
                }
            }
            break;

        case NETCAT_STATE_SEND_ATMY:
            if (netcat_atcmd(xbee, "MY") >= 0) {
                netcat_state = NETCAT_STATE_WAIT_FOR_ATMY;
            }
            break;

        case NETCAT_STATE_WAIT_FOR_ATMY:
            break;

        case NETCAT_STATE_GOT_ATMY:
            xbee_ipv4_ntoa(linebuf, htobe32(netcat_atmy));
            fprintf(stderr, "My IP is %s\n", linebuf);
            int proto = flags & NETCAT_FLAG_UDP ? XBEE_SOCK_PROTOCOL_UDP
                                                : XBEE_SOCK_PROTOCOL_TCP;
            netcat_socket = xbee_sock_create(xbee, proto,
                                             netcat_sock_notify_cb);
            netcat_state = NETCAT_STATE_WAIT_FOR_CREATE;
            break;

        case NETCAT_STATE_WAIT_FOR_CREATE:
            break;

        case NETCAT_STATE_CREATED:
            xbee_term_set_color(SOURCE_STATUS);
            fprintf(stderr, "Created socket...\n");
            if (flags & NETCAT_FLAG_LISTEN) {
                if (flags & NETCAT_FLAG_UDP) {
                    // bind UDP socket
                    fprintf(stderr, "Binding to port %u...", source_port);
                    result = xbee_sock_bind(netcat_socket, source_port,
                                            netcat_sock_receive_from_cb);
                } else {
                    // listen for TCP connections
                    netcat_listen = netcat_socket;
                    netcat_socket = 0;
                    fprintf(stderr, "Listening on port %u...", source_port);
                    result = xbee_sock_listen(netcat_listen, source_port,
                                              netcat_sock_ipv4_client_cb);
                }
            } else {
                fprintf(stderr, "Connecting to %s:%u...",
                        remote_host, remote_port);
                result = xbee_sock_connect(netcat_socket, remote_port, 0,
                                           remote_host, netcat_sock_receive_cb);
            }

            if (result) {
                fprintf(stderr, "error %d.\n", result);
                netcat_state = NETCAT_STATE_ERROR;
            } else {
                fprintf(stderr, "success.\n");
                netcat_state = NETCAT_STATE_WAIT_FOR_ATTACH;
            }
            break;

        case NETCAT_STATE_WAIT_FOR_ATTACH:
            break;

        case NETCAT_STATE_WAIT_FOR_CONNECT:
            break;

        case NETCAT_STATE_CONNECTED:
            xbee_term_set_color(SOURCE_STATUS);
            fprintf(stderr, "Connected to %s:%u\n", remote_host, remote_port);
            xbee_term_set_color(SOURCE_KEYBOARD);
            netcat_state = NETCAT_STATE_INTERACTIVE;
            break;

        case NETCAT_STATE_INTERACTIVE:
            // leave room at end of linebuf for CRLF or LF
            result = xbee_readline(linebuf, sizeof linebuf - 2);
            if (result == -ENODATA) { // reached EOF
                xbee_term_set_color(SOURCE_STATUS);
                fprintf(stderr, "(got EOF, waiting for socket closure)\n");
                netcat_state = NETCAT_STATE_WAIT_FOR_CLOSE;
            } else if (result >= 0) { // send data to remote
                if (flags & NETCAT_FLAG_CRLF) {
                    linebuf[result++] = '\r';
                }
                linebuf[result++] = '\n';

                if ((flags & NETCAT_FLAG_UDP) && (flags & NETCAT_FLAG_LISTEN)) {
                    // bound UDP socket uses sendto
                    xbee_sock_sendto(netcat_socket, 0,
                                     netcat_last_datagram.addr,
                                     netcat_last_datagram.port,
                                     linebuf, result);
                } else {
                    xbee_sock_send(netcat_socket, 0, linebuf, result);
                }
            }
            break;

        case NETCAT_STATE_WAIT_FOR_CLOSE:
            // drain any input from keyboard/stdin
            xbee_readline(linebuf, sizeof linebuf);
            break;

        case NETCAT_STATE_DONE:
            break;

        case NETCAT_STATE_ERROR:
            break;
        }
    }

    xbee_term_console_restore();
    return netcat_state == NETCAT_STATE_ERROR ? EXIT_FAILURE : EXIT_SUCCESS;
}

void print_help(void)
{
    puts("XBee Netcat");
    puts("Usage: xbee_netcat [options] [hostname] [port]");
    puts("");
    puts("  -b baudrate           Baudrate for XBee interface (default 115200)");
    puts("  -C                    Use CRLF for EOL sequence");
    puts("  -h                    Display this help screen");
    puts("  -p port               Specify source port to bind to/listen on");
    puts("  -u                    Use UDP instead of default TCP");
    puts("  -x serialport         Serial port for XBee interface");
    puts("                        ('COMxx' for Win32, '/dev/ttySxx' for POSIX)");
    puts("");
}

// returns ULONG_MAX if unable to completely parse <str> or <str> is empty
unsigned long parse_num(const char *str)
{
    char *endptr;
    unsigned long parsed_num = strtoul(str, &endptr, 0);

    return *str == '\0' || *endptr != '\0' ? ULONG_MAX : parsed_num;
}

int main(int argc, char *argv[])
{
    xbee_dev_t xbee_dev;
    int c;

    // parsed command-line options with default values
    xbee_serial_t xbee_serial = { .baudrate = 115200 };
    int source_port = -1;
    unsigned opt_flags = 0;
    int parse_error = 0;

    while ( (c = getopt(argc, argv, "b:Chlp:ux:")) != -1) {
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

        case 'p': {
            unsigned long param = parse_num(optarg);
            if (param > 0 && param <= 0xFFFF) {
                source_port = (int)param;
                opt_flags |= NETCAT_FLAG_LISTEN;
            } else {
                fprintf(stderr, "ERROR: failed to parse source port '%s'\n",
                        optarg);
                ++parse_error;
            }
            break;
        }

        case 'C':
            opt_flags |= NETCAT_FLAG_CRLF;
            break;

        case '?':
            ++parse_error;
            // fall through to printing help
        case 'h':
            print_help();
            break;

        case 'u':
            opt_flags |= NETCAT_FLAG_UDP;
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

        default:
            fprintf(stderr, "ERROR: unknown option '%c'\n", c);
            ++parse_error;
        }
    }

    const char *hostname = NULL;
    uint16_t remote_port = 0;
    if ((opt_flags & NETCAT_FLAG_LISTEN) == 0) {
        // parse non-option arguments (hostname and remote port)
        if (optind + 2 > argc) {
            fprintf(stderr, "ERROR: insufficient arguments\n");
            ++parse_error;
        } else {
            hostname = argv[optind++];

            unsigned long param = parse_num(argv[optind]);
            if (param > 0 && param <= 0xFFFF) {
                remote_port = (uint16_t)param;
            } else {
                fprintf(stderr, "ERROR: failed to parse port '%s'\n", argv[optind]);
                ++parse_error;
            }
            ++optind;
        }
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

    // make sure there aren't any open sockets on <xbee> from a previous program
    xbee_sock_reset(&xbee_dev);

    int result = netcat_sample(&xbee_dev, opt_flags, source_port,
                               hostname, remote_port);

    fprintf(stderr, "Closing serial port...\n");
    xbee_ser_close(&xbee_serial);

    return result;
}

const xbee_dispatch_table_entry_t xbee_frame_handlers[] = {
    XBEE_FRAME_HANDLE_LOCAL_AT,
    XBEE_SOCK_FRAME_HANDLERS,
    XBEE_FRAME_TABLE_END
};
