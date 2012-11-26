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

/**
	@addtogroup zcl_basic
	@{
	@file zigbee/zcl_basic.h
	ZigBee Cluster Library; Basic Cluster (ZCL_CLUSTER_BASIC, 0x0000)

	See zigbee/zcl_basic_attributes.h for an easy method to incorporate a
	Basic Cluster Server into an application.

	@todo Explore how we would support the DeviceEnabled attribute.  Does each
			application need to check it?  Does the endpoint dispatcher need to
			know about it?  What about clients?
*/

#ifndef __XBEE_ZCL_BASIC_H
#define __XBEE_ZCL_BASIC_H

#include "zigbee/zcl.h"

XBEE_BEGIN_DECLS

/**
	@name ZCL Basic Cluster Attribute IDs
	@{
*/
#define ZCL_BASIC_ATTR_ZCL_VERSION			0x0000
							///< ZCLVersion, UINT8, read-only
#define ZCL_BASIC_ATTR_APP_VERSION			0x0001
							///< ApplicationVersion, UINT8, read-only
#define ZCL_BASIC_ATTR_STACK_VERSION		0x0002
							///< StackVersion, UINT8, read-only
#define ZCL_BASIC_ATTR_HW_VERSION			0x0003
							///< HWVersion, UINT8, read-only
#define ZCL_BASIC_ATTR_MANUFACTURER_NAME	0x0004
							///< ManufacturerName, 32-char STRING, read-only
#define ZCL_BASIC_ATTR_MODEL_IDENTIFIER	0x0005
							///< ModelIdentifier, 32-char STRING, read-only
#define ZCL_BASIC_ATTR_DATE_CODE				0x0006
							///< DateCode, 16-char STRING, read-only
#define ZCL_BASIC_ATTR_POWER_SOURCE			0x0007
							///< PowerSource, ENUM8, read-only

#define ZCL_BASIC_ATTR_LOCATION_DESC		0x0010
							///< LocationDescription, 16-char STRING, r/w
#define ZCL_BASIC_ATTR_PHYSICAL_ENV			0x0011
							///< PhysicalEnvironment, ENUM8, r/w
#define ZCL_BASIC_ATTR_DEVICE_ENABLED		0x0012
							///< DeviceEnabled, BOOLEAN, r/w
#define ZCL_BASIC_ATTR_ALARM_MASK			0x0013
							///< AlarmMask, BITMAP8, r/w
//@}

/**
	ZCL_VERSION is sent as ZCLVersion attribute.
	Per the ZCL Spec, 3.2.2.2.2: "For the initial version of the ZCL, this
	attribute shall be set to 0x01."
*/
#define ZCL_VERSION	0x01

/**
	@name
	Enumerated values for b0-b6 of PowerSource attribute.
	@{
*/
#define ZCL_BASIC_PS_UNKNOWN				0x00
											///< Unknown
#define ZCL_BASIC_PS_SINGLE_PHASE		0x01
											///< Mains (single phase)
#define ZCL_BASIC_PS_THREE_PHASE			0x02
											///< Mains (3 phase)
#define ZCL_BASIC_PS_BATTERY				0x03
											///< Battery
#define ZCL_BASIC_PS_DC						0x04
											///< DC source
#define ZCL_BASIC_PS_EMERGENCY_CONST	0x05
											///< Emergency mains, constant power
#define ZCL_BASIC_PS_EMERGENCY_SWITCH	0x06
											///< Emergency mains, transfer switch
	// 0x07 to 0x7F are reserved
//@}

/// definition of b7 of PowerSource attribute (device has battery backup)
#define ZCL_BASIC_PS_BATTERY_BACKUP		0x80

/// Single command of the Basic Server Cluster
#define ZCL_BASIC_CMD_FACTORY_DEFAULTS	0x00

int _zcl_basic_server( const wpan_envelope_t FAR *envelope,
	void FAR *context);

/**
	Macro used to add a Basic Cluster Server to an endpoint's cluster table.
	If #ZCL_FACTORY_RESET_FN is defined, the Basic Server Cluster will support
	the optional "Reset to Factory Defaults" command.

	Note that you must #include "zcl_basic_attributes.h" in your program
	(to create constants referenced by the macros) before using these macros
	in a cluster table.
*/
#ifdef ZCL_FACTORY_RESET_FN
	#define ZCL_CLUST_ENTRY_BASIC_SERVER	\
		{ ZCL_CLUST_BASIC,						\
			&_zcl_basic_server,					\
			zcl_basic_attribute_tree,			\
			WPAN_CLUST_FLAG_SERVER }
#else
	#define ZCL_CLUST_ENTRY_BASIC_SERVER	\
		{ ZCL_CLUST_BASIC,						\
			&zcl_general_command,				\
			zcl_basic_attribute_tree,			\
			WPAN_CLUST_FLAG_SERVER }
#endif

XBEE_END_DECLS

#endif		// __XBEE_ZCL_BASIC_H


