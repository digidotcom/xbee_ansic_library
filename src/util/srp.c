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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "xbee/random.h"
#include "mbedtls/sha256.h"
#include "mbedtls/bignum.h"
#include "util/srp.h"

#ifdef DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif


#if DEBUG
static void phex(char const *id, uint8_t const *ptr, int len)
{
    PRINTF("%s: \n", id);
    for (int i = 0 ; i < (len) ; i++) {
        if (i % 32 == 0) {
            PRINTF("\n");
        }
        PRINTF("%02x", (ptr)[i]);
    }
    PRINTF("\n");
}
#endif

struct NGHex
{
    const char * n_hex;
    const char * g_hex;
};

/* All constants here were pulled from Appendix A of RFC 5054 */
// only using the 1024 constant from RFC 5054
static const struct NGHex global_Ng_constant =
 { /* 1024 */
    "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496"
    "EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8E"
    "F4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA"
    "9AFD5138FE8376435B9FC61D2FC0EB06E3",
    "2"
 };


struct SRPVerifier
{
    void                 *username;
    void                 *alloc_B;
    int                   authenticated;

    unsigned char M           [SRP_HASH_LEN];
    unsigned char H_AMK       [SRP_HASH_LEN];
    unsigned char session_key [SRP_HASH_LEN];
};


struct SRPUser
{
    mbedtls_mpi a, A, S, n, g;

    const unsigned char * bytes_A;
    int                   authenticated;

    const char *          username;
    const unsigned char * password;
    int                   password_len;

    unsigned char M           [SRP_HASH_LEN];
    unsigned char H_AMK       [SRP_HASH_LEN];
    unsigned char session_key [SRP_HASH_LEN];
};


static int hash_init( mbedtls_sha256_context *ctx )
{
    mbedtls_sha256_init( ctx );
    return mbedtls_sha256_starts_ret( ctx, 0 );
}


static int hash_update( mbedtls_sha256_context *s, const void *data, size_t len )
{
    return mbedtls_sha256_update_ret( s, data, len);
}


static int hash_final( mbedtls_sha256_context *s, unsigned char *md )
{
    int ret = mbedtls_sha256_finish_ret( s, md );
    mbedtls_sha256_free( s );
    return ret;
}


static int hash( const unsigned char *d, size_t n, unsigned char *md )
{
    return mbedtls_sha256_ret( d, n, md, 0);
}

static void mpi_init_multi(mbedtls_mpi *first, ...)
{
    va_list args;
    mbedtls_mpi *working = first;

    va_start(args, first);
    while (working != NULL) {
        mbedtls_mpi_init(working);
        working = va_arg(args, mbedtls_mpi *);
    }
}

static void mpi_free_multi(mbedtls_mpi *first, ...)
{
    va_list args;
    mbedtls_mpi *working = first;

    va_start(args, first);
    while (working != NULL) {
        mbedtls_mpi_free(working);
        working = va_arg(args, mbedtls_mpi *);
    }
}

#define MP_RAND_MAX_REQUESTED_WORDS (8)

static int
mp_rand(mbedtls_mpi *r, int words)
{
    int ret, to_use;
    uint8_t rbuf[MP_RAND_MAX_REQUESTED_WORDS * sizeof(uint32_t)];

    assert(words <= MP_RAND_MAX_REQUESTED_WORDS);

    to_use = words * sizeof(uint32_t);
    ret = xbee_random(rbuf, to_use);
    if (ret == 0) {
        ret = mbedtls_mpi_read_binary(r, rbuf, to_use);
    }

    return ret;
}


static int H_nn(mbedtls_mpi *ret, mbedtls_mpi *n0, mbedtls_mpi *n1)
{
    unsigned char   buff[ SRP_HASH_LEN ];
    size_t          len_n0 = mbedtls_mpi_size(n0);
    size_t          len_n1 = mbedtls_mpi_size(n1);
    int             nbytes = len_n0 + len_n1;
    int             result;
    unsigned char * bin    = (unsigned char *) malloc(nbytes);

    if (!bin)
        return MBEDTLS_ERR_MPI_ALLOC_FAILED;

    if ((result = mbedtls_mpi_write_binary(n0, bin, len_n0)) ||
        (result = mbedtls_mpi_write_binary(n1, bin + len_n0, len_n1)) ||
        (result = hash(bin, nbytes, buff)) ||
        (result = mbedtls_mpi_read_binary(ret, buff, SRP_HASH_LEN)))
    {
        // Whether or not there was an error, fall through.
        // Remember that the first failure prevents subsequent calls.
    }
    free(bin);
    return result;
}


static int H_ns( mbedtls_mpi *x, mbedtls_mpi *n, const unsigned char *bytes, int len_bytes )
{
    unsigned char   buff[ SRP_HASH_LEN ];
    int             len_n  = mbedtls_mpi_size(n);
    int             nbytes = len_n + len_bytes;
    unsigned char * bin    = (unsigned char *) malloc(nbytes);
    int             result;

    if (!bin)
        return MBEDTLS_ERR_MPI_ALLOC_FAILED;

    result = mbedtls_mpi_write_binary(n, bin, len_n);
    if (result == 0) {
        memcpy(bin + len_n, bytes, len_bytes);
        hash(bin, nbytes, buff);
        result = mbedtls_mpi_read_binary(x, buff, SRP_HASH_LEN);
    }

    free(bin);
    return result;
}


static int calculate_x( mbedtls_mpi *x, mbedtls_mpi *salt, const char * username,
                         const unsigned char * password, int password_len )
{
    unsigned char ucp_hash[SRP_HASH_LEN];
    mbedtls_sha256_context state;

    hash_init(&state);

    hash_update( &state, username, strlen(username) );
    hash_update( &state, ":", 1 );
    hash_update( &state, password, password_len );

    hash_final( &state, ucp_hash );

    return H_ns(x, salt, ucp_hash, SRP_HASH_LEN);
}


static int update_hash_mp( mbedtls_sha256_context *state, mbedtls_mpi *n )
{
    unsigned long len = mbedtls_mpi_size(n);
    unsigned char * n_bytes = (unsigned char *) malloc(len);
    int result = 0;

    if (!n_bytes)
        return MBEDTLS_ERR_MPI_BAD_INPUT_DATA;

    result = mbedtls_mpi_write_binary(n, n_bytes, len);

    if (result == 0) {
        hash_update(state, n_bytes, len);
    }

    free( n_bytes );
    return result;
}


static int hash_num( mbedtls_mpi *n, unsigned char *dest )
{
    int             nbytes  = mbedtls_mpi_size(n);
    unsigned char   *bin    = (unsigned char *) malloc(nbytes);
    int             result;

    if (!bin) {
        return MBEDTLS_ERR_MPI_ALLOC_FAILED;
    }

    if ((result = mbedtls_mpi_write_binary(n, bin, nbytes)) ||
        (result = hash(bin, nbytes, dest)))
    {
        // Whether or not there was an error, fall through.
        // Remember that the first failure prevents subsequent calls.
    }

    free(bin);
    return result;
}


static int calculate_M( unsigned char *dest, const char *I, mbedtls_mpi *s,
                            mbedtls_mpi *A, mbedtls_mpi *B, const unsigned char *K )
{
    mbedtls_mpi n, g;
    unsigned char H_N[ SRP_HASH_LEN ];
    unsigned char H_g[ SRP_HASH_LEN ];
    unsigned char H_I[ SRP_HASH_LEN ];
    unsigned char H_xor[ SRP_HASH_LEN ];
    mbedtls_sha256_context state;
    int           i = 0;
    int           hash_len = SRP_HASH_LEN;

    int result;

    mpi_init_multi(&n, &g, NULL);

    if ((result = mbedtls_mpi_read_string(&n, 16, global_Ng_constant.n_hex)) ||
        (result = mbedtls_mpi_read_string(&g, 16, global_Ng_constant.g_hex)))
    {
        goto cleanup_and_exit;
    }

    if ((result = hash_num(&n, H_N)) ||
        (result = hash_num(&g, H_g)) ||
        (result = hash((const unsigned char *)I, strlen(I), H_I)))
    {
        goto cleanup_and_exit;
    }


    for( i=0; i < hash_len; i++ )
        H_xor[i] = H_N[i] ^ H_g[i];

    hash_init(&state);

    hash_update( &state, H_xor, hash_len );
    hash_update( &state, H_I,   hash_len );
    if ((result = update_hash_mp(&state, s)) ||
        (result = update_hash_mp(&state, A)) ||
        (result = update_hash_mp(&state, B)))
    {
        goto cleanup_and_exit;
    }
    hash_update( &state, K, hash_len );

    hash_final( &state, dest );

cleanup_and_exit:
    mpi_free_multi(&n, &g, NULL);
    return result;
}


static int calculate_H_AMK( unsigned char *dest, mbedtls_mpi *A, const unsigned char *M, const unsigned char *K )
{
    mbedtls_sha256_context state;
    int result;

    hash_init( &state );

    if (0 == (result = update_hash_mp(&state, A))) {
        hash_update(&state, M, SRP_HASH_LEN);
        hash_update(&state, K, SRP_HASH_LEN);
        hash_final(&state, dest);
    }

    return result;
}


/***********************************************************************************************************
 *
 *  Exported Functions
 *
 ***********************************************************************************************************/

int srp_create_salted_verification_key( const char * username,
                                         const unsigned char * password, int len_password,
                                         const unsigned char ** bytes_s, int * len_s,
                                         const unsigned char ** bytes_v, int * len_v )
{
    int result;

    // declare and then initialize all the big numbers we need
    mbedtls_mpi s, v, x, n, g;

    mpi_init_multi(&s, &v, &x, &n, &g, NULL);

    // XXX s needs to be a cryptographically random number, 32 bits
    //      mp_rand is platform dependent, it needs a random function
    // TODO see XXX above

    // one rand digit, each digit is 32 bits, 32 bits of random
    mp_rand( &s, 1 );

    calculate_x( &x, &s, username, password, len_password );

    if ((result = mbedtls_mpi_read_string(&n, 16, global_Ng_constant.n_hex)) ||
        (result = mbedtls_mpi_read_string(&g, 16, global_Ng_constant.g_hex)) ||

        (result = mbedtls_mpi_exp_mod(&v, &g, &x, &n, NULL)))
    {
        goto cleanup_and_exit;
    }

    *len_s = mbedtls_mpi_size(&s);
    *len_v = mbedtls_mpi_size(&v);

    *bytes_s = (const unsigned char *) malloc(*len_s);
    *bytes_v = (const unsigned char *) malloc(*len_v);

    if (!bytes_s || !bytes_v) {
        result = MBEDTLS_ERR_MPI_ALLOC_FAILED;
        goto cleanup_and_exit;
    }

    if ((result = mbedtls_mpi_write_binary(&s, (unsigned char *) *bytes_s, *len_s)) ||
        (result = mbedtls_mpi_write_binary(&v, (unsigned char *) *bytes_v, *len_v)))
    {
        // Whether or not there was an error, fall through.
        // Remember that the first failure prevents subsequent calls.
    }

  cleanup_and_exit:
    mpi_free_multi(&s, &v, &x, &n, &g, NULL);
    return result;
}

/* Out: bytes_B, len_B.
 *
 * Possible return values/conditions:
 *    NULL: unable to allocate memory for verifier and/or MPI values
 *    non-NULL, bytes_B == NULL: failed SRP-6a check (A mod n == 0)
 *    non-NULL, bytes_B != NULL: verifier created; B returned in bytes_B/len_B
 *
 * Note that this code was modified for RTOS use.  When len_B changes to a
 * non-zero value, B is available for use by another task (e.g., to send to
 * the client) while this function continues to calculate M/HAMK.
 */
struct SRPVerifier *  srp_verifier_new( const char * username,
                                        const unsigned char * bytes_s, int len_s,
                                        const unsigned char * bytes_v, int len_v,
                                        const unsigned char * bytes_A, int len_A,
                                        const unsigned char ** bytes_B, int * len_B )
{
    mbedtls_mpi s, v, A, u, B, S, b, k, n, g, tmp1, tmp2, tmp3, zero;
    int ulen = strlen(username) + 1;

    struct SRPVerifier *ver  = NULL;

    // initialize return parameters
    *len_B   = 0;
    *bytes_B = NULL;

    mpi_init_multi( &s, &v, &A, &u, &B, &S, &b, &k, &n, &g,
                    &tmp1, &tmp2, &tmp3, &zero, NULL);

    if (mbedtls_mpi_read_binary(&s, bytes_s, len_s) ||
        mbedtls_mpi_read_binary(&v, bytes_v, len_v) ||
        mbedtls_mpi_read_binary(&A, bytes_A, len_A) ||
        mbedtls_mpi_lset(&zero, 0) ||
        mbedtls_mpi_read_string(&n, 16, global_Ng_constant.n_hex) ||
        mbedtls_mpi_read_string(&g, 16, global_Ng_constant.g_hex) ||
        mbedtls_mpi_mod_mpi(&tmp1, &A, &n))
    {
        goto cleanup_and_exit;
    }

    // use calloc() to initialize all struct elements to 0/NULL
    ver = calloc(1, sizeof *ver);
    if (!ver) {
        goto cleanup_and_exit;
    }

    ver->username = malloc(ulen);
    if (!ver->username) {
        goto free_verifier_and_exit;
    }
    memcpy(ver->username, username, ulen);

    /* SRP-6a safety check */
    if (mbedtls_mpi_cmp_mpi(&tmp1, &zero) == 0) {
        // We want to pass this condition up to the caller, so return a
        // non-NULL SRPVerifier, but leave bytes_B set to NULL.
        goto cleanup_and_exit;
    }

    // XXX s needs to be a cryptographically random number, 256 bits
    //      mp_rand is platform dependent, it needs a random function
    // TODO see XXX above

    // Although the following code is just one big if statement, all the
    // ... conditions are logical OR operations. Due to short circuiting,
    // ... the OR conditions are tested in the order given and the first
    // ... condition that is true causes the whole condition to be met.
    // ... If none of the conditions are met, which is the normal case,
    // ... then all operations were successful.
    // Various steps of the process are delineated by comments.

    // eight rand digit, each digit is 32 bits, 256 bits of random
    if (mp_rand(&b,8) ||
        H_nn(&k, &n, &g) ||

        /* B = kv + g^b */
        mbedtls_mpi_mul_mpi(&tmp1, &k, &v) ||
        mbedtls_mpi_exp_mod(&tmp2, &g, &b, &n, NULL) ||
        mbedtls_mpi_add_mpi(&tmp3, &tmp1, &tmp2) ||
        mbedtls_mpi_mod_mpi(&B, &tmp3, &n) )
    {
        goto free_verifier_and_exit;
    }

#ifdef DEBUG
    // If length is too big, something went terribly wrong
    assert(mbedtls_mpi_size(&B) <= SRP_GROUP_LEN);
#endif
    // mbedtls_mpi_write_binary() will zero fill to the full length.
    ver->alloc_B = malloc(SRP_GROUP_LEN);
    if (ver->alloc_B == NULL ||          // out of heap space
        mbedtls_mpi_write_binary(&B, ver->alloc_B, SRP_GROUP_LEN))
    {
        goto free_verifier_and_exit;
    }

    // Signal to caller (SRP Task) that bytes_B is ready/valid.  It's OK if
    // we later error out, because the task will fail when trying to process
    // the M value provided by the client.
    *bytes_B = ver->alloc_B;
    *len_B = SRP_GROUP_LEN;

    // Move on to calculating M, which takes an additional second.
    if (H_nn(&u, &A, &B) ||

        /* S = (A *(v^u)) ^ b */
        mbedtls_mpi_exp_mod(&tmp1, &v, &u, &n, NULL) ||
        mbedtls_mpi_mul_mpi(&tmp2, &A, &tmp1) ||
        mbedtls_mpi_exp_mod(&S, &tmp2, &b, &n, NULL) ||

        hash_num(&S, ver->session_key) ||

        calculate_M(ver->M, username, &s, &A, &B, ver->session_key) ||
        calculate_H_AMK(ver->H_AMK, &A, ver->M, ver->session_key))
    {
        goto free_verifier_and_exit;
    }

#if DEBUG
    phex("s", bytes_s, len_s);
    phex("v", bytes_v, len_v);
    phex("A", bytes_A, len_A);
    phex("B", *bytes_B, *len_B);
    phex("M1", ver->M, SRP_HASH_LEN);
    phex("M2", ver->H_AMK, SRP_HASH_LEN);
    __iar_dlmalloc_stats();
#endif

    goto cleanup_and_exit;

free_verifier_and_exit:
    srp_verifier_delete(ver);
    ver = NULL;

cleanup_and_exit:
    mpi_free_multi(&s, &v, &A, &u, &B, &S, &b, &k, &n,
                   &g, &tmp1, &tmp2, &tmp3, &zero, NULL);

    return ver;
}


void srp_verifier_delete( struct SRPVerifier * ver )
{
    if (ver) {
        free(ver->username);
        free(ver->alloc_B);
        memset( ver, 0, sizeof(*ver) );
        free(ver);
    }
}



int srp_verifier_is_authenticated( struct SRPVerifier * ver )
{
    return ver->authenticated;
}


const char * srp_verifier_get_username( struct SRPVerifier * ver )
{
    return ver->username;
}


const unsigned char * srp_verifier_get_session_key( struct SRPVerifier * ver, int * key_length )
{
    if (key_length)
        *key_length = SRP_HASH_LEN;
    return ver->session_key;
}


int srp_verifier_get_session_key_length( struct SRPVerifier * ver )
{
    return SRP_HASH_LEN;
}


/* user_M must be exactly SRP_HASH_LEN bytes in size */
void srp_verifier_verify_session( struct SRPVerifier * ver, const unsigned char * user_M, const unsigned char ** bytes_HAMK )
{
    if ( memcmp(ver->M, user_M, SRP_HASH_LEN) == 0 )
    {
        ver->authenticated = 1;
        *bytes_HAMK = ver->H_AMK;
    }
    else
    {
        *bytes_HAMK = NULL;
    }
}

/*******************************************************************************/

struct SRPUser * srp_user_new( const char * username,
                               const unsigned char * bytes_password, int len_password )
{
    struct SRPUser  *usr  = (struct SRPUser *) malloc( sizeof(struct SRPUser) );
    int              ulen = strlen(username) + 1;

    if (!usr)
        goto err_exit;


    mpi_init_multi(&usr->a, &usr->A, &usr->S, &usr->n, &usr->g, NULL);

    if (mbedtls_mpi_read_string(&usr->n, 16, global_Ng_constant.n_hex) ||
        mbedtls_mpi_read_string(&usr->g, 16, global_Ng_constant.g_hex ))
    {
        goto err_exit;
    }

    usr->username     = (const char *) malloc(ulen);
    usr->password     = (const unsigned char *) malloc(len_password);
    usr->password_len = len_password;

    if (!usr->username || !usr->password)
    {
        goto err_exit;
    }

    memcpy( (char *)usr->username, username,       ulen );
    memcpy( (char *)usr->password, bytes_password, len_password );

    usr->authenticated = 0;

    usr->bytes_A = 0;

    return usr;

    err_exit:
    if (usr)
    {
        mpi_free_multi(&usr->a, &usr->A, &usr->S, &usr->n, &usr->g, NULL);

        if (usr->username)
            free((void*)usr->username);

        if (usr->password)
        {
            memset( (void*)usr->password, 0, usr->password_len );
            free((void*)usr->password);
        }

        free(usr);
    }

    return 0;
}



void srp_user_delete( struct SRPUser * usr )
{
    if(usr)
    {
        mpi_free_multi(&usr->a, &usr->A, &usr->S, &usr->n, &usr->g, NULL);

        memset( (void*)usr->password, 0, usr->password_len );

        free((char *)usr->username);
        free((char *)usr->password);

        if(usr->bytes_A)
            free((char *)usr->bytes_A);

        memset( usr, 0, sizeof(*usr) );
        free(usr);
    }
}


int srp_user_is_authenticated( struct SRPUser * usr)
{
    return usr->authenticated;
}


const char * srp_user_get_username( struct SRPUser * usr )
{
    return usr->username;
}



const unsigned char * srp_user_get_session_key( struct SRPUser * usr, int * key_length )
{
    if (key_length)
        *key_length = SRP_HASH_LEN;

    return usr->session_key;
}


int srp_user_get_session_key_length( struct SRPUser * usr )
{
    return SRP_HASH_LEN;
}



/* Output: username, bytes_A, len_A */
int srp_user_start_authentication(struct SRPUser * usr, const char ** username,
                                  const unsigned char ** bytes_A, int * len_A)
{
    int result;         // Handles result codes from bignum.h

    // XXX s needs to be a cryptographically random number, 256 bits
    //      mp_rand is platform dependent, it needs a random function
    // TODO see XXX above

    // eight rand digits, each digit is 32 bits, 256 bits of random
    if ((result = mp_rand(&usr->a, 8)) ||
        (result = mbedtls_mpi_exp_mod(&usr->A, &usr->g, &usr->a, &usr->n, NULL)))
    {
        return result;
    }


#ifdef DEBUG
    assert(mbedtls_mpi_size(&usr->A) <= SRP_GROUP_LEN);
#endif
    // If length is too big, something went terribly wrong
    // But truncating the result is better than asserting in a release build.
    *len_A = SRP_GROUP_LEN;

    *bytes_A = (const unsigned char *) malloc(SRP_GROUP_LEN);

    if (!*bytes_A) {
        // Out of heap space
        *len_A = 0;
        *bytes_A = 0;
        *username = 0;
        return MBEDTLS_ERR_MPI_ALLOC_FAILED;
    }

    // *Len_A has been set to SRP_GROUP_LEN.
    // By doing so, mbedtls_mpi_write_binary() will zero fill to pad out
    // ... to the full length.
    if (0 == (result = mbedtls_mpi_write_binary(&usr->A,
                                                (unsigned char *)*bytes_A,
                                                *len_A)))
    {
        usr->bytes_A = *bytes_A;
        *username = usr->username;
    }

    return result;
}


/* Output: bytes_M. Buffer length is SRP_HASH_LEN */
int srp_user_process_challenge(struct SRPUser * usr,
                               const unsigned char *bytes_s, int len_s,
                               const unsigned char *bytes_B, int len_B,
                               const unsigned char **bytes_M, int *len_M)
{
    mbedtls_mpi s, B, u, x, k, tmp1, tmp2, tmp3, zero;
    int result;

    mpi_init_multi(&s, &B, &u, &x, &k, &tmp1, &tmp2, &tmp3, &zero, NULL);

    if ((result = mbedtls_mpi_read_binary(&s, bytes_s, len_s)) ||
        (result = mbedtls_mpi_read_binary( &B, bytes_B, len_B )))
    {
      goto cleanup_and_exit;
    }

    *len_M = 0;
    *bytes_M = 0;

    if ((result = H_nn(&u, &usr->A, &B)) ||
        (result = calculate_x(&x, &s, usr->username,
                              usr->password, usr->password_len)) ||
        (result = H_nn(&k, &usr->n, &usr->g)))
    {
        goto cleanup_and_exit;
    }

    /* SRP-6a safety check */
    if( (mbedtls_mpi_cmp_mpi(&B, &zero) != 0)
        && (mbedtls_mpi_cmp_mpi(&u, &zero) != 0) )
    {
        // S = (B - k*(g^x)) ^ (a + ux)
        if ((result = mbedtls_mpi_mul_mpi(&tmp1, &u, &x)) ||          // tmp1 = u * x
            (result = mbedtls_mpi_add_mpi(&tmp2, &tmp1, &usr->a)) ||  // tmp2 = (a + (u * x)

            (result = mbedtls_mpi_exp_mod(&tmp1, &usr->g, &x, &usr->n, NULL)) ||

            (result = mbedtls_mpi_mul_mpi(&tmp3, &k, &tmp1)) ||      // tmp3 = k*(g^x
            (result = mbedtls_mpi_sub_mpi(&tmp1, &B, &tmp3)) ||      // tmp1 = (B - K*(g^x)

            (result = mbedtls_mpi_exp_mod(&usr->S, &tmp1, &tmp2,
                                          &usr->n, NULL)) ||

            (result = hash_num(&usr->S, usr->session_key)) ||
            (result = calculate_M(usr->M, usr->username, &s, &usr->A, &B, usr->session_key)) ||
            (result = calculate_H_AMK(usr->H_AMK, &usr->A, usr->M, usr->session_key)))
        {
            goto cleanup_and_exit;
        }

        *bytes_M = usr->M;

        if (len_M)
            *len_M = SRP_HASH_LEN;
    } else {
        *bytes_M = NULL;

        if (len_M)
            *len_M = 0;
    }

cleanup_and_exit:
    mpi_free_multi(&s, &B, &u, &x, &k, &tmp1, &tmp2, &tmp3, &zero, NULL);
    return result;
}


void srp_user_verify_session( struct SRPUser * usr, const unsigned char * bytes_HAMK )
{
    if ( memcmp( usr->H_AMK, bytes_HAMK, SRP_HASH_LEN ) == 0 )
        usr->authenticated = 1;
}
