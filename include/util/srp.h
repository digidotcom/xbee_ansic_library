/*
 * Secure Remote Password 6a implementation
 * Copyright (c) 2010 Tom Cocagne. All rights reserved.
 * https://github.com/cocagne/csrp
 *
 * Modified by Digi International to use Mbed TLS instead of OpenSSL.
 * Copyright (c) 2019 Digi International Inc.
 * All rights not expressly granted are reserved.
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013 Tom Cocagne
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

/*
 *
 * Purpose:       This is a direct implementation of the Secure Remote Password
 *                Protocol version 6a as described by
 *                http://srp.stanford.edu/design.html
 *
 * Author:        tom.cocagne@gmail.com (Tom Cocagne)
 *
 * Dependencies:  original: OpenSSL (and Advapi32.lib on Windows)
 *                modified: Mbed TLS (BIGNUM, CTR_DRBG, ENTROPY, SHA256)
 *
 * Usage:         Refer to test_srp.c for a demonstration
 *
 * Notes:
 *    This library allows multiple combinations of hashing algorithms and
 *    prime number constants. For authentication to succeed, the hash and
 *    prime number constants must match between
 *    srp_create_salted_verification_key(), srp_user_new(),
 *    and srp_verifier_new(). A recommended approach is to determine the
 *    desired level of security for an application and globally define the
 *    hash and prime number constants to the predetermined values.
 *
 *    As one might suspect, more bits means more security. As one might also
 *    suspect, more bits also means more processing time. The test_srp.c
 *    program can be easily modified to profile various combinations of
 *    hash & prime number pairings.
 */

#ifndef SRP_H
#define SRP_H

// Digi International's XBee products use the following SRP configuration.
#define SRP_GROUP_LEN       128
#define SRP_HASH_LEN        32

struct SRPVerifier;
struct SRPUser;

/* Out: bytes_s, len_s, bytes_v, len_v
 *
 * The caller is responsible for freeing the memory allocated for bytes_s and bytes_v
 */
int srp_create_salted_verification_key( const char * username,
                                         const unsigned char * password, int len_password,
                                         const unsigned char ** bytes_s, int * len_s,
                                         const unsigned char ** bytes_v, int * len_v );


/* Out: bytes_B, len_B.
 *
 * On failure, bytes_B will be set to NULL and len_B will be set to 0
 */
struct SRPVerifier *  srp_verifier_new( const char * username,
                                        const unsigned char * bytes_s, int len_s,
                                        const unsigned char * bytes_v, int len_v,
                                        const unsigned char * bytes_A, int len_A,
                                        const unsigned char ** bytes_B, int * len_B );


void                  srp_verifier_delete( struct SRPVerifier * ver );


int                   srp_verifier_is_authenticated( struct SRPVerifier * ver );


const char *          srp_verifier_get_username( struct SRPVerifier * ver );

/* key_length may be null */
const unsigned char * srp_verifier_get_session_key( struct SRPVerifier * ver, int * key_length );


int                   srp_verifier_get_session_key_length( struct SRPVerifier * ver );


/* user_M must be exactly srp_verifier_get_session_key_length() bytes in size */
void                  srp_verifier_verify_session( struct SRPVerifier * ver,
                                                   const unsigned char * user_M,
                                                   const unsigned char ** bytes_HAMK );

/*******************************************************************************/

struct SRPUser *      srp_user_new( const char * username,
                                    const unsigned char * bytes_password, int len_password );

void                  srp_user_delete( struct SRPUser * usr );

int                   srp_user_is_authenticated( struct SRPUser * usr);


const char *          srp_user_get_username( struct SRPUser * usr );

/* key_length may be null */
const unsigned char * srp_user_get_session_key( struct SRPUser * usr, int * key_length );

int                   srp_user_get_session_key_length( struct SRPUser * usr );

/* Output: username, bytes_A, len_A */
int                   srp_user_start_authentication( struct SRPUser * usr, const char ** username,
                                                     const unsigned char ** bytes_A, int * len_A );

/* Output: bytes_M, len_M  (len_M may be null and will always be
 *                          srp_user_get_session_key_length() bytes in size) */
int                   srp_user_process_challenge( struct SRPUser * usr,
                                                  const unsigned char * bytes_s, int len_s,
                                                  const unsigned char * bytes_B, int len_B,
                                                  const unsigned char ** bytes_M, int * len_M );

/* bytes_HAMK must be exactly srp_user_get_session_key_length() bytes in size */
void                  srp_user_verify_session( struct SRPUser * usr, const unsigned char * bytes_HAMK );

#endif /* Include Guard */
