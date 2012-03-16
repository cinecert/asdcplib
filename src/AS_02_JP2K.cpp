/*
  Copyright (c) 2011-2012, Robert Scheler, Heiko Sparenberg Fraunhofer IIS, John Hurst
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

using namespace ASDCP::JP2K;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string JP2K_PACKAGE_LABEL = "File Package: SMPTE 429-4 frame wrapping of JPEG 2000 codestreams";
static std::string PICT_DEF_LABEL = "Image Track";

//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader


class lf__Reader : public AS_02::h__Reader
{
  RGBAEssenceDescriptor*        m_EssenceDescriptor;
  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;
  ASDCP::Rational               m_EditRate;
  ASDCP::Rational               m_SampleRate;
  EssenceType_t                 m_Format;

  ASDCP_NO_COPY_CONSTRUCT(lf__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  lf__Reader(const Dictionary& d) :
    AS_02::h__Reader(d), m_EssenceDescriptor(0), m_EssenceSubDescriptor(0), m_Format(ESS_UNKNOWN) {}
  Result_t    OpenRead(const char*, EssenceType_t);
  Result_t    ReadFrame(ui32_t, JP2K::FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    MD_to_JP2K_PDesc(JP2K::PictureDescriptor& PDesc);

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
lf__Reader::MD_to_JP2K_PDesc(ASDCP::JP2K::PictureDescriptor& PDesc)
{
  memset(&PDesc, 0, sizeof(PDesc));
  MXF::RGBAEssenceDescriptor* PDescObj = (MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

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
lf__Reader::OpenRead(const char* filename, ASDCP::EssenceType_t type)
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
      else if ( type == ASDCP::ESS_JPEG_2000_S )
	{
	  if ( m_EditRate == EditRate_24 )
	    {
	      if ( m_SampleRate != EditRate_48 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 24/48 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_25 )
	    {
	      if ( m_SampleRate != EditRate_50 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 25/50 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_30 )
	    {
	      if ( m_SampleRate != EditRate_60 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 30/60 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else
	    {
	      DefaultLogSink().Error("EditRate not correct for stereoscopic essence: %d/%d.\n",
				     m_EditRate.Numerator, m_EditRate.Denominator);
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

  if( ASDCP_SUCCESS(result) )
    result = InitMXFIndex();

  if( ASDCP_SUCCESS(result) )
    result = InitInfo();

  return result;
}

//
//
ASDCP::Result_t
lf__Reader::ReadFrame(ui32_t FrameNum, JP2K::FrameBuffer& FrameBuf,
		      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEG2000Essence), Ctx, HMAC);
}


//
class AS_02::JP2K::MXFReader::h__Reader : public lf__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  h__Reader(const Dictionary& d) : lf__Reader(d) {}
};

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
ASDCP::MXF::OPAtomHeader&
AS_02::JP2K::MXFReader::OPAtomHeader()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomHeader);
      return *g_OPAtomHeader;
    }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
/*
ASDCP::MXF::OPAtomIndexFooter&
AS_02::JP2K::MXFReader::OPAtomIndexFooter()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Reader->m_FooterPart;
}
*/

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
Result_t AS_02::JP2K::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename, ASDCP::ESS_JPEG_2000);
}

//
Result_t AS_02::JP2K::MXFReader::ReadFrame(ui32_t FrameNum, ASDCP::JP2K::FrameBuffer& FrameBuf,
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

//
void
AS_02::JP2K::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
AS_02::JP2K::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_FooterPart.Dump(stream);
}

// standard method of opening an MXF file for read
Result_t
lf__Reader::OpenMXFRead(const char* filename)
{
  m_LastPosition = 0;
  AS_02::MXF::OP1aIndexBodyPartion* pCurrentBodyPartIndex = NULL;
  Partition* pPart = NULL;
  ui64_t EssenceStart = 0;
  Result_t result = m_File.OpenRead(filename);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.InitFromFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t partition_size = m_HeaderPart.m_RIP.PairArray.size();
      
      if ( partition_size > 3 )
	{
	  //for all entry except the first and the last(header&footer)
	  Array<RIP::Pair>::iterator r_i = m_HeaderPart.m_RIP.PairArray.begin();
	  r_i++;
	  ui32_t i=2;

	  while(r_i != m_HeaderPart.m_RIP.PairArray.end() && i<partition_size)
	    {
	      m_File.Seek((*r_i).ByteOffset);
	      pPart = new Partition(this->m_Dict);
	      result = pPart->InitFromFile(m_File);

	      if(pPart->BodySID != 0 && pPart->IndexSID == 0)
		{ // AS_02::IS_FOLLOW
		  r_i++; i++;
		  EssenceStart = m_File.Tell();

		  m_File.Seek((*r_i).ByteOffset);
		  pCurrentBodyPartIndex = new AS_02::MXF::OP1aIndexBodyPartion(this->m_Dict);
		  pCurrentBodyPartIndex->m_Lookup = &m_HeaderPart.m_Primer;
		  result = pCurrentBodyPartIndex->InitFromFile(m_File);
		}
	      else
		{ // AS_02::IS_LEAD
		  delete pPart;
		  m_File.Seek((*r_i).ByteOffset);
		  pCurrentBodyPartIndex = new AS_02::MXF::OP1aIndexBodyPartion(this->m_Dict);
		  pCurrentBodyPartIndex->m_Lookup = &m_HeaderPart.m_Primer;
		  result = pCurrentBodyPartIndex->InitFromFile(m_File);
		  
		  if ( ASDCP_FAILURE(result) )
		    {
		      break;
		    }
	
		  r_i++; i++;
		  m_File.Seek((*r_i).ByteOffset);
		  pPart = new Partition(this->m_Dict);
		  result = pPart->InitFromFile(m_File);
		  EssenceStart = m_File.Tell();
		}

	      if ( ASDCP_FAILURE(result) )
		{
		  break;
		}

	      if(i==3)
		{
		  this->m_EssenceStart = EssenceStart;
		  this->m_pCurrentBodyPartition = pPart;
		  this->m_pCurrentIndexPartition = pCurrentBodyPartIndex;
		}

	      this->m_BodyPartList.push_back(pCurrentBodyPartIndex);
	      this->m_BodyPartList.push_back(pPart);
	      r_i++; i++;
	    }
	}
    }

  return result;
}

//
class lf__Writer : public AS_02::h__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(lf__Writer);
  lf__Writer();

  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

public:
  PictureDescriptor m_PDesc;
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  //new attributes for AS-02 support 
  AS_02::IndexStrategy_t m_IndexStrategy; //Shim parameter index_strategy_frame/clip
  ui32_t m_PartitionSpace; //Shim parameter partition_spacing

  lf__Writer(const Dictionary& d) : h__Writer(d), m_EssenceSubDescriptor(0), m_IndexStrategy(AS_02::IS_FOLLOW), m_PartitionSpace(60) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  ~lf__Writer(){}

  Result_t OpenWrite(const char*, EssenceType_t type, const AS_02::IndexStrategy_t& IndexStrategy,
		     const ui32_t& PartitionSpace, const ui32_t& HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&, const std::string& label,
			   ASDCP::Rational LocalEditRate = ASDCP::Rational(0,0));
  Result_t WriteFrame(const JP2K::FrameBuffer&, bool add_index, AESEncContext*, HMACContext*);
  Result_t Finalize();
  Result_t JP2K_PDesc_to_MD(JP2K::PictureDescriptor& PDesc);

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
lf__Writer::JP2K_PDesc_to_MD(JP2K::PictureDescriptor& PDesc)
{
  assert(m_EssenceDescriptor);
  assert(m_EssenceSubDescriptor);
  MXF::RGBAEssenceDescriptor* PDescObj = (MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

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
lf__Writer::OpenWrite(const char* filename, EssenceType_t type, const AS_02::IndexStrategy_t& IndexStrategy,
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
lf__Writer::SetSourceStream(const PictureDescriptor& PDesc, const std::string& label, ASDCP::Rational LocalEditRate)
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

      result = WriteMXFHeader(label, UL(m_Dict->ul(MDD_JPEG_2000Wrapping)),
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
lf__Writer::WriteFrame(const ASDCP::JP2K::FrameBuffer& FrameBuf, bool add_index,
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
lf__Writer::Finalize()
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


//
class AS_02::JP2K::MXFWriter::h__Writer : public lf__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  h__Writer(const Dictionary& d) : lf__Writer(d) {}
};


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
ASDCP::MXF::OPAtomHeader&
AS_02::JP2K::MXFWriter::OPAtomHeader()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomHeader);
      return *g_OPAtomHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
/*
ASDCP::MXF::OPAtomIndexFooter&
AS_02::JP2K::MXFWriter::OPAtomIndexFooter()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Writer->m_FooterPart;
}
*/

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


//------------------------------------------------------------------------------------------

// standard method of reading a plaintext or encrypted frame
Result_t
lf__Reader::ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			  const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;
  IndexTableSegment::IndexEntry TmpEntry;

  assert(m_pCurrentIndexPartition != NULL);

  if( m_pCurrentIndexPartition->Lookup(FrameNum, TmpEntry, m_start_pos) == false)
    {
      m_start_pos = 0;

      //compute new indexPartition for this FrameNum
      for (ui32_t i = 0; i < m_BodyPartList.size(); i+=2 )
	{
	  if( m_IndexStrategy == AS_02::IS_FOLLOW )
	    {
	      m_pCurrentBodyPartition = m_BodyPartList.at(i);
	      m_pCurrentIndexPartition = dynamic_cast<AS_02::MXF::OP1aIndexBodyPartion*> (m_BodyPartList.at(i+1));
	    }
	  else if( m_IndexStrategy == AS_02::IS_LEAD )
	    {
	      m_pCurrentIndexPartition = dynamic_cast<AS_02::MXF::OP1aIndexBodyPartion*> (m_BodyPartList.at(i));
	      m_pCurrentBodyPartition = m_BodyPartList.at(i+1);
	    }
	  else
	    {
	      return RESULT_FORMAT; //return error
	    }
	  
	  if( m_pCurrentIndexPartition == 0 )
	    return RESULT_FORMAT;
	  
	  if(m_pCurrentIndexPartition->Lookup(FrameNum, TmpEntry, m_start_pos))
	    {
	      if ( FrameNum % m_PartitionSpace == 0 )
		{
		  ui64_t offset = m_pCurrentBodyPartition->ThisPartition - m_pCurrentBodyPartition->PreviousPartition;
		  offset += m_pCurrentBodyPartition->ArchiveSize();//Offset for the BodyPartitionHeader - Partition::ArchiveSize();
		  m_EssenceStart += offset;
		}
	      break;
	    }
	}
    }
  
  // get frame position and go read the frame's key and length
  Kumu::fpos_t FilePosition = this->m_EssenceStart + TmpEntry.StreamOffset;

  if ( FilePosition != m_LastPosition )
    {
      m_LastPosition = FilePosition;
      result = m_File.Seek(FilePosition);
    }

  if( ASDCP_SUCCESS(result) )
    result = ReadEKLVPacket(FrameNum, FrameNum + 1, FrameBuf, EssenceUL, Ctx, HMAC);

  return result;
}

//
Result_t
lf__Reader::ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
			   const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  KLReader Reader;
  Result_t result = Reader.ReadKLFromFile(m_File);

  if ( ASDCP_FAILURE(result) )
    return result;

  UL Key(Reader.Key());
  ui64_t PacketLength = Reader.Length();
  m_LastPosition = m_LastPosition + Reader.KLLength() + PacketLength;
  assert(m_Dict);

  if ( memcmp(Key.Value(), m_Dict->ul(MDD_CryptEssence), Key.Size() - 1) == 0 )  // ignore the stream numbers
    {
      if ( ! m_Info.EncryptedEssence )
	{
	  DefaultLogSink().Error("EKLV packet found, no Cryptographic Context in header.\n");
	  return RESULT_FORMAT;
	}

      // read encrypted triplet value into internal buffer
      assert(PacketLength <= 0xFFFFFFFFL);
      m_CtFrameBuf.Capacity((ui32_t) PacketLength);
      ui32_t read_count;
      result = m_File.Read(m_CtFrameBuf.Data(), (ui32_t) PacketLength,
			   &read_count);

      if ( ASDCP_FAILURE(result) )
	return result;

      if ( read_count != PacketLength )
	{
	  DefaultLogSink().Error("read length is smaller than EKLV packet length.\n");
	  return RESULT_FORMAT;
	}

      m_CtFrameBuf.Size((ui32_t) PacketLength);

      // should be const but mxflib::ReadBER is not
      byte_t* ess_p = m_CtFrameBuf.Data();

      // read context ID length
      if ( ! Kumu::read_test_BER(&ess_p, UUIDlen) )
	return RESULT_FORMAT;

      // test the context ID
      if ( memcmp(ess_p, m_Info.ContextID, UUIDlen) != 0 )
	{
	  DefaultLogSink().Error("Packet's Cryptographic Context ID does not match the header.\n");
	  return RESULT_FORMAT;
	}
      ess_p += UUIDlen;

      // read PlaintextOffset length
      if ( ! Kumu::read_test_BER(&ess_p, sizeof(ui64_t)) )
	return RESULT_FORMAT;

      ui32_t PlaintextOffset = (ui32_t)KM_i64_BE(Kumu::cp2i<ui64_t>(ess_p));
      ess_p += sizeof(ui64_t);

      // read essence UL length
      if ( ! Kumu::read_test_BER(&ess_p, SMPTE_UL_LENGTH) )
	return RESULT_FORMAT;

      // test essence UL
      if ( memcmp(ess_p, EssenceUL, SMPTE_UL_LENGTH - 1) != 0 ) // ignore the stream number
	{
	  char strbuf[IntBufferLen];
	  const MDDEntry* Entry = m_Dict->FindUL(Key.Value());
	  if ( Entry == 0 )
	    DefaultLogSink().Warn("Unexpected Essence UL found: %s.\n", Key.EncodeString(strbuf, IntBufferLen));
	  else
	    DefaultLogSink().Warn("Unexpected Essence UL found: %s.\n", Entry->name);
	  return RESULT_FORMAT;
	}
      ess_p += SMPTE_UL_LENGTH;

      // read SourceLength length
      if ( ! Kumu::read_test_BER(&ess_p, sizeof(ui64_t)) )
	return RESULT_FORMAT;

      ui32_t SourceLength = (ui32_t)KM_i64_BE(Kumu::cp2i<ui64_t>(ess_p));
      ess_p += sizeof(ui64_t);
      assert(SourceLength);

      if ( FrameBuf.Capacity() < SourceLength )
	{
	  DefaultLogSink().Error("FrameBuf.Capacity: %u SourceLength: %u\n", FrameBuf.Capacity(), SourceLength);
	  return RESULT_SMALLBUF;
	}

      ui32_t esv_length = calc_esv_length(SourceLength, PlaintextOffset);

      // read ESV length
      if ( ! Kumu::read_test_BER(&ess_p, esv_length) )
	{
	  DefaultLogSink().Error("read_test_BER did not return %u\n", esv_length);
	  return RESULT_FORMAT;
	}

      ui32_t tmp_len = esv_length + (m_Info.UsesHMAC ? klv_intpack_size : 0);

      if ( PacketLength < tmp_len )
	{
	  DefaultLogSink().Error("Frame length is larger than EKLV packet length.\n");
	  return RESULT_FORMAT;
	}

      if ( Ctx )
	{
	  // wrap the pointer and length as a FrameBuffer for use by
	  // DecryptFrameBuffer() and TestValues()
	  ASDCP::FrameBuffer TmpWrapper;
	  TmpWrapper.SetData(ess_p, tmp_len);
	  TmpWrapper.Size(tmp_len);
	  TmpWrapper.SourceLength(SourceLength);
	  TmpWrapper.PlaintextOffset(PlaintextOffset);

	  result = DecryptFrameBuffer(TmpWrapper, FrameBuf, Ctx);
	  FrameBuf.FrameNumber(FrameNum);

	  // detect and test integrity pack
	  if ( ASDCP_SUCCESS(result) && m_Info.UsesHMAC && HMAC )
	    {
	      IntegrityPack IntPack;
	      result = IntPack.TestValues(TmpWrapper, m_Info.AssetUUID, SequenceNum, HMAC);
	    }
	}
      else // return ciphertext to caller
	{
	  if ( FrameBuf.Capacity() < tmp_len )
	    {
	      char intbuf[IntBufferLen];
	      DefaultLogSink().Error("FrameBuf.Capacity: %u FrameLength: %s\n",
				     FrameBuf.Capacity(), ui64sz(PacketLength, intbuf));
	      return RESULT_SMALLBUF;
	    }

	  memcpy(FrameBuf.Data(), ess_p, tmp_len);
	  FrameBuf.Size(tmp_len);
	  FrameBuf.FrameNumber(FrameNum);
	  FrameBuf.SourceLength(SourceLength);
	  FrameBuf.PlaintextOffset(PlaintextOffset);
	}
    }
  else if ( memcmp(Key.Value(), EssenceUL, Key.Size() - 1) == 0 ) // ignore the stream number
    { // read plaintext frame
      if ( FrameBuf.Capacity() < PacketLength )
	{
	  char intbuf[IntBufferLen];
	  DefaultLogSink().Error("FrameBuf.Capacity: %u FrameLength: %s\n",
				 FrameBuf.Capacity(), ui64sz(PacketLength, intbuf));
	  return RESULT_SMALLBUF;
	}

      // read the data into the supplied buffer
      ui32_t read_count;
      assert(PacketLength <= 0xFFFFFFFFL);
      result = m_File.Read(FrameBuf.Data(), (ui32_t) PacketLength, &read_count);

      if ( ASDCP_FAILURE(result) )
	return result;

      if ( read_count != PacketLength )
	{
	  char intbuf1[IntBufferLen];
	  char intbuf2[IntBufferLen];
	  DefaultLogSink().Error("read_count: %s != FrameLength: %s\n",
				 ui64sz(read_count, intbuf1),
				 ui64sz(PacketLength, intbuf2) );

	  return RESULT_READFAIL;
	}

      FrameBuf.FrameNumber(FrameNum);
      FrameBuf.Size(read_count);
    }
  else
    {
      char strbuf[IntBufferLen];
      const MDDEntry* Entry = m_Dict->FindUL(Key.Value());
      if ( Entry == 0 )
	DefaultLogSink().Warn("Unexpected Essence UL found: %s.\n", Key.EncodeString(strbuf, IntBufferLen));
      else
	DefaultLogSink().Warn("Unexpected Essence UL found: %s.\n", Entry->name);
      return RESULT_FORMAT;
    }

  return result;
}


// standard method of writing the header and footer of a completed MXF file
//
Result_t
lf__Writer::WriteMXFFooter()
{
  // Set top-level file package correctly for OP-Atom

  //  m_MPTCSequence->Duration = m_MPTimecode->Duration = m_MPClSequence->Duration = m_MPClip->Duration = 
  //    m_FPTCSequence->Duration = m_FPTimecode->Duration = m_FPClSequence->Duration = m_FPClip->Duration = 

  DurationElementList_t::iterator dli = m_DurationUpdateList.begin();

  for (; dli != m_DurationUpdateList.end(); dli++ )
    **dli = m_FramesWritten;

  m_EssenceDescriptor->ContainerDuration = m_FramesWritten;
  m_FooterPart.PreviousPartition = m_HeaderPart.m_RIP.PairArray.back().ByteOffset;

  Kumu::fpos_t here = m_File.Tell();
  m_HeaderPart.m_RIP.PairArray.push_back(RIP::Pair(0, here)); // Last RIP Entry
  m_HeaderPart.FooterPartition = here;

  assert(m_Dict);
  // re-label the partition
  UL OPAtomUL(m_Dict->ul(MDD_OP1a));
  m_HeaderPart.OperationalPattern = OPAtomUL;
  m_HeaderPart.m_Preface->OperationalPattern = m_HeaderPart.OperationalPattern;

  m_FooterPart.OperationalPattern = m_HeaderPart.OperationalPattern;
  m_FooterPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  m_FooterPart.FooterPartition = here;
  m_FooterPart.ThisPartition = here;

  Result_t result = m_FooterPart.WriteToFile(m_File, m_FramesWritten);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.m_RIP.WriteToFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  //update the value of FooterPartition in all Partitions
  std::vector<Partition*>::iterator iter = this->m_BodyPartList.begin();
  for (; iter != this->m_BodyPartList.end(); iter++ ){
    (*iter)->FooterPartition =  m_FooterPart.ThisPartition;
    if ( ASDCP_SUCCESS(result) )
      result = m_File.Seek((*iter)->ThisPartition);
    if ( ASDCP_SUCCESS(result) ){
      UL BodyUL(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = (*iter)->WriteToFile(m_File, BodyUL);
    }
  } 

  m_File.Close();
  return result;
}

//
// end AS_02_JP2K.cpp
//
