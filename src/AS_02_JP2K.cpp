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

static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE ST 422 frame wrapping of JPEG 2000 codestreams";
static std::string PICT_DEF_LABEL = "Image Track";

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader


class AS_02::JP2K::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  RGBAEssenceDescriptor*        m_EssenceDescriptor;
  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;
  ASDCP::Rational               m_EditRate;
  ASDCP::Rational               m_SampleRate;
  EssenceType_t                 m_Format;

  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  h__Reader(const Dictionary& d) :
    AS_02::h__AS02Reader(d), m_EssenceDescriptor(0), m_EssenceSubDescriptor(0), m_Format(ESS_UNKNOWN) {}

  virtual ~h__Reader() {}

  Result_t    OpenRead(const char*, EssenceType_t);
  Result_t    ReadFrame(ui32_t, ASDCP::JP2K::FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    MD_to_JP2K_PDesc(ASDCP::JP2K::PictureDescriptor& PDesc);

  Result_t OpenMXFRead(const char* filename);
  // positions file before reading
  Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			 const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);

  // reads from current position
  Result_t ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
			  const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);

};

//
ASDCP::Result_t
AS_02::JP2K::MXFReader::h__Reader::MD_to_JP2K_PDesc(ASDCP::JP2K::PictureDescriptor& PDesc)
{
  memset(&PDesc, 0, sizeof(PDesc));
  ASDCP::MXF::RGBAEssenceDescriptor* PDescObj = (ASDCP::MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

  PDesc.EditRate           = m_EditRate;
  PDesc.SampleRate         = m_SampleRate;
  assert(PDescObj->ContainerDuration <= 0xFFFFFFFFL);
  PDesc.ContainerDuration  = (ui32_t) PDescObj->ContainerDuration;
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
      ui32_t tmp_size = m_EssenceSubDescriptor->PictureComponentSizing.Length();

      if ( tmp_size == 17 ) // ( 2 * sizeof(ui32_t) ) + 3 components * 3 byte each
	memcpy(&PDesc.ImageComponents, m_EssenceSubDescriptor->PictureComponentSizing.RoData() + 8, tmp_size - 8);

      else
	DefaultLogSink().Error("Unexpected PictureComponentSizing size: %u, should be 17\n", tmp_size);

      // CodingStyleDefault
      memset(&PDesc.CodingStyleDefault, 0, sizeof(CodingStyleDefault_t));
      memcpy(&PDesc.CodingStyleDefault,
	     m_EssenceSubDescriptor->CodingStyleDefault.RoData(),
	     m_EssenceSubDescriptor->CodingStyleDefault.Length());

      // QuantizationDefault
      memset(&PDesc.QuantizationDefault, 0, sizeof(QuantizationDefault_t));
      memcpy(&PDesc.QuantizationDefault,
	     m_EssenceSubDescriptor->QuantizationDefault.RoData(),
	     m_EssenceSubDescriptor->QuantizationDefault.Length());

      PDesc.QuantizationDefault.SPqcdLength = m_EssenceSubDescriptor->QuantizationDefault.Length() - 1;
    }

  return RESULT_OK;
}

//
//
ASDCP::Result_t
AS_02::JP2K::MXFReader::h__Reader::OpenRead(const char* filename, ASDCP::EssenceType_t type)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;
      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
      m_EssenceDescriptor = static_cast<RGBAEssenceDescriptor*>(tmp_iobj);

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &tmp_iobj);
      m_EssenceSubDescriptor = static_cast<JPEG2000PictureSubDescriptor*>(tmp_iobj);

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_FORMAT;
	}

      m_EditRate = ((Track*)ObjectList.front())->EditRate;
      m_SampleRate = m_EssenceDescriptor->SampleRate;

      if ( type == ASDCP::ESS_JPEG_2000 )
	{
	  if ( m_EditRate != m_SampleRate )
	    {
	      DefaultLogSink().Warn("EditRate and SampleRate do not match (%.03f, %.03f).\n",
				    m_EditRate.Quotient(), m_SampleRate.Quotient());

	      if ( m_EditRate == EditRate_24 && m_SampleRate == EditRate_48 )
		{
		  DefaultLogSink().Debug("File may contain JPEG Interop stereoscopic images.\n");
		  return RESULT_SFORMAT;
		}

	      return RESULT_FORMAT;
	    }
	}
      else
	{
	  DefaultLogSink().Error("'type' argument unexpected: %x\n", type);
	  return RESULT_STATE;
	}

      result = MD_to_JP2K_PDesc(m_PDesc);
    }

  return result;
}

//
//
ASDCP::Result_t
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
  return m_Reader->OpenRead(filename, ASDCP::ESS_JPEG_2000);
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
ASDCP::Result_t AS_02::JP2K::MXFReader::FillPictureDescriptor(PictureDescriptor& PDesc) const
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
ASDCP::Result_t AS_02::JP2K::MXFReader::FillWriterInfo(WriterInfo& Info) const
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

  //new attributes for AS-02 support 
  AS_02::IndexStrategy_t m_IndexStrategy; //Shim parameter index_strategy_frame/clip
  ui32_t m_PartitionSpace; //Shim parameter partition_spacing

  h__Writer(const Dictionary& d) : h__AS02Writer(d), m_EssenceSubDescriptor(0), m_IndexStrategy(AS_02::IS_FOLLOW), m_PartitionSpace(60) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const char*, EssenceType_t type, const AS_02::IndexStrategy_t& IndexStrategy,
		     const ui32_t& PartitionSpace, const ui32_t& HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&, const std::string& label,
			   ASDCP::Rational LocalEditRate = ASDCP::Rational(0,0));
  Result_t WriteFrame(const ASDCP::JP2K::FrameBuffer&, bool add_index, ASDCP::AESEncContext*, ASDCP::HMACContext*);
  Result_t Finalize();
  Result_t JP2K_PDesc_to_MD(ASDCP::JP2K::PictureDescriptor& PDesc);

  //void AddSourceClip(const MXF::Rational& EditRate, ui32_t TCFrameRate,
  // const std::string& TrackName, const UL& EssenceUL,
  // const UL& DataDefinition, const std::string& PackageLabel);
  //void AddDMSegment(const MXF::Rational& EditRate, ui32_t TCFrameRate,
  // const std::string& TrackName, const UL& DataDefinition,
  // const std::string& PackageLabel);
  //void AddEssenceDescriptor(const UL& WrappingUL);
  //Result_t CreateBodyPart(const MXF::Rational& EditRate, ui32_t BytesPerEditUnit = 0);

  ////new method to create BodyPartition for essence and index
  //Result_t CreateBodyPartPair();
  ////new method to finalize BodyPartion(index)
  //Result_t CompleteIndexBodyPart();

  // reimplement these functions in AS_02_PCM to support modifications for AS-02
  //Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,
  //	const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC);
  Result_t WriteMXFFooter();


};

const int VideoLineMapSize = 16; // See SMPTE 377M D.2.1
const int PixelLayoutSize = 8*2; // See SMPTE 377M D.2.3
static const byte_t s_PixelLayoutXYZ[PixelLayoutSize] = { 0xd8, 0x0c, 0xd9, 0x0c, 0xda, 0x0c, 0x00 };

//
ASDCP::Result_t
AS_02::JP2K::MXFWriter::h__Writer::JP2K_PDesc_to_MD(ASDCP::JP2K::PictureDescriptor& PDesc)
{
  assert(m_EssenceDescriptor);
  assert(m_EssenceSubDescriptor);
  ASDCP::MXF::RGBAEssenceDescriptor* PDescObj = (ASDCP::MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

  PDescObj->ContainerDuration = PDesc.ContainerDuration;
  PDescObj->SampleRate = PDesc.EditRate;
  PDescObj->FrameLayout = 0;
  PDescObj->StoredWidth = PDesc.StoredWidth;
  PDescObj->StoredHeight = PDesc.StoredHeight;
  PDescObj->AspectRatio = PDesc.AspectRatio;

  //  if ( m_Info.LabelSetType == LS_MXF_SMPTE )
  //    {
  // PictureEssenceCoding UL = 
  // Video Line Map       ui32_t[VideoLineMapSize] = { 2, 4, 0, 0 }
  // CaptureGamma         UL = 
  // ComponentMaxRef      ui32_t = 4095
  // ComponentMinRef      ui32_t = 0
  // PixelLayout          byte_t[PixelLayoutSize] = s_PixelLayoutXYZ
  //    }

  assert(m_Dict);
  if ( PDesc.StoredWidth < 2049 )
    {
      PDescObj->PictureEssenceCoding.Set(m_Dict->ul(MDD_JP2KEssenceCompression_2K));
      m_EssenceSubDescriptor->Rsize = 3;
    }
  else
    {
      PDescObj->PictureEssenceCoding.Set(m_Dict->ul(MDD_JP2KEssenceCompression_4K));
      m_EssenceSubDescriptor->Rsize = 4;
    }

  m_EssenceSubDescriptor->Xsize = PDesc.Xsize;
  m_EssenceSubDescriptor->Ysize = PDesc.Ysize;
  m_EssenceSubDescriptor->XOsize = PDesc.XOsize;
  m_EssenceSubDescriptor->YOsize = PDesc.YOsize;
  m_EssenceSubDescriptor->XTsize = PDesc.XTsize;
  m_EssenceSubDescriptor->YTsize = PDesc.YTsize;
  m_EssenceSubDescriptor->XTOsize = PDesc.XTOsize;
  m_EssenceSubDescriptor->YTOsize = PDesc.YTOsize;
  m_EssenceSubDescriptor->Csize = PDesc.Csize;

  const ui32_t tmp_buffer_len = 1024;
  byte_t tmp_buffer[tmp_buffer_len];

  // slh: this has to be done dynamically since the number of components is not always 3
  *(ui32_t*)tmp_buffer = KM_i32_BE(m_EssenceSubDescriptor->Csize);
  *(ui32_t*)(tmp_buffer+4) = KM_i32_BE(sizeof(ASDCP::JP2K::ImageComponent_t));
  memcpy(tmp_buffer + 8, &PDesc.ImageComponents, sizeof(ASDCP::JP2K::ImageComponent_t) * m_EssenceSubDescriptor->Csize);
  const ui32_t pcomp_size = (sizeof(int) * 2) + (sizeof(ASDCP::JP2K::ImageComponent_t) * m_EssenceSubDescriptor->Csize);

  /*
   *(ui32_t*)tmp_buffer = KM_i32_BE(MaxComponents); // three components
   *(ui32_t*)(tmp_buffer+4) = KM_i32_BE(sizeof(ASDCP::JP2K::ImageComponent_t));	
   memcpy(tmp_buffer + 8, &PDesc.ImageComponents, sizeof(ASDCP::JP2K::ImageComponent_t) * MaxComponents);
   const ui32_t pcomp_size = (sizeof(int) * 2) + (sizeof(ASDCP::JP2K::ImageComponent_t) * MaxComponents);
  */

  memcpy(m_EssenceSubDescriptor->PictureComponentSizing.Data(), tmp_buffer, pcomp_size);
  m_EssenceSubDescriptor->PictureComponentSizing.Length(pcomp_size);

  ui32_t precinct_set_size = 0, i;
  for ( i = 0; PDesc.CodingStyleDefault.SPcod.PrecinctSize[i] != 0 && i < MaxPrecincts; i++ )
    precinct_set_size++;

  ui32_t csd_size = sizeof(CodingStyleDefault_t) - MaxPrecincts + precinct_set_size;
  memcpy(m_EssenceSubDescriptor->CodingStyleDefault.Data(), &PDesc.CodingStyleDefault, csd_size);
  m_EssenceSubDescriptor->CodingStyleDefault.Length(csd_size);

  ui32_t qdflt_size = PDesc.QuantizationDefault.SPqcdLength + 1;
  memcpy(m_EssenceSubDescriptor->QuantizationDefault.Data(), &PDesc.QuantizationDefault, qdflt_size);
  m_EssenceSubDescriptor->QuantizationDefault.Length(qdflt_size);

  return RESULT_OK;
}


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
AS_02::JP2K::MXFWriter::h__Writer::OpenWrite(const char* filename, EssenceType_t type, const AS_02::IndexStrategy_t& IndexStrategy,
		      const ui32_t& PartitionSpace, const ui32_t& HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  ASDCP::Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_IndexStrategy = IndexStrategy;
      m_PartitionSpace = PartitionSpace;
      m_HeaderSize = HeaderSize;
      RGBAEssenceDescriptor* tmp_rgba = new RGBAEssenceDescriptor(m_Dict);
      tmp_rgba->ComponentMaxRef = 4095;
      tmp_rgba->ComponentMinRef = 0;

      m_EssenceDescriptor = tmp_rgba;
      m_EssenceSubDescriptor = new JPEG2000PictureSubDescriptor(m_Dict);
      m_EssenceSubDescriptorList.push_back((InterchangeObject*)m_EssenceSubDescriptor);

      GenRandomValue(m_EssenceSubDescriptor->InstanceUID);
      m_EssenceDescriptor->SubDescriptors.push_back(m_EssenceSubDescriptor->InstanceUID);
      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
ASDCP::Result_t
AS_02::JP2K::MXFWriter::h__Writer::SetSourceStream(const PictureDescriptor& PDesc, const std::string& label, ASDCP::Rational LocalEditRate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( LocalEditRate == ASDCP::Rational(0,0) )
    LocalEditRate = PDesc.EditRate;

  m_PDesc = PDesc;
  Result_t result = JP2K_PDesc_to_MD(m_PDesc);

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
ASDCP::Result_t
AS_02::JP2K::MXFWriter::h__Writer::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf, bool add_index,
		       AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() ){
    result = m_State.Goto_RUNNING(); // first time through
  }
  ui64_t StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) && add_index )
    {
      //create new Index and add it to the IndexTableSegment in the IndexPartition
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = StreamOffset;
      m_CurrentIndexBodyPartition->m_FramesWritten = m_FramesWritten;
      m_CurrentIndexBodyPartition->PushIndexEntry(Entry);

      //here we must check if the number of frames per partition are reached 
      if(m_FramesWritten!=0 &&((m_FramesWritten+1) % m_PartitionSpace) == 0){
	this->m_BodyOffset += m_StreamOffset;
	//StreamOffset - Offset in bytes from the start of the Essence
	//Container of first Essence Element in this Edit Unit of
	//stored Essence within the Essence Container Stream
	//this->m_StreamOffset = 0; ???

	//Complete the Index-BodyPartion
	result = CompleteIndexBodyPart();
	  //Create new BodyPartions for Essence and Index
	  result = CreateBodyPartPair();		 
      }
      //else do nothing, we must only insert the current frame
      //else{}
    }
  m_FramesWritten++;
  return result;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
AS_02::JP2K::MXFWriter::h__Writer::Finalize()
{
  Result_t result = RESULT_OK;

  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  //the last Frame was written, complete the BodyPartion(Index), after that write the MXF-Footer
  result = CompleteIndexBodyPart();

  if ( ASDCP_FAILURE(result) ){
    return result;
  }
  return WriteMXFFooter();
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
ASDCP::Result_t
AS_02::JP2K::MXFWriter::OpenWrite(const char* filename, const ASDCP::WriterInfo& Info,
				  const ASDCP::JP2K::PictureDescriptor& PDesc,
				  const IndexStrategy_t& Strategy,
				  const ui32_t& PartitionSpace,
				  const ui32_t& HeaderSize)
{
  m_Writer = new AS_02::JP2K::MXFWriter::h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  ASDCP::Result_t result = m_Writer->OpenWrite(filename, ASDCP::ESS_JPEG_2000, Strategy, PartitionSpace, HeaderSize);

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
ASDCP::Result_t 
AS_02::JP2K::MXFWriter::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, true, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
AS_02::JP2K::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}


//
// end AS_02_JP2K.cpp
//
