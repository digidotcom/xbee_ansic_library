/*
 * Copyright (c) 2011-2019 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

/*
    Simple code for managing a table of xbee_node_id_t objects -- the
    XBee-specific node record used for ATND responses and Join Notifications.
*/

#ifndef _NODETABLE_H
#define _NODETABLE_H

#include "xbee/discovery.h"

#define NODE_TABLE_SIZE  10
extern xbee_node_id_t node_table[NODE_TABLE_SIZE];

xbee_node_id_t *node_by_addr( const addr64 FAR *ieee_be);
xbee_node_id_t *node_by_name( const char *name);
xbee_node_id_t *node_by_index( int idx);
int node_add( const xbee_node_id_t *node_id);
void node_table_dump( void);

// sample_cli handlers
void print_cli_help_nodetable(void);
void handle_nd_cmd(xbee_dev_t *xbee, char *command);
void handle_nodes_cmd(xbee_dev_t *xbee, char *command);
#define NODETABLE_CLI_ENTRIES \
    { "nd",             &handle_nd_cmd },       \
    { "nodes",          &handle_nodes_cmd },    \

#endif
