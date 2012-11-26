/*
 * Copyright (c) 2012 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#ifndef XBEE_TERM_H_
#define XBEE_TERM_H_

#include "xbee/platform.h"

enum char_source {
	SOURCE_UNKNOWN,				// startup condition
	SOURCE_KEYBOARD,				// from user entering data at keyboard
	SOURCE_SERIAL,					// from XBee on serial port
	SOURCE_STATUS,					// status messages
};

// Platform-specific functions used by xbee_term.c.
void xbee_term_console_restore( void);
void xbee_term_console_init( void);
void xbee_term_set_color( enum char_source source);
int xbee_term_getchar( void);

// open an interactive terminal to \a port
void xbee_term( xbee_serial_t *port);

#endif /* XBEE_TERM_H_ */
