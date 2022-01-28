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

#include "AS_02_ACES.h"
#include <KM_fileio.h>
#include <assert.h>
#include <KM_log.h>

using Kumu::DefaultLogSink;

namespace {
    class FileList : public std::list<std::string>
    {
      std::string m_DirName;

    public:
      FileList() {}
      ~FileList() {}

      const FileList& operator=(const std::list<std::string>& pathlist)
      {
        std::list<std::string>::const_iterator i;
        for(i = pathlist.begin(); i != pathlist.end(); i++)
          push_back(*i);
        return *this;
      }

      //
      ASDCP::Result_t InitFromDirectory(const std::string& path)
      {
        char next_file[Kumu::MaxFilePath];
        Kumu::DirScanner Scanner;

        ASDCP::Result_t result = Scanner.Open(path);

        if(ASDCP_SUCCESS(result))
        {
          m_DirName = path;

          while(ASDCP_SUCCESS(Scanner.GetNext(next_file)))
          {
            if(next_file[0] == '.') // no hidden files or internal links
              continue;

            std::string Str(m_DirName);
            Str += "/";
            Str += next_file;

            if(!Kumu::PathIsDirectory(Str))
              push_back(Str);
          }

          sort();
        }

        return result;
      }
    };
}


class AS_02::ACES::SequenceParser::h__SequenceParser
{
  ui32_t             m_FramesRead;
  ASDCP::Rational    m_PictureRate;
  FileList           m_FileList;
  FileList::iterator m_CurrentFile;
  CodestreamParser   m_Parser;
  bool               m_Pedantic;

  Result_t OpenRead();

  ASDCP_NO_COPY_CONSTRUCT(h__SequenceParser);

public:
  PictureDescriptor  m_PDesc;
  ResourceList_t   m_ResourceList_t;

  h__SequenceParser() : m_FramesRead(0), m_Pedantic(false)
  {
    memset(&m_PDesc, 0, sizeof(m_PDesc));
    m_PDesc.EditRate = ASDCP::Rational(24, 1);
  }

  ~h__SequenceParser()
  {
    Close();
  }

  Result_t OpenRead(const std::string& filename, bool pedantic);
  Result_t OpenRead(const std::list<std::string>& file_list, bool pedantic);
  // Opens a files sequence for reading. The sequence is expected to contain one or more
  // PNG or TIFF files which will be added as Ancillary Data.
  Result_t OpenTargetFrameSequenceRead(const std::list<std::string> &target_frame_file_list);

  void     Close() {}

  Result_t Reset()
  {
    m_FramesRead = 0;
    m_CurrentFile = m_FileList.begin();
    return RESULT_OK;
  }

  Result_t ReadFrame(FrameBuffer&);
};

AS_02::Result_t AS_02::ACES::SequenceParser::h__SequenceParser::OpenRead()
{

  if(m_FileList.empty())
    return RESULT_ENDOFFILE;

  m_CurrentFile = m_FileList.begin();
  CodestreamParser Parser;
  FrameBuffer TmpBuffer;

  Kumu::fsize_t file_size = Kumu::FileSize((*m_CurrentFile).c_str());

  if(file_size == 0)
    return RESULT_NOT_FOUND;

  assert(file_size <= 0xFFFFFFFFL);
  Result_t result = TmpBuffer.Capacity((ui32_t)file_size);
  if(ASDCP_SUCCESS(result))
    result = Parser.OpenReadFrame((*m_CurrentFile).c_str(), TmpBuffer);

  if(ASDCP_SUCCESS(result))
    result = Parser.FillPictureDescriptor(m_PDesc);

  // how big is it?
  if(ASDCP_SUCCESS(result))
    m_PDesc.ContainerDuration = m_FileList.size();

  return result;
}

AS_02::Result_t AS_02::ACES::SequenceParser::h__SequenceParser::OpenRead(const std::string& filename, bool pedantic)
{

  m_Pedantic = pedantic;

  Result_t result = m_FileList.InitFromDirectory(filename);

  if(ASDCP_SUCCESS(result))
    result = OpenRead();

  return result;
}

AS_02::Result_t AS_02::ACES::SequenceParser::h__SequenceParser::OpenRead(const std::list<std::string>& file_list, bool pedantic)
{

  m_Pedantic = pedantic;
  m_FileList = file_list;
  return OpenRead();
}

AS_02::Result_t AS_02::ACES::SequenceParser::h__SequenceParser::OpenTargetFrameSequenceRead(const std::list<std::string> &target_frame_file_list)
{
  AS_02::Result_t result = AS_02::RESULT_OK;
  std::list<std::string>::const_iterator i;
  byte_t read_buffer[16];

  for (i = target_frame_file_list.begin(); i != target_frame_file_list.end(); i++)
  {
      std::string abs_filename = Kumu::PathMakeAbsolute(*i);
        Kumu::FileReader reader;
        result = reader.OpenRead(abs_filename);

        if ( KM_SUCCESS(result) )
        {
          result = reader.Read(read_buffer, 16);
          reader.Close();
        }
        if ( KM_SUCCESS(result) )
        {
      // is it PNG or TIFF?
        MIMEType_t media_type = MT_UNDEF;
        if ( memcmp(read_buffer, PNGMagic, sizeof(PNGMagic)) == 0) media_type = MT_PNG;
        if ( memcmp(read_buffer, TIFFMagicLE, sizeof(TIFFMagicLE)) == 0) media_type = MT_TIFF;
        if ( memcmp(read_buffer, TIFFMagicBE, sizeof(TIFFMagicBE)) == 0) media_type = MT_TIFF;
        if (media_type != MT_UNDEF)
        {
          AS_02::ACES::AncillaryResourceDescriptor resource_descriptor;
          Kumu::UUID asset_id;
          result = CreateTargetFrameAssetId(asset_id, abs_filename);
          memcpy(&resource_descriptor.ResourceID, asset_id.Value(), Kumu::UUID_Length);
          resource_descriptor.Type = media_type;
          resource_descriptor.filePath = *i;
          if ( KM_SUCCESS(result) ) m_ResourceList_t.push_back(resource_descriptor);

        }
    }
   }
  return result;
}


AS_02::Result_t AS_02::ACES::SequenceParser::h__SequenceParser::ReadFrame(FrameBuffer& FB)
{

  if(m_CurrentFile == m_FileList.end())
    return RESULT_ENDOFFILE;

  // open the file
  Result_t result = m_Parser.OpenReadFrame((*m_CurrentFile).c_str(), FB);

  if(ASDCP_SUCCESS(result) && m_Pedantic)
  {
    PictureDescriptor PDesc;
    result = m_Parser.FillPictureDescriptor(PDesc);

    if(ASDCP_SUCCESS(result) && !(m_PDesc == PDesc))
    {
      Kumu::DefaultLogSink().Error("ACES codestream parameters do not match at frame %d\n", m_FramesRead + 1);
      result = ASDCP::RESULT_RAW_FORMAT;
    }
  }

  if(ASDCP_SUCCESS(result))
  {
    FB.FrameNumber(m_FramesRead++);
    m_CurrentFile++;
  }

  return result;
}

//------------------------------------------------------------------------------


AS_02::ACES::SequenceParser::SequenceParser() {}

AS_02::ACES::SequenceParser::~SequenceParser() {}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
AS_02::Result_t AS_02::ACES::SequenceParser::OpenRead(const std::string &directory, bool pedantic /*= false*/, const std::list<std::string> &target_frame_file_list /* = std::list<std::string>()*/) const
{

  const_cast<AS_02::ACES::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(directory, pedantic);

  if(ASDCP_SUCCESS(result))
    if (target_frame_file_list.size() > 0 ) result = m_Parser->OpenTargetFrameSequenceRead(target_frame_file_list);

  if(ASDCP_FAILURE(result))
    const_cast<AS_02::ACES::SequenceParser*>(this)->m_Parser.release();

  return result;
}

AS_02::Result_t AS_02::ACES::SequenceParser::OpenRead(const std::list<std::string> &file_list, bool pedantic /*= false*/, const std::list<std::string> &target_frame_file_list /* = std::list<std::string>()*/) const
{

  const_cast<AS_02::ACES::SequenceParser*>(this)->m_Parser = new h__SequenceParser;

  Result_t result = m_Parser->OpenRead(file_list, pedantic);

  if(ASDCP_SUCCESS(result))
    if (target_frame_file_list.size() > 0 ) result = m_Parser->OpenTargetFrameSequenceRead(target_frame_file_list);

  if(ASDCP_FAILURE(result))
    const_cast<AS_02::ACES::SequenceParser*>(this)->m_Parser.release();

  return result;
}

AS_02::Result_t AS_02::ACES::SequenceParser::FillPictureDescriptor(PictureDescriptor &PDesc) const
{

  if(m_Parser.empty())
    return RESULT_INIT;

  PDesc = m_Parser->m_PDesc;
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::SequenceParser::FillResourceList(ResourceList_t &rResourceList_t) const
{

  if(m_Parser.empty())
    return RESULT_INIT;

  rResourceList_t = m_Parser->m_ResourceList_t;
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::SequenceParser::Reset() const
{

  if(m_Parser.empty())
    return RESULT_INIT;

  return m_Parser->Reset();
}

AS_02::Result_t AS_02::ACES::SequenceParser::ReadFrame(FrameBuffer &FB) const
{

  if(m_Parser.empty())
    return RESULT_INIT;

  return m_Parser->ReadFrame(FB);
}

AS_02::Result_t AS_02::ACES::SequenceParser::ReadAncillaryResource(const std::string &filename, FrameBuffer &FB) const
{
  if ( m_Parser.empty() )
  return RESULT_INIT;
  Kumu::FileReader reader;
  Result_t result = Kumu::RESULT_OK;
  result = reader.OpenRead(filename);
  if (KM_SUCCESS(result))
  {
    FB.Capacity(reader.Size());
    ui32_t read_count;
    result = reader.Read(FB.Data(), reader.Size(), &read_count);
    FB.Size(read_count);
    if (read_count <  reader.Size()) result = Kumu::RESULT_FAIL;
  }
  return result;
}
