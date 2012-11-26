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
// Common macros for xbee unit tests using loopback minicore.

// Can configure the master serial port as B, C, D or E.  Slave and pins
// used for hardware flow-control all derive from this setting.
#ifndef TEST_MASTER
	#define TEST_MASTER			SER_PORT_E
#endif

// Buffer Sizes -- may want to run test with different sizes
#ifndef INBUFSIZE
	#define INBUFSIZE		255
#endif
#ifndef OUTBUFSIZE
	#define OUTBUFSIZE	255
#endif

//  End of Configuration  //

#define BINBUFSIZE         INBUFSIZE
#define BOUTBUFSIZE        OUTBUFSIZE
#define CINBUFSIZE         INBUFSIZE
#define COUTBUFSIZE        OUTBUFSIZE
#define DINBUFSIZE         INBUFSIZE
#define DOUTBUFSIZE        OUTBUFSIZE
#define EINBUFSIZE         INBUFSIZE
#define EOUTBUFSIZE        OUTBUFSIZE

// Serial Port B flow control, PE6/PE7 (DMA-capable)
#define SERB_CTS_PORT      PEDR
#define SERB_CTS_BIT       7
#define SERB_RTS_PORT      PEDR
#define SERB_RTS_SHADOW    PEDRShadow
#define SERB_RTS_BIT       6

// Serial Port C flow control, PC0/PC1
#define SERC_CTS_PORT      PCDR
#define SERC_CTS_BIT       1
#define SERC_RTS_PORT      PCDR
#define SERC_RTS_SHADOW    PCDRShadow
#define SERC_RTS_BIT       0

// Serial Port D flow control, PC2/PC3
#define SERD_CTS_PORT      PCDR
#define SERD_CTS_BIT       3
#define SERD_RTS_PORT      PCDR
#define SERD_RTS_SHADOW    PCDRShadow
#define SERD_RTS_BIT       2

// Serial Port E flow control, PC4/PC5
#define SERE_CTS_PORT      PCDR
#define SERE_CTS_BIT       5
#define SERE_RTS_PORT      PCDR
#define SERE_RTS_SHADOW    PCDRShadow
#define SERE_RTS_BIT       4

// Serial Port E is on PE6/PE7
#define SERE_TXPORT        PEDR           // Tx on PE6
#define SERE_RXPORT        PEDR           // Rx on PE7

#if __SEPARATE_INST_DATA__
	#define SID "SID: on"
#else
	#define SID "SID: off"
#endif

#ifndef TEST_MASTER
	#fatal "Must define TEST_MASTER to one of SER_PORT_[BCDE]"
#elif TEST_MASTER == SER_PORT_B
	#define XBEE_ON_SERB
	#define XBEE_ON_SERC
	#define TEST_SLAVE			SER_PORT_C
	#define SPUT "C"

	#define master_rts(x)	\
						BitWrPortI( SERB_RTS_PORT, &SERB_RTS_SHADOW, x, SERB_RTS_BIT)
	#define master_cts()		\
						BitRdPortI( SERB_CTS_PORT, SERB_CTS_BIT)
	#define master_rxpin()		\
						BitRdPortI( PCDR, 5)
#elif TEST_MASTER == SER_PORT_C
	#define XBEE_ON_SERB
	#define XBEE_ON_SERC
	#define TEST_SLAVE			SER_PORT_B
	#define SPUT "B"

	#define master_rts(x)	\
						BitWrPortI( SERC_RTS_PORT, &SERC_RTS_SHADOW, x, SERC_RTS_BIT)
	#define master_cts()		\
						BitRdPortI( SERC_CTS_PORT, SERC_CTS_BIT)
	#define master_rxpin()		\
						BitRdPortI( PCDR, 3)
#elif TEST_MASTER == SER_PORT_D
	#define XBEE_ON_SERD
	#define XBEE_ON_SERE
	#define TEST_SLAVE			SER_PORT_E
	#define SPUT "E"

	#define master_rts(x)	\
						BitWrPortI( SERD_RTS_PORT, &SERD_RTS_SHADOW, x, SERD_RTS_BIT)
	#define master_cts()		\
						BitRdPortI( SERD_CTS_PORT, SERD_CTS_BIT)
	#define master_rxpin()		\
						BitRdPortI( PCDR, 1)
#elif TEST_MASTER == SER_PORT_E
	#define XBEE_ON_SERD
	#define XBEE_ON_SERE
	#define TEST_SLAVE			SER_PORT_D
	#define SPUT "D"

	#define master_rts(x)	\
						BitWrPortI( SERE_RTS_PORT, &SERE_RTS_SHADOW, x, SERE_RTS_BIT)
	#define master_cts()		\
						BitRdPortI( SERE_CTS_PORT, SERE_CTS_BIT)
	#define master_rxpin()		\
						BitRdPortI( PEDR, 7)
#else
	#fatal "TEST_MASTER defined as invalid setting"
#endif

