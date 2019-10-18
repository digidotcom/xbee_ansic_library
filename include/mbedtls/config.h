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
    Stripped down mbedtls configuration file for the AES, SHA256 and MPI
    libraries we're using from the Mbed TLS project.

    srp.c requires BIGNUM, CTR_DRBG, ENTROPY and SHA256
    XBee3 "Secure Session" feature requires srp.c and AES
*/

#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

#include <stddef.h>
#include <stdint.h>

#define MBEDTLS_PLATFORM_C

#define MBEDTLS_AES_C
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_SHA256_C

// Save RAM by adjusting to our exact needs
#define MBEDTLS_MPI_MAX_SIZE            32 // 384 bits is 48 bytes

// Reduce stack usage in mbedtls_mpi_exp_mod() by reducing the MPI window size.
#define MBEDTLS_MPI_WINDOW_SIZE         3

#include "mbedtls/check_config.h"

#endif // MBEDTLS_CONFIG_H
