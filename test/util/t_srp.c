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

// Test util/srp.c to ensure that the client and server can initiate a session
// with each other.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xbee/platform.h"
#include "xbee/random.h"
#include "util/srp.h"

#include "../unittest.h"

#define TEST_USERNAME "xbee"
#define TEST_BYTES_PW "passw0rd"
#define TEST_LEN_PW strlen(TEST_BYTES_PW)

uint8_t SecureSalt[] = { 0x20, 0xf0, 0x34, 0x90 };
uint8_t SecureVerifier[] = {
    0x17, 0xdb, 0xda, 0x64, 0xa7, 0x63, 0xaa, 0x28, 0x7f, 0x01, 0x28, 0x3f, 0x4c, 0x56, 0xb2, 0xa3,
    0xc7, 0x51, 0x71, 0x18, 0x72, 0x1b, 0xf4, 0x03, 0xec, 0x57, 0x69, 0xbf, 0x3e, 0x5c, 0x00, 0x11,
    0xe8, 0x9b, 0xe4, 0x7b, 0x04, 0x56, 0xeb, 0x55, 0xe4, 0xf5, 0x3b, 0xda, 0xc2, 0xc3, 0xe2, 0xf4,
    0x12, 0x9a, 0x63, 0x38, 0x6e, 0x21, 0x03, 0x6d, 0x03, 0x10, 0x89, 0x18, 0x7b, 0xe0, 0xfe, 0x41,
    0x89, 0xab, 0xa6, 0x55, 0x41, 0xa8, 0xe7, 0x34, 0x17, 0x23, 0x33, 0xeb, 0x70, 0x39, 0x2a, 0xf6,
    0x21, 0x88, 0x57, 0x29, 0x38, 0x25, 0xd4, 0x63, 0x2c, 0xf1, 0x83, 0x51, 0xdb, 0x8f, 0xa0, 0xe1,
    0x76, 0x65, 0x8d, 0xe4, 0xc0, 0xa2, 0x60, 0xa0, 0x24, 0xbe, 0x03, 0xb3, 0x83, 0x96, 0x2d, 0x06,
    0xdf, 0xff, 0x2d, 0xd1, 0xef, 0xab, 0x5b, 0x57, 0x9c, 0x06, 0x19, 0xa2, 0xfd, 0x43, 0x38, 0x81,
};

#define CHECK_SRP_ERR(e, fn) \
    if (e < 0) { printf("Error %d calling %s\n", e, fn); \
                 goto test_abort; }

void t_srp(void)
{
    struct SRPVerifier          *ver = NULL;
    struct SRPUser              *usr = NULL;
    const char                  *username;
    const uint8_t               *bytes_A, *bytes_B, *bytes_M, *bytes_HAMK;
    int                         len_A, len_B, len_M;

    const uint8_t               *usr_key, *ver_key;
    int                         usr_keylen, ver_keylen;

    usr = srp_user_new(TEST_USERNAME,
                       (const uint8_t *)TEST_BYTES_PW,
                       TEST_LEN_PW);

    if (usr == NULL) {
        test_fail("usr: failed to create new SRPUser");
        goto test_abort;
    }

    srp_user_start_authentication(usr, &username, &bytes_A, &len_A);

    if (bytes_A == NULL) {
        test_fail("usr: failed to generate A");
        goto test_abort;
    }

    test_compare(len_A, SRP_GROUP_LEN, NULL, "len_A incorrect");

    ver = srp_verifier_new(TEST_USERNAME,
        SecureSalt, sizeof(SecureSalt),
        SecureVerifier, sizeof(SecureVerifier),
        bytes_A, len_A,
        &bytes_B, &len_B);

    if (ver == NULL) {
        test_fail("ver: failed to create new SRPVerifier");
        goto test_abort;
    }

    if (bytes_B == NULL) {
        test_fail("ver: failed to generate B");
        goto test_abort;
    }

    test_compare(len_B, SRP_GROUP_LEN, NULL, "len_B incorrect");

    srp_user_process_challenge(usr,
                               SecureSalt, sizeof(SecureSalt),
                               bytes_B, len_B,
                               &bytes_M, &len_M);

    if (bytes_M == NULL) {
        test_fail("usr: failed to generate M");
        goto test_abort;
    }

    test_compare(len_M, SRP_HASH_LEN, NULL, "len_M incorrect");

    srp_verifier_verify_session(ver, bytes_M, &bytes_HAMK);

    if (test_bool(srp_verifier_is_authenticated(ver),
                  "ver: authentication failed"))
    {
        goto test_abort;
    }
    srp_user_verify_session(usr, bytes_HAMK);

    if (test_bool(srp_user_is_authenticated(usr),
                  "usr: authentication failed"))
    {
        goto test_abort;
    }

    usr_key = srp_user_get_session_key(usr, &usr_keylen);
    ver_key = srp_verifier_get_session_key(ver, &ver_keylen);

    test_compare(usr_keylen, SRP_HASH_LEN, NULL, "usr_keylen incorrect");
    test_compare(ver_keylen, SRP_HASH_LEN, NULL, "ver_keylen incorrect");
    if (usr_keylen == ver_keylen) {
        test_compare(memcmp(usr_key, ver_key, usr_keylen), 0, NULL,
                     "keys do not match");
    }

    goto clean_exit;

test_abort:
    test_fail("aborting test");

clean_exit:
    srp_user_delete(usr);
    srp_verifier_delete(ver);
}


int main(int argc, char *argv[])
{
    int failures = 0;

    failures += DO_TEST(t_srp);

    return test_exit(failures);
}