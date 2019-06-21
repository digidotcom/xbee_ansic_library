/*
 * Copyright (c) 2011-2012 Digi International Inc.,
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
	@file xbee_sxa_socket.c
   Simple XBee API, TCP sockets over ZigBee.

	An additional layer of functions for the Rabbit/Dynamic C platform.
   Implements reliable stream-based transport using the Rabbit TCP/IP
   socket API.  The TCP protocol is transformed in order to run between
   ZigBee endpoints.  This relies on custom loopback interface functionality
   in the Rabbit TCP/IP stack.

   How it works:

   For an actively opened connection, the socket is opened to 127.1.0.x
   (where x is the SXA node table index of the
   desired peer device).  Source and destination ports must currently be
   in the range 61616 to 61871 inclusive, since the port numbers (src and dest)
   must be packed into a 16-bit field.  The field used is the cluster ID,
   since cluster IDs are otherwise irrelevant for this endpoint.

   For a passively opened connection (i.e. listen), port 61616 is opened.
   Since the socket will receive connections from any interface by default,
   if you want it to only accept connections over ZigBee, specify the
   specific interface IF_LOOPBACK when opening it.

   As with any other TCP socket, you can use tcp_reserveport(61616) in order
   to allow queueing of multiple sessions (from distinct peer devices).
   (Note: you can use other ports as well for listening, up to 61871).

   Given a special loopback device packet handler registered with the normal
   loopback device driver, TCP segments sent to the above address get
   intercepted and modified to remove most of the 40 odd bytes of IP and
   TCP header, since this is not required and wasteful given the small MTU of
   the ZigBee link.  The only field in the IP header which is relevant is the
   destination IP address, which basically indexes the appropriate destination
   node.

   The TCP header is condensed as follows:
     seqnum: 16 bits
     acknum: 16 bits
     flags: 4 bits
     window: 12 bits
   These 6 octets become the condensed TCP header in the ZigBee payload for the
   endpoint.

   Sequence numbers are shortened from the normal 32 bits, by assuming that
   the peers keep track of the implied 16 MSBs.  Since no more than a few
   hundred bytes in the stream could be outstanding on the network, the seq
   and ack numbers cannot change very much and hence the additional MSBs to
   make up the full 32-bit seq numbers can be implied.  For safety with this
   truncation scheme, all socket buffers must be 32k or less.  Note that
   the MSBs of the sequence number are supplied in the initial SYN segment,
   which transports the entire normal TCP header.  In order to obviate the
   need for this level of driver to maintain the high part of the sequence
   numbers, the TCP socket handler (TCP.LIB) maintains the necessary state
   information. TCP assumes that truncated sequence numbers are used if the
   non-standard tcp_FlagTRUNC flag is set in the TCP header.  This bit is
   set by the lower level driver.

   Flags supports only SYN,FIN,ACK and RST.  Push and URG are not used.
   RST (if it appears) must be alone or with ACK in order to have its usual
   TCP meaning.  SYN+RST and FIN+RST are reserved for special use like
   path MTU discovery (TBD).

   When the SYN flag is set, no data payload is attached, but the original
   (uncondensed) TCP header is supplied as the payload.

   Window encodes the actual window: when the MSB is zero, the remaining bits
   encode the window directly, from 0 to 2047 bytes.  When the MSB is 1,
   the LSBs (in the range 1..2047) encode a multiple of 16 bytes in the
   window (hence encode window 2048..32752).  Windows larger than this encode as
   100000000000b.  When encoding from the true 16-bit window size, the next
   lower encodable value is used.

   When SYN flag is set, no data is allowed, but the full original TCP
   header is supplied.  In particular, the TCP options are
   parsed just like "real" TCP, although currently the Rabbit only uses
   the MSS.  MSS is used symmetrically, as the minimum of both side's
   MSS option.

   When reconstructing the IP and TCP headers for incoming segments, the
   library sets the source IP address to a suitable value depending on the
   peer node e.g. 127.1.0.5 if received from SXA node 5 (in its local
   node table).  The dest IP address is set to 127.1.0.0.  All other fields
   are reconstructed in a reasonable manner,
   except that IP and TCP checksums are not generated.  It is assumed to
	be superflous given the ZigBee frame CRCs.

   Determining the path MTU can be tricky.  For now, it is set to 104 octets,
   (default definition of SXA_SOCKET_MTU, which can be overridden), but with
   IP and TCP header compression, this results in a TCP MSS of 64 octets.
   Better throughput may be obtained by experimentation.  It is imperative
   with TCP to avoid fragmentation at lower layers.

   @todo  Need a better path MTU discovery mechanism.

*/

/*** BeginHeader */
#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/sxa.h"

#use "dcrtcp.lib"

#if !(USE_LOOPBACK+0)
	#error "Use of sxa_socket requires TCPCONFIG 11/15 or equivalent (USE_LOOPBACK)"
#endif
#if (LOOPBACK_HANDLERS+0)<2
	#error "Use of sxa_socket requires LOOPBACK_HANDLERS >= 2"
#endif

/*** EndHeader */

/*** BeginHeader _sxa_socket_tx_handler */
/*** EndHeader */
_xbee_sxa_debug
int _sxa_socket_tx_handler(struct LoopbackHandler __far * lh, ll_Gather * g)
{
	char seg[SXA_SOCKET_MTU];
	int status;
	wpan_envelope_t env;
   const sxa_node_t FAR *sxa;
   in_Header FAR *ip;
   tcp_Header FAR *tp;
   uint8_t index;
   _sxa_sock_hdr_t *ssh;
   uint8_t *data;
   uint16_t w;
   uint16_t length;
   uint16_t tplen;
   uint16_t srcPort, dstPort;

   // First, make sure it's TCP over IP.  This handler only gets called from
   // the loopback interface device when it already knows it's an IP header.
   // (since the 2nd octet of the IP dest encodes the handler number itself).
   // Note that the stack guarantees all the headers are in the first extent
   // (data1) of g and, furthermore, g->data2 points to the first part of the
   // TCP payload itself.
	ip = (in_Header FAR *)g->data1;
   if (ip->proto != TCP_PROTO)
   {
   	return -1;	// Not TCP
   }
	index = *((uint8_t FAR *)&ip->destination + 3);	// Get 4th octet (net byte order)
   sxa = sxa_node_by_index( index);
   if (!sxa)
   {
   	return -1;	// No such destination
   }
	tp = (tcp_Header FAR *)((char FAR *)ip + in_GetHdrlenBytes(ip));
   srcPort = ntohs(tp->srcPort);
   dstPort = ntohs(tp->dstPort);
	if (srcPort < SXA_STREAM_PORT || srcPort >= SXA_STREAM_PORT+256 ||
       dstPort < SXA_STREAM_PORT || dstPort >= SXA_STREAM_PORT+256)
   {
   	return -1;	// Can't do ports outside this range
   }
   if (g->len2 + g->len3 > SXA_SOCKET_MTU - sizeof(*ssh))
   {
   	return -1;	// Can't do fragmentation at this point.  MSS too long.
   }
   tplen = tcp_GetDataOffset(tp) << 2;
   ssh = (_sxa_sock_hdr_t *)seg;
	data = (uint8_t *)(ssh+1);

   // set flags field with TCP flags and encoded window
   w = ntohs(tp->window);
   if (w >= _SXA_SOCK_WIN_RANGE<<_SXA_WIN_HIGH_RANGE_SHIFT)
   	ssh->flags = _SXA_SOCK_WIN_RANGE;
   else if (w >= _SXA_SOCK_WIN_RANGE)
		ssh->flags = _SXA_SOCK_WIN_RANGE | w>>_SXA_WIN_HIGH_RANGE_SHIFT;
   else
   	ssh->flags = w;
   w = ntohs(tp->flags);
	if (w & tcp_FlagSYN)
   	ssh->flags |= _SXA_SOCK_SYN;
	if (w & tcp_FlagFIN)
   	ssh->flags |= _SXA_SOCK_FIN;
	if (w & tcp_FlagACK)
   	ssh->flags |= _SXA_SOCK_ACK;
	if (w & tcp_FlagRST)
   	ssh->flags |= _SXA_SOCK_RST;

   // Set sequence and ack numbers (truncated)
   ssh->seqnum = (uint16_t)ntohl(tp->seqnum);
   ssh->acknum = (uint16_t)ntohl(tp->acknum);

   if (ssh->flags & _SXA_SOCK_SYN)
   {
   	// SYN, so copy full TCP header (i.e. including options fields)
      // to "payload" part of packet.
      if (tplen > SXA_SOCKET_MTU - sizeof(*ssh))
      {
      	return -1;	// Strange, the header is enormous!
      }
      _f_memcpy(data, tp, tplen);
      length = sizeof(*ssh) + tplen;
   }
	else
   {
	   // Copy data payload (g->data2/len2 and possibly g->data3/len3)
	   _f_memcpy(data, g->data2, g->len2);
	   _f_memcpy(data + g->len2, g->data3, g->len3);
      length = sizeof(*ssh) + g->len2 + g->len3;
   }

	wpan_envelope_create( &env, &sxa->xbee->wpan_dev, &sxa->id.ieee_addr_be,
		sxa->id.network_addr);
	env.payload = ssh;
	env.length = length;

	env.profile_id = WPAN_PROFILE_DIGI;
	env.source_endpoint = env.dest_endpoint = WPAN_ENDPOINT_DIGI_STREAM;
   // Source and destination port numbers are packed into the cluseter ID
	env.cluster_id = (srcPort-SXA_STREAM_PORT)<<8 | (dstPort-SXA_STREAM_PORT);

#ifdef SXA_SOCKET_VERBOSE
	printf( "SXA Socket: %u bytes to '%ls':\n", env.length, sxa->id.node_info);
	hex_dump( env.payload, env.length, HEX_DUMP_FLAG_TAB);
#endif

	status = wpan_envelope_send( &env);
   // When wpan_envelope_send() returns, the packet has been moved to the
   // serial port output buffer, so env and the payload do not have to be static.

	return status ? -1 : 0;
}


/*** BeginHeader _sxa_socket_rx_handler */
/*** EndHeader */
int _sxa_socket_rx_handler( const wpan_envelope_t FAR *envelope,
									 wpan_ep_state_t	FAR	*ep_state)
{
	uint8_t headers[80];
   xbee_node_id_t ni;
   in_Header *ip;
   tcp_Header *tp;
	sxa_node_t FAR *sxa;
   _sxa_sock_hdr_t FAR *ssh;
   uint8_t FAR *data;
   uint16_t length;
   uint16_t w;
   uint16_t ra;
   ll_Gather g;
   uint16_t srcPort, dstPort;


	sxa = sxa_node_by_addr( &envelope->ieee_address);
#ifdef SXA_SOCKET_VERBOSE
	printf( "SXA Socket: %u bytes from ", envelope->length);
	if (sxa != NULL)
	{
		printf( "'%ls':\n", sxa->id.node_info);
	}
	else
	{
		printf( "0x%04x:\n", envelope->network_address);
	}
	hex_dump( envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
#endif
	if (!sxa)
   {
   	// Peer node is not in our table.  Add new entry based on the
      // available information.  The unknown stuff can be filled in
      // if an ATND is issued.
      ni.ieee_addr_be = envelope->ieee_address;
      ni.network_addr = envelope->network_address;
      ni.parent_addr = WPAN_NET_ADDR_UNDEFINED;
      ni.device_type = 0xFF;	// unknown
      ni.node_info[0] = 0;
   	sxa = sxa_node_add( (xbee_dev_t *)envelope->dev, &ni);
		if (!sxa)
      {
      	return 0;	// should never happen
      }
      _sxa_set_cache_status(sxa, NULL, SXA_CACHED_NODE_ID, _SXA_CACHED_UNKNOWN);
   }
   srcPort = (envelope->cluster_id>>8)+SXA_STREAM_PORT;
   dstPort = (envelope->cluster_id&0xFF)+SXA_STREAM_PORT;
   length = envelope->length;
   ssh = (_sxa_sock_hdr_t FAR *)envelope->payload;
   if (length < sizeof(*ssh))
   {
   	return 0;	// Too short to be legitimate
   }
   data = (uint8_t FAR *)ssh + sizeof(*ssh);

   // Reconstruct fake IP and somewhat fake TCP headers
   ip = (in_Header *)headers;
   tp = (tcp_Header *)(ip + 1);
   g.data1 = headers;
   g.len3 = 0;

   memset(headers, 0, sizeof(*ip) + sizeof(*tp));
   ip->ver_hdrlen = 0x45;
   ip->ttl = 1;
   ip->proto = TCP_PROTO;
   ip->source = htonl(IPADDR(127,1,0,sxa->index));
	ip->destination = htonl(IPADDR(127,1,0,0));
   if (ssh->flags & _SXA_SOCK_SYN)
   {
   	// If SYN flag, payload is original TCP header
      if (length > sizeof(headers)-sizeof(*ip)+sizeof(*ssh))
      {
      	return 0;	// Hack! TCP header ginormous.
      }
      _f_memcpy(tp, data, length - sizeof(*ssh));
   	g.len1 = length - sizeof(*ssh) + sizeof(*ip);
      g.len2 = 0;
      ip->length = htons(g.len1);
#ifdef SXA_SOCKET_VERBOSE
		printf( "SXA Socket: got SYN (hdrlen=%d)\n", g.len1);
#endif
   }
   else
   {
   	g.len1 = sizeof(*ip) + sizeof(*tp);
		g.data2 = data;
      g.len2 = length - sizeof(*ssh);
		ip->length = htons(g.len2 + g.len1);

      tp->srcPort = htons(srcPort);
      tp->dstPort = htons(dstPort);
		tp->seqnum = htonl(ssh->seqnum);
		tp->acknum = htonl(ssh->acknum);
      // Set the non-RFC793 flag to indicate shortened seq numbers...
      tp->flags = htons(tcp_FlagTRUNC | (sizeof(*tp)>>2)<<12);
      if (ssh->flags & _SXA_SOCK_FIN)
			tp->flags |= htons(tcp_FlagFIN);
      if (ssh->flags & _SXA_SOCK_ACK)
			tp->flags |= htons(tcp_FlagACK);
      if (ssh->flags & _SXA_SOCK_RST)
			tp->flags |= htons(tcp_FlagRST);
		ra = ssh->flags & _SXA_SOCK_WIN_RANGE;
      w = ssh->flags & _SXA_SOCK_WIN_COUNT;
      if (ra && !w)
      	tp->window = htons(_SXA_SOCK_WIN_RANGE<<_SXA_WIN_HIGH_RANGE_SHIFT);
      else if (ra)
      	tp->window = htons(w << _SXA_WIN_HIGH_RANGE_SHIFT);
      else
      	tp->window = htons(w);
#ifdef SXA_SOCKET_VERBOSE
		printf( "SXA Socket: got seq=%lu ack=%lu win=%u hdrlen=%u datalen=%u\n",
      			ntohl(tp->seqnum), ntohl(tp->acknum), ntohs(tp->window),
               g.len1, g.len2);
#endif
   }
   // Copy to network receive buffers
	loopback_stowpacket(&g);

	return 0;
}




/*** BeginHeader sxa_socket_init */
/*** EndHeader */
_xbee_sxa_debug
int sxa_socket_init(void)
{
	_lodata[0].loh[LOH_XBEE_STREAM].sendpacket = _sxa_socket_tx_handler;
   _lodata[0].loh[LOH_XBEE_STREAM].mtu = SXA_SOCKET_MTU;
	return 0;
}


