/*
Copyright (c) 2004, John Hurst
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
/*! \file    FortunaRNG.cpp
    \version $Id$
    \brief   Fortuna PRNG implementation
*/


/*
  This module partially implements the Fortuna PRNG described in Chapter
  10 of "Practical Cryptography", Ferguson & Schneier, 2003.

  The generator does not use a hash to stir the key, nor does it limit
  the block size of generated blocks. It does NOT implement a cross-
  platform entropy collector.

  In fact, the seed methods in use here should be regarded with great
  suspicion. This module is provided as a development aid, and is not
  endorsed for any use outside that narrow purpose.
 */


#include <FortunaRNG.h>
#include <string.h>
#include <assert.h>
#include <openssl/aes.h>


const ui32_t RNG_KEY_SIZE = 16;
const ui32_t RNG_KEY_SIZE_BITS = 128;
const ui32_t RNG_BLOCK_SIZE = 16;

// internal implementation class
class FortunaRNG::h__RNG
{
public:
  AES_KEY  m_Context;
  byte_t   m_rng_buf[RNG_BLOCK_SIZE];

  //
  void
  fill_rand(byte_t* buf, ui32_t len)
  {
    ui32_t gen_count = 0;

    while ( gen_count + RNG_BLOCK_SIZE <= len )
      {
	AES_encrypt(m_rng_buf, buf + gen_count, &m_Context);
	*(ui32_t*)(m_rng_buf + 12) += 3;
	gen_count += RNG_BLOCK_SIZE;
      }

    if ( len == gen_count ) // partial count needed?
      return;

    byte_t tmp[RNG_BLOCK_SIZE];
    AES_encrypt(m_rng_buf, tmp, &m_Context);
    *(ui32_t*)(m_rng_buf + 12) += 3;
    memcpy(buf + gen_count, tmp, len - gen_count);
  }

  void
  re_seed()
  {
    // re-seed the generator
    byte_t rng_key[RNG_KEY_SIZE];
    fill_rand(rng_key, RNG_KEY_SIZE);
    AES_set_encrypt_key(rng_key, RNG_KEY_SIZE_BITS, &m_Context);
  }
};


// we have separate constructor implementations depending upon platform:
//

#ifdef WIN32
// on win32 systems, we grab one bit at a time from the performance
// counter and assemble seed bytes from the collected bits

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>

//
static byte_t get_perf_byte()
{
  LARGE_INTEGER ticks;
  byte_t retval;

  for ( int i = 0; i < 8; i++ )
    {
      QueryPerformanceCounter(&ticks);
      retval |= (ticks.LowPart & 0x00000001) << i;
    }

  return retval;
}

//
FortunaRNG::FortunaRNG()
{
  m_RNG = new h__RNG;
  byte_t rng_key[RNG_KEY_SIZE];
  int i;

  for ( i = 0; i < RNG_KEY_SIZE; i++ )
    rng_key[i] = get_perf_byte();

  for ( i = 0; i < RNG_BLOCK_SIZE; i++ )
    m_RNG->m_rng_buf[i] = get_perf_byte();

  AES_set_encrypt_key(rng_key, RNG_KEY_SIZE_BITS, &m_RNG->m_Context);
}

#else // WIN32
// on POSIX systems, we simply read some seed from /dev/urandom

#include <stdio.h>

const char* DEV_URANDOM = "/dev/urandom";

FortunaRNG::FortunaRNG()
{
  m_RNG = new h__RNG;

  byte_t rng_key[RNG_KEY_SIZE];
  FILE*  dev_random = fopen(DEV_URANDOM, "r");

  if ( dev_random == 0 )
    {
      ASDCP::DefaultLogSink().Error("Unable to open DEV_URANDOM\n");
      perror(DEV_URANDOM);
      m_RNG = 0;
      return;
    }

  if ( fread(rng_key, 1, RNG_KEY_SIZE, dev_random) != RNG_KEY_SIZE )
    {
      fclose(dev_random);
      ASDCP::DefaultLogSink().Error("Unable to read DEV_URANDOM\n");
      perror(DEV_URANDOM);
      m_RNG = 0;
      return;
    }

  if ( fread(m_RNG->m_rng_buf, 1, RNG_BLOCK_SIZE, dev_random) != RNG_BLOCK_SIZE )
    {
      fclose(dev_random);
      ASDCP::DefaultLogSink().Error("Unable to read DEV_URANDOM\n");
      perror(DEV_URANDOM);
      m_RNG = 0;
      return;
    }

  fclose(dev_random);
  AES_set_encrypt_key(rng_key, RNG_KEY_SIZE_BITS, &m_RNG->m_Context);
}

#endif // WIN32

FortunaRNG::~FortunaRNG()
{
}

//
byte_t*
FortunaRNG::FillRandom(byte_t* buf, ui32_t len)
{
  assert(buf);

  if ( ! m_RNG )
    return 0;

  m_RNG->fill_rand(buf, len);
  m_RNG->re_seed();

  return buf;
}


//
// end FortunaRNG.cpp
//
