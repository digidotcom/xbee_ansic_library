/*
 * Copyright (c) 2009-2012 Digi International Inc.,
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
	@addtogroup xbee_atmode
	@{
	@file xbee/atmode.h
	Header for working with XBee modules in AT command mode instead of API mode.
*/

#ifndef __XBEE_ATMODE
#define __XBEE_ATMODE

#include "xbee/platform.h"
#include "xbee/device.h"

XBEE_BEGIN_DECLS

int xbee_atmode_enter( xbee_dev_t *xbee);
int xbee_atmode_exit( xbee_dev_t *xbee);
int xbee_atmode_tick( xbee_dev_t *xbee);

int xbee_atmode_send_request( xbee_dev_t *xbee, const char FAR *command);
int xbee_atmode_read_response( xbee_dev_t *xbee, char FAR *response,
	int resp_size, int FAR *bytesread);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_atmode.c"
#endif

#endif


