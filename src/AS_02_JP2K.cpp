/*
  Copyright (c) 2011-2013, Robert Scheler, Heiko Sparenberg Fraunhofer IIS, John Hurst
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
/*! \file    AS_02_JP2K.cpp
  \version $Id$
  \brief   AS-02 library, JPEG 2000 essence reader and writer implementation
*/

#include "AS_02_internal.h"

#include <iostream>
#include <iomanip>

using namespace ASDCP;
using namespace ASDCP::JP2K;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE ST 422 / ST 2067-5 frame wrapping of JPEG 2000 codestreams";
static std::string PICT_DEF_LABEL = "Image Track";

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader


class AS_02::JP2K::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  RGBAEssenceDescriptor*        m_RGBAEssenceDescriptor;
  CDCIEssenceDescriptor*        m_CDCIEssenceDescriptor;
  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;
  ASDCP::Rational               m_EditRate;
  ASDCP::Rational               m_SampleRate;
  EssenceType_t                 m_Format;

  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  h__Reader(const Dictionary& d) :
    AS_02::h__AS02Reader(d), m_RGBAEssenceDescriptor(0), m_CDCIEssenceDescriptor(0),
    m_EssenceSubDescriptor(0), m_Format(ESS_UNKNOWN) {}

  virtual ~h__Reader() {}

  Result_t    OpenRead(const char*);
  Result_t    ReadFrame(ui32_t, ASDCP::JP2K::FrameBuffer&, AESDecContext*, HMACContext*);
};

//
Result_t
AS_02::JP2K::MXFReader::h__Reader::OpenRead(const char* filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(CDCIEssenceDescriptor), &tmp_iobj);
      m_CDCIEssenceDescriptor = static_cast<CDCIEssenceDescriptor*>(tmp_iobj);

      if ( m_CDCIEssenceDescriptor == 0 )
	{
	  m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
	  m_RGBAEssenceDescriptor = static_cast<RGBAEssenceDescriptor*>(tmp_iobj);
	}

      if ( m_CDCIEssenceDescriptor == 0 && m_RGBAEssenceDescriptor == 0 )
	{
	  DefaultLogSink().Error("RGBAEssenceDescriptor nor CDCIEssenceDescriptor found.\n");
	  return RESULT_FORMAT;
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &tmp_iobj);
      m_EssenceSubDescriptor = static_cast<JPEG2000PictureSubDescriptor*>(tmp_iobj);

      if ( m_EssenceSubDescriptor == 0 )
	{
	  DefaultLogSink().Error("JPEG2000PictureSubDescriptor not found.\n");
	  return RESULT_FORMAT;
	}

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_FORMAT;
	}

      if ( m_CDCIEssenceDescriptor != 0 )
	{
	  m_EditRate = ((Track*)ObjectList.front())->EditRate;
	  m_SampleRate = m_CDCIEssenceDescriptor->SampleRate;
	  result = MD_to_JP2K_PDesc(*m_CDCIEssenceDescriptor, *m_EssenceSubDescriptor, m_EditRate, m_SampleRate, m_PDesc);
	}
      else if ( m_RGBAEssenceDescriptor != 0 )
	{
	  m_EditRate = ((Track*)ObjectList.front())->EditRate;
	  m_SampleRate = m_RGBAEssenceDescriptor->SampleRate;
	  result = MD_to_JP2K_PDesc(*m_RGBAEssenceDescriptor, *m_EssenceSubDescriptor, m_EditRate, m_SampleRate, m_PDesc);
	}

      if ( m_PDesc.ContainerDuration == 0 )
	{
	  m_PDesc.ContainerDuration = m_IndexAccess.GetDuration();
	}
    }

  return result;
}

//
//
Result_t
AS_02::JP2K::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, ASDCP::JP2K::FrameBuffer& FrameBuf,
		      ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEG2000Essence), Ctx, HMAC);
}

//------------------------------------------------------------------------------------------
//

AS_02::JP2K::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


AS_02::JP2K::MXFReader::~MXFReader()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::JP2K::MXFReader::OP1aHeader()
{
  if ( m_Reader.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
AS_02::MXF::AS02IndexReader&
AS_02::JP2K::MXFReader::AS02IndexReader()
{
  if ( m_Reader.empty() )
    {
      assert(g_AS02IndexReader);
      return *g_AS02IndexReader;
    }

  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
AS_02::JP2K::MXFReader::RIP()
{
  if ( m_Reader.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Reader->m_RIP;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
Result_t
AS_02::JP2K::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename);
}

//
Result_t
AS_02::JP2K::MXFReader::ReadFrame(ui32_t FrameNum, ASDCP::JP2K::FrameBuffer& FrameBuf,
					   ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
Result_t
AS_02::JP2K::MXFReader::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      PDesc = m_Reader->m_PDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
Result_t
AS_02::JP2K::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------

//
class AS_02::JP2K::MXFWriter::h__Writer : public AS_02::h__AS02Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

public:
  PictureDescriptor m_PDesc;
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  h__Writer(const Dictionary& d) : h__AS02Writer(d), m_EssenceSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const char*, EssenceType_t type, const AS_02::IndexStrategy_t& IndexStrategy,
		     const ui32_t& PartitionSpace, const ui32_t& HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&, const std::string& label,
			   ASDCP::Rational LocalEditRate = ASDCP::Rational(0,0));
  Result_t WriteFrame(const ASDCP::JP2K::FrameBuffer&, ASDCP::AESEncContext*, ASDCP::HMACContext*);
  Result_t Finalize();
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t
AS_02::JP2K::MXFWriter::h__Writer::OpenWrite(const char* filename, EssenceType_t type, const AS_02::IndexStrategy_t& IndexStrategy,
					     const ui32_t& PartitionSpace_sec, const ui32_t& HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  if ( m_IndexStrategy != AS_02::IS_FOLLOW )
    {
      DefaultLogSink().Error("Only strategy IS_FOLLOW is supported at this time.\n");
      return Kumu::RESULT_NOTIMPL;
    }

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_IndexStrategy = IndexStrategy;
      m_PartitionSpace = PartitionSpace_sec; // later converted to edit units by SetSourceStream()
      m_HeaderSize = HeaderSize;

      m_EssenceDescriptor = new RGBAEssenceDescriptor(m_Dict);
      m_EssenceSubDescriptor = new JPEG2000PictureSubDescriptor(m_Dict);
      m_EssenceSubDescriptorList.push_back((InterchangeObject*)m_EssenceSubDescriptor);

      GenRandomValue(m_EssenceSubDescriptor->InstanceUID);
      m_EssenceDescriptor->SubDescriptors.push_back(m_EssenceSubDescriptor->InstanceUID);
      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
Result_t
AS_02::JP2K::MXFWriter::h__Writer::SetSourceStream(const PictureDescriptor& PDesc, const std::string& label, ASDCP::Rational LocalEditRate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( LocalEditRate == ASDCP::Rational(0,0) )
    LocalEditRate = PDesc.EditRate;

  m_PDesc = PDesc;
  assert(m_Dict);
  Result_t result = JP2K_PDesc_to_MD(m_PDesc, *m_Dict,
				     static_cast<ASDCP::MXF::RGBAEssenceDescriptor*>(m_EssenceDescriptor),
				     m_EssenceSubDescriptor);

  static_cast<ASDCP::MXF::RGBAEssenceDescriptor*>(m_EssenceDescriptor)->ComponentMaxRef = 4095; /// TODO: set with magic or some such thing
  static_cast<ASDCP::MXF::RGBAEssenceDescriptor*>(m_EssenceDescriptor)->ComponentMinRef = 0;

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, m_Dict->ul(MDD_JPEG2000Essence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t TCFrameRate = ( m_PDesc.EditRate == EditRate_23_98  ) ? 24 : m_PDesc.EditRate.Numerator;

      result = WriteAS02Header(label, UL(m_Dict->ul(MDD_JPEG_2000Wrapping)),
			       PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
			       LocalEditRate, TCFrameRate);
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
Result_t
AS_02::JP2K::MXFWriter::h__Writer::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf,
		       AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through
 
  ui64_t StreamOffset = m_StreamOffset; // m_StreamOffset will be changed by the call to WriteEKLVPacket

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = StreamOffset;
      m_IndexWriter.PushIndexEntry(Entry);
    }

  m_FramesWritten++;
  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
Result_t
AS_02::JP2K::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  Result_t result = m_State.Goto_FINAL();

  if ( ASDCP_SUCCESS(result) )
    result = WriteAS02Footer();

  return result;
}


//------------------------------------------------------------------------------------------



AS_02::JP2K::MXFWriter::MXFWriter()
{
}

AS_02::JP2K::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::JP2K::MXFWriter::OP1aHeader()
{
  if ( m_Writer.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
AS_02::JP2K::MXFWriter::RIP()
{
  if ( m_Writer.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Writer->m_RIP;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t
AS_02::JP2K::MXFWriter::OpenWrite(const char* filename, const ASDCP::WriterInfo& Info,
				  const ASDCP::JP2K::PictureDescriptor& PDesc,
				  const IndexStrategy_t& Strategy,
				  const ui32_t& PartitionSpace,
				  const ui32_t& HeaderSize)
{
  m_Writer = new AS_02::JP2K::MXFWriter::h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, ASDCP::ESS_JPEG_2000, Strategy, PartitionSpace, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(PDesc, JP2K_PACKAGE_LABEL);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
Result_t 
AS_02::JP2K::MXFWriter::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
Result_t
AS_02::JP2K::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_02_JP2K.cpp
//
