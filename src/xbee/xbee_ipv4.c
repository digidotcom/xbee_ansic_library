/*
 * Copyright (c) 2017 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
 
#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/ipv4.h"

int xbee_ipv4_ntoa(char buffer[16], uint32_t ip_be)
{
	if (!buffer)
		return -EINVAL;

	uint8_t *octets = (uint8_t *) &ip_be;
	sprintf(buffer, "%u.%u.%u.%u", octets[0], octets[1], octets[2], octets[3]);
	return 0;
}

int xbee_ipv4_aton(const char *cp, uint32_t *ip_be)
{
	uint8_t octets[4];

	for (size_t i = 0; i < 4; i++) {
		if (i) {
			cp += 1; // Skip separator
		}

		char *end;
		long octet = strtol(cp, &end, 10);
		if (octet < 0 || octet > UINT8_MAX // Out of range
				|| end == cp // No digits
				|| (*end && *end != '.')) { // Junk before separator
			return -EINVAL;
		}
		octets[i] = octet;

		cp = end;
	}

	*ip_be = xbee_get_unaligned32(octets);
	return 0;
}

char *xbee_ipv4_protocol_str(char buffer[8], uint8_t protocol)
{
	if (buffer) {
		switch (protocol) {
			case XBEE_IPV4_PROTOCOL_UDP:
				strcpy(buffer, "UDP");
				break;
			case XBEE_IPV4_PROTOCOL_TCP:
				strcpy(buffer, "TCP");
				break;
			case XBEE_IPV4_PROTOCOL_SSL:
				strcpy(buffer, "SSL");
				break;
			default:
				sprintf(buffer, "[P%u]", protocol);
		}
	}
	
	return buffer;
}

int xbee_ipv4_envelope_reply(xbee_ipv4_envelope_t FAR *reply,
	const xbee_ipv4_envelope_t FAR *original)
{
	if (!reply || !original) {
		return -EINVAL;
	}
	
	_f_memcpy(reply, original, offsetof(xbee_ipv4_envelope_t, options));
	_f_memset(&reply->options, 0x00, sizeof *reply - offsetof(xbee_ipv4_envelope_t, options));
	
	return 0;
}


int xbee_ipv4_envelope_send(const xbee_ipv4_envelope_t FAR *envelope)
{
	xbee_dev_t *xbee;
	xbee_header_transmit_ipv4_t header;
	int retval;
	
	if (envelope == NULL || envelope->xbee == NULL) {
		return -EINVAL;
	}
	
	if (envelope->length > XBEE_IPV4_MAX_PAYLOAD) {
		return -E2BIG;
	}
	
	xbee = (xbee_dev_t *) envelope->xbee;
	
	header.frame_type = XBEE_FRAME_TRANSMIT_IPV4;
	header.frame_id = xbee_next_frame_id(xbee);
	header.remote_addr_be = envelope->remote_addr_be;
	header.remote_port_be = htobe16(envelope->remote_port);
	header.local_port_be = htobe16(envelope->local_port);
	header.protocol = envelope->protocol;
	header.options = envelope->options;
	

	#ifdef XBEE_IPV4_VERBOSE
		char protocol_string[8];
		char buf_addr[16];
		xbee_ipv4_protocol_str(protocol_string, envelope->protocol)
		printf( "%s: %u bytes to %s "	\
			"(src_port=0x%04x dest_port=0x%04x prot=%s opt=%02x)\n",
			__FUNCTION__, envelope->length, 
			xbee_ipv4_ntoa(buf_addr, envelope->remote_addr_be),
			envelope->remote_port, envelope->local_port,
			protocol_string, envelope->options);
	#endif
	
	retval = xbee_frame_write( xbee, &header, sizeof(header), 
				envelope->payload, envelope->length, 0);
	
	#ifdef XBEE_IPV4_VERBOSE
		printf( "%s: %s returned %d\n", __FUNCTION__, "xbee_frame_write", retval);
	#endif
	return retval;
}


void xbee_ipv4_envelope_dump(const xbee_ipv4_envelope_t FAR *envelope,
	bool_t include_payload)
{
	if (!envelope) {
		return;
	}

	char buf_proto[8];
	char buf_addr[16];
    xbee_ipv4_ntoa(buf_addr, envelope->remote_addr_be);
	
	printf("Message from %s:%u to %s port %u, %u bytes\n",
		buf_addr,
		envelope->remote_port,
		xbee_ipv4_protocol_str(buf_proto, envelope->protocol),
		envelope->local_port,
		envelope->length);
	if (include_payload) {
		hex_dump(envelope->payload, envelope->length, HEX_DUMP_FLAG_TAB);
	}
}


int xbee_ipv4_receive_dump(xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	const xbee_frame_receive_ipv4_t FAR *ipv4 = raw;
	xbee_ipv4_envelope_t envelope;
	
	envelope.xbee = xbee;
	envelope.remote_addr_be = ipv4->remote_addr_be;
	envelope.local_port = be16toh(ipv4->local_port_be);
	envelope.remote_port = be16toh(ipv4->remote_port_be);
	envelope.protocol = ipv4->protocol;
	envelope.payload = ipv4->payload;
	envelope.length = length - offsetof(xbee_frame_receive_ipv4_t, payload);
	
	xbee_ipv4_envelope_dump(&envelope, TRUE);
	
	return 0;
}
