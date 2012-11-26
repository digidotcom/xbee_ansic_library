/*
 * Copyright (c) 2012 Digi International Inc.,
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
	Displays information stored in the headers of XBee firmware images
	(.EBL files) used for XBee S2, S2B and S2C models.
 */
#include <errno.h>
#include <stdio.h>

#include <windows.h>
#include <conio.h>

#include "xbee/ebl_file.h"

int ebl_dump( const char *filename)
{
	FILE					*f;
	ebl_file_header_t	header;
	
	f = fopen( filename, "rb");
	if (f == NULL)
	{
		puts( "couldn't open file");
		return -EIO;
	}
	
	if (fread( &header, 1, sizeof header, f) != sizeof header)
	{
		puts( "read came up short");
	}
	else
	{
		ebl_header_dump( &header, EBL_HEADER_DUMP_EVERYTHING);
	}
	
	fclose( f);

	return 0;
}

// Call in main() before printing to stdout and store result to decide whether
// to pause before exit (launched from GUI via drag-and-drop) or not pause
// (launched via command-line).
// returns TRUE if program is in its own console (cursor at 0,0) or
// FALSE if it was launched from an existing console.
// See http://support.microsoft.com/kb/99115
int separate_console( void)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	if (!GetConsoleScreenBufferInfo( GetStdHandle( STD_OUTPUT_HANDLE), &csbi))
	{
		printf( "GetConsoleScreenBufferInfo failed: %lu\n", GetLastError());
		return FALSE;
	}

	// if cursor position is (0,0) then we were launched in a separate console
	return ((!csbi.dwCursorPosition.X) && (!csbi.dwCursorPosition.Y));
}

int main( int argc, char *argv[])
{
	int pause;
	int i;
	
	pause = separate_console();
	
	for (i = 1; i < argc; ++i)
	{
		printf( "\n%s:\n", argv[i]);
		ebl_dump( argv[i]);
	}
	
	if (pause)
	{
		puts( "\n\tPress any key to continue...");
		(void) getch();
	}
	
	return 0;
}
