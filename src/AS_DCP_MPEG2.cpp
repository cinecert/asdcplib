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
/*! \file    AS_DCP_MPEG2.cpp
    \version $Id$       
    \brief   AS-DCP library, MPEG2 essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include "MDD.h"


//------------------------------------------------------------------------------------------

//
ASDCP::Result_t
ASDCP::MD_to_MPEG2_VDesc(MXF::MPEG2VideoDescriptor* VDescObj, MPEG2::VideoDescriptor& VDesc)
{
  ASDCP_TEST_NULL(VDescObj);

  VDesc.SampleRate             = VDescObj->SampleRate;
  VDesc.EditRate               = VDescObj->SampleRate;
  VDesc.FrameRate              = VDescObj->SampleRate.Numerator;
  VDesc.ContainerDuration      = VDescObj->ContainerDuration;

  VDesc.FrameLayout            = VDescObj->FrameLayout;
  VDesc.StoredWidth            = VDescObj->StoredWidth;
  VDesc.StoredHeight           = VDescObj->StoredHeight;
  VDesc.AspectRatio            = VDescObj->AspectRatio;

  VDesc.ComponentDepth         = VDescObj->ComponentDepth;
  VDesc.HorizontalSubsampling  = VDescObj->HorizontalSubsampling;
  VDesc.VerticalSubsampling    = VDescObj->VerticalSubsampling;
  VDesc.ColorSiting            = VDescObj->ColorSiting;
  VDesc.CodedContentType       = VDescObj->CodedContentType;

  VDesc.LowDelay               = VDescObj->LowDelay;
  VDesc.BitRate                = VDescObj->BitRate;
  VDesc.ProfileAndLevel        = VDescObj->ProfileAndLevel;
  return RESULT_OK;
}


//
ASDCP::Result_t
ASDCP::MPEG2_VDesc_to_MD(MPEG2::VideoDescriptor&, MXF::MPEG2VideoDescriptor*)
{
  return RESULT_OK;
}


//
void
ASDCP::MPEG2::VideoDescriptorDump(const VideoDescriptor& VDesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
           SampleRate: %lu/%lu\n\
          FrameLayout: %lu\n\
          StoredWidth: %lu\n\
         StoredHeight: %lu\n\
          AspectRatio: %lu/%lu\n\
       ComponentDepth: %lu\n\
HorizontalSubsampling: %lu\n\
  VerticalSubsampling: %lu\n\
          ColorSiting: %lu\n\
     CodedContentType: %lu\n\
             LowDelay: %lu\n\
              BitRate: %lu\n\
      ProfileAndLevel: %lu\n\
    ContainerDuration: %lu\n",
	  VDesc.SampleRate.Numerator ,VDesc.SampleRate.Denominator,
	  VDesc.FrameLayout,
	  VDesc.StoredWidth,
	  VDesc.StoredHeight,
	  VDesc.AspectRatio.Numerator ,VDesc.AspectRatio.Denominator,
	  VDesc.ComponentDepth,
	  VDesc.HorizontalSubsampling,
	  VDesc.VerticalSubsampling,
	  VDesc.ColorSiting,
	  VDesc.CodedContentType,
	  VDesc.LowDelay,
	  VDesc.BitRate,
	  VDesc.ProfileAndLevel,
	  VDesc.ContainerDuration
	  );
}

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of MPEG2 reader

class ASDCP::MPEG2::MXFReader::h__Reader : public ASDCP::h__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  VideoDescriptor m_VDesc;        // video parameter list

  h__Reader() {}
  ~h__Reader() {}
  Result_t    OpenRead(const char*);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    ReadFrameGOPStart(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    FindFrameGOPStart(ui32_t, ui32_t&);
};


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::OpenRead(const char* filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* Object;
      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(MPEG2VideoDescriptor), &Object)) )
	{
	  assert(Object);
	  result = MD_to_MPEG2_VDesc((MXF::MPEG2VideoDescriptor*)Object, m_VDesc);
	}
    }

  if( ASDCP_SUCCESS(result) )
    result = InitMXFIndex();

  if( ASDCP_SUCCESS(result) )
    result = InitInfo(m_Info);

  return result;
}


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::ReadFrameGOPStart(ui32_t FrameNum, FrameBuffer& FrameBuf,
						      AESDecContext* Ctx, HMACContext* HMAC)
{
  ui32_t KeyFrameNum;

  Result_t result = FindFrameGOPStart(FrameNum, KeyFrameNum);

  if ( ASDCP_SUCCESS(result) )
    result = ReadFrame(KeyFrameNum, FrameBuf, Ctx, HMAC);

  return result;
}


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::FindFrameGOPStart(ui32_t FrameNum, ui32_t& KeyFrameNum)
{
  KeyFrameNum = 0;

  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  // look up frame index node
  IndexTableSegment::IndexEntry TmpEntry;

  if ( ASDCP_FAILURE(m_FooterPart.Lookup(FrameNum, TmpEntry)) )
    {
      DefaultLogSink().Error("Frame value out of range: %lu\n", FrameNum);
      return RESULT_RANGE;
    }

  KeyFrameNum = FrameNum - TmpEntry.KeyFrameOffset;

  return RESULT_OK;
}


//
//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
					      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  Result_t result = ReadEKLVPacket(FrameNum, FrameBuf, MPEGEssenceUL_Data, Ctx, HMAC);

  if ( ASDCP_FAILURE(result) )
    return result;

  IndexTableSegment::IndexEntry TmpEntry;
  m_FooterPart.Lookup(FrameNum, TmpEntry);

  switch ( ( TmpEntry.Flags >> 4 ) & 0x03 )
    {
    case 0:  FrameBuf.FrameType(FRAME_I); break;
    case 2:  FrameBuf.FrameType(FRAME_P); break;
    case 3:  FrameBuf.FrameType(FRAME_B); break;
    default: FrameBuf.FrameType(FRAME_U);
    }

  FrameBuf.TemporalOffset(TmpEntry.TemporalOffset);
  FrameBuf.GOPStart(TmpEntry.Flags & 0x40 ? true : false);
  FrameBuf.ClosedGOP(TmpEntry.Flags & 0x80 ? true : false);

  return RESULT_OK;
}

//------------------------------------------------------------------------------------------


//
void
ASDCP::MPEG2::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06lu, %c%-2hu, %7lu bytes",
	  m_FrameNumber, FrameTypeChar(m_FrameType), m_TemporalOffset, m_Size);

  if ( m_GOPStart )
    fprintf(stream, " (start %s GOP)", ( m_ClosedGOP ? "closed" : "open"));
  
  fputc('\n', stream);

  if ( dump_len > 0 )
    hexdump(m_Data, dump_len, stream);
}


//------------------------------------------------------------------------------------------

ASDCP::MPEG2::MXFReader::MXFReader()
{
  m_Reader = new h__Reader;
}


ASDCP::MPEG2::MXFReader::~MXFReader()
{
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename);
}

//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::ReadFrameGOPStart(ui32_t FrameNum, FrameBuffer& FrameBuf,
					   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrameGOPStart(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


//
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FindFrameGOPStart(ui32_t FrameNum, ui32_t& KeyFrameNum) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->FindFrameGOPStart(FrameNum, KeyFrameNum);

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FillVideoDescriptor(VideoDescriptor& VDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      VDesc = m_Reader->m_VDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::MPEG2::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
void
ASDCP::MPEG2::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::MPEG2::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_FooterPart.Dump(stream);
}


//------------------------------------------------------------------------------------------

//
class ASDCP::MPEG2::MXFWriter::h__Writer : public ASDCP::h__Writer
{
public:
  VideoDescriptor m_VDesc;
  ui32_t          m_GOPOffset;

  ASDCP_NO_COPY_CONSTRUCT(h__Writer);

  h__Writer() : m_GOPOffset(0) {}
  ~h__Writer(){}

  Result_t OpenWrite(const char*, ui32_t HeaderSize);
  Result_t SetSourceStream(const VideoDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::OpenWrite(const char* filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      //      m_EssenceDescriptor = new MDObject("MPEG2VideoDescriptor");
      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the MPEG stream.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::SetSourceStream(const VideoDescriptor& VDesc)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  m_VDesc = VDesc;
  Result_t result = RESULT_OK; // MPEG2_VDesc_to_MD(m_VDesc, *m_EssenceDescriptor);

  if ( ASDCP_SUCCESS(result) )
    result = WriteMXFHeader(ESS_MPEG2_VES, m_VDesc.EditRate, 24 /* TCFrameRate */);

  if ( ASDCP_SUCCESS(result) )
    result = m_State.Goto_READY();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx,
					       HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through, get the body location

  ui64_t ThisOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, MPEGEssenceUL_Data, Ctx, HMAC);

  if ( ASDCP_FAILURE(result) )
    return result;

  // create mxflib flags
  int Flags = 0;

  switch ( FrameBuf.FrameType() )
    {
    case FRAME_I: Flags = 0x00; break;
    case FRAME_P: Flags = 0x22; break;
    case FRAME_B: Flags = 0x33; break;
    }

  if ( FrameBuf.GOPStart() )
    {
      m_GOPOffset = 0;
      Flags |= 0x40;

      if ( FrameBuf.ClosedGOP() )
	Flags |= 0x80;
    }

  // update the index manager
#if 0
  m_IndexMan->OfferEditUnit(0, m_FramesWritten, m_GOPOffset, Flags);
  m_IndexMan->OfferTemporalOffset(m_FramesWritten, m_GOPOffset - FrameBuf.TemporalOffset());
  m_IndexMan->OfferOffset(0, m_FramesWritten, ThisOffset);
#endif

  m_FramesWritten++;
  m_GOPOffset++;

  return RESULT_OK;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteMXFFooter(ESS_MPEG2_VES);
}


//------------------------------------------------------------------------------------------



ASDCP::MPEG2::MXFWriter::MXFWriter()
{
}

ASDCP::MPEG2::MXFWriter::~MXFWriter()
{
}


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::OpenWrite(const char* filename, const WriterInfo& Info,
				   const VideoDescriptor& VDesc, ui32_t HeaderSize)
{
  m_Writer = new h__Writer;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      m_Writer->m_Info = Info;
      result = m_Writer->SetSourceStream(VDesc);
    }

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}


// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::MPEG2::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_DCP_MPEG2.cpp
//
