/*
Copyright (c) 2007-2009, John Hurst
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
/*! \file    fips-186-rng-test.cpp
    \version $Id$       
    \brief   FIPS 186-2 RNG test wrapper
*/

#include <KM_util.h>
#include <KM_prng.h>
#include <openssl/sha.h>

using namespace Kumu;

//
int
main(int argc, const char** argv)
{
  if ( argc != 5 )
    {
      fprintf(stderr, "USAGE: fips-186-test <rounds> <b> <XKey-hex> <X-hex>\n");
      return 2;
    }

  ui32_t const X_buf_len = 1024;
  byte_t XKey_buf[X_buf_len];
  byte_t X_test_buf[X_buf_len];
  ui32_t char_count;

  char* end = 0;
  ui32_t rounds = strtol(argv[1], &end, 10);
  ui32_t B = strtol(argv[2], &end, 10) / 8;

  if ( hex2bin(argv[3], XKey_buf, X_buf_len, &char_count) != 0 )
    {
      fprintf(stderr, "Error parsing <XKey-hex> value\n");
      return 3;
    }

  if ( char_count != B )
    {
      fprintf(stderr, "Incorrect length for <XKey-hex> value (expecting %d decoded bytes)\n", B);
      return 3;
    }

  if ( hex2bin(argv[4], X_test_buf, X_buf_len, &char_count) != 0 )
    {
      fprintf(stderr, "Error parsing <X-hex> value\n");
      return 3;
    }

  ui32_t const meg = 1024*1024;
  ui32_t out_size = char_count;
  byte_t X_buf[meg];
  ui32_t fill_size = rounds * 20;
  assert(fill_size < meg);
  Gen_FIPS_186_Value(XKey_buf, B, X_buf, fill_size);

  byte_t* test_key = &X_buf[fill_size - out_size];

  if ( memcmp(test_key, X_test_buf, char_count) != 0 )
    {
      fprintf(stderr, "Test vector value does not match:\n");
      hexdump(test_key, char_count);
      return 4;
    }

  return 0;
}

//
// end rng.cpp
//
