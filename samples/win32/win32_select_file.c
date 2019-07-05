/*
 * Copyright (c) 2010-2019 Digi International Inc.,
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
    Use Win32 GetOpenFileName() to prompt user to select a file.
*/

// Requires Win2K or newer for GetConsoleWindow() function
#define WINVER 0x0500

#include <windows.h>
#include <wincon.h>
#include <commdlg.h>

// hook for GetOpenFileName to move window to foreground
UINT APIENTRY OFNHookProc(HWND h, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndDlg, hwndOwner;
    RECT rc, rcDlg, rcOwner;

    // This function follows the standard API for an OPENFILENAME lpfnHook,
    // but we aren't using all of its parameters.  Since we set the OFN_EXPLORER
    // flag, this is an OFNHookProc.
    (void)wParam;
    (void)lParam;

    // Code to center window over console based on code from
    // http://msdn.microsoft.com/en-us/library/ms644996(v=VS.85).aspx#init_box
    if (msg == WM_INITDIALOG) {
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

// Prompt user to select a file.
// Returns file selected or NULL on Cancel/error.
char *win32_select_file(const char *title, const char *filter)
{
    OPENFILENAME ofn;
    static char file[256] = "";

    ZeroMemory(&ofn, sizeof ofn);
    ofn.lStructSize = sizeof ofn;
    ofn.nMaxFile = sizeof file;
    ofn.lpstrFile = file;
    ofn.lpstrFilter = filter;
    ofn.lpstrTitle = title;
    ofn.lpfnHook = OFNHookProc;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ENABLEHOOK | OFN_EXPLORER;

    return GetOpenFileName(&ofn) != 0 ? file : NULL;
}


