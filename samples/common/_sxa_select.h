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

#ifndef _SXA_SELECT_H_INCL
#define _SXA_SELECT_H_INCL

extern addr64 target_ieee;
extern int have_target;
extern int ever_had_target;
extern sxa_node_t FAR *sxa;

void set_sxa(void);
int set_target(const char *str, addr64 FAR *address);
void sxa_select_help(void);
int sxa_select_command( char *cmdstr);


#endif	// _SXA_SELECT_H_INCL

