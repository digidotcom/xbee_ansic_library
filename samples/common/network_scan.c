/*
 * Copyright (c) 2010-2013 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc. 11001 Bren Road East, Minnetonka, MN 55343
 * =======================================================================
 */

#include <stdio.h>

#include "../../samples/common/parse_serial_args.h"
#include "../../samples/common/_atinter.h"
#include "xbee/platform.h"
#include "xbee/byteorder.h"
#include "xbee/device.h"
#include "xbee/atcmd.h"
#include "xbee/scan.h"
#include "wpan/types.h"

xbee_dev_t my_xbee;

const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
   XBEE_FRAME_HANDLE_LOCAL_AT,
   { XBEE_FRAME_LOCAL_AT_RESPONSE, 0, xbee_scan_dump_response, NULL },
   XBEE_FRAME_TABLE_END
};

void print_menu( void)
{
   puts( "help                 This list of options.");
   puts( "scan                 Initiate active scan.");
   puts( "quit                 Quit the program.");
   puts( "");
}

void active_scan( void)
{
   puts( "Initiating active scan...");
   xbee_cmd_execute( &my_xbee, "AS", NULL, 0);
}

/*
   main

   Initiate communication with the XBee module, then accept AT commands from
   STDIO, pass them to the XBee module and print the result.
*/
int main( int argc, char *argv[])
{
   char cmdstr[80];
   int status;
   xbee_serial_t XBEE_SERPORT = { 115200, 0 };

   parse_serial_arguments( argc, argv, &XBEE_SERPORT);

   // initialize the serial and device layer for this XBee device
   if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
   {
      printf( "Failed to initialize device.\n");
      return 0;
   }

   // Initialize the AT Command layer for this XBee device and have the
   // driver query it for basic information (hardware version, firmware version,
   // serial number, IEEE address, etc.)
   xbee_cmd_init_device( &my_xbee);
   printf( "Waiting for driver to query the XBee device...\n");
   do {
      xbee_dev_tick( &my_xbee);
      status = xbee_cmd_query_status( &my_xbee);
   } while (status == -EBUSY);
   if (status)
   {
      printf( "Error %d waiting for query to complete.\n", status);
   }

   // report on the settings
   xbee_dev_dump_settings( &my_xbee, XBEE_DEV_DUMP_FLAG_DEFAULT);

   // Take responsibility for responding to certain ZDO requests.
   // This is necessary for other devices to discover our endpoints.
   xbee_cmd_simple( &my_xbee, "AO", 3);

   print_menu();
   active_scan();

   while (1)
   {
      int linelen;
      do {
         linelen = xbee_readline(cmdstr, sizeof cmdstr);
         status = xbee_dev_tick(&my_xbee);
         if (status < 0) {
            printf("Error %d from xbee_dev_tick().\n", status);
            return -1;
         }
      } while (linelen == -EAGAIN);

      if (linelen == -ENODATA || ! strcmpi( cmdstr, "quit"))
      {
         return 0;
      }
      else if (! strcmpi( cmdstr, "help") || ! strcmp( cmdstr, "?"))
      {
         print_menu();
      }
      else if (! strcmpi( cmdstr, "scan"))
      {
         active_scan();
      }
      else if (! strncmpi( cmdstr, "AT", 2))
      {
         process_command( &my_xbee, &cmdstr[2]);
      }
      else
      {
         printf( "unknown command: %s\n", cmdstr);
      }
   }
}
