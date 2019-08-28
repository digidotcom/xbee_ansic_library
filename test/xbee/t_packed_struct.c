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

// Test struct packing macro on structures with embedded structures.

#include <stdio.h>
#include <stdlib.h>

#include "zigbee/zcl.h"

#define FAIL_IF(cond)  if (cond) { printf("FAIL: %s\n", #cond); ++failures; }

int main(int argc, char *argv[])
{
    int failures = 0;

    FAIL_IF(sizeof(zcl_header_common_t) != 3);
    FAIL_IF(sizeof(zcl_header_t) != 6);
    FAIL_IF(offsetof(zcl_header_t, type.std.common) != 1);
    FAIL_IF(offsetof(zcl_header_t, type.mfg.common) != 3);

    FAIL_IF(sizeof(zcl_header_response_t) != 5);
    FAIL_IF(offsetof(zcl_header_response_t, u.mfg.mfg_code_le) != 1);
    FAIL_IF(offsetof(zcl_header_response_t, sequence) != 3);

    if (failures) {
        return EXIT_FAILURE;
    }

    printf("PASSED\n");
    return EXIT_SUCCESS;
}
