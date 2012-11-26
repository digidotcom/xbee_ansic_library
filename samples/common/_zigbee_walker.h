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
	Routines to use ZDO and ZCL to discover and dump all attributes.

	Uses ZDO Active Endpoints request to get a list of the endpoints, then,
	for each endpoint, uses a ZDO Simple Descriptor request to get a list of
	clusters for that endpoint.  Then, for each cluster, uses ZCL Discover
	Attributes requests to build up a list of attributes.

	Finally, uses ZCL Read Attributes requests to get each attribute so it
	can display the type and value.
*/

#ifndef _ZIGBEE_WALKER_H
#define _ZIGBEE_WALKER_H

#define WALKER_VER			0x0103
#define WALKER_VER_STR		"1.03"

#include <time.h>
#include "xbee/platform.h"
#include "wpan/aps.h"
#include "zigbee/zcl.h"

#define SAMPLE_ENDPOINT 1

struct _endpoints {
	wpan_endpoint_table_entry_t		zdo;
	wpan_endpoint_table_entry_t		zcl;
	uint8_t									end_of_list;
};

extern struct _endpoints sample_endpoints;

enum walker_status_t {
	WALKER_INIT,
	WALKER_NWK_ADDR_REQ,			///< send ZDO NWK_Addr request
	WALKER_NWK_ADDR_RSP,			///< waiting for ZDO NWK_Addr response
	WALKER_ACTIVE_EP_REQ,		///< send ZDO Active Endpoints request
	WALKER_SIMPLE_DESC_REQ,		///< send next ZDO Simple Descriptor request

	WALKER_DISCOVER_ATTR_REQ,	///< send a ZCL Discover Attributes request

	WALKER_READ_ATTR_REQ,		///< send a ZCL Read Attributes request
	WALKER_WAIT,					///< waiting for a response or timeout
	WALKER_DONE,
	WALKER_ERROR
};

// maximum number of clusters on an active endpoint
#define WALKER_MAX_CLUST	200

#if WALKER_MAX_CLUST > 255
	#error "This code can't support more than 255 clusters on an endpoint."
#endif

// maximum number of attributes to process at a time (note that this
// only needs to be large enough to handle the response from a Discover
// Attributes request).
#define WALKER_MAX_ATTRIB	150

typedef struct walker_state_t {
	// current state of the walker
	enum walker_status_t		status;

	// start time
	time_t						start;

	// stop time
	time_t						stop;

	// device used to send and receive requests
	wpan_dev_t					*dev;

	// various flags to control the walker, some set by the user
	uint16_t						flags;
		#define WALKER_FLAGS_NONE			0x0000
		// arbitrarily select lower byte for user flags, upper byte for flags
		// used by the walker
		#define WALKER_USER_FLAG_MASK		0x00FF

	// reusable variable for multiple states
	uint16_t						timeout;

	// target device we're walking
	wpan_address_t				target;

	// bit array of active endpoints of the target
	uint8_t						active_ep[256/8];
		#define WALKER_SET_EP(w,ep)	\
					(w).active_ep[(ep) >> 3] |= 1 << ((ep) & 0x07)
		#define WALKER_EP_IS_SET(w,ep)	\
					((w).active_ep[(ep) >> 3] & (1 << ((ep) & 0x07)))
	// current endpoint we're walking
	uint8_t						current_endpoint;

	// profile ID of endpoint
	uint16_t						profile_id;

	struct {
		// list of clusters (input followed by output) for current_endpoint
		uint16_t						list[WALKER_MAX_CLUST];

		// index into cluster_list[] of first output cluster
		uint_fast8_t				out_index;

		// number of clusters in the cluster list
		uint_fast8_t				count;

		// index of cluster we're walking
		uint_fast8_t				index;
	} cluster;

	struct {
		// attribute list returned from last discover_attributes
		zcl_rec_attrib_report_t	list[WALKER_MAX_ATTRIB];

		uint_fast8_t				count;

		uint_fast8_t				index;

		// FALSE if we need to discover more attributes
		bool_t						discovery_complete;
	} attribute;
} walker_state_t;


int walker_init( wpan_dev_t *dev, addr64 *target, uint16_t flags);

enum walker_status_t walker_tick( void);

#endif
