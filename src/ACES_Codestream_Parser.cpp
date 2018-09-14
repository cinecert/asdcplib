/*
Copyright (c) 2018, Bjoern Stresing, Patrick Bichiou, Wolfgang Ruppel,
John Hurst

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

#include "ACES.h"
#include <KM_fileio.h>
#include <assert.h>
#include <KM_log.h>


using Kumu::DefaultLogSink;

AS_02::Result_t AS_02::ACES::ParseMetadataIntoDesc(const FrameBuffer &FB, PictureDescriptor &PDesc, byte_t *start_of_data /*= NULL*/) {

  const byte_t* p = FB.RoData();
  const byte_t* end_p = p + FB.Size();
  Result_t result = RESULT_OK;
  Attribute NextAttribute;

  result = CheckMagicNumber(&p);
  if(ASDCP_FAILURE(result)) return result;
  result = CheckVersionField(&p);
  if(ASDCP_FAILURE(result)) return result;
  NextAttribute.Move(p);

  while(p < end_p && ASDCP_SUCCESS(result)) {

    if(NextAttribute.IsValid()) {
      switch(NextAttribute.GetAttribute()) {
        case AcesImageContainerFlag:
          result = NextAttribute.GetValueAsBasicType(PDesc.AcesImageContainerFlag);
          break;
        case Channels:
          result = NextAttribute.GetValueAsChlist(PDesc.Channels);
          break;
        case Chromaticities:
          result = NextAttribute.GetValueAsChromaticities(PDesc.Chromaticities);
          break;
        case Compression:
          result = NextAttribute.GetValueAsBasicType(PDesc.Compression);
          break;
        case DataWindow:
          result = NextAttribute.GetValueAsBox2i(PDesc.DataWindow);
          break;
        case DisplayWindow:
          result = NextAttribute.GetValueAsBox2i(PDesc.DisplayWindow);
          break;
        case LineOrder:
          result = NextAttribute.GetValueAsBasicType(PDesc.LineOrder);
          break;
        case PixelAspectRatio:
          result = NextAttribute.GetValueAsBasicType(PDesc.PixelAspectRatio);
          break;
        case ScreenWindowCenter:
          result = NextAttribute.GetValueAsV2f(PDesc.ScreenWindowCenter);
          break;
        case SreenWindowWidth:
          result = NextAttribute.GetValueAsBasicType(PDesc.ScreenWindowWidth);
          break;
        case Other:
          result = NextAttribute.CopyToGenericContainer(PDesc.Other);
          break;
        default:
          DefaultLogSink().Error("Attribute mismatch.\n");
          result = RESULT_FAIL;
          break;
      }
      if(ASDCP_FAILURE(result))
      {
        result = RESULT_FAIL;
        break;
      }
    }
    result = GetNextAttribute(&p, NextAttribute);
    if(result == RESULT_ENDOFFILE)
    { // Indicates end of header.
      p = end_p;
      result = RESULT_OK;
    }
  }
  return result;
}

class AS_02::ACES::CodestreamParser::h__CodestreamParser
{

  ASDCP_NO_COPY_CONSTRUCT(h__CodestreamParser);

public:
  PictureDescriptor  m_PDesc;
  Kumu::FileReader   m_File;

  h__CodestreamParser()
  {
    memset(&m_PDesc, 0, sizeof(m_PDesc));
    m_PDesc.EditRate = ASDCP::Rational(24, 1);
    m_PDesc.SampleRate = m_PDesc.EditRate;
  }

  ~h__CodestreamParser() {}

  Result_t OpenReadFrame(const std::string& filename, FrameBuffer& FB)
  {
    m_File.Close();
    Result_t result = m_File.OpenRead(filename);

    if(ASDCP_SUCCESS(result))
    {
      Kumu::fsize_t file_size = m_File.Size();
      if(FB.Capacity() < file_size)
      {
        DefaultLogSink().Error("FrameBuf.Capacity: %u frame length: %u\n", FB.Capacity(), (ui32_t)file_size);
        return RESULT_SMALLBUF;
      }
    }

    ui32_t read_count;

    if(ASDCP_SUCCESS(result)) result = m_File.Read(FB.Data(), FB.Capacity(), &read_count);

    if(ASDCP_SUCCESS(result)) FB.Size(read_count);

    if(ASDCP_SUCCESS(result))
    {
      byte_t start_of_data = 0; // out param
      result = ParseMetadataIntoDesc(FB, m_PDesc, &start_of_data);
      if(ASDCP_SUCCESS(result)) FB.PlaintextOffset(start_of_data);
    }
    return result;
  }
};

AS_02::ACES::CodestreamParser::CodestreamParser()
{

}

AS_02::ACES::CodestreamParser::~CodestreamParser()
{

}

AS_02::Result_t AS_02::ACES::CodestreamParser::OpenReadFrame(const std::string &filename, FrameBuffer &FB) const
{

  const_cast<AS_02::ACES::CodestreamParser*>(this)->m_Parser = new h__CodestreamParser;
  return m_Parser->OpenReadFrame(filename, FB);
}

AS_02::Result_t AS_02::ACES::CodestreamParser::FillPictureDescriptor(PictureDescriptor &PDesc) const
{

  if(m_Parser.empty()) return RESULT_INIT;
  PDesc = m_Parser->m_PDesc;
  return RESULT_OK;
}
