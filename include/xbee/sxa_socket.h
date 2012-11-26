/*
 * Copyright (c) 2010-2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

/**
	@addtogroup SXA
	@{
	@file xbee/sxa_socket.h
	Header for Simplified XBee API TCP sockets over ZigBee

*/
#ifndef __DC__
	#error "This header is only supported on the Rabbit platform with Dynamic C."
#endif

#ifndef __XBEE_SXA_SOCKET
#define __XBEE_SXA_SOCKET

#include "xbee/sxa.h"

XBEE_BEGIN_DECLS

#ifndef SXA_SOCKET_MTU
	/**
		@brief Network interface Maximum Transmission Unit, as seen by TCP stack.

   	Selection of this constant is a bit tricky.  ZigBee has a small MTU,
      however the XBee modules accept larger payloads and fragments them
      if necessary when transmitting.  TCP likes to avoid fragmentation and
      tries to select an MSS (TCP payload size) as large as possible, but not
      so large as to require any fragmentation along the transmission path.

      There is no reliable method to set the MSS, because the ZigBee MTU
      depends on various header options such as encryption and source
      routing.  Because of this, we select a conservative value for the
      TCP MSS of 64 bytes.  To this, we add 40 bytes (for the IP and TCP
      headers) to get an MTU of 104, since TCP computes its MSS by assuming
      IP and TCP headers are required for each segment.  This MTU of 104
      exceeds that for ZigBee, however out implementation of SXA sockets
      effectively compresses the IP and TCP headers into a 6-byte header.
      This gives a real MTU of 64+6 = 70 bytes.  This is an acceptable
      envelope payload size, which will avoid fragmentation.

      Thus, whatever this constant is, the real MTU will be 34 bytes
      less than this, and the TCP MSS will be 40 bytes less.
   */
	#define SXA_SOCKET_MTU	104
#endif

/// Macro to generate the appropriate "loopback" IP address, i.e. the
/// IP address to use to talk to a particular remote node defined by its
/// SXA node table index.
#define SXA_INDEX_TO_IP(index) IPADDR(127,1,0,index)

/// Macro to use for first and last valid TCP port numbers, for
/// both source port and destination port.  This range is selected
/// since port numbers must compress into 8-bit fields.
#define SXA_STREAM_PORT				UINT16_C(61616)
#define SXA_MAX_STREAM_PORT		UINT16_C(61616+255)

/// Provisional endpoint for reliable stream
#define WPAN_ENDPOINT_DIGI_STREAM	0x66

// Since this is Rabbit-specific, all the following fields are sent
// little-endian over the air.  This struct forms the condensed
// TCP/IP header, which forms the first 6 octets of the WPAN envelope
// payload.
typedef struct _sxa_sock_hdr_t
{
	uint16_t		seqnum;	///< shortened sequence number
	uint16_t		acknum;	///< shortened acknowledgment number
	uint16_t		flags;	///< flags plus encoded window
#define _SXA_SOCK_SYN	0x8000
#define _SXA_SOCK_FIN	0x4000
#define _SXA_SOCK_ACK	0x2000
#define _SXA_SOCK_RST	0x1000
#define _SXA_SOCK_FLAG_MASK	0xF000
#define _SXA_SOCK_WIN_MASK		0x0FFF
#define _SXA_SOCK_WIN_RANGE	0x0800
#define _SXA_SOCK_WIN_COUNT	0x07FF
#define _SXA_WIN_LOW_RANGE_SHIFT		1
#define _SXA_WIN_HIGH_RANGE_SHIFT	4
} _sxa_sock_hdr_t;

// Definitions in TCP libraries
struct ll_Gather;
struct LoopbackHandler;

int _sxa_socket_tx_handler(struct LoopbackHandler __far * lh, ll_Gather * g);
int _sxa_socket_rx_handler( const wpan_envelope_t FAR *envelope,
									 wpan_ep_state_t	FAR	*ep_state);
int sxa_socket_init(void);

/// Place this entry in endpoint table to enable SXA sockets
#define SXA_SOCKET_ENDPOINT { \
        WPAN_ENDPOINT_DIGI_STREAM,	\
        WPAN_PROFILE_DIGI,				\
        _sxa_socket_rx_handler,		\
        NULL,								\
        0x0000,							\
        0x00,								\
        NULL }

XBEE_END_DECLS

#use "xbee_sxa_socket.c"


#endif		// __XBEE_SXA_SOCKET

//@}
