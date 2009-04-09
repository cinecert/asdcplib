/*
Copyright (c) 2004-2009, John Hurst
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
/*! \file    asdcp-mem-test.cpp
    \version $Id$
    \brief   AS-DCP frame buffer allocation test
*/


#include <AS_DCP_internal.h>
//#include <KM_platform.h>
#include <KM_prng.h>

#include <iostream>
#include <assert.h>

using namespace ASDCP;
using namespace Kumu;

const ui32_t buf_size = 1024;
FortunaRNG RNG;

//
int a()
{
  FrameBuffer FB;
  FB.Capacity(buf_size);
  assert(FB.Capacity() == buf_size);
  RNG.FillRandom(FB.Data(), FB.Capacity());

  return 0;
}

//
int b()
{
  byte_t* buf = (byte_t*)malloc(buf_size);
  assert(buf);
  RNG.FillRandom(buf, buf_size);

  {
    FrameBuffer FB;
    FB.SetData(buf, buf_size);
    assert(FB.Data() == buf);
    assert(FB.Capacity() == buf_size);
    // ~FB() is called...
  }

  free(buf);
  return 0;
}

//
int c()
{
  byte_t* buf = (byte_t*)malloc(buf_size);
  assert(buf);
  RNG.FillRandom(buf, buf_size);

  {
    FrameBuffer FB;
    FB.SetData(buf, buf_size);
    assert(FB.Data() == buf);
    assert(FB.Capacity() == buf_size);

    FB.SetData(0,0);
    assert(FB.Data() == 0);
    assert(FB.Capacity() == 0);

    FB.Capacity(buf_size);
    assert(FB.Capacity() == buf_size);
    RNG.FillRandom(FB.Data(), FB.Capacity());
    // ~FB() is called...
  }

  free(buf);
  return 0;
}

//
int d()
{
  //  MPEG2::Parser     mPFile;
  MPEG2::MXFReader  mRFile;
  Result_t result = mRFile.OpenRead("../tests/write_test_mpeg.mxf");
  assert(ASDCP_SUCCESS(result));

  //  MPEG2::MXFWriter  mWFile;
  JP2K::CodestreamParser  jPCFile;
  JP2K::SequenceParser    jPSFile;
  JP2K::MXFReader   jRFile;
  JP2K::MXFWriter   jWFile;

  PCM::WAVParser    pPFile;
  PCM::MXFReader    pRFile;
  PCM::MXFWriter    pWFile;
  return 0;
}

//
int
main( int argc, char **argv )
{
  ui32_t i = 0x00010000;
  fputs("Watch your process monitor, memory usage should not change after startup.\n", stderr);

  while ( i-- )
    {
      a();
      b();
      c();
      d();

      if ( i && ( i % 1000 ) == 0 )
	fputc('.', stderr);
    }

  fputc('\n', stderr);
  return 0;
}


//
// end asdcp-mem-test.cpp
//
