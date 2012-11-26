/*
 * Copyright (c) 2011 Digi International Inc.,
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
	@addtogroup xbee_io
	@{
	@file xbee/io.h
	Header for code related to built-in I/Os on the XBee module
   (the ATIS command, 0x92 frames)
*/

#ifndef XBEE_IO_H
#define XBEE_IO_H

#include "xbee/platform.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "wpan/types.h"

XBEE_BEGIN_DECLS

/***
	@brief This structure stores broken-out I/O state for an XBee's configurable
   I/Os.

*/
typedef struct xbee_io_t {
	xbee_command_list_context_t
   						clc;				///< Command list context used for
                     					///< querying configuration
	uint16_t				din_enabled;	///< Which pins have digital input sampling
   											///<  enabled. See #XBEE_IO_DIO0 etc.
	uint16_t				dout_enabled;	///< Which pins have digital output
   											///<  enabled.
   uint16_t				din_state;		///< Sampled state of digital inputs
   uint16_t				dout_state;		///< Shadow state of digital outputs
   uint16_t				pullup_state;	///< Shadow state of pull-up resistors
   											///< 0:no pull-up; 1:pull-up applied
   uint16_t				pulldown_state;///< Shadow state of pull-down resistors
   											///< 0:no pull-down; 1:pull-down applied

	uint8_t				analog_enabled;///< Which analog inputs are enabled
   											///< See #XBEE_IO_AD0 etc.
   uint16_t				adc_sample[8];	///< Analog samples (raw count 0..1023)
   											///< Entries 0..3 correspond to AD0..3,
                                    ///< Entries 4..6 not currently used,
                                    ///< Entry 7 for Vcc reading (only obtained
                                    ///< if ATV+ issued, and Vcc is less than
                                    ///< that threshold).
   uint16_t				sample_rate;	///< Shadow value of ATIR
   uint16_t				change_mask;	///< Shadow value of ATIC
   uint8_t				config[16];		///< First 16 configuration states (from
   											///< ATD0 etc. commands).  Not all used with
                                    ///< current hardware.
   uint16_t				pullup_enable;	///< Raw ATPR command value
   uint16_t				pullup_direction;	///< Raw ATPD command value
   uint16_t				pwm0_level;		///< Output PWM level (ATM0)
   uint16_t				pwm1_level;		///< Output PWM level (ATM1)
   uint8_t				pwm_timeout;	///< ATPT value
   uint8_t				tx_samples;		///< ATIT value
} xbee_io_t;

/** @name XBee configurable I/O pin names.  Bitmask applied to certain fields.
	@{
*/
/// AD0 (pin 20)
#define XBEE_IO_AD0							0x0001
/// DIO0 (pin 20) - also commissioning button if so configured
#define XBEE_IO_DIO0							0x0001
/// AD1 (pin 19)
#define XBEE_IO_AD1							0x0002
/// DIO1 (pin 19)
#define XBEE_IO_DIO1							0x0002
/// AD2 (pin 18)
#define XBEE_IO_AD2							0x0004
/// DIO2 (pin 18)
#define XBEE_IO_DIO2							0x0004
/// AD3 (pin 17)
#define XBEE_IO_AD3							0x0008
/// DIO3 (pin 17)
#define XBEE_IO_DIO3							0x0008
/// DIO4 (pin 11)
#define XBEE_IO_DIO4							0x0010
/// DIO5 (pin 15) - also ASSOC indicator if so configured
#define XBEE_IO_DIO5							0x0020
/// DIO6 (pin 16) - also RTS flow control if so configured
#define XBEE_IO_DIO6							0x0040
/// DIO7 (pin 12) - also CTS flow control if so configured
#define XBEE_IO_DIO7							0x0080
#define XBEE_IO_GPIO7						0x0080
/// Supply voltage sense (in xbee_io_t.analog_enabled field)
#define XBEE_IO_VCC_SENSE					0x80
/// DIO10 (pin 6) - also RSSI PWM if so configured
#define XBEE_IO_DIO10						0x0400
/// DIO11 (pin 7)
#define XBEE_IO_DIO11						0x0800
/// DIO12 (pin 4) - also CD signal if so configured
#define XBEE_IO_DIO12						0x1000
//@}


typedef PACKED_STRUCT xbee_frame_io_response_t {
	uint8_t				frame_type;			///< XBEE_FRAME_IO_RESPONSE (0x92)
	addr64				ieee_address_be;			///< ATSH and ATSL of sender
	uint16_t				network_address_be;		///< ATMY value of sender
	uint8_t				options;
   uint8_t				num_samples;		///< Currently always 1
   uint16_t				data[1];				///< First of variable amount of data
                                       ///< Parse using xbee_io_response_parse()
} xbee_frame_io_response_t;

/**
	@brief Configuration type for XBee built-in I/Os.

	Follows encoding in ATD0 etc., but also add pullup state as
   bit encoding.
*/
enum xbee_io_type {
	XBEE_IO_TYPE_DISABLED = 0,					///< Disabled
	XBEE_IO_TYPE_SPECIAL = 1,					///< Special function e.g. association
   													///< indicator for DIO5
	XBEE_IO_TYPE_ANALOG_INPUT = 2,			///< Analog input
	XBEE_IO_TYPE_DIGITAL_INPUT = 3,			///< Digital input
	XBEE_IO_TYPE_DIGITAL_OUTPUT_LOW = 4,	///< Digital output low
	XBEE_IO_TYPE_DIGITAL_OUTPUT_HIGH = 5,	///< Digital output high
	XBEE_IO_TYPE_TXEN_ACTIVE_LOW = 6,		///< RS485 transmit enable (act low)
	XBEE_IO_TYPE_TXEN_ACTIVE_HIGH = 7,		///< RS485 transmit enable (act high)

   XBEE_IO_TYPE_MASK = 0x0F,					///< Mask for above settings

	XBEE_IO_TYPE_CHANGE_DETECT = 0x10,		///< OR in to the above to specify
   													///< automatic sampling when edge
                                          ///< detected (ATIC command).
	XBEE_IO_TYPE_PULLUP = 0x20,				///< OR in to the above to specify
   													///< pull-up resistor active
                                          ///< (ATPR command).
	XBEE_IO_TYPE_PULLDOWN = 0x40,				///< OR in to the above to specify
   													///< pull-down resistor active
                                          ///< (reserved for future hardware).
   XBEE_IO_FORCE = 0x80,				///< OR in to the above to force transmit
   					 						///< to the device (else only transmits
                                    ///< if changed from last known state)


	XBEE_IO_TYPE_DIGITAL_INPUT_PULLUP =
   		XBEE_IO_TYPE_DIGITAL_INPUT | XBEE_IO_TYPE_PULLUP,
};


/**
	@brief Digital output state for XBee built-in I/Os configured as outputs.
*/
enum xbee_io_digital_output_state {
	XBEE_IO_SET_LOW = XBEE_IO_TYPE_DIGITAL_OUTPUT_LOW,		///< Low (0V)
	XBEE_IO_SET_HIGH = XBEE_IO_TYPE_DIGITAL_OUTPUT_HIGH,	///< High (Vcc)

   XBEE_IO_FORCE_LOW = XBEE_IO_SET_LOW|XBEE_IO_FORCE,
   XBEE_IO_FORCE_HIGH = XBEE_IO_SET_HIGH|XBEE_IO_FORCE,
};


int xbee_io_response_parse( xbee_io_t FAR *parsed, const void FAR *source);
void xbee_io_response_dump( const xbee_io_t FAR *io);

/// Return code from analog I/O query functions if selected I/O is not valid
#define XBEE_IO_ANALOG_INVALID		INT16_C(-32768)

/// Minimum automatic I/O sample period in ms.
#define XBEE_IO_MIN_SAMPLE_PERIOD	50

int xbee_io_get_digital_input( const xbee_io_t FAR *io, uint_fast8_t index);
int xbee_io_get_digital_output( const xbee_io_t FAR *io, uint_fast8_t index);
int16_t xbee_io_get_analog_input( const xbee_io_t FAR *io, uint_fast8_t index);
extern const FAR xbee_at_cmd_t xbee_io_cmd_by_index[];
int xbee_io_set_digital_output( xbee_dev_t *xbee, xbee_io_t FAR *io,
				uint_fast8_t index, enum xbee_io_digital_output_state state,
            const wpan_address_t FAR *address);
int xbee_io_configure( xbee_dev_t *xbee, xbee_io_t FAR *io,
				uint_fast8_t index, enum xbee_io_type type,
            const wpan_address_t FAR *address);
int xbee_io_set_options( xbee_dev_t *xbee, xbee_io_t FAR *io,
				uint16_t sample_rate, uint16_t change_mask,
            const wpan_address_t FAR *address);
int xbee_io_query( xbee_dev_t *xbee, xbee_io_t FAR *io,
            const wpan_address_t FAR *address);
int xbee_io_query_status( xbee_io_t FAR *io);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "xbee_io.c"
#endif

#endif
