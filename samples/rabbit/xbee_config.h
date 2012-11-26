/* Digi International, Copyright © 2005-2011.  All rights reserved. */
/*
	This header is used by all of the XBee samples to configure the serial port
	options necessary to communicate with the XBee module.  It includes settings
	for the BL4S100 Series and RCM4510W hardware.

	If you are using other hardware, you will need to define a series of macros
	and functions that each sample can reference.

	Required macros and functions:

		Define one or more of XBEE_ON_SERx (where x is a letter from A to F)
		to enable support for that serial port in the XBee Serial Library.

		Use the configuration macros from RS232.LIB to configure the serial port:
		-	xINBUFSIZE and xOUTBUFSIZE should be 255 for maximum performance.
		-	See "DEFINING RTS/CTS PINS FOR FLOW CONTROL" from RS232.LIB's library
			description for instructions on configuring flow control.

		Define an xbee_serial_t object, XBEE_SERPORT, with the baud rate and
		serial port to use for connecting to the XBee module.

		Create two functions, xbee_reset_pin() and xbee_awake_pin() for
		setting the XBee module's /RESET pin (pin 5) and reading its AWAKE pin
		(pin 13).  If the Rabbit isn't connected to those pins, you can define
		the names as macros to NULL.

			#define xbee_reset_pin		NULL
			#define xbee_awake_pin		NULL
*/

#include "xbee/platform.h"
#include "xbee/serial.h"
#include "xbee/device.h"

#ifndef XBEE_STD_CONFIG
	// Make use of XBEE_STD_CONFIG in a similar manner to TCPCONFIG
	// 0 means application must set all macros, 1 means use default which is
   // appropriate for boards with built-in XBee support, others for custom
   // and special configurations.
	#define XBEE_STD_CONFIG	1
#endif

#if XBEE_STD_CONFIG == 0
	// Application needs to set all macros
#elif XBEE_STD_CONFIG == 1
	#if _BOARD_TYPE_ == BL4S100 || _BOARD_TYPE_ == BL4S150
	   #define XBEE_ON_SERE

	   #define EINBUFSIZE            255
	   #define EOUTBUFSIZE           255

	   #define SERE_TXPORT           PDDR           // Tx on PD6
	   #define EDRIVE_TXD            6
	   #define SERE_RXPORT           PDDR           // Rx on PD7
	   #define EDRIVE_RXD            7

	   // Configure XBee CTS for PD5
	   #define SERE_CTS_PORT         PDDR
	   #define SERE_CTS_BIT          5
	   // Configure XBee RTS for PE6
	   #define SERE_RTS_PORT         PEDR
	   #define SERE_RTS_SHADOW       PEDRShadow
	   #define SERE_RTS_BIT          6

	   const xbee_serial_t XBEE_SERPORT = { 115200, SER_PORT_E };

	   // BL4S1xx.LIB needed for RIO definitions/functions and _zb_reset function
	   #use "BL4S1xx.LIB"

	   void xbee_reset_pin( xbee_dev_t *xbee, int enable)
	   {
	      _zb_reset(enable);
	   }
	   int xbee_awake_pin( xbee_dev_t *xbee)
	   {
	      return BitRdPortI( PDDR, 3);
	   }

	#elif RCM4500W_SERIES
	   #define XBEE_ON_SERB

	   // Use TXD+ (NAPCR3) to chip select XBee (since it shares serial port B)
	   #define _xb_cs(selected)      BitWrPortI (NAPCR, &NAPCRShadow, !selected, 3)

	   #define BINBUFSIZE            255
	   #define BOUTBUFSIZE           255

	   #define SERB_TXPORT           PCDR           // Tx on PC4
	   #define BDRIVE_TXD            4
	   #define SERB_RXPORT           PCDR           // Rx on PC5
	   #define BDRIVE_RXD            5

	   // Configure XBee CTS for RXD+ (NAPCR1)
	   #define SERB_CTS_PORT         NAPCR
	   #define SERB_CTS_BIT          1
	   // Configure XBee RTS for TXD- (NAPCR2)
	   #define SERB_RTS_PORT         NAPCR
	   #define SERB_RTS_SHADOW       NAPCRShadow
	   #define SERB_RTS_BIT          2

	   const xbee_serial_t XBEE_SERPORT = { 115200, SER_PORT_B };

	   void xbee_reset_pin( xbee_dev_t *xbee, int enable)
	   {
	      // Use TXDD+ (NAPCR5) to put XBee in reset state
	      BitWrPortI (NAPCR, &NAPCRShadow, enable, 5);
	   }

	   // The RCM4510W is always awake -- the XBee module controls power to the
	   // core module, so when the XBee is sleeping, the RCM4510W is off.
	   #define xbee_awake_pin  NULL
   #elif RCM6600W_SERIES
   	/* The 6600 does not come with a factory-installed XBee (i.e.
         XBEE_ONBOARD macro will default to zero), however for
         test purposes it is assumed that an Xbee module would be attached
         according to this scheme:
      */
	   #define XBEE_ON_SERD

	   #define DINBUFSIZE            255
	   #define DOUTBUFSIZE           255

	   #define SERD_TXPORT           PCDR           // Tx on PC0
	   #define DDRIVE_TXD            0
	   #define SERD_RXPORT           PCDR           // Rx on PC1
	   #define DDRIVE_RXD            1

	   // Configure XBee CTS for PC3
	   #define SERD_CTS_PORT      PCDR        //CTS is input flowcontrol
	   #define SERD_CTS_BIT       3           //PC3

	   // Configure XBee RTS for PC2
	   #define SERD_RTS_PORT      PCDR        //RTS is output flowcontrol
	   #define SERD_RTS_SHADOW    PCDRShadow
	   #define SERD_RTS_BIT       2           //PC2

	   const xbee_serial_t XBEE_SERPORT = { 115200, SER_PORT_D };

	   // Assume PD0 is reset, PD2 is 'awake'
	   void xbee_reset_pin( xbee_dev_t *xbee, int enable)
	   {
	      printf("Reset: enable=%d\n", enable);
         if (!(PDDDRShadow & 0x01))
         {
         	// First time called, need to initialize port pins and directions
	         BitWrPortI( PDDR, &PDDRShadow, 1, 1);  // Set to output inactive (Reset)
	         BitWrPortI( PDDDR, &PDDDRShadow, 1, 0);   // Set to output (Reset)
	         BitWrPortI( PDDDR, &PDDDRShadow, 1, 1);   // Set to output (DTR)
	         BitWrPortI( PDDR, &PDDRShadow, 0, 1);     // DTR on
	         BitWrPortI( PDDDR, &PDDDRShadow, 0, 2);   // Set to input (awake)
         }

	      BitWrPortI( PDDR, &PDDRShadow, !enable, 0);
	   }
	   int xbee_awake_pin( xbee_dev_t *xbee)
	   {
	      return BitRdPortI( PDDR, 2);
	   }


	#else
	   #error "This target has no default XBee configuration."
	   #fatal "Require XBEE_STD_CONFIG to be defined to other than 1."
	#endif
#elif XBEE_STD_CONFIG == 2
	#define XBEE_ON_SERD

	#define DINBUFSIZE				255
	#define DOUTBUFSIZE				255

	#define SERD_TXPORT           PCDR           // Tx on PC0
	#define DDRIVE_TXD				0
	#define SERD_RXPORT           PCDR           // Rx on PC1
	#define DDRIVE_RXD				1

	// Configure XBee CTS for PC3
   #define SERD_CTS_PORT		PCDR        //CTS is input flowcontrol
	#define SERD_CTS_BIT			3           //PC3

   // Configure XBee RTS for PC2
	#define SERD_RTS_PORT		PCDR        //RTS is output flowcontrol
	#define SERD_RTS_SHADOW		PCDRShadow
	#define SERD_RTS_BIT			2           //PC2

	const xbee_serial_t XBEE_SERPORT = { 115200, SER_PORT_D };

	// In this configuration, we can't reset the XBee or see if it's awake.
	#define xbee_reset_pin		NULL
	#define xbee_awake_pin		NULL
#elif XBEE_STD_CONFIG == 3
	#define XBEE_ON_SERF

   #define FINBUFSIZE				255
	#define FOUTBUFSIZE				255

	#define SERF_TXPORT			PEDR			//Tx on PE2
   #define FDRIVE_TXF				2
	#define SERF_RXPORT			PEDR			//Rx on PE3
   #define FDRIVE_RXF				3

   #define SERF_CTS_PORT		PDDR
	#define SERF_CTS_BIT			0

	#define SERF_RTS_PORT		PADR
	#define SERF_RTS_SHADOW		PADRShadow
	#define SERF_RTS_BIT			0

	const xbee_serial_t XBEE_SERPORT = { 115200, SER_PORT_F };

	#define xbee_reset_pin		NULL
	#define xbee_awake_pin		NULL

#else
   #error "Invalid XBee configuration for this target."
   #fatal "XBEE_STD_CONFIG value not recognized."
#endif



