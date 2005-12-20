/*
Copyright (c) 2005, John Hurst
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
/*! \file    Wav.h
    \version $Id$
    \brief   Wave file common elements
*/

#ifndef _WAV_H_
#define _WAV_H_

#include <FileIO.h>

namespace ASDCP
{
namespace Wav
{
  const ui32_t MaxWavHeader = 1024*32; // must find "data" within this space or no happy

  //
  class fourcc
    {
    private:
      byte_t data[4];

    public:
      inline fourcc() { memset( data, 0, 4 ); }
      inline fourcc( const char* v )   { memcpy( this->data, v, 4 ); }
      inline fourcc( const byte_t* v ) { memcpy( this->data, v, 4 ); }
      inline fourcc& operator=(const fourcc & s) { memcpy( this->data, s.data, 4); return *this; }
      inline bool operator==(const fourcc &rhs)  { return memcmp(data, rhs.data, 4) == 0 ? true : false; }
      inline bool operator!=(const fourcc &rhs)  { return memcmp(data, rhs.data, 4) != 0 ? true : false; }
    };

  const fourcc FCC_RIFF("RIFF");
  const fourcc FCC_WAVE("WAVE");
  const fourcc FCC_fmt_("fmt ");
  const fourcc FCC_data("data");

  //
  class SimpleWaveHeader
    {
    public:
      ui16_t	format;
      ui16_t	nchannels;
      ui32_t	samplespersec;
      ui32_t	avgbps;
      ui16_t	blockalign;
      ui16_t	bitspersample;
      ui16_t	cbsize;
      ui32_t	data_len;

      SimpleWaveHeader() :
	format(0), nchannels(0), samplespersec(0), avgbps(0),
	blockalign(0), bitspersample(0), cbsize(0), data_len(0) {}
      
      SimpleWaveHeader(ASDCP::PCM::AudioDescriptor& ADesc);

      Result_t  ReadFromBuffer(const byte_t* buf, ui32_t buf_len, ui32_t* data_start);
      Result_t  ReadFromFile(const ASDCP::FileReader& InFile, ui32_t* data_start);
      Result_t  WriteToFile(ASDCP::FileWriter& OutFile) const;
      void      FillADesc(ASDCP::PCM::AudioDescriptor& ADesc, Rational PictureRate) const;
   };

} // namespace Wav
} // namespace ASDCP

#endif // _WAV_H_

//
// end Wav.h
//
