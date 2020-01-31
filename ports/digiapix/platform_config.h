/*
 * Copyright (c) 2019 Digi International Inc.,
 * All rights not expressly granted are reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Digi International Inc., 9350 Excelsior Blvd., Suite 700, Hopkins, MN 55343
 * ===========================================================================
 */

// digiapix platforms use a slightly modified version of the POSIX platform
#include "../posix/platform_config.h"

/**
    @addtogroup hal_digiapix
    @sa hal_posix
    @{ @file 
*/

int digiapix_platform_init(void);
#define XBEE_PLATFORM_INIT()    digiapix_platform_init()

struct xbee_dev_t;
void digiapix_xbee_reset(struct xbee_dev_t *xbee, bool_t asserted);
int digiapix_xbee_is_awake(struct xbee_dev_t *xbee);

#define XBEE_RESET_FN           &digiapix_xbee_reset
#define XBEE_IS_AWAKE_FN        &digiapix_xbee_is_awake

///@}
