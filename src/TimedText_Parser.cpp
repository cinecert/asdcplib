/*
Copyright (c) 2007, John Hurst
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
/*! \file    AS_DCP_TimedText.cpp
    \version $Id$       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/


#include "AS_DCP_internal.h"
#include "AS_DCP_TimedText.h"
#include "KM_xml.h"

using namespace Kumu;
using namespace ASDCP;

//------------------------------------------------------------------------------------------

class ASDCP::TimedText::DCSubtitleParser::h__DCSubtitleParser
{
  ui32_t             m_FileReadCount;

  ASDCP_NO_COPY_CONSTRUCT(h__DCSubtitleParser);

public:
  TimedTextDescriptor  m_TDesc;

  h__DCSubtitleParser() : m_FileReadCount(0)
  {
    memset(&m_TDesc, 0, sizeof(m_TDesc));
  }

  ~h__DCSubtitleParser()
  {
    Close();
  }

  Result_t OpenRead(const char* filename);
  void     Close() {}

  Result_t Reset()
  {
    m_FileReadCount = 0;
    return RESULT_OK;
  }

  Result_t ReadFrame(FrameBuffer&);
};

//------------------------------------------------------------------------------------------

ASDCP::TimedText::DCSubtitleParser::DCSubtitleParser()
{
}

ASDCP::TimedText::DCSubtitleParser::~DCSubtitleParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::OpenRead(const char* filename) const
{
  const_cast<ASDCP::TimedText::DCSubtitleParser*>(this)->m_Parser = new h__DCSubtitleParser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::TimedText::DCSubtitleParser*>(this)->m_Parser.release();

  return result;
}

// Rewinds the stream to the beginning.
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::Reset() const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->Reset();
}

// Places a frame of data in the frame buffer. Fails if the buffer is too small
// or the stream is empty.
ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::ReadFrame(FrameBuffer& FB) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

ASDCP::Result_t
ASDCP::TimedText::DCSubtitleParser::FillDescriptor(TimedTextDescriptor& PDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  PDesc = m_Parser->m_TDesc;
  return RESULT_OK;
}

//
// end AS_DCP_timedText.cpp
//
