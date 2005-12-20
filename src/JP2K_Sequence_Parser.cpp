/*
Copyright (c) 2004-2005, John Hurst
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
/*! \file    JP2K_Sequence_Parser.cpp
    \version $Id$
    \brief   AS-DCP library, JPEG 2000 codestream essence reader implementation
*/

#include <AS_DCP.h>
#include <FileIO.h>
#include <DirScanner.h>
#include <list>
#include <string>
#include <algorithm>
#include <string.h>
#include <assert.h>

using namespace ASDCP;


//------------------------------------------------------------------------------------------
  
class FileList : public std::list<std::string>
{
  std::string m_DirName;

public:
  FileList() {}
  ~FileList() {}

  //
  Result_t InitFromDirectory(const char* path)
  {
    char next_file[ASDCP_MAX_PATH];
    DirScanner Scanner;

    Result_t result = Scanner.Open(path);

    if ( ASDCP_SUCCESS(result) )
      {
	m_DirName = path;

	while ( ASDCP_SUCCESS(Scanner.GetNext(next_file)) )
	  {
	    if ( next_file[0] == '.' ) // no hidden files or internal links
	      continue;

	    std::string Str(m_DirName);
	    Str += "/";
	    Str += next_file;
	    push_back(Str);
	  }

	sort();
      }

    return result;
  }
};

//------------------------------------------------------------------------------------------

class ASDCP::JP2K::SequenceParser::h__SequenceParser
{
  ui32_t             m_FramesRead;
  Rational           m_PictureRate;
  FileList           m_FileList;
  FileList::iterator m_CurrentFile;
  CodestreamParser   m_Parser;

  ASDCP_NO_COPY_CONSTRUCT(h__SequenceParser);

public:
  PictureDescriptor  m_PDesc;

  h__SequenceParser() : m_FramesRead(0)
  {
    memset(&m_PDesc, 0, sizeof(m_PDesc));
    m_PDesc.EditRate = Rational(24,1); 
  }

  ~h__SequenceParser()
  {
    Close();
  }

  Result_t OpenRead(const char* filename);
  void     Close() {}

  Result_t Reset()
  {
    m_FramesRead = 0;
    m_CurrentFile = m_FileList.begin();
    return RESULT_OK;
  }

  Result_t ReadFrame(FrameBuffer&);
};


//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::h__SequenceParser::OpenRead(const char* filename)
{
  ASDCP_TEST_NULL_STR(filename);

  Result_t result = m_FileList.InitFromDirectory(filename);

  if ( m_FileList.empty() )
    return RESULT_ENDOFFILE;

  if ( ASDCP_SUCCESS(result) )
    {
      m_CurrentFile = m_FileList.begin();

      CodestreamParser Parser;
      FrameBuffer TmpBuffer;

      fsize_t file_size = FileSize((*m_CurrentFile).c_str());

      if ( file_size == 0 )
	result = RESULT_NOT_FOUND;

      if ( ASDCP_SUCCESS(result) )
	result = TmpBuffer.Capacity(file_size);

      if ( ASDCP_SUCCESS(result) )
	result = Parser.OpenReadFrame((*m_CurrentFile).c_str(), TmpBuffer);
      
      if ( ASDCP_SUCCESS(result) )
	result = Parser.FillPictureDescriptor(m_PDesc);

      // how big is it?
      if ( ASDCP_SUCCESS(result) )
	m_PDesc.ContainerDuration = m_FileList.size();
    }

  return result;
}

//
//
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::h__SequenceParser::ReadFrame(FrameBuffer& FB)
{
  if ( m_CurrentFile == m_FileList.end() )
    return RESULT_ENDOFFILE;

  // open the file
  Result_t result = m_Parser.OpenReadFrame((*m_CurrentFile).c_str(), FB);

  if ( ASDCP_SUCCESS(result) )
    {
      FB.FrameNumber(m_FramesRead++);
      m_CurrentFile++;
    }

  return result;
}


//------------------------------------------------------------------------------------------

ASDCP::JP2K::SequenceParser::SequenceParser()
{
}

ASDCP::JP2K::SequenceParser::~SequenceParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::OpenRead(const char* filename, bool pedantic) const
{
  const_cast<ASDCP::JP2K::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(filename);

  if ( ASDCP_FAILURE(result) )
    const_cast<ASDCP::JP2K::SequenceParser*>(this)->m_Parser.release();

  return result;
}

// Rewinds the stream to the beginning.
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::Reset() const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->Reset();
}

// Places a frame of data in the frame buffer. Fails if the buffer is too small
// or the stream is empty.
ASDCP::Result_t
ASDCP::JP2K::SequenceParser::ReadFrame(FrameBuffer& FB) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

ASDCP::Result_t
ASDCP::JP2K::SequenceParser::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Parser.empty() )
    return RESULT_INIT;

  PDesc = m_Parser->m_PDesc;
  return RESULT_OK;
}


//
// end JP2K_Sequence_Parser.cpp
//
