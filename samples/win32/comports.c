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
    List all COM ports available via the efficient QueryDosDevice() method.
*/

#include <stdio.h>
#include <windows.h>
#include <winbase.h>

#define MAX_COMPORT 4096

char path[5000];
int main(int argc, char *argv[])
{
    printf("Checking COM1 through COM%d:\n", MAX_COMPORT);
    for (unsigned i = 1; i <= MAX_COMPORT; ++i) {
        char buffer[10];
        sprintf(buffer, "COM%u", i);
        DWORD result = QueryDosDevice(buffer, path, sizeof path);
        if (result != 0) {
            printf("%s:\t%s\n", buffer, path);
        } else if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            printf("%s:\t%s\n", buffer, "(error, path buffer too small)");
        }
    }

    return 0;
}
