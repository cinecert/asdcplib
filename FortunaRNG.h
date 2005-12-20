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
/*! \file    FortunaRNG.h
    \version $Id$
    \brief   Fortuna PRNG interface
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



#ifndef __FORTUNARNG_H__
#define __FORTUNARNG_H__

#include <AS_DCP.h> // re-using handy typedefs


class FortunaRNG
{
  class h__RNG;
  ASDCP::mem_ptr<h__RNG> m_RNG;
  ASDCP_NO_COPY_CONSTRUCT(FortunaRNG);

 public:
  FortunaRNG();
  ~FortunaRNG();
  byte_t* FillRandom(byte_t* buf, ui32_t len);
};



#endif // __FORTUNARNG_H__

//
// end FortunaRNG.h
//
