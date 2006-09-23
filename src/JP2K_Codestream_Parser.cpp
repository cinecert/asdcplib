/*
Copyright (c) 2004-2006, John Hurst
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
/*! \file    Codestream_Parser.cpp
    \version $Id$
    \brief   AS-DCP library, JPEG 2000 codestream essence reader implementation
*/

#include <KM_fileio.h>
#include <AS_DCP.h>
#include <JP2K.h>
#include <assert.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;

//------------------------------------------------------------------------------------------

class ASDCP::JP2K::CodestreamParser::h__CodestreamParser
{
  ASDCP_NO_COPY_CONSTRUCT(h__CodestreamParser);

public:
  PictureDescriptor  m_PDesc;
  Kumu::FileReader   m_File;

  h__CodestreamParser()
  {
    memset(&m_PDesc, 0, sizeof(m_PDesc));
    m_PDesc.EditRate = Rational(24,1);
    m_PDesc.SampleRate = m_PDesc.EditRate;
  }

  ~h__CodestreamParser() {}

  Result_t OpenReadFrame(const char* filename, FrameBuffer& FB)
  {
    ASDCP_TEST_NULL_STR(filename);
    m_File.Close();
    Result_t result = m_File.OpenRead(filename);

    if ( ASDCP_SUCCESS(result) )
      {
	Kumu::fsize_t file_size = m_File.Size();

	if ( FB.Capacity() < file_size )
	  {
	    DefaultLogSink().Error("FrameBuf.Capacity: %u frame length: %u\n", FB.Capacity(), (ui32_t)file_size);
	    return RESULT_SMALLBUF;
	  }
      }

    ui32_t read_count;

    if ( ASDCP_SUCCESS(result) )
      result = m_File.Read(FB.Data(), FB.Capacity(), &read_count);

    if ( ASDCP_SUCCESS(result) )
      FB.Size(read_count);

    if ( ASDCP_SUCCESS(result) )
      {
	Marker NextMarker;
	ui32_t i;
	const byte_t* p = FB.RoData();
	const byte_t* end_p = p + FB.Size();

	while ( p < end_p && ASDCP_SUCCESS(result) )
	  {
	    result = GetNextMarker(&p, NextMarker);

	    if ( ASDCP_FAILURE(result) )
	      {
		result = RESULT_RAW_ESS;
		break;
	      }
#if 0
	    fprintf(stderr, "%s Length: %u\n",
		    GetMarkerString(NextMarker.m_Type), NextMarker.m_DataSize);
#endif

	    switch ( NextMarker.m_Type )
	      {
	      case MRK_SOD:
		FB.PlaintextOffset(p - FB.RoData());
		p = end_p;
		break;

	      case MRK_SIZ:
		{
		  Accessor::SIZ SIZ_(NextMarker);
		  m_PDesc.StoredWidth = SIZ_.Xsize();
		  m_PDesc.StoredHeight = SIZ_.Ysize();
		  m_PDesc.AspectRatio = Rational(SIZ_.Xsize(), SIZ_.Ysize());
		  m_PDesc.Rsize = SIZ_.Rsize();
		  m_PDesc.Xsize = SIZ_.Xsize();
		  m_PDesc.Ysize = SIZ_.Ysize();
		  m_PDesc.XOsize = SIZ_.XOsize();
		  m_PDesc.YOsize = SIZ_.YOsize();
		  m_PDesc.XTsize = SIZ_.XTsize();
		  m_PDesc.YTsize = SIZ_.YTsize();
		  m_PDesc.XTOsize = SIZ_.XTOsize();
		  m_PDesc.YTOsize = SIZ_.YTOsize();
		  m_PDesc.Csize = SIZ_.Csize();

		  if ( m_PDesc.Csize != 3 )
		    {
		      DefaultLogSink().Error("Unexpected number of components: %u\n", m_PDesc.Csize);
		      return RESULT_RAW_FORMAT;
		    }

		  for ( i = 0; i < m_PDesc.Csize; i++ )
		    SIZ_.ReadComponent(i, m_PDesc.ImageComponents[i]);
		}
		break;

	      case MRK_COD:
		if ( NextMarker.m_DataSize > DefaultCodingDataLength )
		  {
		    DefaultLogSink().Error("Unexpectedly large CodingStyle data: %u\n", NextMarker.m_DataSize);
		    return RESULT_RAW_FORMAT;
		  }

		m_PDesc.CodingStyleLength = NextMarker.m_DataSize;
		memcpy(m_PDesc.CodingStyle, NextMarker.m_Data, m_PDesc.CodingStyleLength);
		break;

	      case MRK_QCD:
		if ( NextMarker.m_DataSize > DefaultCodingDataLength )
		  {
		    DefaultLogSink().Error("Unexpectedly large QuantDefault data: %u\n", NextMarker.m_DataSize);
		    return RESULT_RAW_FORMAT;
		  }

		m_PDesc.QuantDefaultLength = NextMarker.m_DataSize;
		memcpy(m_PDesc.QuantDefault, NextMarker.m_Data, m_PDesc.QuantDefaultLength);
		break;
	      }
	  }
      }

    return result;
  }
};

//------------------------------------------------------------------------------------------

ASDCP::JP2K::CodestreamParser::CodestreamParser()
{
}

ASDCP::JP2K::CodestreamParser::~CodestreamParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::JP2K::CodestreamParser::OpenReadFrame(const char* filename, FrameBuffer& FB) const
{
  const_cast<ASDCP::JP2K::CodestreamParser*>(this)->m_Parser = new h__CodestreamParser;
  return m_Parser->OpenReadFrame(filename, FB);
}

//
ASDCP::Result_t
ASDCP::JP2K::CodestreamParser::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  PDesc = m_Parser->m_PDesc;
  return RESULT_OK;
}


//
// end Codestream_Parser.cpp
//
