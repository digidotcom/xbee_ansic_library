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
	@file zcl_basic.c

	Support for the "Reset to Factory Defaults" command on the Basic Server
	Cluster.
*/

/*** BeginHeader */
#include <stdio.h>

#include "xbee/platform.h"
#include "zigbee/zcl.h"
#include "zigbee/zcl_basic.h"
/*** EndHeader */

/*** BeginHeader _zcl_basic_server */
/*** EndHeader */
#ifdef ZCL_FACTORY_RESET_FN
	void ZCL_FACTORY_RESET_FN( void);
#endif


/**
	@internal
	@brief
	Handles commands for the Basic Server Cluster.

	Currently supports the
	only command ID in the spec, 0x00 - Reset to Factory Defaults.

	@note You must define the macro ZCL_FACTORY_RESET_FN in your program, and have
		it point to a function to be called when a factory reset command is sent.

	@sa ZCL_CLUST_ENTRY_BASIC_SERVER
*/
int _zcl_basic_server( const wpan_envelope_t FAR *envelope,
	void FAR *context)
{
	zcl_command_t	zcl;

	if (zcl_command_build( &zcl, envelope, context) == 0 &&
		ZCL_CMD_MATCH( &zcl.frame_control, GENERAL, CLIENT_TO_SERVER, CLUSTER))
	{
		// This function only handles command 0x00, reset to factory defaults.
		if (zcl.command == ZCL_BASIC_CMD_FACTORY_DEFAULTS)
		{
			#ifdef ZCL_BASIC_VERBOSE
				printf( "%s: resetting to factory defaults\n", __FUNCTION__);
			#endif
			#ifdef ZCL_FACTORY_RESET_FN
				ZCL_FACTORY_RESET_FN();
			#endif
			return zcl_default_response( &zcl, ZCL_STATUS_SUCCESS);
		}
	}

	return zcl_general_command( envelope, context);
}
