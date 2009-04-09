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
  if ( argc != 3 )
    {
      fprintf(stderr, "USAGE: fips-186-test <XKey-hex> <X-hex>\n");
      return 2;
    }

  byte_t XKey_buf[SHA_DIGEST_LENGTH];
  byte_t X_test_buf[SHA_DIGEST_LENGTH*2];
  byte_t X_buf[SHA_DIGEST_LENGTH*2];
  ui32_t char_count;

  if ( hex2bin(argv[1], XKey_buf, SHA_DIGEST_LENGTH, &char_count) != 0 )
    {
      fprintf(stderr, "Error parsing <XKey-hex> value\n");
      return 3;
    }

  if ( char_count != SHA_DIGEST_LENGTH )
    {
      fprintf(stderr, "Incorrect length for <XKey-hex> value (expecting 40 hexadecimal characters)\n");
      return 3;
    }

  if ( hex2bin(argv[2], X_test_buf, SHA_DIGEST_LENGTH*2, &char_count) != 0 )
    {
      fprintf(stderr, "Error parsing <XKey-hex> value\n");
      return 3;
    }

  if ( char_count != (SHA_DIGEST_LENGTH * 2) )
    {
      fprintf(stderr, "Incorrect length for <X-hex> value (expecting 80 hexadecimal characters)\n");
      return 3;
    }

  Gen_FIPS_186_Value(XKey_buf, SHA_DIGEST_LENGTH, X_buf, SHA_DIGEST_LENGTH*2);

  if ( memcmp(X_buf, X_test_buf, SHA_DIGEST_LENGTH*2) != 0 )
    {
      fprintf(stderr, "Test vector value does not match:\n");
      hexdump(X_buf, SHA_DIGEST_LENGTH*2);
      return 4;
    }

  return 0;
}

//
// end rng.cpp
//
