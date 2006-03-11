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
/*! \file    AS_DCP_JP2k.cpp
    \version $Id$
    \brief   AS-DCP library, JPEG 2000 essence reader and writer implementation
*/

#include "AS_DCP_internal.h"


//------------------------------------------------------------------------------------------

//
const byte_t JP2KEssenceCompressionLabel[klv_key_size] =
{
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x09,
  0x04, 0x01, 0x02, 0x02, 0x03, 0x01, 0x01, 0x01 };


//
void
ASDCP::JP2K::PictureDescriptorDump(const PictureDescriptor& PDesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
      AspectRatio: %lu/%lu\n\
         EditRate: %lu/%lu\n\
      StoredWidth: %lu\n\
     StoredHeight: %lu\n\
            Rsize: %lu\n\
            Xsize: %lu\n\
            Ysize: %lu\n\
           XOsize: %lu\n\
           YOsize: %lu\n\
           XTsize: %lu\n\
           YTsize: %lu\n\
          XTOsize: %lu\n\
          YTOsize: %lu\n\
ContainerDuration: %lu\n",
	  PDesc.AspectRatio.Numerator ,PDesc.AspectRatio.Denominator,
	  PDesc.EditRate.Numerator ,PDesc.EditRate.Denominator,
	  PDesc.StoredWidth,
	  PDesc.StoredHeight,
	  PDesc.Rsize,
	  PDesc.Xsize,
	  PDesc.Ysize,
	  PDesc.XOsize,
	  PDesc.YOsize,
	  PDesc.XTsize,
	  PDesc.YTsize,
	  PDesc.XTOsize,
	  PDesc.YTOsize,
	  PDesc.ContainerDuration
	  );

  fprintf(stream, "Color Components:\n");

  for ( ui32_t i = 0; i < PDesc.Csize; i++ )
    {
      fprintf(stream, "  %lu.%lu.%lu\n",
	      PDesc.ImageComponents[i].Ssize,
	      PDesc.ImageComponents[i].XRsize,
	      PDesc.ImageComponents[i].YRsize
	      );
    }

  const ui32_t tmp_buf_len = 256;
  char tmp_buf[tmp_buf_len];

  if ( PDesc.CodingStyleLength )
    fprintf(stream, "Default Coding (%lu): %s\n",
	    PDesc.CodingStyleLength,
	    bin2hex(PDesc.CodingStyle, PDesc.CodingStyleLength,
		    tmp_buf, tmp_buf_len)
	    );

  if ( PDesc.QuantDefaultLength )
    fprintf(stream, "Default Coding (%lu): %s\n",
	    PDesc.QuantDefaultLength,
	    bin2hex(PDesc.QuantDefault, PDesc.QuantDefaultLength,
		    tmp_buf, tmp_buf_len)
	    );
}

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader

class ASDCP::JP2K::MXFReader::h__Reader : public ASDCP::h__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  h__Reader() {}
  Result_t    OpenRead(const char*);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    ReadFrameGOPStart(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    MD_to_JP2K_PDesc(MXF::RGBAEssenceDescriptor* PDescObj, JP2K::PictureDescriptor& PDesc);
};

//
ASDCP::Result_t
ASDCP::JP2K::MXFReader::h__Reader::MD_to_JP2K_PDesc(MXF::RGBAEssenceDescriptor* PDescObj, JP2K::PictureDescriptor& PDesc)
{
  ASDCP_TEST_NULL(PDescObj);
  memset(&PDesc, 0, sizeof(PDesc));

  PDesc.EditRate           = PDescObj->SampleRate;
  PDesc.ContainerDuration  = PDescObj->ContainerDuration;
  PDesc.StoredWidth        = PDescObj->StoredWidth;
  PDesc.StoredHeight       = PDescObj->StoredHeight;
  PDesc.AspectRatio        = PDescObj->AspectRatio;

  InterchangeObject* MDObject;
  if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &MDObject)) )
    {
      if ( MDObject == 0 )
	{
	  DefaultLogSink().Error("Unable to locate JPEG2000PictureSubDescriptor");
	  return RESULT_FALSE;
	}

      JPEG2000PictureSubDescriptor* PSubDescObj = (JPEG2000PictureSubDescriptor*)MDObject;

      PDesc.Rsize   = PSubDescObj->Rsize;
      PDesc.Xsize   = PSubDescObj->Xsize;
      PDesc.Ysize   = PSubDescObj->Ysize;
      PDesc.XOsize  = PSubDescObj->XOsize;
      PDesc.YOsize  = PSubDescObj->YOsize;
      PDesc.XTsize  = PSubDescObj->XTsize;
      PDesc.YTsize  = PSubDescObj->YTsize;
      PDesc.XTOsize = PSubDescObj->XTOsize;
      PDesc.YTOsize = PSubDescObj->YTOsize;
      PDesc.Csize   = PSubDescObj->Csize;
    }

#if 0
      // PictureComponentSizing

      if ( DC3.Size == 17 ) // ( 2* sizeof(ui32_t) ) + 3 components * 3 byte each
	{
	  memcpy(&PDesc.ImageComponents, DC3.Data + 8, DC3.Size - 8);
	}
      else
	{
	  DefaultLogSink().Error("Unexpected PictureComponentSizing size: %lu, should be 17\n", DC3.Size);
	}
#endif

  // CodingStyleDefault
      //      PDesc.CodingStyleLength = DC1.Size;
      //      memcpy(PDesc.CodingStyle, DC1.Data, DC1.Size);

  // QuantizationDefault
      //      PDesc.QuantDefaultLength = DC2.Size;
      //      memcpy(PDesc.QuantDefault, DC2.Data, DC2.Size);

  return RESULT_OK;
}

//
//
ASDCP::Result_t
ASDCP::JP2K::MXFReader::h__Reader::OpenRead(const char* filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* Object;
      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &Object)) )
	{
	  assert(Object);
	  result = MD_to_JP2K_PDesc((MXF::RGBAEssenceDescriptor*)Object, m_PDesc);
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
ASDCP::JP2K::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
					      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  return ReadEKLVPacket(FrameNum, FrameBuf, JP2KEssenceUL_Data, Ctx, HMAC);
}

//------------------------------------------------------------------------------------------


//
void
ASDCP::JP2K::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06lu, %7lu bytes", m_FrameNumber, m_Size);
  
  fputc('\n', stream);

  if ( dump_len > 0 )
    hexdump(m_Data, dump_len, stream);
}


//------------------------------------------------------------------------------------------

ASDCP::JP2K::MXFReader::MXFReader()
{
  m_Reader = new h__Reader;
}


ASDCP::JP2K::MXFReader::~MXFReader()
{
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::JP2K::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename);
}

//
ASDCP::Result_t
ASDCP::JP2K::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JP2K::MXFReader::FillPictureDescriptor(PictureDescriptor& PDesc) const
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
ASDCP::Result_t
ASDCP::JP2K::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::JP2K::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::JP2K::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_FooterPart.Dump(stream);
}


//------------------------------------------------------------------------------------------



//
class ASDCP::JP2K::MXFWriter::h__Writer : public ASDCP::h__Writer
{
public:
  PictureDescriptor m_PDesc;
  ui32_t            m_GOPOffset;

  ASDCP_NO_COPY_CONSTRUCT(h__Writer);

  h__Writer() : m_GOPOffset(0) {}
  ~h__Writer(){}

  Result_t OpenWrite(const char*, ui32_t HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
  Result_t JP2K_PDesc_to_MD(JP2K::PictureDescriptor& PDesc, MXF::RGBAEssenceDescriptor* PDescObj);
};


//
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::h__Writer::JP2K_PDesc_to_MD(JP2K::PictureDescriptor& PDesc, MXF::RGBAEssenceDescriptor* PDescObj)
{
  // Codec
  PDescObj->SampleRate = PDesc.EditRate;
  PDescObj->ContainerDuration = PDesc.ContainerDuration;
  PDescObj->StoredWidth = PDesc.StoredWidth;
  PDescObj->StoredHeight = PDesc.StoredHeight;
  PDescObj->AspectRatio = PDesc.AspectRatio;
  PDescObj->FrameLayout = 0;

  InterchangeObject* MDObject;
  if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &MDObject)) )
    {
      assert(MDObject);
      JPEG2000PictureSubDescriptor* PSubDescObj = (JPEG2000PictureSubDescriptor*)MDObject;

      PSubDescObj->Rsize = PDesc.Rsize;
      PSubDescObj->Xsize = PDesc.Xsize;
      PSubDescObj->Ysize = PDesc.Ysize;
      PSubDescObj->XOsize = PDesc.XOsize;
      PSubDescObj->YOsize = PDesc.YOsize;
      PSubDescObj->XTsize = PDesc.XTsize;
      PSubDescObj->YTsize = PDesc.YTsize;
      PSubDescObj->XTOsize = PDesc.XTOsize;
      PSubDescObj->YTOsize = PDesc.YTOsize;
      PSubDescObj->Csize = PDesc.Csize;
    }
#if 0
  const ui32_t tmp_buffer_len = 64;
  byte_t tmp_buffer[tmp_buffer_len];

  *(ui32_t*)tmp_buffer = ASDCP_i32_BE(3L); // three components
  *(ui32_t*)(tmp_buffer+4) = ASDCP_i32_BE(3L);
  memcpy(tmp_buffer + 8, &PDesc.ImageComponents, sizeof(ASDCP::JP2K::ImageComponent) * 3L);

  PSubDescObj->SetValue("PictureComponentSizing", DataChunk(17, tmp_buffer));
  PSubDescObj->SetValue("CodingStyleDefault", DataChunk(PDesc.CodingStyleLength, PDesc.CodingStyle));
  PSubDescObj->SetValue("QuantizationDefault", DataChunk(PDesc.QuantDefaultLength, PDesc.QuantDefault));
#endif

  return RESULT_OK;
}


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::h__Writer::OpenWrite(const char* filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      RGBAEssenceDescriptor* rgbDesc = new RGBAEssenceDescriptor;

      JPEG2000PictureSubDescriptor* jp2kSubDesc = new JPEG2000PictureSubDescriptor;
      m_HeaderPart.AddChildObject(jp2kSubDesc);
      rgbDesc->SubDescriptors.push_back(jp2kSubDesc->InstanceUID);

      m_EssenceDescriptor = rgbDesc;
      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::h__Writer::SetSourceStream(const PictureDescriptor& PDesc)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  m_PDesc = PDesc;
  Result_t result = JP2K_PDesc_to_MD(m_PDesc, (RGBAEssenceDescriptor*)m_EssenceDescriptor);

  if ( ASDCP_SUCCESS(result) )
      result = WriteMXFHeader(JP2K_PACKAGE_LABEL,
			      UL(WrappingUL_Data_JPEG_2000),
			      m_PDesc.EditRate, 24 /* TCFrameRate */);

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
ASDCP::JP2K::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx,
					       HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through

  fpos_t ThisOffset = m_StreamOffset;
 
  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, JP2KEssenceUL_Data, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = ThisOffset - m_FooterPart.m_ECOffset;
      m_FooterPart.PushIndexEntry(Entry);
      m_FramesWritten++;
    }

  return result;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteMXFFooter();
}


//------------------------------------------------------------------------------------------



ASDCP::JP2K::MXFWriter::MXFWriter()
{
}

ASDCP::JP2K::MXFWriter::~MXFWriter()
{
}


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::OpenWrite(const char* filename, const WriterInfo& Info,
				  const PictureDescriptor& PDesc, ui32_t HeaderSize)
{
  m_Writer = new h__Writer;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      m_Writer->m_Info = Info;
      result = m_Writer->SetSourceStream(PDesc);
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
ASDCP::JP2K::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_DCP_JP2K.cpp
//
