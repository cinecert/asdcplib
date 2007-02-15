/*
Copyright (c) 2006, John Hurst
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
#include <KM_log.h>
#include <KM_mutex.h>
#include <string.h>
#include <assert.h>
#include <openssl/aes.h>
#include <openssl/sha.h>
#include <openssl/bn.h>

using namespace Kumu;


#ifdef KM_WIN32
# include <wincrypt.h>
#else // KM_WIN32
# include <KM_fileio.h>
const char* DEV_URANDOM = "/dev/urandom";
#endif // KM_WIN32


const ui32_t RNG_KEY_SIZE = 512UL;
const ui32_t RNG_KEY_SIZE_BITS = 256UL;
const ui32_t RNG_BLOCK_SIZE = 16UL;
const ui32_t MAX_SEQUENCE_LEN = 0x00040000UL;


// internal implementation class
class h__RNG
{
  KM_NO_COPY_CONSTRUCT(h__RNG);

public:
  AES_KEY   m_Context;
  byte_t    m_ctr_buf[RNG_BLOCK_SIZE];
  Mutex     m_Lock;

  h__RNG()
  {
    memset(m_ctr_buf, 0, RNG_BLOCK_SIZE);
    byte_t rng_key[RNG_KEY_SIZE];

    { // this block scopes the following AutoMutex so that it will be
      // released before the call to set_key() below.
      AutoMutex Lock(m_Lock);

#ifdef KM_WIN32
      HCRYPTPROV hProvider = 0;
      CryptAcquireContext(&hProvider, 0, 0, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
      CryptGenRandom(hProvider, RNG_KEY_SIZE, rng_key);
#else // KM_WIN32
      // on POSIX systems we simply read some seed from /dev/urandom
      FileReader URandom;

      Result_t result = URandom.OpenRead(DEV_URANDOM);

      if ( KM_SUCCESS(result) )
	{
	  ui32_t read_count;
	  result = URandom.Read(rng_key, RNG_KEY_SIZE, &read_count);
	}

      if ( KM_FAILURE(result) )
	DefaultLogSink().Error("Error opening random device: %s\n", DEV_URANDOM);

#endif // KM_WIN32
    } // end AutoMutex context

    set_key(rng_key);
  }
	
  //
  void
  set_key(const byte_t* key_fodder)
  {
    assert(key_fodder);
    byte_t sha_buf[20];
    SHA_CTX SHA;
    SHA1_Init(&SHA);

    SHA1_Update(&SHA, (byte_t*)&m_Context, sizeof(m_Context));
    SHA1_Update(&SHA, key_fodder, RNG_KEY_SIZE);
    SHA1_Final(sha_buf, &SHA);

    AutoMutex Lock(m_Lock);
    AES_set_encrypt_key(sha_buf, RNG_KEY_SIZE_BITS, &m_Context);
    *(ui32_t*)(m_ctr_buf + 12) = 1;
  }
	
  //
  void
  fill_rand(byte_t* buf, ui32_t len)
  {
    assert(len <= MAX_SEQUENCE_LEN);
    ui32_t gen_count = 0;
    AutoMutex Lock(m_Lock);

    while ( gen_count + RNG_BLOCK_SIZE <= len )
      {
	AES_encrypt(m_ctr_buf, buf + gen_count, &m_Context);
	*(ui32_t*)(m_ctr_buf + 12) += 1;
	gen_count += RNG_BLOCK_SIZE;
      }
			
    if ( len != gen_count ) // partial count needed?
      {
	byte_t tmp[RNG_BLOCK_SIZE];
	AES_encrypt(m_ctr_buf, tmp, &m_Context);
	memcpy(buf + gen_count, tmp, len - gen_count);
      }
  }
};


static h__RNG* s_RNG = 0;


//------------------------------------------------------------------------------------------
//
// Fortuna public interface

Kumu::FortunaRNG::FortunaRNG()
{
  if ( s_RNG == 0 )
    s_RNG = new h__RNG;
}

Kumu::FortunaRNG::~FortunaRNG() {}

//
const byte_t*
Kumu::FortunaRNG::FillRandom(byte_t* buf, ui32_t len)
{
  assert(buf);
  assert(s_RNG);

  while ( len )
    {
      // 2^20 bytes max per seeding, use 2^19 to save
      // room for generating reseed values
      ui32_t gen_size = xmin(len, MAX_SEQUENCE_LEN);
      s_RNG->fill_rand(buf, gen_size);
      buf += gen_size;
      len -= gen_size;
	  
      // re-seed the generator
      byte_t rng_key[RNG_KEY_SIZE];
      s_RNG->fill_rand(rng_key, RNG_KEY_SIZE);
      s_RNG->set_key(rng_key);
  }
  
  return buf;
}

//
const byte_t*
Kumu::FortunaRNG::FillRandom(Kumu::ByteString& Buffer)
{
  FillRandom(Buffer.Data(), Buffer.Capacity());
  Buffer.Length(Buffer.Capacity());
  return Buffer.Data();
}

//------------------------------------------------------------------------------------------

//
// FIPS 186-2 Sec. 3.1 as modified by Change 1, section entitled "General Purpose Random Number Generation"
void
Kumu::Gen_FIPS_186_Value(const byte_t* key, ui32_t key_size, byte_t* out_buf, ui32_t out_buf_len)
{
  byte_t sha_buf[SHA_DIGEST_LENGTH];
  const ui32_t key_buf_len = 64;
  byte_t key_buf[key_buf_len];
  SHA_CTX SHA;
  BN_CTX* ctx1 = BN_CTX_new(); // used by BN_* functions
  assert(ctx1);

  // init key
  memset(key_buf, 0, key_buf_len);
  memcpy(key_buf, key, xmin<ui32_t>(key_buf_len, key_size));

  // create the 2^160 constant
  BIGNUM c_2powb, c_2, c_160;
  BN_init(&c_2powb);  BN_init(&c_2);  BN_init(&c_160);
  BN_set_word(&c_2, 2);
  BN_set_word(&c_160, 160);
  BN_exp(&c_2powb, &c_2, &c_160, ctx1);

  for (;;)
    {
      // step c -- x = G(t,xkey)
      SHA1_Init(&SHA);
      SHA1_Update(&SHA, key_buf, key_buf_len);
      ui32_t* buf_p = (ui32_t*)sha_buf;
      *buf_p++ = KM_i32_BE(SHA.h0);
      *buf_p++ = KM_i32_BE(SHA.h1);
      *buf_p++ = KM_i32_BE(SHA.h2);
      *buf_p++ = KM_i32_BE(SHA.h3);
      *buf_p++ = KM_i32_BE(SHA.h4);
      memcpy(out_buf, sha_buf, xmin<ui32_t>(out_buf_len, SHA_DIGEST_LENGTH));

      if ( out_buf_len <= SHA_DIGEST_LENGTH )
	break;

      out_buf_len -= SHA_DIGEST_LENGTH;
      out_buf += SHA_DIGEST_LENGTH;

      // step d ...
      BIGNUM xkey1, xkey_buf, x0;
      BN_init(&xkey1);  BN_init(&xkey_buf);    BN_init(&x0);

      BN_bin2bn(key_buf, SHA_DIGEST_LENGTH, &xkey1);
      BN_bin2bn(sha_buf, SHA_DIGEST_LENGTH, &x0);
      BN_add_word(&xkey1, 1);            // xkey += 1
      BN_add(&xkey_buf, &xkey1, &x0);       // xkey += x
      BN_mod(&xkey1, &xkey_buf, &c_2powb, ctx1);  // xkey = xkey mod (2^160)

      ui32_t bn_buf_len = BN_num_bytes(&xkey1);
      assert(bn_buf_len < SHA_DIGEST_LENGTH+1);
      memset(key_buf, 0, key_buf_len);
      BN_bn2bin(&xkey1, key_buf);
    }

  BN_CTX_free(ctx1);
}

//
// end KM_prng.cpp
//
