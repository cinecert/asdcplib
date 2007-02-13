/*
Copyright (c) 2007, John Hurst
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
  /*! \file    KM_prng.cpp
    \version $Id$
    \brief   Fortuna pseudo-random number generator
  */


#include <KM_prng.h>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/err.h>

using namespace Kumu;


//
// SMPTE 429.6 MIC key generation
void
Kumu::Gen_FIPS_186_Value(const byte_t* key, byte_t* out_value)
{
  // FIPS 186-2 Sec. 3.1 as modified by Change 1, section entitled "General Purpose Random Number Generation"
  //
  byte_t sha_buf0[SHA_DIGEST_LENGTH];
  byte_t sha_buf1[SHA_DIGEST_LENGTH];
  byte_t key2[64];
  SHA_CTX SHA;
  BN_CTX* ctx1 = BN_CTX_new(); // used by BN_* functions
  assert(ctx1);

  memset(key2, 0, 64);
  memcpy(key2, key, SHA_DIGEST_LENGTH);
  // create the 2^160 constant
  BIGNUM c_2powb, c_2, c_160;
  BN_init(&c_2powb);  BN_init(&c_2);  BN_init(&c_160);
  BN_set_word(&c_2, 2);
  BN_set_word(&c_160, 160);
  BN_exp(&c_2powb, &c_2, &c_160, ctx1);

  // ROUND 1
  // step a -- SMPTE 429.6 sets XSEED = 0, so no need to do anything for this step
  // step b -- (key mod 2^160) is moot because the input value is only 128 bits in length

  // step c -- x = G(t,xkey)
  SHA1_Init(&SHA);
  SHA1_Update(&SHA, key2, 64);

  ui32_t* buf_p = (ui32_t*)sha_buf0;
  *buf_p++ = KM_i32_BE(SHA.h0);
  *buf_p++ = KM_i32_BE(SHA.h1);
  *buf_p++ = KM_i32_BE(SHA.h2);
  *buf_p++ = KM_i32_BE(SHA.h3);
  *buf_p++ = KM_i32_BE(SHA.h4);
  memcpy(out_value, sha_buf0, SHA_DIGEST_LENGTH);

  // step d ...
  BIGNUM xkey1, xkey2, x0;
  BN_init(&xkey1);  BN_init(&xkey2);    BN_init(&x0);

  BN_bin2bn(key2, SHA_DIGEST_LENGTH, &xkey1);
  BN_bin2bn(sha_buf0, SHA_DIGEST_LENGTH, &x0);
  BN_add_word(&xkey1, 1);            // xkey += 1
  BN_add(&xkey2, &xkey1, &x0);       // xkey += x
  BN_mod(&xkey1, &xkey2, &c_2powb, ctx1);  // xkey = xkey mod (2^160)

  // ROUND 2
  // step a -- SMPTE 429.6 sets XSEED = 0, so no need to do anything for this step
  // step b -- (key mod 2^160) is moot because xkey1 is the result of the same operation

  byte_t bin_buf[70]; // we need xkey1 in bin form for use by SHA1_Update
  ui32_t bin_buf_len = BN_num_bytes(&xkey1);
  assert(bin_buf_len < SHA_DIGEST_LENGTH+1);
  memset( bin_buf, 0, 64);
  BN_bn2bin(&xkey1, bin_buf);

  // step c -- x = G(t,xkey)
  SHA1_Init(&SHA);
  SHA1_Update(&SHA, bin_buf, 64);

  buf_p = (ui32_t*)sha_buf1;
  *buf_p++ = KM_i32_BE(SHA.h0);
  *buf_p++ = KM_i32_BE(SHA.h1);
  *buf_p++ = KM_i32_BE(SHA.h2);
  *buf_p++ = KM_i32_BE(SHA.h3);
  *buf_p++ = KM_i32_BE(SHA.h4);

  assert(memcmp(sha_buf1, sha_buf0, SHA_DIGEST_LENGTH) != 0); // are x0 and x1 different?

  BN_CTX_free(ctx1);
  memcpy(out_value + SHA_DIGEST_LENGTH, sha_buf1, SHA_DIGEST_LENGTH);
}


//
// end fips-186-rng.cpp
//
