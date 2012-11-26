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
	@addtogroup zcl
	@{
	@file zigbee/zcl.h
	Header for implementation of ZigBee Cluster Library.

	Type definitions and frame formats.

	Based on document 075123r01ZB, updated for 075123r02ZB

	Even though the XBee modules uses big-endian byte order for its API,
	the payload data should be in the correct format for Rabbit (little-endian).
	Multi-byte variables and structure members use an "_le" suffix if the value
	is little-endian, a "_be" suffix if the value is big-endian, and no suffix
	if the value is in host byte order.

	See xbee/byteorder.h for functions to convert between big/little-endian and
	host byte order.

*/

#ifndef __ZIGBEE_ZCL_H
#define __ZIGBEE_ZCL_H

#include "xbee/platform.h"				// platform-specific data types
#include "wpan/aps.h"
#include "zigbee/zcl_types.h"

XBEE_BEGIN_DECLS

/** @name
	0x0000-0x00ff General Clusters
	@{
*/
#define ZCL_CLUST_BASIC								0x0000
#define ZCL_CLUST_POWER_CONFIG					0x0001
#define ZCL_CLUST_DEVICE_TEMP_CONFIG			0x0002
#define ZCL_CLUST_IDENTIFY							0x0003
#define ZCL_CLUST_GROUPS							0x0004
#define ZCL_CLUST_SCENES							0x0005
#define ZCL_CLUST_ONOFF								0x0006
#define ZCL_CLUST_ONOFF_SWITCH_CONFIG			0x0007
#define ZCL_CLUST_LEVEL_CONTROL					0x0008
#define ZCL_CLUST_ALARMS							0x0009
#define ZCL_CLUST_TIME								0x000a
#define ZCL_CLUST_RSSI_LOCATION					0x000b
#define ZCL_CLUST_ANALOG_IN						0x000c
#define ZCL_CLUST_ANALOG_OUT						0x000d
#define ZCL_CLUST_ANALOG_VALUE					0x000e
#define ZCL_CLUST_BINARY_IN						0x000f
#define ZCL_CLUST_BINARY_OUT						0x0010
#define ZCL_CLUST_BINARY_VALUE					0x0011
#define ZCL_CLUST_MULTI_IN							0x0012
#define ZCL_CLUST_MULTI_OUT						0x0013
#define ZCL_CLUST_MULTI_VALUE						0x0014
#define ZCL_CLUST_COMMISSIONING					0x0015
//@}

/** @name
	0x0100-0x01ff Closure Clusters
	@{
*/
#define ZCL_CLUST_SHADE_CONFIG					0x0100
//@}

/** @name
	0x0200-0x02ff HVAC Clusters
	@{
*/
#define ZCL_CLUST_PUMP_CONFIG						0x0200
#define ZCL_CLUST_THERMOSTAT						0x0201
#define ZCL_CLUST_FAN_CONTROL						0x0202
#define ZCL_CLUST_DEHUMIDIFY_CONTROL			0x0203
#define ZCL_CLUST_THERMOSTAT_UI_CONFIG			0x0204
//@}

/** @name
	0x0300-0x03ff Lighting Clusters
	@{
*/
#define ZCL_CLUST_COLOR_CONTROL					0x0300
#define ZCL_CLUST_BALLAST_CONFIG					0x0301
//@}

/** @name
	0x0400-0x04ff Measurement and Sensing Clusters
	@{
*/
#define ZCL_CLUST_ILLUM_MEASURE					0x0400
#define ZCL_CLUST_ILLUM_LEVEL_SENSING			0x0401
#define ZCL_CLUST_TEMP_MEASURE					0x0402
#define ZCL_CLUST_PRESSURE_MEASURE				0x0403
#define ZCL_CLUST_FLOW_MEASURE					0x0404
#define ZCL_CLUST_HUMIDITY_MEASURE				0x0405
#define ZCL_CLUST_OCCUPANCY_SENSING				0x0406
//@}

/** @name
	0x0500-0x05ff Security and Safety Clusters
	@{
*/
#define ZCL_CLUST_IAS_ZONE							0x0500
#define ZCL_CLUST_IAS_ACE							0x0501
#define ZCL_CLUST_IAS_WD							0x0502
//@}

/** @name
	0x0600-0x06ff Protocol Interface Clusters
	@{
*/
#define ZCL_CLUST_GENERIC_TUNNEL					0x0600
#define ZCL_CLUST_BACNET_TUNNEL					0x0601
#define ZCL_CLUST_BACNET_REG_ANALOG_IN			0x0602
#define ZCL_CLUST_BACNET_EXT_ANALOG_IN			0x0603
#define ZCL_CLUST_BACNET_REG_ANALOG_OUT		0x0604
#define ZCL_CLUST_BACNET_EXT_ANALOG_OUT		0x0605
#define ZCL_CLUST_BACNET_REG_ANALOG_VALUE		0x0606
#define ZCL_CLUST_BACNET_EXT_ANALOG_VALUE		0x0607
#define ZCL_CLUST_BACNET_REG_BINARY_IN			0x0608
#define ZCL_CLUST_BACNET_EXT_BINARY_IN			0x0609
#define ZCL_CLUST_BACNET_REG_BINARY_OUT		0x060a
#define ZCL_CLUST_BACNET_EXT_BINARY_OUT		0x060b
#define ZCL_CLUST_BACNET_REG_BINARY_VALUE		0x060c
#define ZCL_CLUST_BACNET_EXT_BINARY_VALUE		0x060d
#define ZCL_CLUST_BACNET_REG_MULTI_IN			0x060e
#define ZCL_CLUST_BACNET_EXT_MULTI_IN			0x060f
#define ZCL_CLUST_BACNET_REG_MULTI_OUT			0x0610
#define ZCL_CLUST_BACNET_EXT_MULTI_OUT			0x0611
#define ZCL_CLUST_BACNET_REG_MULTI_VALUE		0x0612
#define ZCL_CLUST_BACNET_EXT_MULTI_VALUE		0x0613
//@}


//////////
//
// General ZCL Frame Format
// 3 or 5-byte ZCL header followed by variable-length payload
//

/// Common portion of ZCL header (appears after optional Manufacturer Code)
typedef PACKED_STRUCT zcl_header_common_t {
	uint8_t	sequence;
	uint8_t	command;
	uint8_t	payload[1];		///< first byte of variable-length field
} zcl_header_common_t;

/// General header for casting onto an incoming frame.  Reference .type.mfg
/// if (.frame_control & ZCL_FRAME_MFG_SPECIFIC != 0).  Reference .type.std
/// otherwise.
typedef PACKED_STRUCT zcl_header_t {
	uint8_t	frame_control;
	union {
		PACKED_STRUCT {
	      uint16_t					mfg_code_le;
	      zcl_header_common_t	common;
	   } mfg;					///< manufacturer-specific extension
		PACKED_STRUCT {
	      zcl_header_common_t	common;
	   } std;					///< standard
	} type;
} zcl_header_t;

/// ZCL header structure used for building response frames that may or may not
/// be manufacturer-specific.  Frame is sent starting with element
/// .u.mfg.frame_control or .u.std.frame_control.
typedef PACKED_STRUCT zcl_header_response_t {
	union {
		PACKED_STRUCT {
			uint8_t		frame_control;
			uint16_t		mfg_code_le;
		} mfg;
		PACKED_STRUCT {
			uint16_t		dummy;
			uint8_t		frame_control;
		} std;
	} u;
	uint8_t						sequence;
	uint8_t						command;
} zcl_header_response_t;

/// ZLC header structure used when building a manufacturer-specific response
/// frame.
typedef PACKED_STRUCT zcl_header_withmfg_t {
	uint8_t	frame_control;
	uint16_t	mfg_code_le;
	uint8_t	sequence;
	uint8_t	command;
} zcl_header_withmfg_t;

/// ZLC header structure used when building a normal (non-manufacturer-specific
/// response frame.
typedef PACKED_STRUCT zcl_header_nomfg_t {
	uint8_t	frame_control;
	uint8_t	sequence;
	uint8_t	command;
} zcl_header_nomfg_t;

/** @name
	Bit masks for the frame_control field of a ZCL header (zcl_header_t,
	zcl_header_response_t, zcl_header_withmfg_t, or zcl_header_nomfg_t)
	@{
*/
#define ZCL_FRAME_TYPE_MASK			0x03
												///< Bits of frame type.
#define ZCL_FRAME_TYPE_PROFILE		0x00
												///< cmd acts across entire profile
#define ZCL_FRAME_TYPE_CLUSTER		0x01
												///< cmd is specific to a cluster
#define ZCL_FRAME_TYPE_RESERVED1		0x02
#define ZCL_FRAME_TYPE_RESERVED2		0x03

#define ZCL_FRAME_GENERAL				0x00
												///< mfg code field not present
#define ZCL_FRAME_MFG_SPECIFIC		0x04
												///< mfg code field present

#define ZCL_FRAME_DIRECTION			0x08
												///< direction bit
#define ZCL_FRAME_CLIENT_TO_SERVER	0
												///< cmd sent from client to server
#define ZCL_FRAME_SERVER_TO_CLIENT	ZCL_FRAME_DIRECTION
												///< cmd sent from server to client

#define ZCL_FRAME_DISABLE_DEF_RESP	0x10
												///< default response only on error
#define ZCL_FRAME_RESERVED_BITS		0xE0
												///< should be set to 0
//@}

/**
	@brief
	Macro for checking the frame control field of a ZCL command.

	Used to filter out commands that are (or are not) handled by the
	current function.

	@param[in]	p		pointer to first byte of payload (typically
							\c envelope->payload when called from a standard
							ZCL command handler, or the address of \c frame_control
							from a zcl_command_t built with zcl_command_build)
	@param[in]	mfg	either MFG_SPECIFIC or GENERAL
	@param[in]	dir	either CLIENT_TO_SERVER or SERVER_TO_CLIENT
	@param[in]	type	either PROFILE (e.g., general commands) or CLUSTER
							(cluster-specific command)

	@retval	TRUE		ZCL command matches all three filters
	@retval	FALSE		ZCL command does not match
*/
#define ZCL_CMD_MATCH(p, mfg, dir, type)	\
	((*(const uint8_t FAR *)(p) & 										\
	(ZCL_FRAME_MFG_SPECIFIC | ZCL_FRAME_DIRECTION | ZCL_FRAME_TYPE_MASK)) ==	\
	(ZCL_FRAME_ ## mfg | ZCL_FRAME_ ## dir | ZCL_FRAME_TYPE_ ## type))

/**
	@brief
	Identify profile commands (as opposed to cluster commands) using the frame
	control field of a ZCL command.

	Ignores the manufacturer-specific bit (which specifies use of
	manufacturer-specific attributes).

	@param[in]	p	pointer to first byte of payload

	@retval	TRUE		ZCL command is not cluster-specific (profile command)
	@retval	FALSE		ZCL command is cluster-specific
*/
#define ZCL_CMD_IS_PROFILE(p)	\
	(ZCL_FRAME_TYPE_PROFILE == ((const uint8_t FAR *)(p) & ZCL_FRAME_TYPE_MASK))

/**
	@brief
	Identify manufacturer-specific cluster commands using the frame control
	field of a ZCL command.

	@param[in]	p	pointer to first byte of payload

	@retval	TRUE		ZCL command is manufacturer and cluster-specific
	@retval	FALSE		ZCL command doesn't have the manufacturer-specific bit
							set, or is a profile-wide command
*/
#define ZCL_CMD_IS_MFG_CLUSTER(p)	\
	((ZCL_FRAME_TYPE_CLUSTER | ZCL_FRAME_MFG_SPECIFIC) ==		\
	((const uint8_t FAR *)(p) & (ZCL_FRAME_TYPE_MASK | ZCL_FRAME_MFG_SPECIFIC)))

/**
	@brief
	Identify cluster commands that are not manufacturer-specific, using the
	frame control field of a ZCL command.

	@param[in]	p	pointer to first byte of payload

	@retval	TRUE		ZCL command is manufacturer and cluster-specific
	@retval	FALSE		ZCL command doesn't have the manufacturer-specific bit
							set, or is a profile-wide command
*/
#define ZCL_CMD_IS_CLUSTER(p)		(ZCL_FRAME_TYPE_CLUSTER ==		\
	((const uint8_t FAR *)(p) & (ZCL_FRAME_TYPE_MASK | ZCL_FRAME_MFG_SPECIFIC)))

/** @name ZCL Status Enumerations
	Note that these defines use the exact names from the ZCL standard, with
	ZCL_STATUS_ prepended.
	@{
*/
#define ZCL_STATUS_SUCCESS									0x00
#define ZCL_STATUS_FAILURE									0x01
#define ZCL_STATUS_NOT_AUTHORIZED						0x7e
#define ZCL_STATUS_RESERVED_FIELD_NOT_ZERO			0x7f
#define ZCL_STATUS_MALFORMED_COMMAND					0x80
#define ZCL_STATUS_UNSUP_CLUSTER_COMMAND				0x81
#define ZCL_STATUS_UNSUP_GENERAL_COMMAND				0x82
#define ZCL_STATUS_UNSUP_MANUF_CLUSTER_COMMAND		0x83
#define ZCL_STATUS_UNSUP_MANUF_GENERAL_COMMAND		0x84
#define ZCL_STATUS_INVALID_FIELD							0x85
#define ZCL_STATUS_UNSUPPORTED_ATTRIBUTE				0x86
#define ZCL_STATUS_INVALID_VALUE							0x87
#define ZCL_STATUS_READ_ONLY								0x88
#define ZCL_STATUS_INSUFFICIENT_SPACE					0x89
#define ZCL_STATUS_DUPLICATE_EXISTS						0x8a
#define ZCL_STATUS_NOT_FOUND								0x8b
#define ZCL_STATUS_UNREPORTABLE_ATTRIBUTE				0x8c
#define ZCL_STATUS_INVALID_DATA_TYPE					0x8d
#define ZCL_STATUS_INVALID_SELECTOR						0x8e
#define ZCL_STATUS_WRITE_ONLY								0x8f
#define ZCL_STATUS_INCONSISTENT_STARTUP_STATE		0x90
#define ZCL_STATUS_DEFINED_OUT_OF_BAND					0x91
#define ZCL_STATUS_HARDWARE_FAILURE						0xc0
#define ZCL_STATUS_SOFTWARE_FAILURE						0xc1
#define ZCL_STATUS_CALIBRATION_ERROR					0xc2
//@}

const char *zcl_status_text( uint_fast8_t status);

/** @name ZCL General Command IDs
	Used for the .command element of the ZCL header.  Command IDs
	0x11 to 0x7F (Standard ZigBee command) and 0x80 to 0xFF are reserved.

	@note
	Only supported commands are currently documented.
*/
/// Read Attributes Command, payload is 1 to n 2-byte attribute IDs.
#define ZCL_CMD_READ_ATTRIB					0x00

/// Read Attributes Response Command, payload is 1 to n variable-length
/// Read Attribute Status records (zcl_rec_read_attrib_status_t).
#define ZCL_CMD_READ_ATTRIB_RESP				0x01

/// Write Attributes Command, payload is 1 to n variable-length
/// Write Attribute Record fields (same as zcl_attrib_t).
#define ZCL_CMD_WRITE_ATTRIB					0x02

/// Write Attributes Undivided Command, same payload as Write Attributes
/// Command (zcl_attrib_t), but attributes are only written if all
/// attributes can be written successfully.
#define ZCL_CMD_WRITE_ATTRIB_UNDIV			0x03

/// Write Attributes Response Command,
/// payload is a single byte (ZCL_STATUS_SUCCESS) if all writes were
/// successful or 1 to n 3-byte Write Attribute Status records
/// (zcl_rec_write_attrib_status_t) for each write that failed
#define ZCL_CMD_WRITE_ATTRIB_RESP			0x04

/// Write Attributes No Response Command,
/// same payload as Write Attributes Command (#ZCL_CMD_WRITE_ATTRIB),
/// handled as a Write Attributes Command except the response is not sent.
#define ZCL_CMD_WRITE_ATTRIB_NORESP			0x05

#define ZCL_CMD_CONFIGURE_REPORT				0x06

#define ZCL_CMD_CONFIGURE_REPORT_RESP		0x07

#define ZCL_CMD_READ_REPORT_CFG				0x08

#define ZCL_CMD_READ_REPORT_CFG_RESP		0x09

#define ZCL_CMD_REPORT_ATTRIB					0x0a

/// Default Response Command,
/// payload is 2 bytes; Command ID and Status Code (zcl_default_response_t).
#define ZCL_CMD_DEFAULT_RESP					0x0b

/// Discover Attributes Command,
/// payload is 3 bytes; start attribute ID and maximum number of attributes
/// to return (zcl_discover_attrib_t).
#define ZCL_CMD_DISCOVER_ATTRIB				0x0c

/// Discover Attributes Response Command
/// payload is a "discovery complete" byte (done = ZCL_BOOL_TRUE, more
/// attributes = ZCL_BOOL_TRUE), followed by 1 to n 3-byte Attribute Report
/// fields (zcl_rec_attrib_report_t).
#define ZCL_CMD_DISCOVER_ATTRIB_RESP		0x0d

#define ZCL_CMD_READ_STRUCT_ATTRIB			0x0e

#define ZCL_CMD_WRITE_STRUCT_ATTRIB			0x0f

#define ZCL_CMD_WRITE_STRUCT_ATTRIB_RESP	0x10

//@}

/// General format for an attribute, used for Write Attributes and Report
/// Attributes commands.
typedef PACKED_STRUCT zcl_attrib_t {
	uint16_t	id_le;
	uint8_t	type;			///< see zigbee/zcl_types.h
	uint8_t	value[1];	///< variable length, depending on type
} zcl_attrib_t;

/// Used for #ZCL_CMD_READ_ATTRIB_RESP.
typedef PACKED_STRUCT zcl_rec_read_attrib_resp_t {
	uint16_t	id_le;
	uint8_t	status;
	uint8_t	type;			///< only included if status == ZCL_STATUS_SUCCESS
	uint8_t	value[1];	///< variable length, only if ZCL_STATUS_SUCCESS
} zcl_rec_read_attrib_resp_t;

typedef PACKED_STRUCT zcl_rec_read_attrib_error_resp_t {
	uint16_t	id_le;
	uint8_t	status;
} zcl_rec_read_attrib_error_resp_t;

typedef PACKED_STRUCT zcl_rec_read_attrib_array_resp_t {
	uint16_t	id_le;
	uint8_t	status;
	uint8_t	type;			///< one of ZCL_TYPE_ARRAY, ZCL_TYPE_SET, ZCL_TYPE_BAG
	uint8_t	element_type;
	uint16_t	count_le;
	uint8_t	value[1];	///< variable length, only if ZCL_STATUS_SUCCESS
} zcl_rec_read_attrib_array_resp_t;

typedef PACKED_STRUCT zcl_rec_read_attrib_struct_resp_t {
	uint16_t	id_le;
	uint8_t	status;
	uint8_t	type;				///< always ZCL_TYPE_STRUCT
	uint16_t	count_le;		///< number of elements to follow
	uint8_t	elements[1];	///< mult. uint8_t type and variable-length value
} zcl_rec_read_attrib_struct_resp_t;

/// Used for #ZCL_CMD_WRITE_ATTRIB_RESP.
typedef PACKED_STRUCT zcl_rec_write_attrib_status_t {
	uint8_t	status;
	uint16_t	id_le;
} zcl_rec_write_attrib_status_t;


// Configure Reporting Command (ZCL_CMD_CONFIGURE_REPORT)
// payload is 1 to n variable-length Attribute Reporting Configuration records
// There are two different packet types, based on whether direction is SEND
// or RECEIVE.
#define ZCL_DIRECTION_SEND		0x00		// attributes sent (or reported)
#define ZCL_DIRECTION_RECEIVE	0x01		// attributes received
typedef PACKED_STRUCT zcl_rec_report_send_t {
	uint8_t	direction;		// 0x00 (ZCL_DIRECTION_SEND)
	uint16_t	attrib_id_le;
	uint8_t	attrib_type;
	uint16_t	min_interval_le;			// minimum reporting interval (in seconds)
	uint16_t	max_interval_le;			// maximum reporting interval (in seconds)
	         // Note:  If max_interval is 0xffff, device shall not issue reports
	         // for the specified attribute.
} zcl_rec_report_send_t;

typedef PACKED_STRUCT zcl_rec_report_receive_t {
	uint8_t	direction;		// 0x01 (ZCL_DIRECTION_RECEIVE)
	uint16_t	attrib_id_le;
	uint16_t	timeout_period_le;		// number of seconds, or 0 for no timeout
} zcl_rec_report_receive_t;

typedef union zcl_rec_report_t {
	PACKED_STRUCT {
		uint8_t	direction;		// ZCL_DIRECTION_SEND or ZCL_DIRECTION_RECEIVE
		uint16_t	attrib_id_le;
	} common;
	zcl_rec_report_send_t		send;
	zcl_rec_report_receive_t	receive;
} zcl_rec_reporting_config_t;

// Configure Reporting Response Command (ZCL_CMD_WRITE_REPORT_CFG_RESP)
// payload is two bytes (ZCL_STATUS_SUCCESS followed by a direction) if all
// writes were successful or 1 to n 4-byte Attribute Status Records for each
// write that failed
// Note: this structure is identical to the one for Read Reporting Configuration
// Command (ZCL_CMD_READ_REPORT_CFG), with the addition of a 1-byte status.
typedef PACKED_STRUCT zcl_rec_reporting_status_t {
	uint8_t	status;
	uint8_t	direction;		// ZCL_DIRECTION_SEND or ZCL_DIRECTION_RECEIVE
	uint16_t	attrib_id_le;
} zcl_rec_reporting_status_t;

// Read Reporting Configuration Command (ZCL_CMD_READ_REPORT_CFG)
// payload is 1 to n 3-byte Attribute Records
typedef PACKED_STRUCT zcl_rec_read_report_cfg_t {
	uint8_t	direction;		// ZCL_DIRECTION_SEND or ZCL_DIRECTION_RECEIVE
	uint16_t	attrib_id_le;
} zcl_rec_read_report_cfg_t;

// Read Reporting Configuration Response Command (ZCL_CMD_READ_REPORT_CFG_RESP)
// payload is 1 to n variable-length records as follows:
//  - for non-success status, the 4-byte record format used by Configure
//    Reporting Response Command (status, direction, attrib_id).
//  - for success status, a status byte followed by a zcl_rec_report_send_t
//    record (if direction is ZCL_DIRECTION_SEND) or a zcl_rec_report_receive_t
//    record (if direction is ZCL_DIRECTION_RECEIVE)
/* combined typedef?
typedef PACKED_STRUCT zcl_rec_report_resp_t {
	uint8_t	status;
	union {
		PACKED_STRUCT {
			uint8_t	direction;
			uint16_t	attrib_id_le;
		} error;				// .status != ZCL_STATUS_SUCCESS
		zcl_rec_report_send_t		send_cfg;
		zcl_rec_report_receive_t	receive_cfg;
	} u;
} zcl_rec_report_resp_t;
*/

typedef PACKED_STRUCT zcl_rec_report_error_resp_t {
	uint8_t	status;			// Set to anything except ZCL_STATUS_SUCCESS (0x00)
	uint8_t	direction;
	uint16_t	attrib_id_le;
} zcl_rec_report_error_resp_t;
typedef PACKED_STRUCT zcl_rec_report_send_resp_t {
	uint8_t							status;		// Set to ZCL_STATUS_SUCCESS (0x00)
	zcl_rec_report_send_t		report_cfg;
		// report_cfg.direction set to 0x00 (ZCL_DIRECTION_SEND)
} zcl_rec_report_send_resp_t;
typedef PACKED_STRUCT zcl_rec_report_receive_resp_t {
	uint8_t							status;		// Set to ZCL_STATUS_SUCCESS (0x00)
	zcl_rec_report_receive_t	report_cfg;
		// report_cfg.direction set to 0x01 (ZCL_DIRECTION_RECEIVE)
} zcl_rec_report_receive_resp_t;


/*
	Logic for handling a Read Reporting Configuration Response Command frame:

	char *buffer;
	zcl_rec_report_error_resp_t		*resp_error;
	zcl_rec_report_send_resp_t			*resp_send;
	zcl_rec_report_receive_resp_t		*resp_receive;

	resp_error = (zcl_rec_report_error_resp_t *) buffer;
	if (resp_error->status != ZCL_STATUS_SUCCESS) {
		// process error in resp_error
		buffer += sizeof (*resp_error);
	} else if (resp_error->direction == ZCL_DIRECTION_SEND) {
		resp_send = (zcl_rec_report_send_resp_t *) buffer;
		// process record in resp_send
		buffer += sizeof (*resp_send);
	} else if (resp_error->direction == ZCL_DIRECTION_RECEIVE) {
		resp_send = (zcl_rec_report_receive_resp_t *) buffer;
		// process record in resp_receive
		buffer += sizeof (*resp_receive);
	} else {
		// bad packet
	}
*/

// Report Attributes Command (ZCL_CMD_REPORT_ATTRIB)
// payload is 1 to n variable-length Attribute Report Records

// use zcl_attrib_t

/// Used for #ZCL_CMD_DEFAULT_RESP.
typedef PACKED_STRUCT zcl_default_response_t {
	uint8_t	command;
	uint8_t	status;
} zcl_default_response_t;

/// Used for #ZCL_CMD_DISCOVER_ATTRIB.
typedef PACKED_STRUCT zcl_discover_attrib_t {
	uint16_t	start_attrib_id_le;
	uint8_t	max_return_count;
} zcl_discover_attrib_t;

/// Used for #ZCL_CMD_DISCOVER_ATTRIB_RESP.
typedef PACKED_STRUCT zcl_rec_attrib_report_t {
	uint16_t	id_le;
	uint8_t	type;
} zcl_rec_attrib_report_t;
/// Used for #ZCL_CMD_DISCOVER_ATTRIB_RESP.
typedef PACKED_STRUCT zcl_discover_attrib_resp_t {
	/// Set to ZCL_BOOL_FALSE if there are more attributes than contained
	/// in the response.  Set to ZCL_BOOL_TRUE if discovery is complete.
	uint8_t							discovery_complete;
	zcl_rec_attrib_report_t		attrib[1];
} zcl_discover_attrib_resp_t;

// Read Attributes Structured Command (ZCL_CMD_READ_STRUCT_ATTRIB)
// payload is 1 to n variable-byte Attribute ID and Selector fields
typedef PACKED_STRUCT zcl_selector_t {
	uint8_t	indicator;
		#define ZCL_INDICATOR_INDICES_MASK		0x0F

		#define ZCL_INDICATOR_ACTION_MASK		0xF0
		#define ZCL_INDICATOR_REPLACE				0x00
		#define ZCL_INDICATOR_ADD_ELEMENT		0x10
		#define ZCL_INDICATOR_REMOVE_ELEMENT	0x20
		// indicator actions 0x30 to 0xF0 are reserved
	uint16_t	index_le[15];
} zcl_selector_t;
typedef PACKED_STRUCT zcl_read_struct_attrib_t {
	uint16_t			attrib_id_le;
	zcl_selector_t	selector;
} zcl_read_struct_attrib_t;


// Write Attributes Structured Command (ZCL_CMD_WRITE_STRUCT_ATTRIB)
// payload is 1 to n variable-length Write Structured Attribute Record fields.
typedef PACKED_STRUCT zcl_rec_write_struct_attrib_t {
	uint16_t			id_le;
	zcl_selector_t	selector;
	uint8_t			type;
	uint8_t			value[1];		// variable length, depending on type
} zcl_rec_write_struct_attrib_t;

// Write Attributes Structured Response Cmd (ZCL_CMD_WRITE_STRUCT_ATTRIB_RESP)
// payload is 1 to n variable-length Write Structure Attribute Status Record
// fields.
typedef PACKED_STRUCT zcl_rec_write_struct_attrib_resp_t {
	uint8_t			status;
	uint16_t			id;
	zcl_selector_t	selector;
} zcl_rec_write_struct_attrib_resp_t;




/*
	Some thoughts on these structures...

	One problem with the attrib status records is that for unknown data types,
	you don't know how long the record is.  Parsing a ZCL_CMD_READ_ATTRIB_RESP
	will be difficult if an attribute has an unknown type (on the plus side,
	a device should only be requesting attributes with types it already
	understands).

*/

typedef union zcl_attribute_minmax_t {
	int32_t			_signed;
	uint32_t			_unsigned;
	float				_float;
} zcl_attribute_minmax_t;

struct zcl_attribute_base_t;
struct zcl_attribute_full_t;
/**
	@brief
	Function pointer assigned to a ZCL attribute record, called to update
	the value of the given attribute.

	Necessary for attributes that are tied to physical hardware (only updated
	when requested) or attributes that change over time (as with the ZCL
	Identify cluster).

	If the return value is not ZCL_STATUS_SUCCESS, the ZCL Read Attributes
	handler will return that as an error in the response frame.

	@param[in,out]	entry		attribute to update

	@retval	ZCL_STATUS_SUCCESS		successfully updated value
	@retval	ZCL_STATUS_HARDWARE_FAILURE
				unable to update value due to hardware problem
	@retval	ZCL_STATUS_SOFTWARE_FAILURE
				unable to update value due to software problem

	@sa zcl_attribute_write_fn(), zcl_attribute_full_t
*/
typedef uint_fast8_t (*zcl_attribute_update_fn)(
	const struct zcl_attribute_full_t FAR *entry);

typedef struct zcl_attribute_write_rec_t
{
	/// Source data to write to attribute.
	const uint8_t FAR *buffer;

	/// Number of bytes in \p buffer.
	int16_t buflen;

	/// Parsing flags (see ZCL_ATTR_WRITE_FLAG_* macros for valid flags).
	uint8_t	flags;
		#define ZCL_ATTR_WRITE_FLAG_NONE			0x00
		/// Assign the new value to the attribute (if not set, just checks the
		/// validity of the data).
		#define ZCL_ATTR_WRITE_FLAG_ASSIGN		0x01

		/// Parsing a Write Attribute Request record.
		#define ZCL_ATTR_WRITE_FLAG_WRITE_REQ	0x00
		/// Parsing a Read Attribute Response record.
		#define ZCL_ATTR_WRITE_FLAG_READ_RESP	0x02

	/// Set to #ZCL_STATUS_SUCCESS if data is valid, or some other ZCL_STATUS_*
	/// macro if there's an error.
	uint8_t	status;
} zcl_attribute_write_rec_t;

/**
	@brief
	Function pointer assigned to a ZCL attribute record, called instead of
	zcl_decode_attribute() to process a Write Request for the attribute.

	This function typically calls zcl_decode_attribute() to convert the
	request to a temporary variable and then perform additional checks on
	it before assigning to the actual variable.

	@param[in]		entry		attribute to update
	@param[in,out]	rec		record from Write Request with new value

	@return	number of bytes consumed from Write Request

	@sa zcl_attribute_update_fn(), zcl_attribute_full_t
*/
typedef int (*zcl_attribute_write_fn)(
	const struct zcl_attribute_full_t	FAR *entry,
	zcl_attribute_write_rec_t					*rec);

/**
	Basic structure for storing a list of attributes in a cluster.  If .min and
	.max are both set to 0, there isn't a limit

	@note Consider putting type after id -- it would then be possible to
			have macros for attributes that include the id AND type.
*/
typedef struct zcl_attribute_base_t
{
	uint16_t							id;
	uint8_t							flags;
		/** @name
			Values for \p flags element of zcl_attribute_base_t.
			@{
		*/
		#define ZCL_ATTRIB_FLAG_NONE			0x00
		#define ZCL_ATTRIB_FLAG_WRITEONLY	0x01
		#define ZCL_ATTRIB_FLAG_READONLY		0x02
		#define ZCL_ATTRIB_FLAG_HAS_MIN		0x04
		#define ZCL_ATTRIB_FLAG_HAS_MAX		0x08
		#define ZCL_ATTRIB_FLAG_REPORTABLE	0x10

		// flags available for future use
		#define ZCL_ATTRIB_FLAG_UNUSED1		0x20

		/// Data is stored as sent/received in ZCL requests (little-endian,
		/// 24-bit types stored as 3 bytes instead of 4).  Not applicable
		/// to string types.
		#define ZCL_ATTRIB_FLAG_RAW			0x40

		/// If set, this is part of a zcl_attribute_full_t structure.
		#define ZCL_ATTRIB_FLAG_FULL			0x80
		//@}

	/// The ZCL Attribute Type, should be one of the ZCL_TYPE_* macros defined
	/// in zigbee/zcl_types.h
	uint8_t							type;

	/// Pointer to global variable of correct type, set as \c const to allow
	/// for both const and non-const values.  (If \c const, flags should be
	/// include #ZCL_ATTRIB_FLAG_READONLY.)  If NULL, the ZCL stack will
	/// respond to attempts to read/write with ZCL_STATUS_DEFINED_OUT_OF_BAND.
	const void				FAR	*value;

} zcl_attribute_base_t;

/**
	Use this structure for attributes with min/max values or the need to have a
	read/write function.  Be sure to set the ZCL_ATTRIB_FLAG_FULL flag in
	.base.flags so the attribute is parsed with the correct type.
*/
typedef struct zcl_attribute_full_t
{
	zcl_attribute_base_t		base;

	/// Minimum value for attribute (currently unused until I pick a value
	/// to represent "none").
	zcl_attribute_minmax_t		min;

	/// Maximum value for attribute, assumes 32-bit limit.  For string
	/// attributes, the maximum length that can be stored at \p value.
	zcl_attribute_minmax_t		max;

	/// On an attribute read, the ZCL dispatcher will call the .read function
	/// to refresh the attribute's value before responding to the request.
	/// Do I need a return value in the function to pass an error up?
	zcl_attribute_update_fn		read;

	/// On an attribute write, the ZCL dispatcher will call the .write function
	/// with a structure containing information about the write request.  See
	/// the documentation for zcl_attribute_write_fn() for the API, and the
	/// Time and Identify clusters for examples of its use.
	zcl_attribute_write_fn		write;
} zcl_attribute_full_t;

/**
	If a zcl_attribute_base_t has a type of ZCL_TYPE_ARRAY, its .value points to
	this structure.  It's possible to read and write values of the array by
	accessing
	@code (uint8_t FAR *)arr->value + index * arr->element_size @endcode
*/
// DEVNOTE: This isn't an extension of zcl_attribute_base_t (like
// zcl_attribute_full_t) because its size can change and therefore can't be
// stored in flash.  It's also necessary to be separate to support arrays of
// arrays.
typedef struct zcl_array_t
{
	uint16_t					max_count;		///< maximum number of elements to store
	uint16_t					current_count;	///< current number of elements
	uint16_t					element_size;	///< number of bytes in each element
	uint8_t					type;				///< type of data stored in array
	const void		FAR	*value;
} zcl_array_t;

/**
	A zcl_struct_t contains an array of these elements, describing each element
	in the structure and its offset from the zcl_struct_t.base_address.
*/
typedef struct zcl_struct_element_t
{
	uint8_t					zcl_type;		///< ZCL data type of element
	uint16_t					offset;			///< offset into structure
} zcl_struct_element_t;

/**
	If a zcl_attribute_base_t has a type of ZCL_TYPE_STRUCT, its .value points
	to this structure.
*/
typedef struct zcl_struct_t
{
	/// number of elements in structure
	uint16_t										element_count;
	/// base address of structure
	const void							FAR	*base_address;
	/// elements in the structure
	const zcl_struct_element_t		FAR	*element;
} zcl_struct_t;

// DEVNOTE: consider a flag in zcl_struct_element_t and zcl_array_t to
// indicate that underlying value should have ZCL_ATTRIB_FLAG_RAW, or to
// provide a maximum length if the element is a writeable string.

/// Attribute ID for end of list marker.
#define ZCL_ATTRIBUTE_END_OF_LIST		0xFFFF

#define ZCL_MFG_NONE							0x0000
typedef struct zcl_attribute_tree_t
{
	/// ID for manufacturer-specific attributes or ZCL_MFG_NONE if general.
	uint16_t								manufacturer_id;

	/// List of attributes for the SERVER cluster (or NULL if none).
	const zcl_attribute_base_t	FAR *server;

	/// List of attributes for the CLIENT cluster (or NULL if none).
	const zcl_attribute_base_t	FAR *client;
} zcl_attribute_tree_t;

/// If a cluster doesn't have attributes, use zcl_attributes_none (a single,
/// empty attribute list) in the cluster definition.  Don't use NULL, since
/// the APS dispatcher will use endpoint's context if the cluster's context
/// is NULL.
extern const zcl_attribute_tree_t FAR zcl_attributes_none[1];

/**
	Structure populated by zcl_command_build() with the ZCL header from a
	received ZCL command frame.  Does not look at the ZCL payload.
*/
typedef struct zcl_command_t {
	/// envelope for the received command, envelope's payload and length
	/// include the ZCL header
	const wpan_envelope_t		FAR	*envelope;
	/// frame control byte from the ZCL header
	uint8_t									frame_control;
	/// manufacturer code from the ZCL header, in host byte order,
	/// or 0x0000 if not mfg specific
	uint16_t									mfg_code;
	/// sequence byte from the ZCL header
	uint8_t									sequence;
	/// command byte from the ZCL header
	uint8_t									command;
	/// Appropriate list of attributes on this device, related to the command.
	/// Based on the \c direction bit from \c frame_control and the \c mfg_code
	/// field.
	const zcl_attribute_base_t	FAR	*attributes;
	/// pointer to the ZCL payload (first byte after ZCL header)
	const void						FAR	*zcl_payload;
	/// length of the ZCL payload
	int16_t									length;
} zcl_command_t;

int zcl_command_build( zcl_command_t *cmd,
	const wpan_envelope_t FAR *envelope, zcl_attribute_tree_t FAR *tree);

void zcl_command_dump( const zcl_command_t *cmd);
void zcl_envelope_payload_dump( const wpan_envelope_t *envelope);

int zcl_general_command( const wpan_envelope_t FAR *envelope,
	void FAR *context);

int zcl_invalid_cluster( const wpan_envelope_t FAR *envelope,
	wpan_ep_state_t FAR *ep_state);
int zcl_invalid_command( const wpan_envelope_t FAR *envelope);

int zcl_build_header( zcl_header_response_t *rsp, zcl_command_t *cmd);
int zcl_parse_attribute_record( const zcl_attribute_base_t FAR *entry,
	zcl_attribute_write_rec_t *rec);
int zcl_default_response( zcl_command_t *cmd, uint8_t status);

int zcl_check_minmax( const zcl_attribute_base_t FAR *entry,
	const uint8_t FAR *buffer_le);

uint32_t zcl_convert_24bit( const void FAR *value_le, bool_t extend_sign);

int zcl_decode_attribute( const zcl_attribute_base_t FAR *entry,
	zcl_attribute_write_rec_t *rec);
int zcl_encode_attribute_value( uint8_t FAR *buffer, int16_t bufsize,
	const zcl_attribute_base_t FAR *entry);

const zcl_attribute_base_t FAR *zcl_attribute_get_next(
	const zcl_attribute_base_t FAR *entry);
const zcl_attribute_base_t FAR *zcl_find_attribute(
	const zcl_attribute_base_t FAR *entry, uint16_t search_id);
int zcl_send_response( zcl_command_t *cmd, const void FAR *payload,
	uint16_t length);

XBEE_END_DECLS

// If compiling in Dynamic C, automatically #use the appropriate C file.
#ifdef __DC__
	#use "zigbee_zcl.c"
#endif

#endif	// __ZIGBEE_ZCL_H





