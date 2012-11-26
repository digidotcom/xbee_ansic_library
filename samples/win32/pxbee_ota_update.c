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
	Usage: pxbee_ota_update.exe [COMx] [115200] [--profile=0xABCD] [--aps]

	Optional Parameters:
		COMx: COM port to use (e.g., COM10); no default -- program will prompt
					for a COM port if none was specified.
		115200: baud rate to use; defaults to 115200.
		--profile=0xABCD:	Profile ID to use for discovering targets.
		--aps: Use APS encryption for sending update.
*/

// version information
#define BUILD "20110525"

// Requires Win2K or newer for GetConsoleWindow() function
#define WINVER 0x0500

#include <windows.h>
#include <wincon.h>
#include <commdlg.h>
#include <stdio.h>

#include "xbee/platform.h"
#include "zigbee/zcl_bacnet.h"
#include "zigbee/zdo.h"
#include "xbee/device.h"
#include "xbee/wpan.h"
#include "xbee/xmodem.h"

#include "../common/_pxbee_ota_update.h"
#include "../common/_atinter.h"
#include "parse_serial_args.h"

supported_profile_t dynamic_profile = { 0, WPAN_CLUST_FLAG_NONE };
const supported_profile_t *current_profile = &dynamic_profile;

// Function used by Xmodem state machine to read source of OTA update.
int fw_read( void FAR *context, void FAR *buffer, int16_t bytes)
{
	return fread( buffer, 1, bytes, (FILE FAR *)context);
}

// hook for GetOpenFileName to move window to foreground
UINT APIENTRY OFNHookProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndDlg, hwndOwner;
	RECT rc, rcDlg, rcOwner;

	// This function follows the standard API for an OPENFILENAME lpfnHook,
	// but we aren't using all of its parameters.  Since we set the OFN_EXPLORER
	// flag, this is an OFNHookProc.
	XBEE_UNUSED_PARAMETER( wParam);
	XBEE_UNUSED_PARAMETER( lParam);

	// Code to center window over console based on code from
	// http://msdn.microsoft.com/en-us/library/ms644996(v=VS.85).aspx#init_box
	if (msg == WM_INITDIALOG)
	{
		hwndDlg = GetParent(h);
		hwndOwner = GetConsoleWindow();

		GetWindowRect(hwndOwner, &rcOwner);
		GetWindowRect(hwndDlg, &rcDlg);
		CopyRect(&rc, &rcOwner);

		// Offset the owner and dialog box rectangles so that right and bottom
		// values represent the width and height, and then offset the owner again
		// to discard space taken up by the dialog box.

		OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
		OffsetRect(&rc, -rc.left, -rc.top);
		OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

		// The new position is the sum of half the remaining space and the owner's
		// original position.

		SetWindowPos(hwndDlg,
							HWND_TOPMOST,
							rcOwner.left + (rc.right / 2),
							rcOwner.top + (rc.bottom / 2),
							0, 0,          // Ignores size arguments.
							SWP_NOSIZE | SWP_SHOWWINDOW);
		return TRUE;
	}

	return FALSE;
}

// Prompt user to select a .abs.bin file.
// Returns file selected or NULL on Cancel/error.
char *get_file()
{
	OPENFILENAME ofn;
	static char file[256] = "";
	FILE *f;
	char appname[60];
	uint32_t appname_offset;
	int result;

	printf( "Select firmware image (*.abs.bin) from file dialog box.\n");
	ZeroMemory( &ofn, sizeof ofn);
	ofn.lStructSize = sizeof ofn;
	ofn.nMaxFile = sizeof file;
	ofn.lpstrFile = file;
	ofn.lpstrFilter = "Firmware Images (*.abs.bin)\0*.abs.bin\0"
							"All Files (*.*)\0*.*\0";
	ofn.lpstrTitle = "Select Firmware Image";
	ofn.lpfnHook = OFNHookProc;
	ofn.Flags = OFN_FILEMUSTEXIST
				| OFN_HIDEREADONLY
				| OFN_ENABLEHOOK
				| OFN_EXPLORER;

	result = GetOpenFileName( &ofn);

	if (! result)
	{
		return NULL;
	}

	printf( "Firmware set to\n  %s\n", file);

	#define PXBEE_BASE_ADDR 0x8400
	f = fopen( file, "rb");
	if (f)
	{
		if (! fseek( f, 0xF1BC - PXBEE_BASE_ADDR, SEEK_SET))
		{
			fread( &appname_offset, 1, 4, f);
			appname_offset = be32toh( appname_offset) - PXBEE_BASE_ADDR;
			if (appname_offset < 0xF200 - PXBEE_BASE_ADDR)
			{
				if (! fseek( f, appname_offset, SEEK_SET))
				{
					fread( &appname, 1, sizeof appname, f);
					printf( "App Name: %-60s\n", appname);
				}
			}
		}
		fclose( f);
	}

	return file;
}

void print_help( void)
{
	puts( "PXBee OTA Update Help:");
	puts( "\tfirmware       - select firmware image to send");
	puts( "\tprofile 0xXXXX - set the profile ID for discovery");
	puts( "\tfind           - find potential targets with PXBee OTA cluster");
	puts( "\ttarget         - show list of known targets");
	puts( "\ttarget <index> - set target to entry <index> from target list");
	puts( "\taps            - toggle APS encryption for update");
	puts( "\tgo             - start sending firmware");
	puts( "\tpassword <pw>  - set password for starting update");
	puts( "\tpassword       - clear password (use default of one 0x00 byte)");
#ifdef XBEE_XMODEM_TESTING
	puts( "");
	puts( "\tACK            - drop next ACK from target");
	puts( "\tFRAME          - drop next frame instead of sending to target");
	puts( "\tCRC            - corrupt CRC of next frame sent to target");
#endif
	puts( "");
	puts( "\tquit           - quit program");
}

xbee_dev_t my_xbee;

void profile_changed( void)
{
	// set Profile ID for our Basic Client Cluster endpoint
	sample_endpoints.zcl.profile_id = current_profile->profile_id;
	digi_data_clusters[0].flags |= current_profile->flags;

	// set OTA flag for encryption
	if (current_profile->flags & WPAN_CLUST_FLAG_ENCRYPT)
	{
		xbee_ota.flags = XBEE_OTA_FLAG_APS_ENCRYPT;
	}
}

void set_password( const char *pw)
{
	xbee_ota.auth_length = (pw != NULL) ? strlen( pw) : 0;

	if (xbee_ota.auth_length > 0)
	{
		if (xbee_ota.auth_length > XBEE_OTA_MAX_AUTH_LENGTH)
		{
			xbee_ota.auth_length = XBEE_OTA_MAX_AUTH_LENGTH;
		}
		memcpy( xbee_ota.auth_data, pw, xbee_ota.auth_length);
	}
}

void parse_args( int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; ++i)
	{
		if (strncmp( argv[i], "--profile=", 10) == 0)
		{
			dynamic_profile.profile_id =
				(uint16_t) strtoul( &argv[i][10], NULL, 16);
		}
		else if (strcmp( argv[i], "--aps") == 0)
		{
			dynamic_profile.flags = WPAN_CLUST_FLAG_ENCRYPT;
		}
		else if (strncmp( argv[i], "--password=", 11) == 0)
		{
			set_password( &argv[i][11]);
		}
	}
	profile_changed();
}

int main( int argc, char *argv[])
{
	const char *firmware = NULL;
	char xmodem_buffer[69];
   char cmdstr[80];
	int status, i;
	xbee_serial_t XBEE_SERPORT;
	FILE *fw_file = NULL;
	#ifdef VERBOSE
		uint16_t last_state;
	#endif
	uint16_t last_packet;
	target_t *target = NULL;

	// turn off buffering so status changes (lines ending in \r) display
	setvbuf( stdout, NULL, _IONBF, 0);

	memset( target_list, 0, sizeof target_list);

	// set serial port
	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	// parse args for this program
	parse_args( argc, argv);

	// initialize the serial and device layer for this XBee device
	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, NULL))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	// Initialize the WPAN layer of the XBee device driver.  This layer enables
	// endpoints and clusters, and is required for all ZigBee layers.
	xbee_wpan_init( &my_xbee, &sample_endpoints.zdo);

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

   print_help();

	if (dynamic_profile.profile_id != 0)
	{
		printf( "Using profile ID 0x%04x with%s APS encryption.\n",
			dynamic_profile.profile_id,
			(dynamic_profile.flags & WPAN_CLUST_FLAG_ENCRYPT) ? "" : "out");
		xbee_ota_find_devices( &my_xbee.wpan_dev, xbee_found, NULL);
	}

	while (1)
   {
      while (xbee_readline( cmdstr, sizeof cmdstr) == -EAGAIN)
      {
      	wpan_tick( &my_xbee.wpan_dev);

			if (fw_file != NULL)
			{
				if (xbee_xmodem_tx_tick( &xbee_ota.xbxm) != 0)
				{
					uint16_t timer;

					printf( "upload complete     \n");
					fclose( fw_file);
					fw_file = NULL;

					// wait one second for device to reboot then rediscover it
					timer = XBEE_SET_TIMEOUT_MS(1000);
					while (! XBEE_CHECK_TIMEOUT_MS( timer));

					xbee_ota_find_devices( &my_xbee.wpan_dev, xbee_found, NULL);
				}

				#ifdef VERBOSE
					if (last_state != xbee_ota.xbxm.state)
					{
						printf( "state change from %u to %u\n", last_state,
							xbee_ota.xbxm.state);
						last_state = xbee_ota.xbxm.state;
					}
				#endif
				if (last_packet != xbee_ota.xbxm.packet_num)
				{
					#ifdef VERBOSE
						printf( "packet #%u\n", xbee_ota.xbxm.packet_num);
					#else
						printf( " %" PRIu32 " bytes\r",
							UINT32_C(64) * xbee_ota.xbxm.packet_num);
					#endif
					last_packet = xbee_ota.xbxm.packet_num;
				}
			}
      }

		if (! strcmpi( cmdstr, "quit"))
      {
			return 0;
		}
		else if (! strcmpi( cmdstr, "help") || ! strcmp( cmdstr, "?"))
		{
			print_help();
		}
		else if (! strcmpi( cmdstr, "find"))
		{
			if (dynamic_profile.profile_id == 0)
			{
				puts( "Error: specify a profile via cmd line or 'profile' cmd");
			}
			else
			{
				xbee_ota_find_devices( &my_xbee.wpan_dev, xbee_found, NULL);
			}
		}
		else if (! strncmpi( cmdstr, "profile ", 8))
		{
			dynamic_profile.profile_id = strtoul( &cmdstr[8], NULL, 16);
			printf( "Profile ID set to 0x%04x\n", dynamic_profile.profile_id);
			profile_changed();
		}
		else if (! strcmpi( cmdstr, "target"))
		{
			puts( " #: --IEEE Address--- Ver. --------Application Name--------"
				" ---Date Code----");
			for (i = 0; i < target_index; ++i)
			{
				print_target( i);
			}
			puts( "End of List");
		}
		else if (! strncmpi( cmdstr, "target ", 7))
		{
			i = (int) strtoul( &cmdstr[7], NULL, 10);
			if (target_index == 0)
			{
				printf( "error, no targets in list, starting search now...\n");
				xbee_ota_find_devices( &my_xbee.wpan_dev, xbee_found, NULL);
			}
			else if (i < 0 || i >= target_index)
			{
				printf( "error, index %d is invalid (must be 0 to %u)\n", i,
					target_index - 1);
			}
			else
			{
				target = &target_list[i];
				puts( "set target to:");
				print_target( i);
			}
		}
		else if (! strcmpi( cmdstr, "F") && target != NULL)
		{
			// If the target is stuck in the bootloader, send an 'F' to start
			// a firmware update.
			wpan_envelope_t envelope;

			wpan_envelope_create( &envelope, &my_xbee.wpan_dev, &target->ieee,
																WPAN_NET_ADDR_UNDEFINED);
			envelope.options = current_profile->flags;
			xbee_transparent_serial_str( &envelope, "F");
		}
		else if (! strcmpi( cmdstr, "firmware"))
		{
			firmware = get_file();
		}
		else if (! strcmpi( cmdstr, "password"))
		{
			set_password( "");
			puts( "cleared password (will use default of a single null byte)");
		}
		else if (! strncmpi( cmdstr, "password ", 9))
		{
			set_password( &cmdstr[9]);
			printf( "set password to [%.*s]\n", xbee_ota.auth_length,
				xbee_ota.auth_data);
		}
		else if (! strcmpi( cmdstr, "aps"))
		{
			xbee_ota.flags ^= XBEE_OTA_FLAG_APS_ENCRYPT;
			printf( "APS encryption %sabled\n",
				(xbee_ota.flags & XBEE_OTA_FLAG_APS_ENCRYPT) ? "en" : "dis");
		}
		else if (! strcmpi( cmdstr, "go"))
		{
			if (target == NULL)
			{
				if (target_index > 0)
				{
					target = &target_list[0];
				}
				else
				{
					puts( "no targets available to send to");
					continue;
				}
			}

			if (firmware == NULL)
			{
				firmware = get_file();
				if (firmware == NULL)
				{
					printf( "Canceled.\n");
					continue;
				}
			}

			printf( "Starting xmodem upload of\n  %s\n", firmware);

			fw_file = fopen( firmware, "rb");
			if (! fw_file)
			{
				printf( "Failed to open '%s'\n", firmware);
				exit( -1);
			}

			status = xbee_ota_init( &xbee_ota, &my_xbee.wpan_dev, &target->ieee);

			if (status)
			{
				printf( "%s returned %d\n", "xbee_ota_init", status);
				continue;
			}

			status = xbee_xmodem_set_source( &xbee_ota.xbxm, xmodem_buffer,
																		fw_read, fw_file);
			if (status)
			{
				printf( "%s returned %d\n", "xbee_xmodem_set_source", status);
				continue;
			}

			// reset the xbee_xmodem_state_t state machine, keeping existing flags
			status = xbee_xmodem_tx_init( &xbee_ota.xbxm, xbee_ota.xbxm.flags);
			if (status)
			{
				printf( "%s returned %d\n", "xbee_xmodem_tx_init", status);
				continue;
			}

			// reset copies of basic cluster -- need to refresh after update
			memset( &target->basic, 0, sizeof(target->basic));

			#ifdef VERBOSE
				last_state = last_packet = 0;
			#endif

			// main loop will tick the xmodem transfre until fw_file == NULL
		}
#ifdef XBEE_XMODEM_TESTING
		else if (! strcmpi( cmdstr, "ACK"))
		{
			xbee_ota.xbxm.flags |= XBEE_XMODEM_FLAG_DROP_ACK;
		}
		else if (! strcmpi( cmdstr, "FRAME"))
		{
			xbee_ota.xbxm.flags |= XBEE_XMODEM_FLAG_DROP_FRAME;
		}
		else if (! strcmpi( cmdstr, "CRC"))
		{
			xbee_ota.xbxm.flags |= XBEE_XMODEM_FLAG_BAD_CRC;
		}
#endif
		else if (! strncmpi( cmdstr, "AT", 2))
		{
			process_command( &my_xbee, &cmdstr[2]);
		}
	   else
	   {
	   	printf( "unknown command: '%s'\n", cmdstr);
	   }
   }
}

