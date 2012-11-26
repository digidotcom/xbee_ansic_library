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

/*
	A simple terminal emulator for communicating with an XBee module.  Include
	this file, and the platform-specific console functions in
	samples/<platform>/xbee_term_<platform>.c to make use of the xbee_term()
	function as part of another program.
 */

#include <ctype.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/serial.h"

#include "parse_serial_args.h"
#include "_xbee_term.h"

void set_color( enum char_source new_source)
{
	static enum char_source last_source = SOURCE_UNKNOWN;

	if (last_source == new_source)
	{
		return;
	}
	last_source = new_source;

	xbee_term_set_color( last_source);
}

void check_cts( xbee_serial_t *port)
{
	static int last_cts = -1;
	int cts;

	cts = xbee_ser_get_cts( port);
	if (cts >= 0 && cts != last_cts)
	{
		last_cts = cts;
		set_color( SOURCE_STATUS);
		printf( "[CTS %s]", cts ? "set" : "cleared");
		fflush( stdout);
	}
}

enum rts_setting { RTS_ASSERT, RTS_DEASSERT, RTS_TOGGLE, RTS_UNKNOWN };
void set_rts( xbee_serial_t *port, enum rts_setting new_setting)
{
	static enum rts_setting current_setting = RTS_UNKNOWN;
	bool_t rts;

	if (new_setting == RTS_TOGGLE)
	{
		new_setting = (current_setting == RTS_ASSERT ? RTS_DEASSERT : RTS_ASSERT);
	}

	if (new_setting != current_setting)
	{
		current_setting = new_setting;
		rts = (current_setting == RTS_ASSERT);
		xbee_ser_set_rts( port, rts);

		set_color( SOURCE_STATUS);
		printf( "[%s RTS]", rts ? "set" : "cleared");
		fflush( stdout);
	}
}

enum break_setting { BREAK_ENTER, BREAK_CLEAR, BREAK_TOGGLE };
void set_break( xbee_serial_t *port, enum break_setting new_setting)
{
	static enum break_setting current_setting = BREAK_CLEAR;
	bool_t set;

	if (new_setting == BREAK_TOGGLE)
	{
		new_setting =
					(current_setting == BREAK_CLEAR ? BREAK_ENTER : BREAK_CLEAR);
	}

	if (new_setting != current_setting)
	{
		current_setting = new_setting;
		set = (current_setting == BREAK_ENTER);
		xbee_ser_break( port, set);

		set_color( SOURCE_STATUS);
		printf( "[%s break]", set ? "set" : "cleared");
		fflush( stdout);
	}
}

void print_baudrate( xbee_serial_t * port)
{
	set_color( SOURCE_STATUS);
	printf( "[%" PRIu32 " bps]", port->baudrate);
	fflush( stdout);
}

void next_baudrate( xbee_serial_t *port)
{
	uint32_t rates[] = { 9600, 19200, 38400, 57600, 115200 };
	int i;

	for (i = 0; i < _TABLE_ENTRIES( rates); ++i)
	{
		if (port->baudrate == rates[i])
		{
			++i;
			break;
		}
	}

	// At end of loop, if port was using a rate not in the list above,
	// the code will end up setting the baudrate to 9600.

	xbee_ser_baudrate( port, rates[i % _TABLE_ENTRIES( rates)]);
	print_baudrate( port);
}

void dump_serial_data( const char *buffer, int length)
{
	/// Set to TRUE if last character was a carriage return with line feed
	/// automatically appended.  When TRUE, we ignore a linefeed character.
	/// So CR -> CRLF and CRLF -> CRLF.
	static bool_t ignore_lf = FALSE;
	int i, ch;

	set_color( SOURCE_SERIAL);
	for (i = length; i; ++buffer, --i)
	{
		ch = *buffer;
		if (ch == '\r')
		{
			puts( "");				// automatically insert a line feed
		}
		else if (ch != '\n' || !ignore_lf)
		{
			putchar( ch);
		}
		ignore_lf = (ch == '\r');
	}
	fflush( stdout);
}

#define CTRL(x)	((x) & 0x1F)
void xbee_term( xbee_serial_t *port)
{
	int ch, retval;
	char buffer[40];

	// set up the console for nonblocking
	xbee_term_console_init();

	puts( "Simple XBee Terminal");
	puts( "CTRL-X to EXIT, CTRL-K toggles break, CTRL-R toggles RTS, "
			"TAB changes bps.");
	print_baudrate( port);
	set_rts( port, RTS_ASSERT);
	check_cts( port);
	puts( "");

	for (;;)
	{
		check_cts( port);

		ch = xbee_term_getchar();

		if (ch == CTRL('X'))
		{
			break;						// exit terminal
		}
		else if (ch == CTRL('R'))
		{
			set_rts( port, RTS_TOGGLE);
		}
		else if (ch == CTRL('K'))
		{
			set_break( port, BREAK_TOGGLE);
		}
		else if (ch == CTRL('I'))		// tab
		{
			next_baudrate( port);
		}
		else if (ch > 0)
		{
			// Pass all characters out serial port, converting LF to CR
			// since XBee expects CR for line endings.
			xbee_ser_putchar( port, ch == '\n' ? '\r' : ch);

			// Only print printable characters or CR, LF or backspace to stdout.
			if (isprint( ch) || ch == '\r' || ch == '\n' || ch == '\b')
			{
				set_color( SOURCE_KEYBOARD);
				// stdout expects LF for line endings
				putchar( ch == '\r' ? '\n' : ch);
				fflush( stdout);
			}
		}

		retval = xbee_ser_read( port, buffer, sizeof buffer);
		if (retval > 0)
		{
			dump_serial_data( buffer, retval);
		}
	}

	xbee_term_console_restore();
	puts( "");
}

