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
/*
	Unit test the xbee_device API layer.

	Uses Tom's RCM5700 with serial ports looped back:

		TXD/PC0 - PE7/RXE
		RXD/PC1 - PE6/TXE
		TXC/PC2 - PC5/RXB
		RXC/PC3 - PC4/TXB

	Be careful -- the pins of the through-hole connector on the DEMO and SERIAL
	add-on boards are mirrored from the pins of the inter-board connector.

	The TEST_MASTER serial port is the one that does the testing using direct
	calls to the xbee_serial layer, and TEST_SLAVE is the one that is
	"under test", using calls to the xbee_device layer.

	Assumes driver has passed UnitTest_xbee_serial.c.

*/

// Configuration:

// Enable Verbose output from TEST.LIB
#define TEST_VERBOSE

// Baud rate for tests
#define BAUD_RATE 115200

// Fake frame type to use for tests -- shouldn't be used by any layer of API
#define DUMMY_FRAME_TYPE 0xF0

#define DUMMY_CONTEXT ((void FAR *) 0xDEADBEEF)

//#define XBEE_DEVICE_VERBOSE
#define XBEE_DEVICE_DEBUG


// END OF CONFIGURATION

#if ! RCM5700_SERIES
	#warnt "not targeting the RCM5700 -- correct serial port selected in DC?"
#endif

#include "unittest_common_serial.h"
#include <xbee/device.h>

#use "test.lib"

xbee_dev_t xbee_slave, xbee_master;

// append char to buffer without overwriting null terminator
void _append( char *buffer, int bufsize, int ch)
{
	int i;
	char *p;

	p = buffer;
	i = 1;
	while (i < bufsize)
	{
		if (! *p)
		{
			*p++ = ch;
			*p = '\0';
			break;
		}
		++p;
		++i;
	}
}

// stub function used for testing calls to xbee_dev_reset
// reset _xbee_reset_buffer before call, then you'll see what reset func did
char _xbee_reset_buffer[10];
void _xbee_reset( xbee_dev_t *xbee, int mode)
{
	_append( _xbee_reset_buffer, sizeof(_xbee_reset_buffer), mode ? '1' : '0');
}

xbee_dev_t test_xbee;

void t_xbee_dev_init()
{
	xbee_dev_t xbee;
	int result;

	result = xbee_dev_init( NULL, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	test_compare( result, -EINVAL, NULL,
		"didn't return error for NULL xbee_dev_t parameter");

	result = xbee_dev_init( &xbee, -1, BAUD_RATE, NULL, NULL);
	test_compare( result, -EINVAL, NULL,
		"didn't return error for negative serial port");

	result = xbee_dev_init( &xbee, TEST_SLAVE, 0, NULL, NULL);
	test_compare( result, -EINVAL, NULL,
		"didn't return error for zero baud rate");

	result = xbee_dev_init( &xbee, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	test_compare( result, 0, NULL,
		"didn't return success for valid parameters");
}

void t_xbee_dev_reset()
{
	xbee_dev_t xbee;
	int result;

	result = xbee_dev_init( &xbee, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	test_compare( result, 0, NULL,
		"xbee_dev_init didn't return success for valid parameters (NULL reset)");

	test_compare( xbee_dev_reset( NULL), -EINVAL, NULL,
		"xbee_dev_reset didn't return error for NULL xbee_dev_t parameter");

	test_compare( xbee_dev_reset( &xbee), -EIO, NULL,
		"xbee_dev_reset didn't return error for NULL reset function pointer");

	result = xbee_dev_init( &xbee, TEST_SLAVE, BAUD_RATE, NULL, _xbee_reset);
	test_compare( result, 0, NULL,
		"xbee_dev_init didn't return success for valid parameters (valid reset)");

	// empty buffer, call xbee_dev_reset() & make sure it calls _xbee_reset twice
	*_xbee_reset_buffer = '\0';

	test_compare( xbee_dev_reset( &xbee), 0, NULL,
		"xbee_dev_reset didn't return success");

	test_compare( strcmp( _xbee_reset_buffer, "10"), 0, NULL,
		"xbee_dev_reset didn't call function pointer correctly");
}

int _handlers_used( xbee_dev_t *xbee)
{
	int i, used = 0;
	xbee_dispatch_table_entry_t *entry;

	entry = xbee->frame_handlers;
	for (i = 0; i < XBEE_FRAME_HANDLER_TABLESIZE; ++i, ++entry)
	{
		used += (entry->frame_type != 0);
	}

	return used;
}

char _xbee_handler_buffer[10];
int _dummy_handler1( const xbee_dev_t *xbee,
						const void FAR *frame, word length, void FAR *context)
{
	_append( _xbee_handler_buffer, sizeof(_xbee_handler_buffer), '1');
	return 0;
}
int _dummy_handler2( const xbee_dev_t *xbee,
						const void FAR *frame, word length, void FAR *context)
{
	_append( _xbee_handler_buffer, sizeof(_xbee_handler_buffer), '2');
	return 0;
}
void t_xbee_frame_handler_add()
{
	char buffer[80];
	xbee_dev_t xbee;
	int result;
	int free, used, i;

	result = xbee_dev_init( &xbee, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	test_compare( result, 0, NULL,
		"xbee_dev_init didn't return success for valid parameters (NULL reset)");

	used = _handlers_used( &xbee);

	test_bool( used >= 0 && used <= XBEE_FRAME_HANDLER_TABLESIZE,
		"after init, used handlers should be between 0 and TABLESIZE");

	// try some invalid settings
	result = xbee_frame_handler_add( &xbee, -1, 0, _dummy_handler1, NULL);
	test_compare( result, -EINVAL, NULL,
		"should have failed for frame type -1");

	result = xbee_frame_handler_add( &xbee, 0x0080, -1, _dummy_handler1, NULL);
	test_compare( result, -EINVAL, NULL,
		"should have failed for frame id -1");

	result = xbee_frame_handler_add( &xbee, 0x0080, 0, NULL, NULL);
	test_compare( result, -EINVAL, NULL,
		"should have failed for NULL handler");

	result = xbee_frame_handler_add( &xbee, 0, 0, _dummy_handler1, NULL);
	test_compare( result, -EINVAL, NULL,
		"should have failed for frame type == 0");
/*
	// This test is no longer applicable -- allowing any non-zero, 8-bit frame ID
	// so we can write a virtual XBee for unit testing higher API layers.
	result = xbee_frame_handler_add( &xbee, 0x0011, 0, _dummy_handler1, NULL);
	test_compare( result, -EINVAL, NULL,
		"should have failed for frame type < 0x80");
*/

	test_compare( used, _handlers_used( &xbee), NULL,
		"previous tests should not have added more handlers to table");

	free = XBEE_FRAME_HANDLER_TABLESIZE - _handlers_used( &xbee);
	test_bool( free >= 5, "need room for at least five more handlers");

	// Add a handler, and then add 4 similar handlers to make sure the dupe
	// checker considers them to be different (in each, one thing changes).
	result = xbee_frame_handler_add( &xbee, 0xFE, 0, _dummy_handler1, NULL);
	test_compare( result, 0, NULL, "dupe check: first handler failed");

	result = xbee_frame_handler_add( &xbee, 0xFD, 0, _dummy_handler1, NULL);
	test_compare( result, 0, NULL, "failed dupe check: changed frame type");

	result = xbee_frame_handler_add( &xbee, 0xFE, 1, _dummy_handler1, NULL);
	test_compare( result, 0, NULL, "failed dupe check: changed frame id");

	result = xbee_frame_handler_add( &xbee, 0xFE, 0, _dummy_handler2, NULL);
	test_compare( result, 0, NULL, "failed dupe check: changed handler");

	result = xbee_frame_handler_add( &xbee, 0xFE, 0, _dummy_handler1, buffer);
	test_compare( result, 0, NULL, "failed dupe check: changed context");


	// reset the table for the next test
	result = xbee_dev_init( &xbee, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	test_compare( result, 0, NULL, "couldn't reset table using xbee_dev_init");

	free = XBEE_FRAME_HANDLER_TABLESIZE - _handlers_used( &xbee);
	// fill the table and make sure it fails on the next entry
	for (i = 0; i < free; ++i)
	{
		sprintf( buffer, "failed to add handler %d of %d", i + 1, free);
		result = xbee_frame_handler_add( &xbee, i + 0x0080, 0, _dummy_handler1,
			NULL);
		test_compare( result, 0, NULL, buffer);

		// try adding again -- should get a duplicate error
		sprintf( buffer, "should have gotten dupe error (%d of %d)", i + 1, free);
		result = xbee_frame_handler_add( &xbee, i + 0x0080, 0, _dummy_handler1,
			NULL);
		test_compare( result, -EEXIST, NULL, buffer);
	}

	// try adding one more entry and make sure the count doesn't change
	result = xbee_frame_handler_add( &xbee, i + 0x0080, 0, _dummy_handler1, NULL);
	test_compare( result, -ENOSPC, NULL, "table should be full");

	test_compare( _handlers_used( &xbee), XBEE_FRAME_HANDLER_TABLESIZE, NULL,
		"handler table should be full at this point");
}

// some quick hard-coded tests, using pre-calculated results
struct checksum_test {
	char *str;
	char sum;
} checksum_test[] = {
	{ "", 0xFF },
	{ "T", 0xFF - 'T' },
	{ "\xFF", 0 },
	{ "The quick brown fox jumps over the lazy dog.", 248 },
	{ "Pack my box with five dozen liquor jugs.", 104 },
};
#define CHECKSUM_TEST_COUNT (sizeof checksum_test / sizeof *checksum_test)
void t_xbee_checksum()
{
	char buffer[80];
	int i;
	struct checksum_test *t;

	test_compare( _xbee_checksum( "X", 0, 0x55), 0x55, "0x%02lX",
		"adding 0 bytes to checksum should be unchanged");

	test_compare( _xbee_checksum( "\0\0\0\0", 4, 0xAA), 0xAA, "0x%02lX",
		"adding nulls to checksum should be unchanged");

	test_compare( _xbee_checksum( "\x55", 1, 0x55), 0, "0x%02lX",
		"start with 0x55, add 0x55 should result in 0x00");

	t = checksum_test;
	for (i = 0; i < CHECKSUM_TEST_COUNT; ++i, ++t)
	{
		sprintf( buffer, "failed for '%s'\n", t->str);
		test_compare( _xbee_checksum( t->str, strlen( t->str), -1), t->sum,
			"0x%02lX", buffer);
	}
}

// use badlen as the length if header or payload are null
// returns the return value from xbee_frame_write
int _write_test( char *header, char *payload, int badlen)
{
	int hlen, plen, length;
	int retval;
	char buffer[300];
	int i;
	unsigned long t0;
	int used, used_new, read;
	int fail = 0;

	hlen = header ? strlen(header) : badlen;
	plen = payload ? strlen(payload) : badlen;
	length =

	retval = xbee_frame_write( &xbee_slave, header, hlen, payload, plen, 0);
	if (!retval)
	{
		// expect data to flow through to master
		// wait until there hasn't been a change in received bytes for 100ms
		used = 0;
		t0 = MS_TIMER;
		do {
			used_new = xbee_ser_rx_used( &xbee_master);
			if (used != used_new)
			{
				used = used_new;
				t0 = MS_TIMER;
			}
		} while (MS_TIMER - t0 < 100);
		if (used > sizeof(buffer))
		{
			fail += test_fail( "received too many bytes");
		}
		else
		{
			read = xbee_ser_read( &xbee_master, buffer, sizeof(buffer));
			fail += test_compare( read, used, NULL, "didn't read all bytes");

			length = 0;
			if (header)
			{
				i = strlen( header);
				fail += test_bool( memcmp( &buffer[3+length], header, i) == 0,
					"header bytes don't match");
				length += i;
			}
			if (payload)
			{
				i = strlen( payload);
				fail += test_bool( memcmp( &buffer[3+length], payload, i) == 0,
					"payload bytes don't match");
				length += i;
			}

			fail += test_compare( *buffer, 0x7E, "0x%02ld", "wrong start byte");
			fail += test_compare( intel16(*(word *)&buffer[1]), length, "0x%04lx",
				"wrong length in frame");
			fail += test_compare( length + 4, used, NULL,
				"frame size not equal to payload + 4");
			if (length)
			{
				// check checksum of frame contents
				// assumes t_xbee_checksum has passed
				fail += test_compare( _xbee_checksum( &buffer[3], length, -1),
					buffer[3 + length], "0x%02lx", "checksum mismatch");
			}
		}

	}

	if (fail)
	{
		mem_dump( buffer, read);	// uncomment for help debugging
		sprintf( buffer, "header=[%s] payload=[%s]\n",
			(header ? header : "NULL"), (payload ? payload : "NULL"));
		test_info( buffer);
	}

	return retval;
}

void t_xbee_frame_write()
{
	int result;
	unsigned long t0;

	result = xbee_dev_init( &xbee_slave, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't open slave XBee"))
	{
		return;
	}
	result = xbee_dev_init( &xbee_master, TEST_MASTER, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't open master XBee"))
	{
		return;
	}

	// Since we'll never call xbee_dev_tick on the master XBee, we don't have to
	// worry about the default frame handlers.  We'll be calling the
	// xbee_serial API layer directly to get at the bytes.

	// hold CTS pin to make sure xbee_frame_write returns -EBUSY
	xbee_ser_set_rts( &xbee_master, 0);
	test_compare( _write_test( "foo", NULL, 0), -EBUSY, NULL,
		"should respond with -EBUSY in CTS test");

	// fill serial port buffer with junk, release CTS and call xbee_frame_write
	// immediately to confirm that it returns -EBUSY when there isn't enough
	// room for a frame in the buffer
	xbee_ser_write( &xbee_slave, t_xbee_frame_write, 1000);
	xbee_ser_set_rts( &xbee_master, 1);
	test_compare( _write_test(
		"Pack my box with five dozen liquor jugs",
		"The quick brown fox jumps over a lazy dog", 0), -EBUSY, NULL,
		"expected -EBUSY when sending large frame after filling tx buffer");

	// wait for all bytes to leave slave and burn through master
	t0 = MS_TIMER;
	do {
		if (xbee_ser_tx_used( &xbee_slave))
		{
			t0 = MS_TIMER;
		}
		xbee_ser_rx_flush( &xbee_master);
	} while (MS_TIMER - t0 < 100);

	// Send a frame as header only
	test_compare( _write_test( "foo", NULL, 0), 0, NULL,
		"couldn't send header foo");

	// Send a frame as data only
	test_compare( _write_test( NULL, "foo", 0), 0, NULL,
		"couldn't send data foo");

	// Send a frame as header and data
	test_compare( _write_test( "foo", "bar", 0), 0, NULL,
		"couldn't send header foo with data bar");

	// Should return error if sum of lengths is zero (NULL header/data should
	// be considered zero length).
	test_compare( _write_test( NULL, "", 5), -ENODATA, NULL,
		"should have failed (NULL/'')");
	test_compare( _write_test( "", NULL, 5), -ENODATA, NULL,
		"should have failed (''/NULL)");
	test_compare( _write_test( NULL, NULL, 5), -ENODATA, NULL,
		"should have failed (NULL/NULL)");
	test_compare( _write_test( "", "", 5), -ENODATA, NULL,
		"should have failed (''/'')");

	// Try sending a frame that is too large for the device and check for
	// -EMSGSIZE return value.
}

// !!! The code below here is getting big and may need to move into a separate
// test.  Maybe a single test file for non-serial tests and another one for
// the serial tests.

// dummy handler used by t_xbee_frame_load and t_xbee_frame_dispatch

struct _frame {
	int			counter;
	word			flags;
		#define T_FLAG_CALL_TICK		0x0001		// call xbee_dev_tick in handler
		#define T_FLAG_NO_RECEIVE		0x0002		// shouldn't receive the frame
		#define T_FLAG_NULL_CONTEXT	0x0004		// context should be NULL
		#define T_FLAG_DOUBLE			0x0008		// should hit twice
	char			data[512];
	word			length;
	void	FAR	*context;
} _dummy_frame;
int _dummy_load_handler( const xbee_dev_t *xbee,
						const void FAR *frame, word length, void FAR *context)
{
	char buffer[80];
	char FAR *frame_ch = (char FAR *) frame;
	void FAR *expected_context;

	if (length > 512)
	{
		sprintf( buffer, "_dummy_load_handler got big frame (%u bytes)", length);
		test_fail( buffer);
		return 0;
	}

	expected_context =
		(_dummy_frame.flags & T_FLAG_NULL_CONTEXT) ? NULL : DUMMY_CONTEXT;
	if (	test_compare( (uint32_t) context, (uint32_t) expected_context,
							"0x%08lx", "_dummy_load_handler got wrong context")
		|| test_compare( *frame_ch, DUMMY_FRAME_TYPE,
							"0x%02lx", "_dummy_load_handler got wrong frame type")
		)
	{
		sprintf( buffer, "payload=[%.*ls]", i_min(length, 40), frame);
		test_info( buffer);
	}

	_f_memcpy( _dummy_frame.data, frame, length);
	_dummy_frame.length = length;
	_dummy_frame.context = context;
	++_dummy_frame.counter;

	if (_dummy_frame.flags & T_FLAG_CALL_TICK)
	{
		test_compare( xbee_dev_tick( xbee), -EBUSY, NULL,
			"xbee_dev_tick didn't return busy");
		test_bool( xbee_dev_tick( &xbee_master) != -EBUSY,
			"couldn't call xbee_dev_tick on master from frame handler on slave");
	}

	return 0;
}

// start-of-frame, length-msb, length-lsb, payload, checksum
int _make_frame( char *dest, int type, char *payload)
{
	int length;
	int checksum;
	int ch;

	*dest++ = 0x7E;
	length = strlen( payload) + 1;			// frame type + bytes in payload
	*dest++ = (length >> 8) & 0x00FF;		// length MSB
	*dest++ = length & 0x00FF;					// length LSB

	*dest++ = type;
	checksum = -1 - type;
	while ( (ch = *payload) )
	{
		checksum -= ch;
		*dest++ = ch;
		++payload;
	}

	*dest = checksum & 0x00FF;

	// total bytes: frame type + payload, 0x7E, 2-byte length, 1-byte checksum
	return length + 1 + 2 + 1;
}

void _t_frame_load( int type, char *content, int flags)
{
	char frame[256];
	int length;
	int fail = 0;
	int match;
	int result;
	int count;

	memset( &_dummy_frame, 0, sizeof(_dummy_frame));
	_dummy_frame.flags = flags;

	length = _make_frame( frame, type, content);
	xbee_ser_write( &xbee_master, frame, length);

	// !!! need a timeout here
	do {
		result = xbee_dev_tick( &xbee_slave);
	} while (result < 1);

	if (flags & T_FLAG_NO_RECEIVE)
	{
		// shouldn't have dispatched the frame
		fail += test_bool( _dummy_frame.counter == 0,
			"shouldn't have dispatched frame (not registered)");
	}
	else if (type != DUMMY_FRAME_TYPE)
	{
		// shouldn't have dispatched the frame
		fail += test_bool( _dummy_frame.counter == 0,
			"shouldn't have dispatched frame (wrong type)");
	}
	else		// we should have received the frame
	{
		count = (flags & T_FLAG_DOUBLE) ? 2 : 1;
		fail += test_bool( _dummy_frame.counter == count, "didn't receive frame");

		// type should match
		fail += test_compare( *_dummy_frame.data, type, NULL,
			"type didn't match");

		// content should match
		match = (0 == memcmp( content, &_dummy_frame.data[1], strlen( content)));
		fail += test_bool( match, "frame payload corrupted");
	}

	#ifdef TEST_VERBOSE
		if (fail)
		{
			mem_dump( _dummy_frame.data, _dummy_frame.length);
		}
	#endif
}

void t_load_and_dispatch()
{
	int result;

	result = xbee_dev_init( &xbee_slave, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't open slave XBee"))
	{
		return;
	}
	result = xbee_dev_init( &xbee_master, TEST_MASTER, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't open master XBee"))
	{
		return;
	}

	// Since we'll never call xbee_dev_tick on the master XBee, we don't have to
	// worry about the default frame handlers.  We'll be calling the
	// xbee_serial API layer directly to get at the bytes.

	// Register the dummy handler so we can make sure the frame is dispatched.
	// Should get called for all frame types for this ID.
	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler, DUMMY_CONTEXT);
	if (test_compare( result, 0, NULL, "failed to add dummy handler"))
	{
		return;
	}

	// This unit test needs to stuff bytes into the XBee's serial port via
	// looped back serial connection.

	// Start off with something easy
	_t_frame_load( DUMMY_FRAME_TYPE, "foo", 0);

	// Insert junk before start of frame (0x7E) and valid frame data.
	xbee_ser_write( &xbee_master, "junk goes here, skip it", 23);
	_t_frame_load( DUMMY_FRAME_TYPE, "bar", 0);

	// Test valid frame with prefix of extra 0x7E (should catch 0x7E in MSB
	// of invalid length field and re-use it as start of frame).
	xbee_ser_write( &xbee_master, "\x7E", 1);
	_t_frame_load( DUMMY_FRAME_TYPE, "bar", 0);

	// Test valid frame with prefix of 0x7E 0x01 (should catch 0x7E in LSB
	// of invalid length field and re-use it as start of frame).
	// Have the callback call xbee_dev_tick() to confirm -EBUSY response.
	xbee_ser_write( &xbee_master, "\x7E\x01", 2);
	_t_frame_load( DUMMY_FRAME_TYPE, "baz", T_FLAG_CALL_TICK);

	// Delete the dummy handler and make sure the frame handler isn't called
	result = xbee_frame_handler_delete( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler);
	test_compare( result, 0, NULL,
		"handler_delete should have removed handler from table");

	// if it was deleted, a sent frame shouldn't get dispatched
	_t_frame_load( DUMMY_FRAME_TYPE, "deleted", T_FLAG_NO_RECEIVE);

	// add with different settings, still shouldn't dispatch
	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE + 1, 0,
		_dummy_load_handler, DUMMY_CONTEXT);
	test_compare( result, 0, NULL,
		"failed to add dummy handler (different type)");

	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 5,
		_dummy_load_handler, DUMMY_CONTEXT);
	test_compare( result, 0, NULL,
		"failed to add dummy handler (different id)");

	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_handler1, DUMMY_CONTEXT);
	test_compare( result, 0, NULL,
		"failed to add dummy handler (different handler)");

	// a sent frame shouldn't match the previous similar handlers
	_t_frame_load( DUMMY_FRAME_TYPE, "close, but not exact", T_FLAG_NO_RECEIVE);

	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler, NULL);
	test_compare( result, 0, NULL,
		"failed to add dummy handler (NULL context)");

	// a sent frame shouldn't match the previous similar handlers
	_t_frame_load( DUMMY_FRAME_TYPE, "null context", T_FLAG_NULL_CONTEXT);


	// reset the frame table by re-opening the slave
	result = xbee_dev_init( &xbee_slave, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't re-open slave XBee"))
	{
		return;
	}

	// how about a handler with a non-zero id?  dispatch to the matching ID and
	// then another ID.
	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 'f',
		_dummy_load_handler, DUMMY_CONTEXT);
	test_compare( result, 0, NULL, "failed to add non-zero id handler");

	// payload starting with 'f' should match...
	_t_frame_load( DUMMY_FRAME_TYPE, "foo (id test)", 0);

	// ...but payload starting with 'b' should not.
	_t_frame_load( DUMMY_FRAME_TYPE, "bar (id test)", T_FLAG_NO_RECEIVE);

	// Now add the same handler, but matching all IDs -- frame should be
	// dispatched to the same handler twice (once for catchall, once for
	// id = 'f').
	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler, DUMMY_CONTEXT);
	test_compare( result, 0, NULL, "failed to add catchall handler");

	// payload starting with 'f' should match twice...
	_t_frame_load( DUMMY_FRAME_TYPE, "foo (double test)", T_FLAG_DOUBLE);

	// ...but payload starting with 'b' should only match once.
	_t_frame_load( DUMMY_FRAME_TYPE, "bar (double test)", 0);

}

void t_xbee_frame_handler_delete()
{
	int result;

	result = xbee_dev_init( &xbee_slave, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't open slave XBee"))
	{
		return;
	}
	result = xbee_frame_handler_add( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler, DUMMY_CONTEXT);
	if (test_compare( result, 0, NULL, "failed to add dummy handler"))
	{
		return;
	}

	// first, mismatch each of the fields and make sure none succeed
	result = xbee_frame_handler_delete( &xbee_slave, DUMMY_FRAME_TYPE + 1, 0,
		_dummy_load_handler);
	test_compare( result, -ENOENT, NULL,
		"should not have matched (different type)");

	result = xbee_frame_handler_delete( &xbee_slave, DUMMY_FRAME_TYPE, 5,
		_dummy_load_handler);
	test_compare( result, -ENOENT, NULL,
		"should not have matched (different id)");

	result = xbee_frame_handler_delete( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_handler1);
	test_compare( result, -ENOENT, NULL,
		"should not have matched (different handler)");

	// this one should succeed
	result = xbee_frame_handler_delete( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler);
	test_compare( result, 0, NULL, "should have removed handler from table");

	// attempting to remove again should fail (could indicate that we aren't
	// properly deleting).
	result = xbee_frame_handler_delete( &xbee_slave, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler);
	test_compare( result, -ENOENT, NULL, "should have failed (already deleted)");

	// test for invalid parameters
	result = xbee_frame_handler_delete( NULL, DUMMY_FRAME_TYPE, 0,
		_dummy_load_handler);
	test_compare( result, -EINVAL, NULL, "should return -EINVAL for NULL xbee");

}

void t__xbee_next_frame_id()
{
	int result;

	// make sure it wraps from 255 around to 1
	result = xbee_dev_init( &xbee_slave, TEST_SLAVE, BAUD_RATE, NULL, NULL);
	if (test_compare( result, 0, NULL, "couldn't open slave XBee"))
	{
		return;
	}

	xbee_slave.frame_id = 255;
	test_compare( xbee_next_frame_id( &xbee_slave), 1, NULL,
		"didn't roll from 255 to 1");

	test_compare( xbee_slave.frame_id, 1, NULL, "not saved as 1");
}

void t_NULL_param()
{
	int result;

	test_compare( xbee_next_frame_id( NULL), -EINVAL, NULL,
		"xbee_next_frame_id accepts NULL");

	test_compare( xbee_dev_init( NULL, TEST_SLAVE, BAUD_RATE, NULL, NULL), -EINVAL,
		NULL, "xbee_dev_init accepts NULL");

	test_compare( xbee_dev_reset( NULL), -EINVAL, NULL,
		"xbee_dev_reset accepts NULL");

	test_compare( xbee_dev_tick( NULL), -EINVAL, NULL,
		"xbee_dev_tick accepts NULL");

	test_compare( xbee_frame_handler_add( NULL, 0x8a, 0, _dummy_handler1, NULL),
		-EINVAL, NULL, "xbee_frame_handler_add accepts NULL");

	test_compare( xbee_frame_handler_delete( NULL, 0x8a, 0, _dummy_handler1),
		-EINVAL, NULL, "xbee_frame_handler_delete accepts NULL");

	result = xbee_frame_write( NULL, t_NULL_param, 5, NULL, 0,
		XBEE_DEV_FLAG_NONE);
	test_compare( result, -EINVAL, NULL, "xbee_frame_write accepts NULL");

	test_compare( _xbee_frame_load( NULL), -EINVAL, NULL,
		"_xbee_frame_load accepts NULL");

	test_compare( _xbee_frame_dispatch( NULL, t_NULL_param, 5), -EINVAL, NULL,
		"_xbee_frame_dispatch accepts NULL");

	// other functions provide more coverage
	// In particular, t_load_and_dispatch makes sure a call to xbee_dev_tick()
	// inside a frame handler generates the correct error.
}

void main()
{
	int failures = 0;

	failures += DO_TEST( t_xbee_dev_init);
	failures += DO_TEST( t_xbee_dev_reset);
	failures += DO_TEST( t_NULL_param);

	failures += DO_TEST( t_xbee_frame_handler_add);
	failures += DO_TEST( t_xbee_frame_handler_delete);

	failures += DO_TEST( t_xbee_frame_write);

	failures += DO_TEST( t_xbee_checksum);

	failures += DO_TEST( t_load_and_dispatch);

	failures += DO_TEST( t__xbee_next_frame_id);

	if (failures)
	{
		printf( "\n\nFAILED\n");
		exit( -1);
	}

	printf( "\n\nPASSED\n");
	return;
}
