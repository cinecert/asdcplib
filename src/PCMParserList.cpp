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
/*! \file    PCMParserList.cpp
    \version $Id$
    \brief   Read WAV file(s), multiplex multiple PCM frame buffers into one
*/

#include <PCMParserList.h>
#include <KM_log.h>
#include <assert.h>

using namespace ASDCP;
using namespace Kumu;


ASDCP::ParserInstance::ParserInstance() : m_p(0), m_SampleSize(0)
{
}

ASDCP::ParserInstance::~ParserInstance()
{
}

// PCM::CalcSampleSize(ADesc);
Result_t
ASDCP::ParserInstance::OpenRead(const char* filename, Rational& PictureRate)
{
  ASDCP_TEST_NULL_STR(filename);

  Result_t result = Parser.OpenRead(filename, PictureRate);

  if ( ASDCP_SUCCESS(result) )
    result = Parser.FillAudioDescriptor(ADesc);

  if ( ASDCP_SUCCESS(result) )
    {
      ADesc.EditRate = PictureRate;
      m_SampleSize = PCM::CalcSampleSize(ADesc);
      result = FB.Capacity(PCM::CalcFrameBufferSize(ADesc));
    }

  return result;
}


// deposit the next available sample into the given framebuffer
Result_t
ASDCP::ParserInstance::PutSample(byte_t* p)
{
  ASDCP_TEST_NULL(p);

  memcpy(p, m_p, m_SampleSize);
  m_p += m_SampleSize;
  return RESULT_OK;
}


//
Result_t
ASDCP::ParserInstance::ReadFrame()
{
  Result_t result = Parser.ReadFrame(FB);
  m_p = ASDCP_SUCCESS(result) ? FB.RoData() : 0;
  return result;
}


//------------------------------------------------------------------------------------------
//


//
ASDCP::PCMParserList::PCMParserList() : m_ChannelCount(0)
{
}

ASDCP::PCMParserList::~PCMParserList()
{
  while ( ! empty() )
    {
      delete back();
      pop_back();
    }
}


//
Result_t
ASDCP::PCMParserList::OpenRead(ui32_t argc, const char** argv, Rational& PictureRate)
{
  ASDCP_TEST_NULL_STR(argv);
  Result_t result = RESULT_OK;

  for ( ui32_t i = 0; i < argc && ASDCP_SUCCESS(result); i++ )
    {
      ParserInstance* I = new ParserInstance;
      result = I->OpenRead(argv[i], PictureRate);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( i == 0 )
	    {
	      m_ADesc = I->ADesc;
	    }
	  else
	    {
	      if ( I->ADesc.AudioSamplingRate != m_ADesc.AudioSamplingRate )
		{
		  DefaultLogSink().Error("AudioSamplingRate mismatch in PCM parser list.");
		  return RESULT_FORMAT;
		}

	      if ( I->ADesc.QuantizationBits  != m_ADesc.QuantizationBits )
		{
		  DefaultLogSink().Error("QuantizationBits mismatch in PCM parser list.");
		  return RESULT_FORMAT;
		}

	      if ( I->ADesc.ContainerDuration < m_ADesc.ContainerDuration )
		m_ADesc.ContainerDuration = I->ADesc.ContainerDuration;

	      m_ADesc.BlockAlign += I->ADesc.BlockAlign;
	    }

	  m_ChannelCount += I->ADesc.ChannelCount;
	}

      if ( ASDCP_SUCCESS(result) )
	result = I->FB.Capacity(PCM::CalcFrameBufferSize(m_ADesc));

      if ( ASDCP_SUCCESS(result) )
	push_back(I);
    }

  m_ADesc.ChannelCount = m_ChannelCount;
  m_ADesc.AvgBps = m_ADesc.AvgBps * m_ChannelCount;

  if ( ASDCP_FAILURE(result) )
    clear();
  
  return result;
}

//
Result_t
ASDCP::PCMParserList::FillAudioDescriptor(PCM::AudioDescriptor& ADesc) const
{
  ADesc = m_ADesc;
  return RESULT_OK;
}

//
Result_t
ASDCP::PCMParserList::Reset()
{
  Result_t result = RESULT_OK;
  PCMParserList::iterator self_i;

  for ( self_i = begin(); self_i != end() && ASDCP_SUCCESS(result) ; self_i++ )
    result = (*self_i)->Parser.Reset();

  return result;
}


//
Result_t
ASDCP::PCMParserList::ReadFrame(PCM::FrameBuffer& OutFB)
{
  Result_t result = RESULT_OK;

  if ( size() == 1 )
    return front()->Parser.ReadFrame(OutFB);

  PCMParserList::iterator self_i;
  assert(PCM::CalcFrameBufferSize(m_ADesc) <= OutFB.Capacity());

  for ( self_i = begin(); self_i != end() && ASDCP_SUCCESS(result) ; self_i++ )
    result = (*self_i)->ReadFrame();

  if ( ASDCP_SUCCESS(result) )
    {
      OutFB.Size(PCM::CalcFrameBufferSize(m_ADesc));

      //      ui32_t sample_size = (PCM::CalcSampleSize(m_ADesc));
      byte_t* Out_p = OutFB.Data();
      byte_t* End_p = Out_p + OutFB.Size();

      while ( Out_p < End_p && ASDCP_SUCCESS(result) )
	{
	  self_i = begin();

	  while ( self_i != end() && ASDCP_SUCCESS(result) )
	    {
	      result = (*self_i)->PutSample(Out_p);
	      Out_p += (*self_i)->SampleSize();
	      self_i++;
	    }
	}

      assert(Out_p == End_p);
    }

  return result;
}

//
// end PCMParserList.cpp
//
