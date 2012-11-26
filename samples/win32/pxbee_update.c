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
	Install .abs.bin file on the Freescale HCS08 processor of a Programmable
	XBee module.  Based on "install_ebl.c", which installs a .ebl file on a
	non-programmable XBee module.
*/

// Requires Win2K or newer for GetConsoleWindow() function
#define WINVER 0x0500

#include <windows.h>
#include <wincon.h>
#include <commdlg.h>
#include <stdio.h>
#include <stdlib.h>

#include "xbee/device.h"
#include "xbee/firmware.h"
#include "xbee/byteorder.h"

#include "xbee/atcmd.h"			// for XBEE_FRAME_HANDLE_LOCAL_AT
const xbee_dispatch_table_entry_t xbee_frame_handlers[] =
{
	XBEE_FRAME_HANDLE_LOCAL_AT,
	XBEE_FRAME_TABLE_END
};

#include "parse_serial_args.h"

/*
	Sample code to read firmware from a firmware file, used to demonstrate
	new non-blocking firmware update API.

	Note that installing a .abs.bin file doesn't actually need a seek function.
	Maybe that should just be removed (set to NULL)?  We may need it in the
	future, so leave it in?  There are benefits to not having it though -- can
	install updates straight from an HTTP download.
*/

xbee_dev_t my_xbee;

int fw_seek( void FAR *context, uint32_t offset)
{
	return fseek( (FILE FAR *)context, offset, SEEK_SET);
}

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

void myxbee_reset( xbee_dev_t *xbee, int reset)
{
	static int state = 0;			// assume released
	char buffer[10];

	// standard API for an XBee reset handler, but there's only one XBee
	// in this sample, so no need to use 'xbee' parameter
	XBEE_UNUSED_PARAMETER( xbee);

	if (state != reset)
	{
		printf( "%s the XBee's reset button, press ENTER.\n",
			reset ? "Press and hold" : "Release");
		fgets( buffer, 10, stdin);

		state = reset;
	}
}

int main( int argc, char *argv[])
{
	xbee_fw_source_t fw = { 0 };
	char *firmware;
	FILE *binfile = NULL;
	char buffer[80];
	uint16_t t;
	int result;
	unsigned int last_state;
	xbee_serial_t XBEE_SERPORT;

	parse_serial_arguments( argc, argv, &XBEE_SERPORT);

	if (argc > 1 && memcmp( argv[argc - 1], "COM", 3) != 0)
	{
		binfile = fopen( argv[argc - 1], "rb");
	}
	if (! binfile)
	{
		firmware = get_file();
		if (firmware == NULL)
		{
			// user canceled file/open dialog
			exit( 0);
		}
		binfile = fopen( firmware, "rb");
		if (! binfile)
		{
			printf( "Error: couldn't open %s\n", firmware);
			exit( -1);
		}
	}

	fw.context = binfile;
	fw.seek = fw_seek;
	fw.read = fw_read;

	if (xbee_dev_init( &my_xbee, &XBEE_SERPORT, NULL, myxbee_reset))
	{
		printf( "Failed to initialize device.\n");
		return 0;
	}

	xbee_fw_install_init( &my_xbee, NULL, &fw);
	do
	{
		t = xbee_millisecond_timer();
		last_state = xbee_fw_install_hcs08_state( &fw);
		result = xbee_fw_install_hcs08_tick( &fw);
		t = xbee_millisecond_timer() - t;
#ifdef BLOCKING_WARNING
		if (t > 50)
		{
			printf( "!!! blocked for %ums in state 0x%04X (now state 0x%04X)\n",
				t, last_state, xbee_fw_install_hcs08_state( &fw));
		}
#endif
		if (last_state != xbee_fw_install_hcs08_state( &fw))
		{
			// print new status
			printf( " %" PRIsFAR "                          \r",
				xbee_fw_status_hcs08( &fw, buffer));
			fflush( stdout);
		}
	} while (result == 0);
	if (result == 1)
	{
		printf( "firmware update completed successfully\n");
	}
	else
	{
		printf( "firmware update failed with error %d\n", result);
	}

	fclose( binfile);

	xbee_ser_close( &my_xbee.serport);

	return 0;
}
