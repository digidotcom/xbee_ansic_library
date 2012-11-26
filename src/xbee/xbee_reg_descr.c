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
	@addtogroup SXA
	@{
	@file xbee_reg_descr.c
	Code related to the register descriptor table.

   The XBee register descriptor table is initialized here.

   @note sxa.h needs to be included before including this header.

   @note This header is sensitive to the macro IDIGI_USE_XBEE.  If defined,
   then extra fields are included in the table in order to support iDigi.
   Otherwise, the fields are omitted so as to save storage.
*/
/*** BeginHeader _xbee_reg_table, _xbee_rci_types */

#include "xbee/reg_descr.h"
#include "xbee/sxa.h"

/*** EndHeader */


#ifdef IDIGI_USE_XBEE

#define _XRT_ENTRY(state,tag,at,cxv,desc,units,min,max,rci_type,sxa_field,group) \
	{ state, #at, cxv, rci_type, \
   		#tag, #desc, #units, #min, #max, \
   		offsetof(sxa_node_t,sxa_field), \
         sizeof((*(sxa_node_t *)NULL).sxa_field), \
         SXA_CACHED_##group }

#else

#define _XRT_ENTRY(state,tag,at,cxv,desc,units,min,max,rci_type,sxa_field,group) \
	{ state, #at, cxv, rci_type, \
   		offsetof(sxa_node_t,sxa_field), \
         sizeof((*(sxa_node_t *)NULL).sxa_field), \
         SXA_CACHED_##group }

#endif

// The following taken from ZigBeeDDOState.hh (NDS source code)
// Flags indicate which module types support a property.
// Flag bits correspond to the module number in the device type.
#define F8      0x0002      // FreeScale 802.15.4
#define EZ      0x0004      // Ember ZNet 2.5
#define ZB      0x0408      // Ember ZB (including S2C)
#define X9      0x0010      // XBee DigiMesh 900
#define X2      0x0020      // XBee DigiMesh 2.4
#define X8      0x0040      // XBee 868
#define DP      0x0080      // XBee DP 900

// Flag bits not based on module number
#define FZ      0x0001      // FreeScale ZigBee
#define SE      0x0100      // XBee Smart Energy
#define ALL     0x07ff		 // Any module type
#define GW      0x8000		 // Gateway radio



const char *_xbee_rci_types[] =
{
	// Must be in same order as XBEE_RCI_TYPE_* enum
	"uint32",
   "0x_hex32",
   "string",
   "string",
   "xbee_ext_addr",
};


// NOTE: if using 'MISC' for the SXA cache group, then the corresponding field
// is the name of an sxa_cached_* struct of the appropriate type.  The first
// field at that address is a cache flags field, which is followed by the
// actual data.  Length of data is
//   sizeof(the cache struct) - _SXA_CACHED_PREFIX_SIZE.

const FAR _xbee_reg_descr_t _xbee_reg_table[] =
{
	// settings
	_XRT_ENTRY(0,aggregation,		AR,EZ|ZB|SE|GW,Aggregation route notification,x 10 sec,		0,255,XBEE_RCI_TYPE_UINT32,aggregation,MISC)
  ,_XRT_ENTRY(0,assoc_led,			LT,EZ|ZB|X9|X2,Associate LED blink time,x 10 ms,				10,255,XBEE_RCI_TYPE_UINT32,assoc_led,MISC)
  ,_XRT_ENTRY(0,beacon_response,	BR,SE|GW,Beacon response,,												,1,XBEE_RCI_TYPE_UINT32,beacon_response,MISC)
  ,_XRT_ENTRY(0,broadcast_hops,	BH,FZ|EZ|ZB|SE|X9|X2|GW,Broadcast radius,,						,32,XBEE_RCI_TYPE_UINT32,broadcast_hops,MISC)
  ,_XRT_ENTRY(0,cca_threshold,	CA,F8|FZ|GW,CCA threshold,-dBm,										36,80,XBEE_RCI_TYPE_UINT32,cca_threshold,MISC)
  ,_XRT_ENTRY(0,channel,			CH,F8|X2|GW,Operating channel,,										0x0B,0x1A,XBEE_RCI_TYPE_HEX32,channel,MISC)
  ,_XRT_ENTRY(0,command_char,		CC,ALL,Command sequence character,,									1,1,XBEE_RCI_TYPE_STRING,command_char,MISC)
  ,_XRT_ENTRY(0,cluster_id,		CI,EZ|ZB,Cluster identifier,,											,0xffff,XBEE_RCI_TYPE_HEX32,cluster_id,MISC)
  ,_XRT_ENTRY(0,command_timeout,	CT,F8|FZ|EZ|ZB|DP|X8|X9|X2,Command mode timeout,x 100 ms,	2,65535,XBEE_RCI_TYPE_UINT32,command_timeout,MISC)
  ,_XRT_ENTRY(0,coord_assoc,		A2,F8|GW,Coordinator association,bitfield,						,7,XBEE_RCI_TYPE_UINT32,coord_assoc,MISC)
  ,_XRT_ENTRY(0,coord_enable,		CE,F8|ZB|SE|X9|X2|GW,Coordinator enable,,							,2,XBEE_RCI_TYPE_UINT32,coord_enable,MISC)
  ,_XRT_ENTRY(0,delay_slots,		RN,F8|FZ|GW,Random delay slots,,										,3,XBEE_RCI_TYPE_UINT32,delay_slots,MISC)
  ,_XRT_ENTRY(0,dest_addr,			DH/DL,ALL,Destination address,,										,,XBEE_RCI_TYPE_ADDR64,dest_addr,DHDL)
  ,_XRT_ENTRY(0,dest_endpoint,	DE,EZ|ZB,Destination endpoint,,										0x1,0xf0,XBEE_RCI_TYPE_HEX32,dest_endpoint,MISC)
  ,_XRT_ENTRY(0,device_options,	DO,SE|GW,Device options,bitfield,									,0xff,XBEE_RCI_TYPE_HEX32,device_options,MISC)
  // The following DIO (and PWM) configs have GW bit added, since the Rabbit GW Xbee device should not
  // have too many restrictions on setting these.
  ,_XRT_ENTRY(0,dio0_config,		D0,GW|F8|EZ|ZB|X9|DP|X8|X2,AD0/DIO0 configuration,,				,5,XBEE_RCI_TYPE_UINT32,io.config[0],IO_CONFIG)
  ,_XRT_ENTRY(0,dio1_config,		D1,GW|F8|EZ|ZB|X9|DP|X8|X2,AD1/DIO1 configuration,,				,5,XBEE_RCI_TYPE_UINT32,io.config[1],IO_CONFIG)
  ,_XRT_ENTRY(0,dio2_config,		D2,GW|F8|EZ|ZB|X9|DP|X8|X2,AD2/DIO2 configuration,,				,5,XBEE_RCI_TYPE_UINT32,io.config[2],IO_CONFIG)
  ,_XRT_ENTRY(0,dio3_config,		D3,GW|F8|EZ|ZB|X9|DP|X8|X2,AD3/DIO3 configuration,,				,5,XBEE_RCI_TYPE_UINT32,io.config[3],IO_CONFIG)
  ,_XRT_ENTRY(0,dio4_config,		D4,GW|F8|EZ|ZB|X9|DP|X8|X2,AD4/DIO4 configuration,,				,5,XBEE_RCI_TYPE_UINT32,io.config[4],IO_CONFIG)
  ,_XRT_ENTRY(0,dio5_config,		D5,GW|ALL,DIO5/Assoc configuration,,									,5,XBEE_RCI_TYPE_UINT32,io.config[5],IO_CONFIG)
  ,_XRT_ENTRY(0,dio6_config,		D6,GW|F8|EZ|ZB|X9|DP|X8|X2,DIO6 configuration,,						,5,XBEE_RCI_TYPE_UINT32,io.config[6],IO_CONFIG)
  ,_XRT_ENTRY(0,dio7_config,		D7,GW|ALL,DIO7 configuration,,											,7,XBEE_RCI_TYPE_UINT32,io.config[7],IO_CONFIG)
  ,_XRT_ENTRY(0,dio8_config,		D8,GW|F8|EZ|ZB|X9|DP|X8|X2,DIO8/SleepRQ configuration,,			,7,XBEE_RCI_TYPE_UINT32,io.config[8],IO_CONFIG)
  ,_XRT_ENTRY(0,dio9_config,		D9,GW|ZB|X9|X2,DIO9/ON_SLEEP configuration,,							,7,XBEE_RCI_TYPE_UINT32,io.config[9],IO_CONFIG)
  ,_XRT_ENTRY(0,dio10_config,		P0,GW|EZ|ZB|X9|DP|X8|X2,DIO10/PWM0 configuration,,					,5,XBEE_RCI_TYPE_UINT32,io.config[10],IO_CONFIG)
  ,_XRT_ENTRY(0,dio11_config,		P1,GW|EZ|ZB|X9|DP|X8|X2,DIO11/PWM1 configuration,,					,5,XBEE_RCI_TYPE_UINT32,io.config[11],IO_CONFIG)
  ,_XRT_ENTRY(0,dio12_config,		P2,GW|F8|EZ|ZB|X9|DP|X8|X2,DIO12/CD configuration,,				,5,XBEE_RCI_TYPE_UINT32,io.config[12],IO_CONFIG)
  ,_XRT_ENTRY(0,dio13_config,		P3,GW|F8|EZ|ZB|X9|DP|X8,DIO13/DOUT configuration,,					,5,XBEE_RCI_TYPE_UINT32,io.config[13],IO_CONFIG)
  ,_XRT_ENTRY(0,dio_detect,		IC,GW|F8|EZ|ZB|X9|X2,DIO change detect,bitfield,					,0xffff,XBEE_RCI_TYPE_HEX32,io.change_mask,IO_CONFIG)
  ,_XRT_ENTRY(0,discover_timeout,NT,GW|F8|FZ|EZ|ZB|SE,Node discovery timeout,x 100 ms,			32,255,XBEE_RCI_TYPE_UINT32,discover_timeout,MISC)
  ,_XRT_ENTRY(0,discover_timeout,NT,GW|X9|DP|X8|X2,Node discovery timeout,x 100 ms,				32,12000,XBEE_RCI_TYPE_UINT32,discover_timeout,MISC)
  ,_XRT_ENTRY(0,encrypt_enable,	EE,F8|EZ|ZB|X9|DP|X8|X2|GW,Encryption enable,,					,1,XBEE_RCI_TYPE_UINT32,encrypt_enable,MISC)
  ,_XRT_ENTRY(0,encrypt_options,	EO,EZ|ZB|SE|GW,Encryption options,bitfield,						,0x7,XBEE_RCI_TYPE_HEX32,encrypt_options,MISC)
  ,_XRT_ENTRY(0,end_assoc,			A1,F8|GW,End device association,bitfield,							,0xf,XBEE_RCI_TYPE_HEX32,end_assoc,MISC)
  ,_XRT_ENTRY(0,ext_pan_id,		ID,ZB|SE|GW,Extended PAN identifier,,								,18,XBEE_RCI_TYPE_BIN,ext_pan_id,MISC)
  ,_XRT_ENTRY(0,flow_threshold,	FT,X9|DP|X8|X2,Flow control threshold,bytes,						1,382,XBEE_RCI_TYPE_UINT32,flow_threshold,MISC)
  ,_XRT_ENTRY(0,guard_times,		GT,ALL,Guard times,ms,													2,3300,XBEE_RCI_TYPE_UINT32,guard_times,MISC)
  ,_XRT_ENTRY(0,hop_sequence,		HP,X9|DP|GW,Hopping sequence,,										,7,XBEE_RCI_TYPE_UINT32,hop_sequence,MISC)
  ,_XRT_ENTRY(0,io_address,		IA,F8,I/O input address,,												,,XBEE_RCI_TYPE_ADDR64,io_address,MISC)
  ,_XRT_ENTRY(0,io_enable,			IU,F8,I/O output enable,,												,1,XBEE_RCI_TYPE_UINT32,io_enable,MISC)
  ,_XRT_ENTRY(0,initial_pan_id,	II,ZB|SE|GW,Initial PAN identifier,,								,0xffff,XBEE_RCI_TYPE_HEX32,initial_pan_id,MISC)
  ,_XRT_ENTRY(0,join_time,			NJ,SE|FZ|EZ|ZB|GW,Node join time,sec,								,255,XBEE_RCI_TYPE_UINT32,join_time,MISC)
  ,_XRT_ENTRY(0,join_notification,JN,ZB|GW,Join notification,,											,2,XBEE_RCI_TYPE_UINT32,join_notification,MISC)
  ,_XRT_ENTRY(0,join_verification,JV,EZ|ZB|GW,Join verification,,										,1,XBEE_RCI_TYPE_UINT32,join_verification,MISC)
  ,_XRT_ENTRY(0,link_key,			KY,F8|EZ|ZB|SE|X9|DP|X8|X2|GW,Link encryption key,,			,34,XBEE_RCI_TYPE_BIN,link_key,MISC)
  ,_XRT_ENTRY(0,mac_mode,			MM,F8|GW,MAC mode,,														,3,XBEE_RCI_TYPE_UINT32,mac_mode,MISC)
  ,_XRT_ENTRY(0,mac_retries,		RR,X8|X9|DP|X2|GW,Mac retries,,										,15,XBEE_RCI_TYPE_UINT32,mac_retries,MISC)
  ,_XRT_ENTRY(0,max_hops,			NH,X9|X2|ZB|SE|GW,Maximum hops,,										1,255,XBEE_RCI_TYPE_UINT32,max_hops,MISC)
  ,_XRT_ENTRY(0,mesh_retries,		MR,X9|X2|GW,Mesh network retries,,									,7,XBEE_RCI_TYPE_UINT32,mesh_retries,MISC)
  ,_XRT_ENTRY(0,multi_transmit,	MT,X9|DP|X8|X2|GW,Broadcast retries,,								,15,XBEE_RCI_TYPE_UINT32,multi_transmit,MISC)
  ,_XRT_ENTRY(0,net_addr,			MY,F8|GW,Network address,,												,0xffff,XBEE_RCI_TYPE_HEX32,id.network_addr,NONE)
  ,_XRT_ENTRY(0,net_delay_slots,	NN,X9|X2|GW,Network delay slots,,									1,10,XBEE_RCI_TYPE_UINT32,net_delay_slots,MISC)
  ,_XRT_ENTRY(0,network_key,		NK,ZB|SE|GW,Network encryption key,,								,34,XBEE_RCI_TYPE_BIN,network_key,MISC)
  ,_XRT_ENTRY(0,network_watchdog,NW,ZB|GW,Network watchdog timeout,min,								,25855,XBEE_RCI_TYPE_UINT32,network_watchdog,MISC)
  ,_XRT_ENTRY(0,node_id,			NI,F8|FZ|EZ|ZB|X9|DP|X8|X2|GW,Node identifier,,					,20,XBEE_RCI_TYPE_STRING,id.node_info,NODE_ID)
  ,_XRT_ENTRY(0,packet_timeout,	RO,ALL,Packetization timeout,chars,									,255,XBEE_RCI_TYPE_UINT32,packet_timeout,MISC)
  ,_XRT_ENTRY(0,pan_id,				ID,F8|FZ|EZ|X9|DP|X8|X2|GW,PAN identifier,,						,0xffff,XBEE_RCI_TYPE_HEX32,pan_id,MISC)
  ,_XRT_ENTRY(0,polling_rate,		PO,ZB,Polling rate,x 10 ms,											,1000,XBEE_RCI_TYPE_UINT32,polling_rate,MISC)
  ,_XRT_ENTRY(0,power_level,		PL,F8|FZ|EZ|ZB|SE|X2|X8|GW,Transmit power level,,				,4,XBEE_RCI_TYPE_UINT32,power_level,MISC)
  ,_XRT_ENTRY(0,power_mode,		PM,EZ|ZB|SE|GW,Power mode,,											,1,XBEE_RCI_TYPE_UINT32,power_mode,MISC)
  ,_XRT_ENTRY(0,pullup_direction,PD,ZB,Pull-up/down direction,bitfield,								,0x7fff,XBEE_RCI_TYPE_HEX32,io.pullup_direction,IO_CONFIG)
  ,_XRT_ENTRY(0,pullup_enable,	PR,F8|EZ|ZB|X9|DP|X8|X2,Pull-up resistor enable,bitfield,	,0x7fff,XBEE_RCI_TYPE_HEX32,io.pullup_enable,IO_CONFIG)
  ,_XRT_ENTRY(0,pwm0_config,		P0,GW|F8|FZ,PWM0 configuration,,											,2,XBEE_RCI_TYPE_UINT32,io.config[10],IO_CONFIG)
  ,_XRT_ENTRY(0,pwm0_level,		M0,GW|X9|DP|X8|X2,PWM0 output level,,									,1023,XBEE_RCI_TYPE_UINT32,io.pwm0_level,IO_CONFIG)
  ,_XRT_ENTRY(0,pwm1_config,		P1,GW|F8,PWM1 configuration,,												,2,XBEE_RCI_TYPE_UINT32,io.config[11],IO_CONFIG)
  ,_XRT_ENTRY(0,pwm1_level,		M1,GW|X9|DP|X8|X2,PWM1 output level,,									,1023,XBEE_RCI_TYPE_UINT32,io.pwm1_level,IO_CONFIG)
  ,_XRT_ENTRY(0,pwm_timeout,		PT,GW|F8,PWM output timeout,x 100 ms,									,255,XBEE_RCI_TYPE_UINT32,io.pwm_timeout,IO_CONFIG)
  ,_XRT_ENTRY(0,rssi_timer,		RP,ALL,RSSI PWM timer,x 100 ms,										,255,XBEE_RCI_TYPE_UINT32,rssi_timer,MISC)
  ,_XRT_ENTRY(0,sample_rate,		IR,F8|EZ|ZB|X9|X2,I/O sample rate,ms,								0,65535,XBEE_RCI_TYPE_UINT32,io.sample_rate,IO_CONFIG)
  ,_XRT_ENTRY(0,scan_channels,	SC,F8|FZ|EZ|ZB|SE|GW,Scan channels,bitmask,						0x1,0xffff,XBEE_RCI_TYPE_HEX32,scan_channels,MISC)
  ,_XRT_ENTRY(0,scan_duration,	SD,F8|FZ|EZ|ZB|SE|GW,Scan duration,exponent,						,15,XBEE_RCI_TYPE_UINT32,scan_duration,MISC)
  ,_XRT_ENTRY(0,serial_parity,	NB,ALL,Serial interface parity,,										,4,XBEE_RCI_TYPE_UINT32,serial_parity,MISC)
  ,_XRT_ENTRY(0,serial_rate,		BD,F8|EZ|ZB|X9|DP|X8|X2,Serial interface data rate,,			,115200,XBEE_RCI_TYPE_UINT32,serial_rate,MISC)
  ,_XRT_ENTRY(0,sleep_count,		SN,EZ|ZB|GW,Peripheral sleep count,									,1,65535,XBEE_RCI_TYPE_UINT32,sleep_count,MISC)
  ,_XRT_ENTRY(0,sleep_disassoc,	DP,F8,Disassociated cyclic sleep period,x 10 ms,				,26800,XBEE_RCI_TYPE_UINT32,sleep_disassoc,MISC)
  ,_XRT_ENTRY(0,sleep_mode,		SM,F8|EZ|ZB|X9|X2|GW,Sleep mode,,									,8,XBEE_RCI_TYPE_UINT32,sleep_mode,MISC)
  ,_XRT_ENTRY(0,sleep_options,	SO,F8|EZ|ZB|X9|X2|GW,Sleep options,bitfield,						,0xff,XBEE_RCI_TYPE_HEX32,sleep_options,MISC)
  ,_XRT_ENTRY(0,sleep_period,		SP,EZ|ZB|SE|F8|GW,Cyclic sleep period,x 10 ms,					,26800,XBEE_RCI_TYPE_UINT32,sleep_period,MISC)
  ,_XRT_ENTRY(0,sleep_sample_rate,IF,X9|X2,I/O sample from sleep rate,,								1,255,XBEE_RCI_TYPE_UINT32,sleep_sample_rate,MISC)
  ,_XRT_ENTRY(0,sleep_time,		ST,F8|EZ|ZB,Time before sleep,ms,									1,65535,XBEE_RCI_TYPE_UINT32,sleep_time,MISC)
  ,_XRT_ENTRY(0,sleep_time,		ST,X9|X2|GW,Wake time,ms,												1,3600000,XBEE_RCI_TYPE_UINT32,sleep_time,MISC)
  ,_XRT_ENTRY(0,sleep_wake,		SW,X2,Sleep early wakeup,x 0.1%,										,1000,XBEE_RCI_TYPE_UINT32,sleep_wake,MISC)
  ,_XRT_ENTRY(0,source_endpoint,	SE,EZ|ZB,Source endpoint,,												0x1,0xff,XBEE_RCI_TYPE_HEX32,source_endpoint,MISC)
  ,_XRT_ENTRY(0,stack_profile,	ZS,ZB|GW,ZigBee stack profile,,										,2,XBEE_RCI_TYPE_UINT32,stack_profile,MISC)
  ,_XRT_ENTRY(0,stop_bits,			SB,ZB|X9|DP|X8,Stop bits,,												,1,XBEE_RCI_TYPE_UINT32,stop_bits,MISC)
  ,_XRT_ENTRY(0,supply_threshold,V+,EZ|ZB|SE,Supply voltage high threshold,mV,					,1200,XBEE_RCI_TYPE_UINT32,supply_threshold,MISC)
  ,_XRT_ENTRY(0,tx_samples,		IT,F8,I/O samples before transmit,,									1,255,XBEE_RCI_TYPE_UINT32,io.tx_samples,IO_CONFIG)
  ,_XRT_ENTRY(0,xbee_retries,		RR,F8|GW,XBee retries,,													,6,XBEE_RCI_TYPE_UINT32,xbee_retries,MISC)
  ,_XRT_ENTRY(0,wake_host_delay,	WH,ZB|X9|X2,Wake host delay,ms,										,65535,XBEE_RCI_TYPE_UINT32,wake_host_delay,MISC)
  ,_XRT_ENTRY(0,zigbee_enable,	ZA,EZ,ZigBee addressing enable,,										,1,XBEE_RCI_TYPE_UINT32,zigbee_enable,MISC)

	// state
  ,_XRT_ENTRY(1,pan_id,				ID,F8|FZ|X9|DP|X8|X2|GW,PAN identifier,,							,,XBEE_RCI_TYPE_HEX32,pan_id,MISC)
  ,_XRT_ENTRY(1,pan_id,				OP,EZ|GW,PAN identifier,,												,,XBEE_RCI_TYPE_HEX32,pan_id,MISC)
  ,_XRT_ENTRY(1,pan_id,				OI,ZB|SE|GW,PAN identifier,,											,,XBEE_RCI_TYPE_HEX32,pan_id,MISC)
  ,_XRT_ENTRY(1,ext_pan_id,		OP,ZB|SE|GW,Extended PAN identifier,,								,18,XBEE_RCI_TYPE_BIN,ext_pan_id,MISC)
  ,_XRT_ENTRY(1,channel,			CH,F8|FZ|EZ|ZB|SE|X2|GW,Operating channel,,						,,XBEE_RCI_TYPE_HEX32,channel,MISC)
  ,_XRT_ENTRY(1,net_addr,			MY,F8|FZ|EZ|ZB|SE|GW,Network address,,								,,XBEE_RCI_TYPE_HEX32,id.network_addr,NONE)
  ,_XRT_ENTRY(1,parent_addr,		MP,FZ|ZB|SE|GW,Parent address,,										,,XBEE_RCI_TYPE_HEX32,id.parent_addr,NONE)
  ,_XRT_ENTRY(1,association,		AI,F8|FZ|EZ|ZB|SE|GW,Association indication,,					,,XBEE_RCI_TYPE_HEX32,association,MISC)
  ,_XRT_ENTRY(1,firmware_version,VR,ALL|GW,Firmware version,,											,,XBEE_RCI_TYPE_HEX32,firmware_version,DEVICE_INFO)
  ,_XRT_ENTRY(1,hardware_version,HV,ALL|GW,Hardware version,,											,,XBEE_RCI_TYPE_HEX32,hardware_version,DEVICE_INFO)
  ,_XRT_ENTRY(1,device_type,		DD,F8|EZ|ZB|X9|DP|X8|X2|GW,Device type identifier,,			,,XBEE_RCI_TYPE_HEX32,dd,DEVICE_INFO)
  ,_XRT_ENTRY(1,children,			NC,EZ|ZB|SE|GW,Number of remaining children,,					,,XBEE_RCI_TYPE_UINT32,children,MISC)
  ,_XRT_ENTRY(1,max_payload,		NP,ZB|SE|X9|DP|X8|X2|GW,Maximum RF payload,,						,,XBEE_RCI_TYPE_UINT32,max_payload,MISC)
  ,_XRT_ENTRY(1,config_code,		CK,X9|X2|GW,Configuration code,,										,,XBEE_RCI_TYPE_UINT32,config_code,MISC)
  ,_XRT_ENTRY(1,verify_cert,		VC,SE|GW,Verify certificate,,											,,XBEE_RCI_TYPE_UINT32,verify_cert,MISC)
  ,_XRT_ENTRY(1,stack_profile,	ZS,SE|GW,ZigBee stack profile,,										,,XBEE_RCI_TYPE_UINT32,stack_profile,MISC)
  ,_XRT_ENTRY(1,supply_voltage,	%V,EZ|ZB|SE|X9|DP|X8|GW,Supply voltage,mV,						,,XBEE_RCI_TYPE_UINT32,supply_voltage,MISC)
  ,_XRT_ENTRY(1,temperature,		TP,ZB|SE|X9|DP|X8|GW,Temperature,deg C,							,,XBEE_RCI_TYPE_UINT32,temperature,MISC)
  ,_XRT_ENTRY(1,duty_cycle,		DC,X8|GW,Duty cycle available,%,										,,XBEE_RCI_TYPE_UINT32,duty_cycle,MISC)
  ,_XRT_ENTRY(1,rssi,				DB,F8|EZ|ZB|X9|DP|X8|X2|GW,Received signal strength,-dBm,	,,XBEE_RCI_TYPE_UINT32,rssi,MISC)
  ,_XRT_ENTRY(1,tx_power,			PP,ZB|SE|GW,Transmit power at PL4,dBm,								,,XBEE_RCI_TYPE_UINT32,tx_power,MISC)
  ,_XRT_ENTRY(1,sleep_status,		SS,X9|X2|GW,Sleep status,,												,,XBEE_RCI_TYPE_HEX32,sleep_status,MISC)
  ,_XRT_ENTRY(1,sleep_time,		OS,X9|X2|GW,Operating sleep time,x 10 ms,							,,XBEE_RCI_TYPE_UINT32,sleep_time_op,MISC)
  ,_XRT_ENTRY(1,wake_time,			OW,X9|X2|GW,Operating wake time,ms,									,,XBEE_RCI_TYPE_UINT32,wake_time,MISC)
  ,_XRT_ENTRY(1,ack_failures,		EA,F8|ZB|GW,ACK failures,,												,,XBEE_RCI_TYPE_UINT32,ack_failures,MISC)
  ,_XRT_ENTRY(1,cca_failures,		EC,F8|GW,CCA failures,,													,,XBEE_RCI_TYPE_UINT32,cca_failures,MISC)
  ,_XRT_ENTRY(1,good_packets,		GD,X9|DP|X8|X2|GW,Good packets,,										,,XBEE_RCI_TYPE_UINT32,good_packets,MISC)
  ,_XRT_ENTRY(1,missed_syncs,		SQ,X9|X2|GW,Missed sync count,,										,,XBEE_RCI_TYPE_UINT32,missed_syncs,MISC)
  ,_XRT_ENTRY(1,missed_sync_msg,	MS,X9|X2|GW,Missed sync messages,,									,,XBEE_RCI_TYPE_UINT32,missed_sync_msg,MISC)
  ,_XRT_ENTRY(1,rf_errors,			ER,X9|DP|X8|X2|GW,RF errors,,											,,XBEE_RCI_TYPE_UINT32,rf_errors,MISC)
  ,_XRT_ENTRY(1,tx_errors,			TR,X9|DP|X8|X2|GW,Transmission errors,,							,,XBEE_RCI_TYPE_UINT32,tx_errors,MISC)

  // I/O sampling... (ATIS used to sample)
  // 'state' field value of 2 used to indicate non-standard for RCI
  ,_XRT_ENTRY(2,ain0,				IS,0,Analog 0 sample,,													,,XBEE_RCI_TYPE_UINT32,io.adc_sample[0],IO_CONFIG)
  ,_XRT_ENTRY(2,ain1,				IS,0,Analog 1 sample,,													,,XBEE_RCI_TYPE_UINT32,io.adc_sample[1],IO_CONFIG)
  ,_XRT_ENTRY(2,ain2,				IS,0,Analog 2 sample,,													,,XBEE_RCI_TYPE_UINT32,io.adc_sample[2],IO_CONFIG)
  ,_XRT_ENTRY(2,ain3,				IS,0,Analog 3 sample,,													,,XBEE_RCI_TYPE_UINT32,io.adc_sample[3],IO_CONFIG)
  ,_XRT_ENTRY(2,ain7,				IS,0,Analog 7 sample,,													,,XBEE_RCI_TYPE_UINT32,io.adc_sample[7],IO_CONFIG)
  ,_XRT_ENTRY(2,din_enabled,		IS,0,Digital inputs enabled,bitmask,								,,XBEE_RCI_TYPE_HEX32,io.din_enabled,IO_CONFIG)
  ,_XRT_ENTRY(2,dout_enabled,		IS,0,Digital outputs enabled,bitmask,								,,XBEE_RCI_TYPE_HEX32,io.dout_enabled,IO_CONFIG)
  ,_XRT_ENTRY(2,din_state,			IS,0,Digital inputs state,bitmask,									,,XBEE_RCI_TYPE_HEX32,io.din_state,IO_CONFIG)
  ,_XRT_ENTRY(2,dout_state,		IS,0,Digital outputs state,bitmask,									,,XBEE_RCI_TYPE_HEX32,io.dout_state,IO_CONFIG)
  ,_XRT_ENTRY(2,pullup_state,		IS,0,Pullups enabled,bitmask,											,,XBEE_RCI_TYPE_HEX32,io.pullup_state,IO_CONFIG)
  ,_XRT_ENTRY(2,pulldown_state,	IS,0,Pulldowns enabled,bitmask,										,,XBEE_RCI_TYPE_HEX32,io.pulldown_state,IO_CONFIG)
  ,_XRT_ENTRY(2,analog_enabled,	IS,0,Analog inputs enabled,bitmask,									,,XBEE_RCI_TYPE_HEX32,io.analog_enabled,IO_CONFIG)
  , {0,NULL}
};


#undef F8
#undef EZ
#undef ZB
#undef X9
#undef X2
#undef X8
#undef DP

// Flag bits not based on module number
#undef FZ
#undef SE
#undef ALL
#undef GW


