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

/**
    @addtogroup hal_win32
    @{
    @file xbee_platform_win32.c
    Platform-specific functions for use by the XBee Driver on Win32/gcc target.
*/

#include "xbee/platform.h"
#include <time.h>

uint32_t xbee_seconds_timer()
{
    return time(NULL);
}

uint32_t xbee_millisecond_timer()
{
    return GetTickCount();
}

///@}
