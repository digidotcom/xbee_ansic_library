/*
 * Copyright (c) 2011-2012 Digi International Inc.,
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
	@file xbee_io.c

	Code related to I/O sampling and configuration (the ATIS command,
   0x92 frames).

   @note This code is not directly concerned with collecting I/O samples.
   To do that, it is necessary to register a command handler for
   ATIS commands, and/or appropriate endpoints to handle the incoming
   data frames.  If not using the Simple XBee API (SXA) layer, then
   see xbee_sxa.c for examples of how to register the necessary callback
   functions.  Otherwise, it is recommended to use SXA for all I/O
   manipulation.
*/

/*** BeginHeader */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/byteorder.h"
#include "xbee/io.h"

#ifndef __DC__
	#define _xbee_io_debug
#elif defined XBEE_IO_DEBUG
	#define _xbee_io_debug	__debug
#else
	#define _xbee_io_debug	__nodebug
#endif



/*** EndHeader */

/*** BeginHeader xbee_io_response_parse */
/*** EndHeader */
/**
	@brief
	Parse an I/O response and store it in an xbee_io_t structure.

   @param[out] parsed	Result structure
   #param[in] source		Pointer to the start of the data i.e. the num_samples
                        field of the I/O response.
*/
_xbee_io_debug
int xbee_io_response_parse( xbee_io_t FAR *parsed, const void FAR *source)
{
	const char FAR *p = source;
   uint16_t amask, dmask;
   uint16_t asamp, dsamp;

	// The num_samples field is always '1' so include that as a sanity check.
	if (parsed == NULL || source == NULL || *p != 1)
	{
		return -EINVAL;
	}
	++p;	// skip sample count field
   dmask = be16toh( *(uint16_t FAR *)p);
	parsed->din_enabled = dmask & ~parsed->dout_enabled;
   p += sizeof(uint16_t);
   parsed->analog_enabled = *p++;
   // If reported as analog, turn off the shadow indicator
   // that pin was a digital I/O...
   //@todo: the 0x0F constant is an artifact of the sharing of AD0-3 with
   // DIO0-3.  This assumption may not be true with future hardware.
   amask = ~(parsed->analog_enabled & 0x0F);
   parsed->dout_enabled &= amask;
   parsed->din_enabled &= amask;
   if (dmask)
   {
   	// Touch only the bits that were configured as digital ins/outs.
      dsamp = be16toh( *(uint16_t FAR *)p);
   	p += sizeof(uint16_t);
   	parsed->din_state &= ~parsed->din_enabled;
   	parsed->din_state |= parsed->din_enabled & dsamp;
   	parsed->dout_state &= ~parsed->dout_enabled;
   	parsed->dout_state |= parsed->dout_enabled & dsamp;
   }
   if (parsed->analog_enabled)
   {
   	for (amask = 0x01, asamp = 0; asamp < 8; amask <<= 1, ++asamp)
      {
      	if (parsed->analog_enabled & amask)
         {
         	parsed->adc_sample[asamp] = be16toh( *(uint16_t FAR *)p);
   			p += sizeof(uint16_t);
         }
      }
   }

	return 0;
}


/*** BeginHeader xbee_io_response_dump */
/*** EndHeader */
_xbee_io_debug
char * _xbee_mask_to_binary(char * buf, uint16_t mask, char * digits, int maxbit)
{
	uint16_t m;
   char * p = buf;

   for (m = UINT16_C(1)<<(uint8_t)maxbit; m; m >>= 1)
   {
		*p++ = digits[(mask & m) != 0];
   }
	*p = '\0';
   return buf;
}

_xbee_io_debug
char * _xbee_2masks_to_quaternary(char * buf,
		uint16_t mask_lo, uint16_t mask_hi, char * digits, int maxbit)
{
	uint16_t m;
   char * p = buf;

   for (m = UINT16_C(1)<<(uint8_t)maxbit; m; m >>= 1)
   {
		*p++ = digits[((mask_lo & m) != 0) | ((mask_hi & m) != 0)<<1];
   }
	*p = '\0';
   return buf;
}

/**
	@brief
	Debugging function used to dump an xbee_io_t structure to stdout.

	@param[in]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_response_parse()

*/
_xbee_io_debug
void xbee_io_response_dump( const xbee_io_t FAR *io)
{
	uint16_t m;
   int i;
   char buf[20];

	printf( "Digital I/Os:   CBA9876543210\n");
	printf( "  DIN state:    %s\n",
   	_xbee_2masks_to_quaternary(buf, io->din_state, io->din_enabled, "..01", 12));
	printf( "  Pullup/dn:    %s\n",
   	_xbee_2masks_to_quaternary(buf, io->pulldown_state, io->pullup_state, "-01?", 12));
	printf( "  DOUT state:   %s\n",
   	_xbee_2masks_to_quaternary(buf, io->dout_state, io->dout_enabled, "..01", 12));
   if (io->analog_enabled)
   {
	   printf( "Analog state:\n");
	   for (m = 1, i = 0;
	         i < _TABLE_ENTRIES(io->adc_sample);
	         m <<= 1, ++i)
	   {
	      if (io->analog_enabled & m)
	      {
	         printf( " %4u", io->adc_sample[i]);
	      }
	      else
	      {
	         printf("  -  ");
	      }
	   }
      printf("\n");
   }
}


/*** BeginHeader xbee_io_get_digital_input */
/*** EndHeader */
/**
	@brief
	Return state of a digital input.

	@param[in]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_response_parse()
   @param[in]	index digital input number e.g. 0 for DIO0, 12 for DIO12.
	@retval	0	Input low
	@retval	1	Input high
	@retval	-EINVAL
   				Input unknown because not configured as a digital input,
   				or is a non-existent input index.
*/

_xbee_io_debug
int xbee_io_get_digital_input( const xbee_io_t FAR *io, uint_fast8_t index)
{
	uint16_t m = 1u << index;

   if (io && (m & io->din_enabled))
   {
		return (m & io->din_state) != 0;
   }
   return -EINVAL;
}

/*** BeginHeader xbee_io_get_digital_output */
/*** EndHeader */
/**
	@brief
	Return state of a digital output.  This is a shadow state i.e. the
   last known state setting.

	@param[in]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_query()
   @param[in]	index digital output number e.g. 0 for DIO0, 2 for DIO2.
	@retval	0	Output low
	@retval	1	Output high
	@retval	-EINVAL
					Output unknown because not configured as a digital output,
   				or is a non-existent output index.
*/

_xbee_io_debug
int xbee_io_get_digital_output( const xbee_io_t FAR *io, uint_fast8_t index)
{
	uint16_t m = 1u << index;

   if (io && (m & io->dout_enabled))
   {
		return (m & io->dout_state) != 0;
   }
   return -EINVAL;
}

/*** BeginHeader xbee_io_get_analog_input */
/*** EndHeader */
/**
	@brief
	Return reading of an analog input.

	@param[in]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_response_parse()
   @param[in]	index analog input number e.g. 0 for AD0, 2 for AD2,
   				7 for Vcc reading (where available)
	@retval	0..32767 raw single-ended analog reading, normalized to the
   				range 0..32767.  Current hardware supports 10-bit ADCs,
               hence the 5 LSBs will be zero.  Future hardware with higher
               resolution ADCs will add precision to the LSBs.
	@retval	-16384..16383 raw differential analog reading, normalized to the
   				range -16384..16383.  This is reserved for future hardware.
	@retval	XBEE_IO_ANALOG_INVALID
   				Output unknown because not configured as an analog input,
   				or is a non-existent input index.
*/

_xbee_io_debug
int16_t xbee_io_get_analog_input( const xbee_io_t FAR *io, uint_fast8_t index)
{
	uint16_t m = 1u << index;

   if (io && (m & io->analog_enabled))
   {
		return io->adc_sample[index] << 5;
   }
   return XBEE_IO_ANALOG_INVALID;
}

/*** BeginHeader xbee_io_cmd_by_index */
/*** EndHeader */
// AT command by I/O index.  Null entries used if non-configurable.
const FAR xbee_at_cmd_t xbee_io_cmd_by_index[16] =
{
	{{'D','0'}},
	{{'D','1'}},
	{{'D','2'}},
	{{'D','3'}},
	{{'D','4'}},
	{{'D','5'}},
	{{'\0','\0'}},
	{{'\0','\0'}},
	{{'D','8'}},
	{{'\0','\0'}},
	{{'P','0'}},
	{{'P','1'}},
	{{'P','2'}},
	{{'P','3'}},
};


/*** BeginHeader xbee_io_set_digital_output */
/*** EndHeader */
/**
	@brief
	Set state of a digital output.

   This modifies the shadow state in the io parameter, as well as sending the
   appropriate configuration command to the target device.  If the new
   state is the same as the shadow state, then the function returns without
   doing anything, unless XBEE_IO_FORCE is specified in the state parameter.

	@param[in,out]	xbee	local device through which to action the request
	@param[in,out]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_query().  Shadow state of I/O
                     may be modified
   @param[in]	index digital output number e.g. 0 for DIO0, 2 for DIO2.
   @param[in]  state new state of I/O.  If the XBEE_IO_FORCE flag is
   				ORed in, then force a state update to the device, whether or
               not configured as an input or with shadow state unchanged.
               This can be used to initially configure and set a digital
               output.  Otherwise, a state change will only be sent to
               the device if the shadow (i.e. last known) state is opposite
               AND the I/O is configured as a digital output.
   @param[in]  address NULL for local XBee, or destination IEEE (64-bit)
               or network (16 bit) address of a remote device.  See
               \a wpan_envelope_create() for use of IEEE and network
               addressing.
	@retval	0	Output low
	@retval	1	Output high
	@retval	-EINVAL
					Output unknown because not configured as a digital output
               (and force was not specified), or is a non-existent output
               index, or bad parameter passed.
	@retval	<0
   				Other negative value indicates problem transmitting the
               configuration request.
*/


_xbee_io_debug
int xbee_io_set_digital_output( xbee_dev_t *xbee, xbee_io_t FAR *io,
				uint_fast8_t index, enum xbee_io_digital_output_state state,
            const wpan_address_t FAR *address)
{
	uint16_t m = 1u << index;
	bool_t force = (state & XBEE_IO_FORCE) != 0;
   const xbee_at_cmd_t FAR *cmdtab;
   uint16_t next_state;
	int16_t request;

   state &= XBEE_IO_TYPE_MASK;

   if (!io || !xbee || !m ||
        (state != XBEE_IO_SET_LOW &&
         state != XBEE_IO_SET_HIGH))
   {
   	return -EINVAL;
   }
   cmdtab = xbee_io_cmd_by_index + index;
   if (!cmdtab->w || (!force && !(io->dout_enabled & m)))
   {
   	return -EINVAL;
   }
   next_state = io->dout_state & ~m;
   if (state == XBEE_IO_SET_HIGH)
   {
   	next_state |= m;
   }
   if (force || io->dout_state != next_state)
   {
   	// Transmit I/O command
		request = xbee_cmd_create( xbee, cmdtab->str);
		if (request < 0)
      {
      	return request;
      }
      xbee_cmd_set_param( request, state);
		if (address)
      {
      	xbee_cmd_set_target( request, &address->ieee, address->network);
      }
      xbee_cmd_send( request);
      // Maintain shadow...
      io->config[index] = (uint8_t)state;
   }
   // Perhaps it is a bit premature to assume command will complete, but
   // subsequent I/O sample will correct the situation if necessary.
   io->dout_state = next_state;
   io->dout_enabled |= m;
   return state == XBEE_IO_SET_HIGH;
}


/*** BeginHeader xbee_io_configure */
/*** EndHeader */
/**
	@brief Configure XBee digital and analog I/Os.

   This modifies the shadow state in the io parameter, as well as sending the
   appropriate configuration command to the target device.  If the new
   state is the same as the shadow state, then the function returns without
   doing anything, unless XBEE_IO_FORCE is specified in the type parameter.

	@param[in,out]	xbee	local device through which to action the request
	@param[in,out]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_query().  Shadow state of I/O
                     may be modified
   @param[in]	index digital or analog I/O number e.g. 0 for DIO0 or AD0.
   @param[in]  type type of I/O to configure.  If the XBEE_IO_FORCE flag is
   				ORed in, force a configuration update to the device.
   				Otherwise, a configuration change will only be sent to
               the device if the shadow (i.e. last known) configuration is
               different.
   @param[in]  address NULL for local XBee, or destination IEEE (64-bit)
               or network (16 bit) address of a remote device.

	@retval	0	Success
	@retval	-EINVAL
					Specified I/O index does not exist, or bad parameter.
	@retval	-ENOSPC	the AT command table is full (increase the compile-time
							macro XBEE_CMD_REQUEST_TABLESIZE)
	@retval	-EPERM
               Specified I/O cannot be configured as requested because the
               hardware or firmware does not support it.
	@retval	<0
   				Other negative value indicates problem transmitting the
               configuration request.
*/

_xbee_io_debug
int xbee_io_configure( xbee_dev_t *xbee, xbee_io_t FAR *io,
				uint_fast8_t index, enum xbee_io_type type,
            const wpan_address_t FAR *address)
{
	uint16_t m = 1u << index;
   uint16_t not_m;
	bool_t force = (type & XBEE_IO_FORCE) != 0;
   const xbee_at_cmd_t FAR *cmdtab;
	int16_t request;

   type &= XBEE_IO_TYPE_MASK;

   if (!io || !xbee || !m)
   {
   	return -EINVAL;
   }
   //@todo: this is a relatively crude test.  Need to look at hardware
   // and firmware versions to really make a proper check.  For now, this
   // is about as tight as we can get it.
   if (type > XBEE_IO_TYPE_TXEN_ACTIVE_HIGH ||
       (type > XBEE_IO_TYPE_DIGITAL_OUTPUT_HIGH && index != 7) ||
       (type == XBEE_IO_TYPE_ANALOG_INPUT && index >= 4) ||
       (type == XBEE_IO_TYPE_SPECIAL &&
       		(index >= 11 || (index >= 1 && index < 5)))
       )
   {
   	return -EPERM;
   }

   cmdtab = xbee_io_cmd_by_index + index;
   if (!cmdtab->w)
   {
   	return -EINVAL;
   }
   if (force || io->config[index] != type)
   {
   	// Transmit I/O command
		request = xbee_cmd_create( xbee, cmdtab->str);
		if (request < 0)
      {
      	return request;
      }
      xbee_cmd_set_param( request, type);
		if (address)
      {
      	xbee_cmd_set_target( request, &address->ieee, address->network);
      }
      xbee_cmd_send( request);
      // Maintain shadow...
      io->config[index] = (uint8_t)type;
      not_m = ~m;
		io->din_enabled &= not_m;
		io->dout_enabled &= not_m;
		io->analog_enabled &= not_m;
		io->dout_state &= not_m;
      switch (type)
      {
      	default:
            break;
      	case XBEE_IO_TYPE_ANALOG_INPUT:
				io->analog_enabled |= m;
				break;
      	case XBEE_IO_TYPE_DIGITAL_INPUT:
				io->din_enabled |= m;
				break;
      	case XBEE_IO_TYPE_DIGITAL_OUTPUT_LOW:
				io->dout_enabled |= m;
				break;
      	case XBEE_IO_TYPE_DIGITAL_OUTPUT_HIGH:
				io->dout_enabled |= m;
            io->dout_state |= m;
				break;
      }
   }
   return 0;
}


/*** BeginHeader xbee_io_set_options */
/*** EndHeader */
/**
	@brief Configure XBee automatic I/O sampling options.

   This basically controls the ATIR and ATIC settings.  IR specifies an
   automatic sampling interval, and IC specifies sampling on digital
   I/O change.

	@param[in,out]	xbee	local device through which to action the request
	@param[in,out]	io		pointer to an xbee_io_t structure as set up by
   						xbee_io_query().  Shadow state of I/O
                     may be modified
   @param[in]	sample_rate sample period in ms.  0 to turn off sampling,
   					otherwise must be a value geater than
                  XBEE_IO_MIN_SAMPLE_PERIOD (50 ms with current hardware).
   @param[in]  change_mask bitmask of I/Os which are to generate samples when
   					their state changes.  Construct from ORed combination of
                  #XBEE_IO_DIO0, #XBEE_IO_DIO1 etc.
   @param[in]  address NULL for local XBee, or destination IEEE (64-bit)
               or network (16 bit) address of a remote device.
	@retval	0	Success
	@retval	-EINVAL
					Bad parameter.
	@retval	-EPERM
               Specified sampling rate or I/O bitmask not supported.
	@retval	<0
   				Other negative value indicates problem transmitting the
               configuration request.
*/

_xbee_io_debug
int xbee_io_set_options( xbee_dev_t *xbee, xbee_io_t FAR *io,
				uint16_t sample_rate, uint16_t change_mask,
            const wpan_address_t FAR *address)
{
	int16_t request;

   if (!io || !xbee)
   {
   	return -EINVAL;
   }
   if (sample_rate && sample_rate < XBEE_IO_MIN_SAMPLE_PERIOD)
   {
   	return -EPERM;
   }

   if (sample_rate != io->sample_rate)
   {
		request = xbee_cmd_create( xbee, "IR");
		if (request < 0)
      {
      	return request;
      }
      xbee_cmd_set_param( request, sample_rate);
		if (address)
      {
      	xbee_cmd_set_target( request, &address->ieee, address->network);
      }
      xbee_cmd_send( request);
      // Maintain shadow...
      io->sample_rate = sample_rate;
   }
   if (change_mask != io->change_mask)
   {
		request = xbee_cmd_create( xbee, "IC");
		if (request < 0)
      {
      	return request;
      }
      xbee_cmd_set_param( request, change_mask);
		if (address)
      {
      	xbee_cmd_set_target( request, &address->ieee, address->network);
      }
      xbee_cmd_send( request);
      // Maintain shadow...
      io->change_mask = change_mask;
   }
   return 0;
}


/*** BeginHeader xbee_io_query */
/*** EndHeader */
/**
	@brief Read current configuration of XBee digital and analog I/Os.

   This sets the shadow state in the io parameter by querying the
   device configuration with a sequence of AT commands (D0,D0,...P0,...,PR).
   Unless the application has prior knowledge of the I/O config, this
   function should be used when a new node is discovered.

   Since several commands must be executed, the results are not available
   immediately on return from this function.  Instead, the application
   must call xbee_io_query_status() in order to poll for
   command completion.

	@param[in,out]	xbee	local device through which to action the request
	@param[in,out]	io		pointer to an xbee_io_t structure, which will be
   							modified by this call.  Since the results are
                        returned asynchronously, the data pointed to must
                        be static.
   @param[in]  address NULL for local XBee, or destination IEEE (64-bit)
               or network (16 bit) address of a remote device.
	@retval	0	Success
	@retval	-EINVAL
					Bad parameter.
	@retval	-EBUSY
					Device is currently busy with another request for this
               device.  Try again later (after calling xbee_cmd_tick()).
               In general, several get configuration requests can run
               simultaneously, however only one per remote device.
	@retval	<0
   				Other negative value indicates problem transmitting the
               configuration query commands.

   @see xbee_io_query_status()
*/

const uint16_t _xbee_pullup_atcmd_to_iomask[16] =
{
	// Map bit number from ATPR command to normal I/O bitmask.  Zero for
   // unimplemented bits
	1u<<4,
	1u<<3,
	1u<<2,
	1u<<1,
	1u<<0,
	1u<<6,
	1u<<8,
	0,			// DIN/CONFIG is not a configurable I/O
	1u<<5,
	1u<<9,
	1u<<12,
	1u<<10,
	1u<<11,
	1u<<7,
	0,
	0
};

_xbee_io_debug
void _xbee_io_query_handle_pr(
			const xbee_cmd_response_t FAR *response,
   		const struct xbee_atcmd_reg_t FAR	*reg,
			void FAR										*base
			)
{
	xbee_io_t FAR *io = base;
   uint_fast8_t i;
   uint16_t mask = (uint16_t)response->value;
   uint16_t imask;
   uint16_t pmask;

   for (i = 0, imask = 1; i < 16; ++i, imask <<= 1)
   {
		pmask = _xbee_pullup_atcmd_to_iomask[i];
   	if (pmask)
      {
      	if (mask & imask)
         {
         	io->pullup_state |= pmask;
         }
         else
         {
         	io->pullup_state &= ~pmask;
         }
      }
   }
}


_xbee_io_debug
void _xbee_io_query_handle_end(
			const xbee_cmd_response_t FAR *response,
   		const struct xbee_atcmd_reg_t FAR	*reg,
			void FAR										*base
			)
{
	xbee_io_t FAR *io = base;
   uint_fast8_t i;
   uint16_t imask, nimask;

   // Command list completed, translate from config[] field numbers
   // to I/O state bitmasks
   for (i = 0, imask = 1; i < 16; ++i, imask <<= 1)
   {
   	nimask = ~imask;
      // Assume nothing...
      io->din_enabled &= nimask;
      io->dout_enabled &= nimask;
      io->analog_enabled &= nimask;
   	switch (io->config[i])
      {
      default:	// special or disbled
      	break;
      case XBEE_IO_TYPE_ANALOG_INPUT:
      	io->analog_enabled |= imask;
      	break;
      case XBEE_IO_TYPE_DIGITAL_INPUT:
      	io->din_enabled |= imask;
      	break;
      case XBEE_IO_TYPE_DIGITAL_OUTPUT_LOW:
      	io->dout_enabled |= imask;
         io->dout_state &= nimask;
      	break;
      case XBEE_IO_TYPE_DIGITAL_OUTPUT_HIGH:
      	io->dout_enabled |= imask;
         io->dout_state |= imask;
      	break;
      }
   }
}


const xbee_atcmd_reg_t _xbee_io_query_regs[] = {
	XBEE_ATCMD_REG( 'D', '0', XBEE_CLT_COPY, xbee_io_t, config[0]),
	XBEE_ATCMD_REG( 'D', '1', XBEE_CLT_COPY, xbee_io_t, config[1]),
	XBEE_ATCMD_REG( 'D', '2', XBEE_CLT_COPY, xbee_io_t, config[2]),
	XBEE_ATCMD_REG( 'D', '3', XBEE_CLT_COPY, xbee_io_t, config[3]),
	XBEE_ATCMD_REG( 'D', '4', XBEE_CLT_COPY, xbee_io_t, config[4]),
	XBEE_ATCMD_REG( 'D', '5', XBEE_CLT_COPY, xbee_io_t, config[5]),
	XBEE_ATCMD_REG( 'D', '6', XBEE_CLT_COPY, xbee_io_t, config[6]),
	XBEE_ATCMD_REG( 'D', '7', XBEE_CLT_COPY, xbee_io_t, config[7]),
	XBEE_ATCMD_REG( 'P', '0', XBEE_CLT_COPY, xbee_io_t, config[10]),
	XBEE_ATCMD_REG( 'P', '1', XBEE_CLT_COPY, xbee_io_t, config[11]),
	XBEE_ATCMD_REG( 'P', '2', XBEE_CLT_COPY, xbee_io_t, config[12]),
	XBEE_ATCMD_REG_CB( 'P', 'R', _xbee_io_query_handle_pr, 0),
	XBEE_ATCMD_REG( 'I', 'R', XBEE_CLT_COPY_BE, xbee_io_t, sample_rate),
	XBEE_ATCMD_REG( 'I', 'C', XBEE_CLT_COPY_BE, xbee_io_t, change_mask),
   XBEE_ATCMD_REG_END_CB(_xbee_io_query_handle_end, 0)
};


_xbee_io_debug
int xbee_io_query( xbee_dev_t *xbee, xbee_io_t FAR *io,
            const wpan_address_t FAR *address)
{
	if (!xbee || !io)
   {
   	return -EINVAL;
   }
	if (xbee_io_query_status(io) == -EBUSY)
   {
   	return -EBUSY;
   }
	return xbee_cmd_list_execute(xbee,
   								&io->clc,
									_xbee_io_query_regs,
                           io,
                           address);
}


/*** BeginHeader xbee_io_query_status */
/*** EndHeader */
/**
	@brief
	Check the status of querying an XBee device, as initiated by
	xbee_io_query().

	@param[in]	io		I/O query to check (pointer as passed to
   						xbee_io_query()).

	@retval	0				query completed
	@retval	-EINVAL		\a io is NULL
	@retval	-EBUSY		query underway
	@retval	-ETIMEDOUT	query timed out
   @retval  -EIO     	halted, but query may not have completed
   								(unexpected response)
*/
_xbee_io_debug
int xbee_io_query_status( xbee_io_t FAR *io)
{
	if (! io)
	{
		return -EINVAL;
	}

	xbee_cmd_tick();

   return xbee_cmd_list_status(&io->clc);
}




