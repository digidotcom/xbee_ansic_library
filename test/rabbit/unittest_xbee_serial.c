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
	UnitTest_xbee_serial.c

	Tests xbee_serial.c, using a Mini-Core module with the following connections:

	TXD/PC0 - PE7/RXE
	RXD/PC1 - PE6/TXE
	TXC/PC2 - PC5/RXB
	RXC/PC3 - PC4/TXB

	Be careful -- the pins of the through-hole connector on the DEMO and SERIAL
	add-on boards are mirrored from the pins of the inter-board connector.

	The TEST_MASTER serial port is the one that does the testing, and TEST_SLAVE
	is the one that is "under test".  TEST_MASTER uses the serial library for
	some tests, and bypasses it for others.

	This library was based on UnitTest_rs232.c, for testing RS232.LIB.

*/

// Configuration:

// Enable Verbose output from TEST.LIB
#define TEST_VERBOSE

// Maximum baud rate to test -- on RCM5700, 460800 does not work
#define MAX_BAUD 230400

// Do fast tests -- 115200 only
#define FAST_TESTS

// If serial port B is one of the ports used, run it in DMA mode
//#define TEST_DMA

#include "unittest_common_serial.h"
#include <xbee/device.h>

#use "test.lib"

xbee_dev_t	xbee_master, xbee_slave;
#define XBEE_MASTER	&xbee_master
#define XBEE_SLAVE	&xbee_slave

// List of baud rates to test
const long _baud[] = {
	300, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800
};

void delay_ms( word ms)
{
	word timer;

	timer = _SET_SHORT_TIMEOUT( ms);
	while (!_CHK_SHORT_TIMEOUT(timer));
}

// return max milliseconds for a character to send (use for timeouts)
word _chartime( long baud)
{
	return (word) (10000 / baud) + 2;
}

int _getchar( xbee_dev_t *get_xbee, long baud)
{
	int inchar;
	word timer;

   timer = _SET_SHORT_TIMEOUT( _chartime( baud));
   do {
      inchar = xbee_ser_getchar( get_xbee);
   } while ((inchar == -ENODATA) && !_CHK_SHORT_TIMEOUT(timer));

	return inchar;
}
// Send/receive a single character at the given baud rate
void t_getputch_txrx( xbee_dev_t *put_xbee, xbee_dev_t *get_xbee, long baud,
	int outchar, char *errstr)
{
	int inchar;

   xbee_ser_putchar( put_xbee, outchar);
	inchar = _getchar( get_xbee, baud);
   test_compare( outchar, inchar, "0x%02lx", errstr);
}
void t_getputch( long baud)
{
	char errstr[80];

	xbee_ser_open( XBEE_MASTER, baud);
	xbee_ser_open( XBEE_SLAVE, baud);

   // both ports are open, try to send a byte from one to the other
   sprintf( errstr, "read from MASTER to SLAVE failed @%lubps", baud);
   t_getputch_txrx( XBEE_MASTER, XBEE_SLAVE, baud, 'x', errstr);
   sprintf( errstr, "read from SLAVE to MASTER failed @%lubps", baud);
   t_getputch_txrx( XBEE_SLAVE, XBEE_MASTER, baud, 0x55, errstr);

	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);
}

// Make sure we can open the serial ports at standard baud rates
void t_baudrate( long baud)
{
	int i;
	xbee_dev_t *port;
	char errstr[80];

   for (i = 0; i < 2; ++i)
   {
      sprintf( errstr, "Couldn't open %s within 5%% of %lubps",
         i ? "SLAVE" : "MASTER", baud);
      port = i ? XBEE_SLAVE : XBEE_MASTER;
      test_compare( xbee_ser_open( port, baud), 0, NULL, errstr);
   }

   xbee_ser_close( XBEE_MASTER);
   xbee_ser_close( XBEE_SLAVE);
}

void t_writeread( long baud)
{
	char outstr[80];
	char instr[80];
	int txlen;
	int outlen, inlen;
	int fails;

	xbee_ser_open( XBEE_MASTER, baud);
	xbee_ser_open( XBEE_SLAVE, baud);

	// turn flow control on -- at higher speeds (230kbps) it will be necessary
	// to keep from overflowing the receive buffer.
	xbee_ser_flowcontrol( XBEE_MASTER, 1);
	xbee_ser_flowcontrol( XBEE_SLAVE, 1);

//	memset( &sxd[TEST_MASTER]->outbuf[8], 0, 32);
//	memset( &sxd[TEST_SLAVE]->inbuf[8], 0, 32);

	strcpy( outstr, "Pack my box with five dozen liquor jugs.");
	txlen = strlen( outstr);

	// writing 40 bytes to 32-byte buffer, so there will be overlap and
	// writing of bytes while bytes are getting pulled from the buffer
	outlen = xbee_ser_write( XBEE_MASTER, outstr, txlen);
	inlen = xbee_ser_read( XBEE_SLAVE, instr, sizeof(instr), 10000/baud+3);

	fails = 0;
	if (test_compare( inlen, outlen, NULL,
		"bytecount received differed from that sent") )
	{
		++fails;
	}
	else if (test_bool( strncmp( outstr, instr, inlen) == 0,
			"received different bytes") )
   {
      ++fails;
   }

	if (fails)
	{
      printf( "\ttx: %.*s\n\trx: %.*s\n", outlen, outstr, inlen, instr);

      #ifdef RETADDRESS_SIZE
      // old CBUF and RS232
      printf( "\tOUTBUF: %.*s\n\tINBUF: %.*s\n",
      	OUTBUFSIZE+1, &sxd[TEST_MASTER]->outbuf[8],
      	INBUFSIZE+1, &sxd[TEST_SLAVE]->inbuf[8]);
		#else
		// new CBUF and RS232
      printf( "\tOUTBUF: %.*ls\n\tINBUF: %.*ls\n",
      	OUTBUFSIZE+1, sxd[TEST_MASTER]->outbuf->buffer,
      	INBUFSIZE+1, sxd[TEST_SLAVE]->inbuf->buffer);
		#endif
	}
	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);
}

void t_rts( long baud)
{
	char buffer[80];
	int i, sent, check;

	// Tests whether or not slave asserts RTS when its buffer fills up.

	// Turn flow control on for slave but manually control RTS/CTS on master.
	// Have master send until slave tells it to stop.  Read from slave until
	// master is allowed to send again.

	xbee_ser_open( XBEE_MASTER, baud);
	xbee_ser_open( XBEE_SLAVE, baud);
	xbee_ser_flowcontrol( XBEE_SLAVE, 1);

	test_bool( master_cts() == 0, "slave not asserting /RTS (pre-send)");
	for (i = 0; i < INBUFSIZE; ++i)
	{
		if (master_cts())
		{
			sprintf( buffer, "/RTS changed after writing %d/%d bytes",
				i, INBUFSIZE);
			test_info( buffer);
			test_bool( i > (INBUFSIZE >> 1),
				"/RTS changed before buffer half full");
			break;
		}
		sprintf( buffer, "couldn't send character %d", i + 1);
		test_bool( xbee_ser_putchar( XBEE_MASTER, i) == 0, buffer);

		// make sure the byte has gone out before continuing
		while (xbee_ser_tx_used( XBEE_MASTER));
	}
	test_bool( i < INBUFSIZE, "slave never asserted /RTS (post-send)");
	sent = i;

	check = 1;
	for (i = 0; i < sent; ++i)
	{
		if (check && !master_cts())
		{
			check = 0;
			sprintf( buffer, "/RTS re-asserted after reading %d/%d bytes",
				i, sent);
			test_info( buffer);
		}
		sprintf( buffer, "mis-read character %d from slave", i + 1);
		test_bool( i == xbee_ser_getchar( XBEE_SLAVE), buffer);
	}
	test_bool( !master_cts(), "slave never re-asserted /RTS (post-read)");
	test_compare( xbee_ser_getchar( XBEE_SLAVE), -ENODATA, "0x%02lx",
		"read extra byte from slave");

	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);
}

void t_cts( long baud)
{
	char buffer[80];
	unsigned long timeout;
	int i, stop, check;

	// Tests whether or not slave listens to CTS (master's RTS).

	// Turn flow control on for slave but manually control RTS/CTS on master.
	// De-assert CTS on the master and spool bytes on the slave.  It should
	// buffer ALL bytes until the master releases CTS.

	xbee_ser_open( XBEE_MASTER, baud);
	xbee_ser_open( XBEE_SLAVE, baud);
	xbee_ser_flowcontrol( XBEE_SLAVE, 1);

	// Initial test of tx_used, tx_free, rx_used and rx_free
	test_compare( xbee_ser_tx_used( XBEE_SLAVE), 0, NULL,
		"xbee_ser_tx_used() reporting incorrect value (buffer empty)");
	test_compare( xbee_ser_tx_free( XBEE_SLAVE), OUTBUFSIZE, NULL,
		"xbee_ser_tx_free() reporting incorrect value (buffer empty)");

	test_compare( xbee_ser_rx_used( XBEE_MASTER), 0, NULL,
		"xbee_ser_rx_used() reporting incorrect value (buffer empty)");
	test_compare( xbee_ser_rx_free( XBEE_MASTER), INBUFSIZE, NULL,
		"xbee_ser_rx_free() reporting incorrect value (buffer empty)");

	// master telling slave not to send
	master_rts(1);
	for (i = 0; i < INBUFSIZE; )
	{
		sprintf( buffer, "couldn't send character %d", i + 1);
		test_bool( xbee_ser_putchar( XBEE_SLAVE, i) == 0, buffer);

		if (test_compare( xbee_ser_rx_used( XBEE_MASTER), 0, NULL,
			"master received byte with RTS de-asserted"))
		{
			break;
		}

		++i;

		// Make sure tx_used and tx_free are working
		test_compare( xbee_ser_tx_used( XBEE_SLAVE), i, NULL,
			"xbee_ser_tx_used() reporting incorrect value");
		test_compare( xbee_ser_tx_free( XBEE_SLAVE), OUTBUFSIZE - i, NULL,
			"xbee_ser_tx_free() reporting incorrect value");
	}

	if (! test_bool( i == INBUFSIZE, "slave couldn't fill OUT buffer"))
	{
	   // have master tell the slave it's OK to send, but tell it to stop after
	   // receiving a few bytes
	   master_rts(0);
	   timeout = MS_TIMER;
	   while ( (stop = xbee_ser_rx_used( XBEE_MASTER)) < 5)
	   {
	   	if (MS_TIMER - timeout > 1000)
	      {
	      	test_bool( 0,
	      		"timeout waiting for slave to send 5 bytes to master");
	         break;
	      }
	   }
	   master_rts(1);
	   // delay long enough to receive about 30 bytes at the given baud rate
	   // 30 bytes * 10 bits/byte * 1000 ms/s / <baud> bits/second
	   // 300,000 bit*ms/s / <baud> bits/s = 300,000ms / <baud>
		delay_ms( (word) (300000ul / baud));
	   i = xbee_ser_rx_used( XBEE_MASTER) - stop;
	   sprintf( buffer, "slave sent %d bytes after master de-asserted RTS", i);
	   if (i > 4)
	   {
	      test_bool( 0, buffer);
	   }
	   else
	   {
	      test_info( buffer);
	   }

	   // master tells slave it's OK to send, wait for slave to send all bytes
	   master_rts(0);
	   timeout = MS_TIMER;
	   while (xbee_ser_tx_used( XBEE_SLAVE))
		{
			// even @ 300 bps, should be able to receive 30 chars in 1 second
	   	if (MS_TIMER - timeout > 1000)
	      {
	      	test_bool( 0,
	      		"timeout waiting for slave to send all bytes to master");
	         break;
	      }
	   }

		// bytes that were in slave's Tx buffer should be in master's Rx buffer
		test_compare( xbee_ser_tx_used( XBEE_SLAVE), 0, NULL,
			"xbee_ser_tx_used() reporting incorrect value (buffer empty)");
		test_compare( xbee_ser_tx_free( XBEE_SLAVE), OUTBUFSIZE, NULL,
			"xbee_ser_tx_free() reporting incorrect value (buffer empty)");

		test_compare( xbee_ser_rx_used( XBEE_MASTER), INBUFSIZE, NULL,
			"xbee_ser_rx_used() reporting incorrect value (buffer full)");
		test_compare( xbee_ser_rx_free( XBEE_MASTER), 0, NULL,
			"xbee_ser_rx_free() reporting incorrect value (buffer full)");

		// at low baud rates, it's possible to read bytes from master before
		// last byte has left slave's serial shift register.  Add a brief
		// delay here for that last byte to get out.
		delay_ms( (word) (20000 / baud + 2));

	   for (i = 0; i < INBUFSIZE; )
	   {
	      sprintf( buffer, "mis-read character %d from master", i + 1);
	      test_bool( i == xbee_ser_getchar( XBEE_MASTER), buffer);

	      ++i;

			// test Rx buffer
			test_compare( xbee_ser_rx_used( XBEE_MASTER), INBUFSIZE - i, NULL,
				"xbee_ser_rx_used() reporting incorrect value (clearing buffer)");
			test_compare( xbee_ser_rx_free( XBEE_MASTER), i, NULL,
				"xbee_ser_rx_free() reporting incorrect value (clearing buffer)");
	   }
	   test_compare( xbee_ser_getchar( XBEE_MASTER), -ENODATA, "0x%02lx",
	   	"read extra byte from master");

	}

	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);
}

int t_allbauds( char *testname, int (*subtest)())
{
	char buffer[80];
	int i;

	test_header( testname);
	#ifdef FAST_TESTS
		subtest( 115200ul);
	#else
	   for (i = 0; i < sizeof(_baud)/sizeof(_baud[0]); ++i)
	   {
	      if (_baud[i] <= MAX_BAUD)
	      {
	      	sprintf( buffer, "testing @ %lu baud", _baud[i]);
	      	test_info( buffer);
	         subtest( _baud[i]);
	      }
	   }
	#endif

	return test_footer();
}

void t_pins()
{
	int bit;

	/* Test the pins used for this test, confirm that they're looped back.
	   TXD/PC0 - PE7/RXE
	   RXD/PC1 - PE6/TXE
	   TXC/PC2 - PC5/RXB
	   RXC/PC3 - PC4/TXB
	*/

	// PC0 is an output
	BitWrPortI( PCDDR, &PCDDRShadow, 1, 0);
	// PC2 is an output
	BitWrPortI( PCDDR, &PCDDRShadow, 1, 2);
	// PC4 is an output
	BitWrPortI( PCDDR, &PCDDRShadow, 1, 4);
	// PE6 is an output
	BitWrPortI( PEDDR, &PEDDRShadow, 1, 6);

	for (bit = 0; bit < 2; ++bit)
	{
		// output on PC0, should read in on PE7
		BitWrPortI( PCDR, &PCDRShadow, bit, 0);
		delay_ms(2);
		test_compare( bit, BitRdPortI( PEDR, 7), NULL,
			"Failed to read output of PC0 on PE7");

		// output on PC2, should read in on PC5
		BitWrPortI( PCDR, &PCDRShadow, bit, 2);
		delay_ms(2);
		test_compare( bit, BitRdPortI( PCDR, 5), NULL,
			"Failed to read output of PC2 on PC5");

		// output on PC4, should read in on PC3
		BitWrPortI( PCDR, &PCDRShadow, bit, 4);
		delay_ms(2);
		test_compare( bit, BitRdPortI( PCDR, 3), NULL,
			"Failed to read output of PC4 on PC3");

		// output on PE6, should read in on PC1
		BitWrPortI( PEDR, &PEDRShadow, bit, 6);
		delay_ms(2);
		test_compare( bit, BitRdPortI( PCDR, 1), NULL,
			"Failed to read output of PE6 on PC1");
	}
}

void t_break( long baud)
{
	word timer;
	int pin;

	xbee_ser_open( XBEE_MASTER, baud);
	xbee_ser_open( XBEE_SLAVE, baud);

	// make sure master's RX pin is high (slave's TX is idle)
	test_compare( master_rxpin(), 1, NULL, "slave's TX is not idle");

	// initiate a long break and make sure RX drops and stays low
	xbee_ser_break( XBEE_SLAVE, 1);
	test_compare( master_rxpin(), 0, NULL, "slave didn't enter break");
   timer = _SET_SHORT_TIMEOUT( _chartime( baud) * 3);
   do {
      pin = master_rxpin();
   } while ((pin == 0) && !_CHK_SHORT_TIMEOUT(timer));
	test_compare( master_rxpin(), 0, NULL, "slave didn't stay in break");

	// leave break condition
	xbee_ser_break( XBEE_SLAVE, 0);

	// make sure master's RX pin is high (slave's TX is idle)
	test_compare( master_rxpin(), 1, NULL, "slave's TX is not idle");

	// re-enter break condition
	xbee_ser_break( XBEE_SLAVE, 1);
	test_compare( master_rxpin(), 0, NULL, "slave didn't enter break");

	// make sure opening serial port resets break
	xbee_ser_open( XBEE_SLAVE, baud);

	// make sure master's RX pin is high (slave's TX is idle)
	test_compare( master_rxpin(), 1, NULL, "slave's TX is not idle");

	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);
}

/*
#define dump_reg8(addr)	printf( #addr "\t= 0x%02x\n", RdPortI( addr))
#define dump_reg8_bit(a, b)	printf( #a #b "\t= %d\n", BitRdPortI( a, b))
void t_PE6()
{
	int out;

	out = 1;
	for (;;)
	{
		printf( "\n");
	   dump_reg8_bit(PEDR, 6);
	   dump_reg8_bit(PEDDR, 6);
	   dump_reg8_bit(PEDCR, 6);
	   dump_reg8_bit(PEFR, 6);

	   dump_reg8_bit(PCDR, 1);
	   dump_reg8_bit(PCDDR, 1);
	   dump_reg8_bit(PCDCR, 1);
	   dump_reg8_bit(PCFR, 1);

		if (getchar() == ' ')
		{
			out = !out;
			printf( "setting out to %d\n", out);
	      // write new value to PE6
	      BitWrPortI( PEDR, &PEDRShadow, out, 6);
	      delay_ms(5);
		}
	}
}
*/

const char _perf[] =
	"\"Pack my box with five dozen liquor jugs,\" said the quick brown fox.";
__nodebug void t_performance()
{
	int i;
	unsigned long t1, t2;
	unsigned long sent, received, loops;
	char	buffer[80];

#if defined RS232_DEBUG
	#warns "performance test skipped -- RS232_DEBUG is defined"
#elif defined BUFFER_DEBUG
	#warns "performance test skipped -- BUFFER_DEBUG is defined"
#else

	xbee_ser_open( XBEE_MASTER, MAX_BAUD);
	xbee_ser_open( XBEE_SLAVE, MAX_BAUD);

	// run the test with flow control turned on
	xbee_ser_flowcontrol( XBEE_MASTER, 1);
	xbee_ser_flowcontrol( XBEE_SLAVE, 1);

	sprintf( buffer, "running test @ %lubps, max %lu bytes in 1 second",
		MAX_BAUD, MAX_BAUD / 10);
	test_info( buffer);

	// clear any error flags before starting test
	serXgetError( TEST_SLAVE);
	serXgetError( TEST_MASTER);

	// flush slave's receive buffer
	xbee_ser_rx_flush( XBEE_SLAVE);

	sent = received = loops = 0;
	t2 = MS_TIMER + 1100;
	t1 = MS_TIMER + 1000;
	do {
		if (MS_TIMER < t1)
		{
			i = xbee_ser_write( XBEE_MASTER, _perf, sizeof(_perf));
			sent += i;
			++loops;
		}
		i = xbee_ser_read( XBEE_SLAVE, buffer, 80, 0);
		received += i;
	} while (MS_TIMER < t2);

	sprintf( buffer, "%lu bytes (%u%%) in 1 second (looped %lu times)",
		sent, (unsigned) (sent * 1000 / MAX_BAUD), loops);
	test_info( buffer);
	test_compare( sent - received, 0, "%lu", "dropped bytes");
	test_compare( serXgetError( TEST_SLAVE), 0, "0x%02lx", "slave errors");
	test_compare( serXgetError( TEST_MASTER), 0, "0x%02lx", "master errors");

	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);

#endif
}

void t_rtscts()
{
	int bit;

	// Test the RTS/CTS pins between master and slave

	// open the serial port to initialize the pins
	xbee_ser_open( XBEE_MASTER, 115200);
	xbee_ser_open( XBEE_SLAVE, 115200);

	// need to turn flow control on to init xRTSon and xRTSoff function pointers
	xbee_ser_flowcontrol( XBEE_MASTER, 1);
	xbee_ser_flowcontrol( XBEE_SLAVE, 1);

	for (bit = 0; bit < 2; ++bit)
	{
		// output on master RTS, should read in on slave CTS
		xbee_ser_set_rts( XBEE_MASTER, bit);
		delay_ms(2);
		test_compare( bit, xbee_ser_get_cts( XBEE_SLAVE), NULL,
			"CTS on slave did not match RTS on master");

		// output on slave RTS, should read in on master CTS
		xbee_ser_set_rts( XBEE_SLAVE, bit);
		delay_ms(2);
		test_compare( bit, xbee_ser_get_cts( XBEE_MASTER), NULL,
			"CTS on master did not match RTS on slave");
	}

	xbee_ser_close( XBEE_MASTER);
	xbee_ser_close( XBEE_SLAVE);
}

void t_ser_invalid()
{
	xbee_dev_t xbee;

	test_compare( xbee_ser_invalid( NULL), 1, NULL,
		"xbee_ser_invalid accepts NULL");

	xbee.serport = TEST_MASTER;
	test_compare( xbee_ser_invalid( &xbee), 0, NULL,
		"xbee_ser_invalid failed for TEST_MASTER");

	xbee.serport = TEST_SLAVE;
	test_compare( xbee_ser_invalid( &xbee), 0, NULL,
		"xbee_ser_invalid failed for TEST_SLAVE");

	xbee.serport = SER_PORT_A;
	test_compare( xbee_ser_invalid( &xbee), 1, NULL,
		"xbee_ser_invalid succeeded for SER_PORT_A");
}

#define ALLBAUD(testname)	t_allbauds( #testname, testname)
void main()
{
	int failures;

	// these are the only elements of xbee_dev_t used by xbee_serial.c
	xbee_master.serport = TEST_MASTER;
	xbee_slave.serport = TEST_SLAVE;

	printf( "main: INFO (testing port " SPUT " on " _BOARD_NAME_ " " SID ")\n");
	#ifdef FAST_TESTS
		printf( "main: INFO (FAST_TESTS: testing 115200bps ONLY)\n");
	#else
		printf( "main: INFO (testing baud rates up to %lubps)\n", MAX_BAUD);
	#endif
	failures = 0;

	failures += DO_TEST( t_pins);
	failures += DO_TEST( t_rtscts);
	failures += DO_TEST( t_ser_invalid);

	failures += ALLBAUD( t_break);
	failures += ALLBAUD( t_baudrate);
	failures += ALLBAUD( t_getputch);

	// only continue if there haven't been any failures up to this point
	if (!failures)
	{
	   failures += ALLBAUD( t_writeread);
		failures += ALLBAUD( t_rts);
		failures += ALLBAUD( t_cts);
	}

	failures += DO_TEST( t_performance);

	if (failures)
	{
		printf( "\n\nFAILED\n");
		exit( -1);
	}

	printf( "\n\nPASSED\n");
	return;
}


