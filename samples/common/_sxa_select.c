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
	This file has some common routines for selecting SXA target devices.
   The code is used as an adjunct to the sample programs, and is not intended
   to be used in production applications.
*/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>

// Main header required for Simple XBee API
#include "xbee/sxa.h"
#include "_sxa_select.h"

/*
	Some global variables to keep track of current target.
*/
addr64 target_ieee;
int have_target = 0;
int ever_had_target = 0;
sxa_node_t FAR *sxa = NULL;

// Assumed to be in the main program
extern xbee_dev_t my_xbee;


/*
	Called whenever target device changed, so that the SXA pointer is
   kept aligned with it.
*/
void set_sxa(void)
{
	if (have_target)
   {
   	sxa = sxa_node_by_addr(&target_ieee);
	   if (!sxa)
	   {
	      printf("Cannot locate SXA, using local entry\n");
	      sxa = sxa_local_node(&my_xbee);
	   }
   }
   else
   {
   	sxa = sxa_local_node(&my_xbee);
   }
}


/*
	Parse '>' command to locate target device
*/
int set_target(const char *str, addr64 FAR *address)
{
	uint_fast8_t i;
	uint8_t FAR *b;
	int ret;
   sxa_node_t FAR *node;
   char buf[40];

	i = 8;			// bytes to convert
	if (str != NULL && address != NULL)
	{
		// skip over leading spaces
		while (isspace( (uint8_t) *str))
		{
			++str;
		}
		for (b = address->b; i; ++b, --i)
		{
			ret = hexstrtobyte( str);
			if (ret < 0)
			{
         	if (isdigit(*str))
            {
            	node = sxa_node_by_index( *str - '0');
               if (node)
               {
               	i = 0;
                  _f_memcpy(address, &node->id.ieee_addr_be, 8);
               }
               else
               {
               	printf("Table entry not found.  Select from:\n");
                  sxa_node_table_dump();
               }
            }
				break;
			}
			*b = (uint8_t)ret;
			str += 2;					// point past the encoded byte

			// skip over any separator, if present
			if (*str && ! isxdigit( (uint8_t) *str))
			{
				++str;
			}
		}
	}
   if (i == 4)
   {
   	for (i = 7; i >= 4; --i)
      {
      	address->b[i] = address->b[i-4];
      }
      address->b[0] = 0x00;
      address->b[1] = 0x13;
      address->b[2] = 0xA2;
      address->b[3] = 0x00;
      i = 0;
   }

	if (i != 0)
	{
   	printf("Target set to local device\n");
		return 0;
	}

   printf("Target now %" PRIsFAR "\n", addr64_format(buf, address));
	return 1;
}

void sxa_select_help(void)
{
   printf("Target setting for remote commands:\n");
   printf(" ?        (print list of remote and local targets)\n");
   printf(" > addr   (addr is hex ieee addr 8 or 16 digits,\n");
   printf("           high bytes assumed 0013a200 if 8 digits)\n");
   printf(" > N      (single digit: select node from table by index 0..9)\n");
   printf(" >        (reset to local device)\n");
   printf(" <        (reinstate previous remote target)\n");
}

int sxa_select_command( char *cmdstr)
{
   if ( cmdstr[0] == '>')
   {
      // New target device
      have_target = set_target( cmdstr+1, &target_ieee);
      if (have_target)
      {
         ever_had_target = 1;
      }
      set_sxa();
   }
   else if ( cmdstr[0] == '<')
   {
      // Reinstate previous target (useful in case of mistyped address)
      if (ever_had_target)
      {
         have_target = 1;
         printf("Reinstating %" PRIsFAR "\n",
         	addr64_format(cmdstr, &target_ieee));
      }
      else
      {
         printf("Nothing to reinstate\n");
      }
      set_sxa();
   }
   else if ( cmdstr[0] == '?')
   {
      // List discovered targets (nodes)
      sxa_node_table_dump();
   }
   else
   {
      return 0;   // not an SXA select command
   }
   return 1;	// Command was processed
}


