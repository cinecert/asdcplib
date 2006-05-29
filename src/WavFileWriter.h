/*
Copyright (c) 2005-2006, John Hurst
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
/*! \file    WavFileWriter.h
    \version $Id$
    \brief   demux and write PCM data to WAV file(s)
*/

#include <KM_fileio.h>
#include <Wav.h>
#include <list>

#ifndef _WAVFILEWRITER_H_
#define _WAVFILEWRITER_H_

//
class WavFileWriter
{
  ASDCP::PCM::AudioDescriptor m_ADesc;
  std::list<Kumu::FileWriter*> m_OutFile;
  ASDCP_NO_COPY_CONSTRUCT(WavFileWriter);

 public:
  WavFileWriter() {}
  ~WavFileWriter()
    {
      while ( ! m_OutFile.empty() )
	{
	  delete m_OutFile.back();
	  m_OutFile.pop_back();
	}
    }

  ASDCP::Result_t
    OpenWrite(ASDCP::PCM::AudioDescriptor &ADesc, const char* file_root, bool split)
    {
      ASDCP_TEST_NULL_STR(file_root);
      char filename[256];
      ui32_t file_count = 1;
      ASDCP::Result_t result = ASDCP::RESULT_OK;
      m_ADesc = ADesc;

      if ( split )
	{
	  assert ( m_ADesc.ChannelCount % 2 == 0 ); // no support yet for stuffing odd files
	  file_count = m_ADesc.ChannelCount / 2;
	  m_ADesc.ChannelCount = 2;
	}

      for ( ui32_t i = 0; i < file_count && ASDCP_SUCCESS(result); i++ )
	{
	  snprintf(filename, 256, "%s_%u.wav", file_root, (i + 1));
	  m_OutFile.push_back(new Kumu::FileWriter);
	  result = m_OutFile.back()->OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      ASDCP::Wav::SimpleWaveHeader Wav(m_ADesc);
	      result = Wav.WriteToFile(*(m_OutFile.back()));
	    }
	}
      
      return result;
    }

  ASDCP::Result_t
    WriteFrame(ASDCP::PCM::FrameBuffer& FB)
    {
      ui32_t write_count;
      ASDCP::Result_t result = ASDCP::RESULT_OK;
      std::list<Kumu::FileWriter*>::iterator fi;
      assert(! m_OutFile.empty());

      ui32_t sample_size = ASDCP::PCM::CalcSampleSize(m_ADesc);
      const byte_t* p = FB.RoData();
      const byte_t* end_p = p + FB.Size();

      while ( p < end_p )
	{
	  for ( fi = m_OutFile.begin(); fi != m_OutFile.end() && ASDCP_SUCCESS(result); fi++ )
	    {
	      result = (*fi)->Write(p, sample_size, &write_count);
	      assert(write_count == sample_size);
	      p += sample_size;
	    }
	}

      return result;
    }
};


#endif // _WAVFILEWRITER_H_

//
// end WavFileWriter.h
//
