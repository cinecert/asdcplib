/*
Copyright (c) 2018-2021, John Hurst

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
/*! \file    AS_02_ISXD.cpp
  \version $Id$
  \brief   AS-02 library, ISXD (RDD 47) essence reader and writer implementation
*/

#include "AS_02_internal.h"
#include "AS_02.h"

#include <iostream>
#include <iomanip>

using namespace ASDCP;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string AUXDATA_PACKAGE_LABEL = "File Package: RDD 47 frame wrapping of ISXD data";
static std::string PICT_DEF_LABEL = "Isochronous Stream of XML Documents Track";

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of Aux Data reader


class AS_02::ISXD::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  h__Reader(const Dictionary* d, const Kumu::IFileReaderFactory& fileReaderFactory) :
    AS_02::h__AS02Reader(d, fileReaderFactory) {}

  virtual ~h__Reader() {}

  Result_t    OpenRead(const std::string& filename);
  Result_t    ReadFrame(ui32_t, ASDCP::FrameBuffer&, AESDecContext*, HMACContext*);
};

//
Result_t
AS_02::ISXD::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename);

  if( KM_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(ISXDDataEssenceDescriptor), &tmp_iobj);

      if ( tmp_iobj == 0 )
	{
	  DefaultLogSink().Error("ISXDDataEssenceDescriptor not found.\n");
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(ISXDDataEssenceDescriptor), &tmp_iobj);

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_AS02_FORMAT;
	}
    }


  return result;
}

//
Result_t
AS_02::ISXD::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
		      ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_File->IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_FrameWrappedISXDData), Ctx, HMAC);
}



//------------------------------------------------------------------------------------------
//

AS_02::ISXD::MXFReader::MXFReader(const Kumu::IFileReaderFactory& fileReaderFactory)
{
  m_Reader = new h__Reader(&DefaultCompositeDict(), fileReaderFactory);
}


AS_02::ISXD::MXFReader::~MXFReader()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::ISXD::MXFReader::OP1aHeader()
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
AS_02::ISXD::MXFReader::AS02IndexReader()
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
AS_02::ISXD::MXFReader::RIP()
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
//
Result_t
AS_02::ISXD::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

//
Result_t
AS_02::ISXD::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
Result_t
AS_02::ISXD::MXFReader::ReadFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
					   ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);
    }
  
  return RESULT_INIT;
}

//
Result_t
AS_02::ISXD::MXFReader::ReadGenericStreamPartitionPayload(const ui32_t SID, ASDCP::FrameBuffer& frame_buf)
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      return m_Reader->ReadGenericStreamPartitionPayload(SID, frame_buf, 0, 0 /*no encryption*/);
    }

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
Result_t
AS_02::ISXD::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
void
AS_02::ISXD::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      m_Reader->m_HeaderPart.Dump(stream);
    }
}


//
void
AS_02::ISXD::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      m_Reader->m_IndexAccess.Dump(stream);
    }
}

//------------------------------------------------------------------------------------------

//
class AS_02::ISXD::MXFWriter::h__Writer : public AS_02::h__AS02WriterFrame
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  byte_t m_EssenceUL[SMPTE_UL_LENGTH];
  ISXDDataEssenceDescriptor *m_DataEssenceDescriptor;

  h__Writer(const Dictionary *d) : h__AS02WriterFrame(d) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
		     const std::string& isxd_document_namespace,
		     const ASDCP::Rational& edit_rate,
		     const AS_02::IndexStrategy_t& IndexStrategy,
		     const ui32_t& PartitionSpace, const ui32_t& HeaderSize);
  Result_t SetSourceStream(const std::string& label, const ASDCP::Rational& edit_rate);
  Result_t WriteFrame(const ASDCP::FrameBuffer&, ASDCP::AESEncContext*, ASDCP::HMACContext*);
  Result_t Finalize();
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t
AS_02::ISXD::MXFWriter::h__Writer::OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
					     const std::string& isxd_document_namespace,
					     const ASDCP::Rational& edit_rate,
					     const AS_02::IndexStrategy_t& IndexStrategy,
					     const ui32_t& PartitionSpace_sec, const ui32_t& HeaderSize)
{
  m_DataEssenceDescriptor = new ISXDDataEssenceDescriptor(m_Dict);
  m_DataEssenceDescriptor->DataEssenceCoding = m_Dict->ul(MDD_UTF_8_Text_DataEssenceCoding);
  m_DataEssenceDescriptor->SampleRate = edit_rate;
  m_DataEssenceDescriptor->NamespaceURI = isxd_document_namespace;

  if ( ! m_State.Test_BEGIN() )
    {
      KM_RESULT_STATE_HERE();
	return RESULT_STATE;
    }

  if ( m_IndexStrategy != AS_02::IS_FOLLOW )
    {
      DefaultLogSink().Error("Only strategy IS_FOLLOW is supported at this time.\n");
      return Kumu::RESULT_NOTIMPL;
    }

  Result_t result = m_File.OpenWrite(filename);

  if ( KM_SUCCESS(result) )
    {
      m_IndexStrategy = IndexStrategy;
      m_PartitionSpace = PartitionSpace_sec; // later converted to edit units by SetSourceStream()
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = m_DataEssenceDescriptor;
      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first ISXD data fragment stream.
Result_t
AS_02::ISXD::MXFWriter::h__Writer::SetSourceStream(const std::string& label, const ASDCP::Rational& edit_rate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    {
      KM_RESULT_STATE_HERE();
	return RESULT_STATE;
    }

  memcpy(m_EssenceUL, m_Dict->ul(MDD_FrameWrappedISXDData), SMPTE_UL_LENGTH);
  m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
  Result_t result = m_State.Goto_READY();

  if ( KM_SUCCESS(result) )
    {
      result = WriteAS02Header(label, UL(m_Dict->ul(MDD_FrameWrappedISXDContainer)),
			       PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_DataDataDef)),
			       edit_rate);

      if ( KM_SUCCESS(result) )
	{
	  this->m_IndexWriter.SetPrimerLookup(&this->m_HeaderPart.m_Primer);
	}
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
Result_t
AS_02::ISXD::MXFWriter::h__Writer::WriteFrame(const ASDCP::FrameBuffer& FrameBuf,
					      AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("The frame buffer size is zero.\n");
      return RESULT_PARAM;
    }

  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    {
      result = m_State.Goto_RUNNING(); // first time through
    }

  if ( KM_SUCCESS(result) )
    {
      result = WriteEKLVPacket(FrameBuf, m_EssenceUL, MXF_BER_LENGTH, Ctx, HMAC);
      m_FramesWritten++;
    }

  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
Result_t
AS_02::ISXD::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    {
      KM_RESULT_STATE_HERE();
	return RESULT_STATE;
    }

  Result_t result = m_State.Goto_FINAL();

  if ( KM_SUCCESS(result) )
    {
      result = WriteAS02Footer();
    }

  return result;
}

//------------------------------------------------------------------------------------------



AS_02::ISXD::MXFWriter::MXFWriter()
{
}

AS_02::ISXD::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::ISXD::MXFWriter::OP1aHeader()
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
AS_02::ISXD::MXFWriter::RIP()
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
AS_02::ISXD::MXFWriter::OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
				     const std::string& isxd_document_namespace,
				     const ASDCP::Rational& edit_rate, const ui32_t& header_size,
				     const IndexStrategy_t& strategy, const ui32_t& partition_space)
{
  m_Writer = new AS_02::ISXD::MXFWriter::h__Writer(&DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, Info, isxd_document_namespace, edit_rate,
					strategy, partition_space, header_size);

  if ( KM_SUCCESS(result) )
    result = m_Writer->SetSourceStream(AUXDATA_PACKAGE_LABEL, edit_rate);

  if ( KM_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
Result_t 
AS_02::ISXD::MXFWriter::WriteFrame(const ASDCP::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

Result_t 
AS_02::ISXD::MXFWriter::AddDmsGenericPartUtf8Text(const ASDCP::FrameBuffer& FrameBuf, ASDCP::AESEncContext* Ctx,
						  ASDCP::HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;
  
  m_Writer->FlushIndexPartition();
  return m_Writer->AddDmsGenericPartUtf8Text(FrameBuf, Ctx, HMAC);
}


// Closes the MXF file, writing the index and other closing information.
Result_t
AS_02::ISXD::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_02_ISXD.cpp
//
