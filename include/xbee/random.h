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

#include <stdlib.h>
#include "xbee/platform.h"

/**
    @brief
    Generate a sequence of random bytes.

    @param[out] output          Location to store random bytes.
    @param[in]  output_len      Number of bytes to generate.

    @retval     0       Success
    @retval     <0      Failure

    @sa xbee_random_init
*/
int xbee_random(void *output, size_t output_len);
