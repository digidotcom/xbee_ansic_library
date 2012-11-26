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

/*
	Simple code for managing a table of xbee_node_id_t objects -- the
	XBee-specific node record used for ATND responses and Join Notifications.
*/

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "xbee/atcmd.h"

#include "_nodetable.h"

xbee_node_id_t node_table[NODE_TABLE_SIZE] = { { { { 0 } } } };

// search the node table for a node by its IEEE address
xbee_node_id_t *node_by_addr( const addr64 FAR *ieee_be)
{
	xbee_node_id_t *rec;

	for (rec = node_table; rec < &node_table[NODE_TABLE_SIZE]; ++rec)
	{
		if (addr64_equal( ieee_be, &rec->ieee_addr_be))
		{
			return rec;
		}
	}

	return NULL;
}

// search the node table for a node by its name (NI setting)
xbee_node_id_t *node_by_name( const char *name)
{
	xbee_node_id_t *rec;

	for (rec = node_table; rec < &node_table[NODE_TABLE_SIZE]; ++rec)
	{
		if (! addr64_is_zero( &rec->ieee_addr_be)
			&& strcmp( rec->node_info, name) == 0)
		{
			return rec;
		}
	}

	return NULL;
}

// search the node table for a node by its index in the table
xbee_node_id_t *node_by_index( int idx)
{
	xbee_node_id_t *rec;

	if (idx >= 0 && idx < NODE_TABLE_SIZE)
	{
		rec = &node_table[idx];
	   if (! addr64_is_zero( &rec->ieee_addr_be))
	   {
	   	return rec;
	   }
	}

	return NULL;
}

// copy node_id into the node table, possibly updating existing entry
xbee_node_id_t *node_add( const xbee_node_id_t *node_id)
{
	xbee_node_id_t *rec, *copy = NULL;

	for (rec = node_table; rec < &node_table[NODE_TABLE_SIZE]; ++rec)
	{
		if (addr64_equal( &node_id->ieee_addr_be, &rec->ieee_addr_be))
		{
			copy = rec;
			break;
		}
		if (copy == NULL && addr64_is_zero( &rec->ieee_addr_be))
		{
			copy = rec;
		}
	}

	if (copy != NULL)
	{
		*copy = *node_id;
	}

	return copy;
}

void node_table_dump( void)
{
	xbee_node_id_t *rec;

	for (rec = node_table; rec < &node_table[NODE_TABLE_SIZE]; ++rec)
	{
		if (! addr64_is_zero( &rec->ieee_addr_be))
		{
			printf( "%2u: ", (int)(rec - node_table));
			xbee_disc_node_id_dump( rec);
		}
	}
}

