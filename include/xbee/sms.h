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

/**
	@addtogroup xbee_sms
	@{
	@file xbee/sms.h

	Support code for XBee Cellular SMS frames.
*/

#ifndef XBEE_SMS_H
#define XBEE_SMS_H

#include "xbee/platform.h"
#include "xbee/device.h"

#if !XBEE_CELLULAR_ENABLED
	#error "XBEE_CELLULAR_ENABLED must be defined as non-zero " \
		"to use this header."
#endif

XBEE_BEGIN_DECLS

/// Transmit an SMS message. [Cellular]
#define XBEE_FRAME_TRANSMIT_SMS                     0x1F

/// Sent upon receiving an SMS message. [Cellular]
#define XBEE_FRAME_RECEIVE_SMS                      0x9F

#define XBEE_SMS_MAX_PHONE_LENGTH			20
#define XBEE_SMS_MAX_MESSAGE_LENGTH			160

/// Format of XBee API frame type 0x1F (#XBEE_FRAME_TRANSMIT_SMS);
/// sent from host to XBee.
typedef PACKED_STRUCT xbee_frame_transmit_sms_t {
	uint8_t			frame_type;				///< XBEE_FRAME_TRANSMIT_SMS (0x1F)
	uint8_t			frame_id;
	uint8_t			options;					///< reserved for future use
	/// fixed 20 bytes, unused bytes are null
	char				phone[XBEE_SMS_MAX_PHONE_LENGTH];		
	
	/// unterminated, variable-length message, length derived from frame length
	char			message[XBEE_SMS_MAX_MESSAGE_LENGTH];
} xbee_frame_transmit_sms_t;

/// Format of XBee API frame type 0x9F (#XBEE_FRAME_RECEIVE_SMS);
/// received from XBee by host.
typedef PACKED_STRUCT xbee_frame_receive_sms_t {
	uint8_t			frame_type;				///< XBEE_FRAME_RECEIVE_SMS (0x9F)
	/// fixed 20 bytes, unused bytes are null
	char				phone[XBEE_SMS_MAX_PHONE_LENGTH];

	/// unterminated, variable-length message, length derived from frame length
	char			message[1];
} xbee_frame_receive_sms_t;

typedef PACKED_STRUCT xbee_header_transmit_sms_t {
	uint8_t			frame_type;				///< XBEE_FRAME_TRANSMIT_SMS (0x1F)
	uint8_t			frame_id;
	uint8_t			options;					///< reserved for future use
	/// fixed 20 bytes, unused bytes are null
	char			phone[XBEE_SMS_MAX_PHONE_LENGTH];
} xbee_header_transmit_sms_t;

/**
	@brief Send an SMS message.
	
	@param[in]	xbee		XBee device sending the message
	@param[in]	phone		Target phone number, a null-terminated string whose
								length is at most XBEE_SMS_MAX_PHONE_LENGTH
								(not including the terminating null character).
	@param[in]	message	Message to send, null-terminated string limited to
								a length of XBEE_SMS_MAX_MESSAGE_LENGTH characters.
	@param[in]	options	Bitmask of frame options.
						XBEE_SMS_SEND_OPT_NO_STATUS - disable TX Status frame
	
	@retval	0				Message sent with XBEE_SMS_SEND_OPT_NO_STATUS.
	@retval	>0				Frame ID of message, to correlate with Transmit
								Status frame received from XBee module.
	@retval	-EINVAL		Invalid parameter passed to function.
	@retval	-E2BIG		Either phone or message too big for frame.
*/
int xbee_sms_send(xbee_dev_t *xbee, const char *phone, const char *message,
	uint16_t options);

// XBEE_SMS_SEND_OPT_xxx values from 0x0001 to 0x0080 reserved for
// xbee_frame_transmit_sms_t.options.
#define XBEE_SMS_SEND_OPT_NO_STATUS		0x8000

/**
	@brief Dump received SMS message to stdout.
	
	This is a sample frame handler that simply prints received SMS messages
	to the screen.  Include it in your frame handler table as:
	
		{ XBEE_FRAME_RECEIVE_SMS, 0, xbee_sms_receive_dump, NULL }

	See the function help for xbee_frame_handler_fn() for full
	documentation on this function's API.
*/
int xbee_sms_receive_dump(xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_sms.c"
#endif

#endif
