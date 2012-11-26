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
	@addtogroup zcl_commissioning
	@{
	@file zigbee/zcl_commissioning.h

	Header for ZCL Commissioning Cluster.
*/

#ifndef ZCL_COMMISSIONING_H
#define ZCL_COMMISSIONING_H

#include "xbee/platform.h"
#include "wpan/types.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_types.h"

XBEE_BEGIN_DECLS

#define ZCL_CLUST_COMMISSIONING					0x0015

#define ZCL_CLUST_ENTRY_COMMISSIONING_SERVER	\
	{	ZCL_CLUST_COMMISSIONING, &zcl_comm_clust_handler,	\
		zcl_comm_startup_attribute_tree, WPAN_CLUST_FLAG_SERVER }

// Global commissioning EPID for use in defining zcl_comm_default_sas
#define ZCL_COMM_GLOBAL_EPID		ZCL64_INIT( 0x0050C277, 0x10000000 )

extern const zcl64_t zcl_comm_global_epid;

typedef struct zcl_comm_startup_param_t {
	// Must start with short_address so cluster handler can use the attribute
	// tree to find this structure.
	uint16_t		short_address;
	zcl64_t		extended_panid;
	uint16_t		panid;
	uint32_t		channel_mask;
	uint8_t		startup_control;
		#define ZCL_COMM_STARTUP_JOINED			0x00
		#define ZCL_COMM_STARTUP_COORDINATOR	0x01
		#define ZCL_COMM_STARTUP_REJOIN			0x02
		#define ZCL_COMM_STARTUP_FROM_SCRATCH	0x03

/*
	addr64		trust_ctr_addr;
	uint8_t		trust_ctr_master_key_le[16];
*/
	uint8_t		network_key_le[16];
	bool_t		use_insecure_join;
	uint8_t		preconfig_link_key_le[16];
	uint8_t		network_key_seq_num;
	uint8_t		network_key_type;
		#define ZCL_COMM_KEY_TYPE_STANDARD			0x01
		#define ZCL_COMM_KEY_TYPE_HIGH_SECURITY	0x05
	uint16_t		network_mgr_addr;

	uint8_t		scan_attempts;
		#define ZCL_COMM_SCAN_ATTEMPTS_DEFAULT			0x05
	uint16_t		time_between_scans;
		#define ZCL_COMM_TIME_BETWEEN_SCANS_DEFAULT	0x0064
	uint16_t		rejoin_interval;
		#define ZCL_COMM_REJOIN_INTERVAL_DEFAULT		0x003c
	uint16_t		max_rejoin_interval;
		#define ZCL_COMM_MAX_REJOIN_INTERVAL_DEFAULT	0x0e10

	struct {
		bool_t		flag;
		uint8_t		radius;
			#define ZCL_COMM_CONCENTRATOR_RADIUS_DEFAULT		0x0F
		uint8_t		discovery_time;
	} concentrator;
} zcl_comm_startup_param_t;

typedef struct zcl_comm_state_t
{
	zcl_comm_startup_param_t	sas;
	uint8_t		flags;
		/// if radio successfully joins a network, commit changes to NVRAM
		#define ZCL_COMM_FLAG_SAVE_ON_JOIN		0x01
		/// there is a delayed action to perform
		#define ZCL_COMM_FLAG_DELAYED_RESTART	0x02
		/// this device can be a coordinator
		#define ZCL_COMM_FLAG_COORDINATOR_OK	0x04
		/// this device can be a router
		#define ZCL_COMM_FLAG_ROUTER_OK			0x08
		/// this device can be a end device
		#define ZCL_COMM_FLAG_ENDDEV_OK			0x10
		/// reset to factory SAS at the delayed restart
		#define ZCL_COMM_FLAG_FACTORY_RESET		0x40
		/// install the current SAS at the delayed restart
		#define ZCL_COMM_FLAG_INSTALL				0x80
	/// value of xbee_millisecond_timer() when it's time to restart
	uint32_t		restart_ms;

	uint8_t		state;
		/// startup state
		#define ZCL_COMM_STATE_INIT			0x00
		/// loading structure
		#define ZCL_COMM_STATE_LOADING		0x01
		/// xbee_commissioning_tick() can process startup_control
		#define ZCL_COMM_STATE_STARTUP		0x02
		/// radio is not joined to a network
		#define ZCL_COMM_STATE_UNJOINED		0x03
		/// radio has joined a network and we're refreshing the structure
		#define ZCL_COMM_STATE_REFRESH		0x04
		/// radio is joined to a network
		#define ZCL_COMM_STATE_JOINED			0x05
} zcl_comm_state_t;

// attribute list of startup parameters
typedef struct zcl_comm_startup_attr_t {
	zcl_attribute_base_t		short_address;
	zcl_attribute_base_t		extended_panid;
	zcl_attribute_base_t		panid;
	zcl_attribute_base_t		channel_mask;
	zcl_attribute_full_t		startup_control;		// limited range of values
	zcl_attribute_base_t		trust_ctr_addr;
	zcl_attribute_base_t		trust_ctr_master_key;
	zcl_attribute_base_t		network_key;
	zcl_attribute_base_t		use_insecure_join;
	zcl_attribute_base_t		preconfig_link_key;
	zcl_attribute_base_t		network_key_seq_num;
	zcl_attribute_base_t		network_key_type;
	zcl_attribute_base_t		network_mgr_addr;
	zcl_attribute_full_t		scan_attempts;
	zcl_attribute_full_t		time_between_scans;
	zcl_attribute_full_t		rejoin_interval;
	zcl_attribute_full_t		max_rejoin_interval;
	struct {
		zcl_attribute_base_t		flag;
		zcl_attribute_base_t		radius;
		zcl_attribute_base_t		discovery_time;
	} concentrator;
	uint16_t		end_of_list;
} zcl_comm_startup_attr_t;

// current startup attributes
extern zcl_comm_state_t zcl_comm_state;

// defaults, provided by the application
extern const zcl_comm_startup_param_t zcl_comm_default_sas;

// structures for the actual cluster
extern const zcl_comm_startup_attr_t zcl_comm_startup_attributes;
extern const zcl_attribute_tree_t zcl_comm_startup_attribute_tree[];

// Startup Parameters
#define ZCL_COMM_ATTR_SHORT_ADDRESS				0x0000	// UINT16
#define ZCL_COMM_ATTR_EXTENDED_PANID			0x0001	// IEEE Addr
#define ZCL_COMM_ATTR_PANID						0x0002	// UINT16
#define ZCL_COMM_ATTR_CHANNEL_MASK				0x0003	// BITMAP32
#define ZCL_COMM_ATTR_PROTOCOL_VERSION			0x0004	// UINT8
#define ZCL_COMM_ATTR_STACK_PROFILE				0x0005	// UINT8
#define ZCL_COMM_ATTR_STARTUP_CONTROL			0x0006	// ENUM8

// Startup Parameters (Security)
#define ZCL_COMM_ATTR_TRUST_CTR_ADDR			0x0010	// IEEE Addr
#define ZCL_COMM_ATTR_TRUST_CTR_MASTER_KEY	0x0011	// Security Key
#define ZCL_COMM_ATTR_NETWORK_KEY				0x0012	// Security Key
#define ZCL_COMM_ATTR_USE_INSECURE_JOIN		0x0013	// Boolean
#define ZCL_COMM_ATTR_PRECONFIG_LINK_KEY		0x0014	// Security Key
#define ZCL_COMM_ATTR_NETWORK_KEY_SEQ_NUM		0x0015	// UINT8
#define ZCL_COMM_ATTR_NETWORK_KEY_TYPE			0x0016	// ENUM8
#define ZCL_COMM_ATTR_NETWORK_MGR_ADDR			0x0017	// UINT16

// Join Parameters
#define ZCL_COMM_ATTR_SCAN_ATTEMPTS				0x0020	// UINT8
#define ZCL_COMM_ATTR_TIME_BTWN_SCANS			0x0021	// UINT16
#define ZCL_COMM_ATTR_REJOIN_INTERVAL			0x0022	// UINT16
#define ZCL_COMM_ATTR_MAX_REJOIN_INTERVAL		0x0023	// UINT16

// End Device Parameters
#define ZCL_COMM_ATTR_INDIRECT_POLL_RATE		0x0030	// UINT16
#define ZCL_COMM_ATTR_PARENT_RETRY_THRESHOLD	0x0031	// UINT8

// Concentrator Parameters
#define ZCL_COMM_ATTR_CONCENTRATOR_FLAG		0x0040	// Boolean
#define ZCL_COMM_ATTR_CONCENTRATOR_RADIUS		0x0041	// UINT8
#define ZCL_COMM_ATTR_CONCENTRATOR_DISC_TIME	0x0042	// UINT8

// Client-generated commands
#define ZCL_COMM_CMD_RESTART_DEVICE					0x00
typedef PACKED_STRUCT zcl_comm_restart_device_cmd_t
{
	uint8_t		options;
		#define ZCL_COMM_RESTART_OPT_MODE_MASK				0x07
		#define ZCL_COMM_RESTART_OPT_MODE_SAVE_CHANGES	0x00
		#define ZCL_COMM_RESTART_OPT_MODE_NO_CHANGES		0x01
		#define ZCL_COMM_RESTART_OPT_IMMEDIATE				0x08
		#define ZCL_COMM_RESTART_OPT_RESERVED_MASK		0xF0
	uint8_t		delay;		// delay in seconds before restarting
	uint8_t		jitter;		// add RAND(jitter * 80) ms to delay
} zcl_comm_restart_device_cmd_t;

#define ZCL_COMM_CMD_SAVE_STARTUP_PARAM			0x01
typedef PACKED_STRUCT zcl_comm_save_startup_param_t
{
	uint8_t		options;
	uint8_t		index;
} zcl_comm_save_startup_param_t;

#define ZCL_COMM_CMD_RESTORE_STARTUP_PARAM		0x02
typedef PACKED_STRUCT zcl_comm_restore_startup_param_t
{
	uint8_t		options;
	uint8_t		index;
} zcl_comm_restore_startup_param_t;

#define ZCL_COMM_CMD_RESET_STARTUP_PARAM			0x03
typedef PACKED_STRUCT zcl_comm_reset_startup_param_t
{
	uint8_t		options;
		#define ZCL_COMM_RESET_OPT_CURRENT			0x01
		#define ZCL_COMM_RESET_OPT_ALL				0x02
		#define ZCL_COMM_RESET_OPT_ERASE_INDEX		0x04
	uint8_t		index;
} zcl_comm_reset_startup_param_t;


// Server-generated commands
#define ZCL_COMM_CMD_RESTART_DEVICE_RESP			0x00
#define ZCL_COMM_CMD_SAVE_STARTUP_PARAM_RESP		0x01
#define ZCL_COMM_CMD_RESTORE_STARTUP_PARAM_RESP	0x02
#define ZCL_COMM_CMD_RESET_STARTUP_PARAM_RESP	0x03


int zcl_comm_response( const wpan_envelope_t FAR *envelope,
																		uint_fast8_t status);

// Send commands from client to server
int zcl_comm_restart_device( wpan_envelope_t FAR *envelope,
	const zcl_comm_restart_device_cmd_t *parameters);
int zcl_comm_reset_parameters( wpan_envelope_t FAR *envelope,
	const zcl_comm_reset_startup_param_t *parameters);

// Commissioning Server support functions
void zcl_comm_sas_init( zcl_comm_startup_param_t *parameters);
int zcl_comm_clust_handler( const wpan_envelope_t FAR *envelope,
	void FAR *context);

void zcl_comm_factory_reset( uint16_t ms_delay);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "zcl_commissioning.c"
#endif

#endif	// ZCL_COMMISSIONING_H defined
