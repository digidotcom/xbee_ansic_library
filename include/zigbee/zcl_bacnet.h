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
	@file zigbee/zcl_bacnet.h
	Macros associated with supporting BACnet under ZigBee.
*/

#ifndef ZCL_BACNET_H
#define ZCL_BACNET_H

#include "wpan/aps.h"
#include "zigbee/zcl.h"

XBEE_BEGIN_DECLS

#define ZCL_BACNET_PROFILE 0x0105

// Devices from the CBA (Commercial Building Automation) Profile
#define ZCL_BACNET_DEV_CONSTRUCTED					0x000A
#define ZCL_BACNET_DEV_TUNNELED						0x000B

#define ZCL_BACNET_ATTR_ACKED_TRANSITIONS			0x0000	// BITMAP8
#define ZCL_BACNET_ATTR_ACTIVE_TEXT					0x0004	// CHAR STRING
#define ZCL_BACNET_ATTR_ALARM_VALUE					0x0006	// BOOLEAN
#define ZCL_BACNET_ATTR_STATE_TEXT					0x000E	// ARRAY of STRING
#define ZCL_BACNET_ATTR_CHANGE_OF_STATE_COUNT	0x000F	// UINT32
#define ZCL_BACNET_ATTR_CHANGE_OF_STATE_TIME		0x0010	// STRUCT
#define ZCL_BACNET_ATTR_NOTIFICATION_CLASS		0x0011	// UINT16
#define ZCL_BACNET_ATTR_COV_INCREMENT				0x0016	// FLOAT32
#define ZCL_BACNET_ATTR_DEADBAND						0x0019	// FLOAT32
#define ZCL_BACNET_ATTR_DESCRIPTION					0x001C	// CHAR STRING
#define ZCL_BACNET_ATTR_DEVICE_TYPE					0x001F	// CHAR STRING
#define ZCL_BACNET_ATTR_ELAPSED_ACTIVE_TIME		0x0021	// UINT32
#define ZCL_BACNET_ATTR_EVENT_ENABLE				0x0023	// BITMAP8
#define ZCL_BACNET_ATTR_EVENT_STATE					0x0024	// ENUM8
#define ZCL_BACNET_ATTR_FAULT_VALUES				0x0025	// SET of UINT16
#define ZCL_BACNET_ATTR_FEEDBACK_VALUE				0x0028	// ENUM8
#define ZCL_BACNET_ATTR_HIGH_LIMIT					0x002D	// FLOAT32
#define ZCL_BACNET_ATTR_INACTIVE_TEXT				0x002E	// CHAR STRING
#define ZCL_BACNET_ATTR_LIMIT_ENABLE				0x0034	// BITMAP8
#define ZCL_BACNET_ATTR_LOW_LIMIT					0x003B	// FLOAT32
#define ZCL_BACNET_ATTR_MAX_PRESENT_VALUE			0x0041	// FLOAT32
#define ZCL_BACNET_ATTR_MINIMUM_OFF_TIME			0x0042	// UINT32
#define ZCL_BACNET_ATTR_MINIMUM_ON_TIME			0x0043	// UINT32
#define ZCL_BACNET_ATTR_MIN_PRESENT_VALUE			0x0045	// FLOAT32
#define ZCL_BACNET_ATTR_NOTIFY_TYPE					0x0048	// ENUM8
#define ZCL_BACNET_ATTR_NUMBER_OF_STATES			0x004A	// UINT16
#define ZCL_BACNET_ATTR_OBJECT_IDENTIFIER			0x004B	// BACnet OID
#define ZCL_BACNET_ATTR_OBJECT_NAME					0x004D	// CHAR STRING
#define ZCL_BACNET_ATTR_OBJECT_TYPE					0x004F	// ENUM16
#define ZCL_BACNET_ATTR_OUT_OF_SERVICE				0x0051	// BOOLEAN
#define ZCL_BACNET_ATTR_POLARITY						0x0054	// ENUM8
#define ZCL_BACNET_ATTR_PRESENT_VALUE				0x0055	// FLOAT32
#define ZCL_BACNET_ATTR_PRIORITY_ARRAY				0x0057	// ARRAY of STRUCT
#define ZCL_BACNET_ATTR_RELIABILITY					0x0067	// ENUM8
#define ZCL_BACNET_ATTR_RELINQUISH_DEFAULT		0x0068	// FLOAT32
#define ZCL_BACNET_ATTR_RESOLUTION					0x006A	// FLOAT32
#define ZCL_BACNET_ATTR_STATUS_FLAGS				0x006F	// BITMAP8
#define ZCL_BACNET_ATTR_TIME_DELAY					0x0071	// UINT8
#define ZCL_BACNET_ATTR_TIME_OF_AT_RESET			0x0072	// STRUCT
#define ZCL_BACNET_ATTR_TIME_OF_SC_RESET			0x0073	// STRUCT
#define ZCL_BACNET_ATTR_ENGINEERING_UNITS			0x0075	// ENUM16
#define ZCL_BACNET_ATTR_UPDATE_INTERVAL			0x0076	// UINT8
#define ZCL_BACNET_ATTR_EVENT_TIME_STAMPS			0x0082	// ARRAY
#define ZCL_BACNET_ATTR_PROFILE_NAME				0x00A8	// CHAR STRING
#define ZCL_BACNET_ATTR_APPLICATION_TYPE			0x0100	// UINT32

// Values less than 0x0400 are reserved; 0x0400 to 0xFFFF are reserved for
// vendor-specific attributes.

// bits of the ZCL_BACNET_ATTR_STATUS_FLAGS attribute
#define ZCL_BACNET_STATUS_FLAG_IN_ALARM			(1<<0)
#define ZCL_BACNET_STATUS_FLAG_FAULT				(1<<1)
#define ZCL_BACNET_STATUS_FLAG_OVERRIDDEN			(1<<2)
#define ZCL_BACNET_STATUS_FLAG_OUT_OF_SERVICE	(1<<3)

// values for ZCL_BACNET_ATTR_POLARITY
#define ZCL_BACNET_POLARITY_NORMAL					0
#define ZCL_BACNET_POLARITY_REVERSE					1

// Attributes structure for ZCL_CLUST_BINARY_OUT
typedef struct zcl_binary_output_attr_t {
	zcl_attribute_base_t		active_text;
	zcl_attribute_base_t		description;
	zcl_attribute_base_t		inactive_text;
	zcl_attribute_base_t		out_of_service;
	zcl_attribute_full_t		present_value;
	zcl_attribute_full_t		status_flags;
	uint16_t						end_of_list;
} zcl_binary_output_attr_t;

// Variable data for ZCL_CLUST_BINARY_OUT
typedef struct zcl_binary_output_t {
	bool_t		present_value;
	bool_t		out_of_service;
	uint8_t		status_flags;
} zcl_binary_output_t;

// macro to create attributes for binary output cluster (ZCL_CLUST_BINARY_OUT)
// TODO need to pass in function pointers for value_write and status_read
// maybe split cluster table and ep state out, since there could be a need
// to create an endpoint with both an output and an input?
#define ZCL_BINARY_OUTPUT_VARS( var, desc, zbot, value_write, status_read)	\
	const zcl_binary_output_attr_t var ## _attr = {									\
		{	ZCL_BACNET_ATTR_ACTIVE_TEXT,		ZCL_ATTRIB_FLAG_READONLY,			\
			ZCL_TYPE_STRING_CHAR,				desc " ON" },							\
		{	ZCL_BACNET_ATTR_DESCRIPTION,		ZCL_ATTRIB_FLAG_READONLY,			\
			ZCL_TYPE_STRING_CHAR,				desc },									\
		{	ZCL_BACNET_ATTR_INACTIVE_TEXT,	ZCL_ATTRIB_FLAG_READONLY,			\
			ZCL_TYPE_STRING_CHAR,				desc " OFF" },							\
		{	ZCL_BACNET_ATTR_OUT_OF_SERVICE,	ZCL_ATTRIB_FLAG_READONLY,			\
			ZCL_TYPE_LOGICAL_BOOLEAN,			&zbot.out_of_service },				\
		{	{	ZCL_BACNET_ATTR_PRESENT_VALUE,	ZCL_ATTRIB_FLAG_FULL,			\
				ZCL_TYPE_LOGICAL_BOOLEAN,		&zbot.present_value					\
			},	0, 0, NULL, value_write },													\
		{	{	ZCL_BACNET_ATTR_STATUS_FLAGS,												\
				ZCL_ATTRIB_FLAG_FULL | ZCL_ATTRIB_FLAG_READONLY,					\
				ZCL_TYPE_BITMAP_8BIT,			&zbot.status_flags					\
			}, 0, 0, status_read, NULL },													\
		ZCL_ATTRIBUTE_END_OF_LIST };														\
	const zcl_attribute_tree_t var ## _tree[] =										\
		{ { ZCL_MFG_NONE, &var ## _attr.active_text, NULL } };					\
	const wpan_cluster_table_entry_t var ## _cluster_table[] =					\
		{ 	ZCL_CLUST_ENTRY_BASIC_SERVER, 												\
			ZCL_CLUST_ENTRY_IDENTIFY_SERVER,												\
			{	ZCL_CLUST_BINARY_OUT, 														\
				&zcl_general_command, 														\
				var ## _tree, 																	\
				WPAN_CLUST_FLAG_SERVER														\
			}, 																					\
			{ WPAN_CLUST_ENTRY_LIST_END }													\
		};																							\
	wpan_ep_state_t var ## _ep_state;

// extern declarations
// @sa ZCL_BINARY_OUTPUT_VARS, ZCL_BACNET_ENDPOINT
#define ZCL_BINARY_OUTPUT_EXTERN( var)												\
	extern wpan_ep_state_t var ## _ep_state;										\
	extern const wpan_cluster_table_entry_t var ## _cluster_table[]



/**
	Create an entry in the endpoint table for a constructed BACnet device.
	Parameter \c name should refer to a wpan_ep_state_t global (named
	{name}_ep_state) and a const array of wpan_cluster_table_entry_t
	(named {name}_cluster_table).
*/
#define ZCL_BACNET_ENDPOINT( id, profile, name) \
	{	id, profile, zcl_invalid_cluster, &name ## _ep_state, \
		ZCL_BACNET_DEV_CONSTRUCTED, 0x00, name ## _cluster_table }

XBEE_END_DECLS

#endif
