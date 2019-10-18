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

/*
    Implementation of xbee_random() API using mbedtls CTR-DRBG module.
*/

#include <string.h>

#include "xbee/platform.h"
#include "xbee/random.h"

#include "mbedtls/config.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"

static mbedtls_ctr_drbg_context ctr_drbg;
static mbedtls_entropy_context entropy;

static int xbee_random_init(const char *seed)
{
    int ret;

    mbedtls_entropy_init(&entropy);
    mbedtls_ctr_drbg_init(&ctr_drbg);

    ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                (const uint8_t *)seed, strlen(seed));

    if (ret != 0) {
        // TODO: Consider mapping some MBEDTLS_ERR_xxx values to platform's
        // error codes.  For now use a generic error.
        return -EIO;
    }

    return 0;
}

int xbee_random(void *output, size_t output_len)
{
    static bool_t needs_init = TRUE;
    int ret;

    if (needs_init) {
        ret = xbee_random_init("xbee_ansic_library");
        if (ret) {
            return ret;
        }
        needs_init = FALSE;
    }

    ret = mbedtls_ctr_drbg_random(&ctr_drbg, output, output_len);

    if (ret != 0) {
        // TODO: Consider mapping some MBEDTLS_ERR_xxx values to platform's
        // error codes.  For now use a generic error.
        return -EIO;
    }

    return 0;
}
