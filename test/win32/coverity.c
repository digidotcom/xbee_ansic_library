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
	Dummy file used to trigger coverage of functions normally called via
	function pointers in "dispatch" tables.

	- XBee Frame Handlers
	- Endpoint Handlers
	- Cluster Handlers
*/

#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"
#include "xbee/device.h"
#include "xbee/ota_server.h"
#include "xbee/wpan.h"

#include "wpan/types.h"

#include "zigbee/zcl.h"
#include "zigbee/zcl_commissioning.h"

xbee_dev_t my_xbee = { { 0 } };
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_TABLE_END
};

const zcl_attribute_base_t attr_server[] = {
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};
const zcl_attribute_base_t attr_client[] = {
	{ ZCL_ATTRIBUTE_END_OF_LIST }
};

zcl_attribute_tree_t attribute_tree[] = {
	{ ZCL_MFG_NONE, attr_server, attr_client }
};

const zcl_comm_startup_param_t zcl_comm_default_sas;

void ZCL_FACTORY_RESET_FN( void)
{
}

const char *xbee_update_firmware_ota( const wpan_envelope_t FAR *envelope,
        void FAR *context)
{
	return NULL;
}

#define T_HANDLER(fn)	do {													\
									fn( &my_xbee, buffer, buflen, NULL);	\
									fn( NULL, buffer, buflen, NULL);			\
									fn( &my_xbee, NULL, buflen, NULL);		\
									fn( &my_xbee, buffer, 0, NULL);			\
								} while (0)

void frame_handlers( const char *buffer, int buflen)
{
	// xbee/xbee_atcmd.c
	T_HANDLER( _xbee_cmd_handle_response);
	T_HANDLER( _xbee_cmd_modem_status);

	// xbee/xbee_device.c
	T_HANDLER( xbee_frame_dump_modem_status);

	// xbee/xbee_wpan.c
	T_HANDLER( _xbee_handle_receive_explicit);
	T_HANDLER( _xbee_handle_transmit_status);
}
#undef T_HANDLER

#define T_HANDLER(fn)	do {											\
									fn( &envelope, attribute_tree);	\
									fn( NULL, attribute_tree);		\
									fn( &envelope, NULL);				\
									fn( NULL, NULL);						\
								} while (0)

void cluster_handlers( const char *buffer, int buflen)
{
	wpan_envelope_t envelope;

	wpan_envelope_create( &envelope, &my_xbee.wpan_dev,
		WPAN_IEEE_ADDR_COORDINATOR, WPAN_NET_ADDR_COORDINATOR);
	envelope.payload = buffer;
	envelope.length = buflen;

	T_HANDLER( zcl_general_command);
	zcl_general_command( NULL, NULL);
}

#undef T_HANDLER

int main( int argc, char *argv[])
{
	char buffer[256];
	int buflen;

	fgets( buffer, sizeof buffer, stdin);
	buflen = strlen( buffer);

	frame_handlers( buffer, buflen);
	cluster_handlers( buffer, buflen);

	return 0;
}
