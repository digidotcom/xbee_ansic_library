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

#include <stdio.h>
#include <string.h>
#include "xbee/platform.h"
#include "xbee/sms.h"

int xbee_sms_send(xbee_dev_t *xbee, const char *phone, const char *message,
	uint16_t options)
{
	if (!xbee || !phone || !message) {
		return -EINVAL;
	}

	size_t ph_length = strlen(phone);
	size_t msg_length = strlen(message);
	int retval;
	xbee_header_transmit_sms_t header;
	
	if ((ph_length > XBEE_SMS_MAX_PHONE_LENGTH)
		|| (msg_length > XBEE_SMS_MAX_MESSAGE_LENGTH)) {
		return -E2BIG;
	}
	
	header.frame_type = XBEE_FRAME_TRANSMIT_SMS;
	if (options & XBEE_SMS_SEND_OPT_NO_STATUS) {
		header.frame_id = 0;
	} else {
		header.frame_id = xbee_next_frame_id(xbee);
	}
	header.options = options & 0xFF;
	strncpy(header.phone, phone, XBEE_SMS_MAX_PHONE_LENGTH);
	
	retval = xbee_frame_write( xbee, &header, sizeof(header), 
		message, msg_length, 0);
	return retval;
}

int xbee_sms_receive_dump(xbee_dev_t *xbee, const void FAR *raw,
	uint16_t length, void FAR *context)
{
	const xbee_frame_receive_sms_t FAR *sms = raw;
	
	printf("Message from %.*s:\n%.*s\n",
		XBEE_SMS_MAX_PHONE_LENGTH,
		sms->phone,
		(int) (length - offsetof(xbee_frame_receive_sms_t, message)),
		sms->message);
	return 0;
}
