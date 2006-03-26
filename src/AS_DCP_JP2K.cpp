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

static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE XXXM frame wrapping of JPEG 2000 codestreams";
static std::string PICT_DEF_LABEL = "Picture Track";

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
    fprintf(stream, "Quantization Default (%lu): %s\n",
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
  RGBAEssenceDescriptor*        m_EssenceDescriptor;
  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  h__Reader() : m_EssenceDescriptor(0), m_EssenceSubDescriptor(0) {}
  Result_t    OpenRead(const char*);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    ReadFrameGOPStart(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    MD_to_JP2K_PDesc(JP2K::PictureDescriptor& PDesc);
};

//
ASDCP::Result_t
ASDCP::JP2K::MXFReader::h__Reader::MD_to_JP2K_PDesc(JP2K::PictureDescriptor& PDesc)
{
  memset(&PDesc, 0, sizeof(PDesc));
  MXF::RGBAEssenceDescriptor* PDescObj = (MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

  PDesc.EditRate           = PDescObj->SampleRate;
  PDesc.ContainerDuration  = PDescObj->ContainerDuration;
  PDesc.StoredWidth        = PDescObj->StoredWidth;
  PDesc.StoredHeight       = PDescObj->StoredHeight;
  PDesc.AspectRatio        = PDescObj->AspectRatio;

  if ( m_EssenceSubDescriptor != 0 )
    {
      PDesc.Rsize   = m_EssenceSubDescriptor->Rsize;
      PDesc.Xsize   = m_EssenceSubDescriptor->Xsize;
      PDesc.Ysize   = m_EssenceSubDescriptor->Ysize;
      PDesc.XOsize  = m_EssenceSubDescriptor->XOsize;
      PDesc.YOsize  = m_EssenceSubDescriptor->YOsize;
      PDesc.XTsize  = m_EssenceSubDescriptor->XTsize;
      PDesc.YTsize  = m_EssenceSubDescriptor->YTsize;
      PDesc.XTOsize = m_EssenceSubDescriptor->XTOsize;
      PDesc.YTOsize = m_EssenceSubDescriptor->YTOsize;
      PDesc.Csize   = m_EssenceSubDescriptor->Csize;

      // PictureComponentSizing
      ui32_t tmp_size = m_EssenceSubDescriptor->PictureComponentSizing.Size();

      if ( tmp_size == 17 ) // ( 2 * sizeof(ui32_t) ) + 3 components * 3 byte each
	memcpy(&PDesc.ImageComponents, m_EssenceSubDescriptor->PictureComponentSizing.RoData() + 8, tmp_size - 8);

      else
	DefaultLogSink().Error("Unexpected PictureComponentSizing size: %lu, should be 17\n", tmp_size);

      // CodingStyleDefault
      if ( ( PDesc.CodingStyleLength = m_EssenceSubDescriptor->CodingStyleDefault.Size() ) != 0 )
	memcpy(PDesc.CodingStyle, m_EssenceSubDescriptor->CodingStyleDefault.RoData(), PDesc.CodingStyleLength);

      // QuantizationDefault
      if ( ( PDesc.QuantDefaultLength = m_EssenceSubDescriptor->QuantizationDefault.Size() ) != 0 )
	memcpy(PDesc.QuantDefault, m_EssenceSubDescriptor->QuantizationDefault.RoData(), PDesc.QuantDefaultLength);
    }

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
      if ( m_EssenceDescriptor == 0 )
	{
	  m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), (InterchangeObject**)&m_EssenceDescriptor);
	  m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), (InterchangeObject**)&m_EssenceSubDescriptor);
	}

      result = MD_to_JP2K_PDesc(m_PDesc);
    }

  if( ASDCP_SUCCESS(result) )
    result = InitMXFIndex();

  if( ASDCP_SUCCESS(result) )
    result = InitInfo();

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

  return ReadEKLVPacket(FrameNum, FrameBuf, Dict::ul(MDD_JPEG2000Essence), Ctx, HMAC);
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
  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

public:
  PictureDescriptor m_PDesc;
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  ASDCP_NO_COPY_CONSTRUCT(h__Writer);

  h__Writer() : m_EssenceSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  ~h__Writer(){}

  Result_t OpenWrite(const char*, ui32_t HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
  Result_t JP2K_PDesc_to_MD(JP2K::PictureDescriptor& PDesc);
};


//
ASDCP::Result_t
ASDCP::JP2K::MXFWriter::h__Writer::JP2K_PDesc_to_MD(JP2K::PictureDescriptor& PDesc)
{
  assert(m_EssenceDescriptor);
  assert(m_EssenceSubDescriptor);
  MXF::RGBAEssenceDescriptor* PDescObj = (MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

  PDescObj->Codec.Set(Dict::ul(MDD_JP2KEssenceCompression));
  PDescObj->SampleRate = PDesc.EditRate;
  PDescObj->ContainerDuration = PDesc.ContainerDuration;
  PDescObj->StoredWidth = PDesc.StoredWidth;
  PDescObj->StoredHeight = PDesc.StoredHeight;
  PDescObj->AspectRatio = PDesc.AspectRatio;
  PDescObj->FrameLayout = 0;

  m_EssenceSubDescriptor->Rsize = PDesc.Rsize;
  m_EssenceSubDescriptor->Xsize = PDesc.Xsize;
  m_EssenceSubDescriptor->Ysize = PDesc.Ysize;
  m_EssenceSubDescriptor->XOsize = PDesc.XOsize;
  m_EssenceSubDescriptor->YOsize = PDesc.YOsize;
  m_EssenceSubDescriptor->XTsize = PDesc.XTsize;
  m_EssenceSubDescriptor->YTsize = PDesc.YTsize;
  m_EssenceSubDescriptor->XTOsize = PDesc.XTOsize;
  m_EssenceSubDescriptor->YTOsize = PDesc.YTOsize;
  m_EssenceSubDescriptor->Csize = PDesc.Csize;

  const ui32_t tmp_buffer_len = 64;
  byte_t tmp_buffer[tmp_buffer_len];

  *(ui32_t*)tmp_buffer = ASDCP_i32_BE(3L); // three components
  *(ui32_t*)(tmp_buffer+4) = ASDCP_i32_BE(3L);
  memcpy(tmp_buffer + 8, &PDesc.ImageComponents, sizeof(ASDCP::JP2K::ImageComponent) * 3L);

  memcpy(m_EssenceSubDescriptor->PictureComponentSizing.Data(), tmp_buffer, 17);
  m_EssenceSubDescriptor->PictureComponentSizing.Size(17);

  memcpy(m_EssenceSubDescriptor->CodingStyleDefault.Data(), PDesc.CodingStyle, PDesc.CodingStyleLength);
  m_EssenceSubDescriptor->CodingStyleDefault.Size(PDesc.CodingStyleLength);

  memcpy(m_EssenceSubDescriptor->QuantizationDefault.Data(), PDesc.QuantDefault, PDesc.QuantDefaultLength);
  m_EssenceSubDescriptor->QuantizationDefault.Size(PDesc.QuantDefaultLength);

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
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new RGBAEssenceDescriptor;
      m_EssenceSubDescriptor = new JPEG2000PictureSubDescriptor;
      m_HeaderPart.AddChildObject(m_EssenceSubDescriptor);
      m_EssenceDescriptor->SubDescriptors.push_back(m_EssenceSubDescriptor->InstanceUID);
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
  Result_t result = JP2K_PDesc_to_MD(m_PDesc);

  if ( ASDCP_SUCCESS(result) )
      result = WriteMXFHeader(JP2K_PACKAGE_LABEL, UL(Dict::ul(MDD_JPEG_2000Wrapping)),
			      PICT_DEF_LABEL,     UL(Dict::ul(MDD_PictureDataDef)),
			      m_PDesc.EditRate, 24 /* TCFrameRate */);

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, Dict::ul(MDD_JPEG2000Essence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

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
 
  IndexTableSegment::IndexEntry Entry;
  Entry.StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    {  
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
