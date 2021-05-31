/*
Copyright (c) 2004-2016, John Hurst,
Copyright (c) 2020, Thomas Richter Fraunhofer IIS
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
/*! \file    AS_DCP_JXS.cpp
    \version $Id$
    \brief   AS-DCP library, JPEG XS essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include "AS_DCP_JXS.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace ASDCP::JXS;
using Kumu::GenRandomValue;

static std::string JXS_PACKAGE_LABEL = "File Package: SMPTE 2124 frame wrapping of JPEG XS codestreams";
//static std::string JP2K_S_PACKAGE_LABEL = "File Package: SMPTE 429-10 frame wrapping of stereoscopic JPEG XS codestreams";
static std::string PICT_DEF_LABEL = "Picture Track";



//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG XS reader


class ih__Reader : public ASDCP::h__ASDCPReader
{
  RGBAEssenceDescriptor*        m_EssenceDescriptor;
  JPEGXSPictureSubDescriptor*   m_EssenceSubDescriptor;
  ASDCP::Rational               m_EditRate;
  ASDCP::Rational               m_SampleRate;
  EssenceType_t                 m_Format;

  ASDCP_NO_COPY_CONSTRUCT(ih__Reader);

public:

  ih__Reader(const Dictionary *d) :
    ASDCP::h__ASDCPReader(d), m_EssenceDescriptor(0), m_EssenceSubDescriptor(0), m_Format(ESS_UNKNOWN) {}

  virtual ~ih__Reader() {}

  Result_t    OpenRead(const std::string&, EssenceType_t);
  Result_t    ReadFrame(ui32_t, JXS::FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    CalcFrameBufferSize(ui64_t &size);
};


//
//
ASDCP::Result_t
ih__Reader::OpenRead(const std::string& filename, EssenceType_t type)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;
      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
      m_EssenceDescriptor = static_cast<RGBAEssenceDescriptor*>(tmp_iobj);

      if ( m_EssenceDescriptor == 0 )
	{
	  DefaultLogSink().Error("RGBAEssenceDescriptor object not found.\n");
	  return RESULT_FORMAT;
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEGXSPictureSubDescriptor), &tmp_iobj);
      m_EssenceSubDescriptor = static_cast<JPEGXSPictureSubDescriptor*>(tmp_iobj);

      if ( m_EssenceSubDescriptor == 0 )
	{
	  m_EssenceDescriptor = 0;
	  DefaultLogSink().Error("JPEGXSPictureSubDescriptor object not found.\n");
	  return RESULT_FORMAT;
	}

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_FORMAT;
	}

      m_EditRate = ((Track*)ObjectList.front())->EditRate;
      m_SampleRate = m_EssenceDescriptor->SampleRate;

      if ( type == ASDCP::ESS_JPEG_XS )
	{
	  if ( m_EditRate != m_SampleRate )
	    {
	      DefaultLogSink().Warn("EditRate and SampleRate do not match (%.03f, %.03f).\n",
				    m_EditRate.Quotient(), m_SampleRate.Quotient());
	      /*
	      ** thor: currently, I do not see support for stereoskopic content,
	      ** thus skip this at the time being.
	      if ( ( m_EditRate == EditRate_24 && m_SampleRate == EditRate_48 )
		   || ( m_EditRate == EditRate_25 && m_SampleRate == EditRate_50 )
		   || ( m_EditRate == EditRate_30 && m_SampleRate == EditRate_60 )
		   || ( m_EditRate == EditRate_48 && m_SampleRate == EditRate_96 )
		   || ( m_EditRate == EditRate_50 && m_SampleRate == EditRate_100 )
		   || ( m_EditRate == EditRate_60 && m_SampleRate == EditRate_120 )
		   || ( m_EditRate == EditRate_96 && m_SampleRate == EditRate_192 )
		   || ( m_EditRate == EditRate_100 && m_SampleRate == EditRate_200 )
		   || ( m_EditRate == EditRate_120 && m_SampleRate == EditRate_240 ) )
		{
		  DefaultLogSink().Debug("File may contain JPEG Interop stereoscopic images.\n");
		  return RESULT_SFORMAT;
		}
	      **
	      */

	      return RESULT_FORMAT;
	    }
	}
      /*
      ** thor: support for stereoskopic JPEG XS disabled as it may
      ** not yet be supported by a standard.
      else if ( type == ASDCP::ESS_JPEG_XS_S )
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
	  else if ( m_EditRate == EditRate_48 )
	    {
	      if ( m_SampleRate != EditRate_96 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 48/96 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_50 )
	    {
	      if ( m_SampleRate != EditRate_100 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 50/100 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_60 )
	    {
	      if ( m_SampleRate != EditRate_120 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 60/120 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_96 )
	    {
	      if ( m_SampleRate != EditRate_192 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 96/192 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_100 )
	    {
	      if ( m_SampleRate != EditRate_200 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 100/200 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_120 )
	    {
	      if ( m_SampleRate != EditRate_240 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 120/240 stereoscopic essence.\n");
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
      */
      else
	{
	  DefaultLogSink().Error("'type' argument unexpected: %x\n", type);
	  return RESULT_STATE;
	}
    }

  return result;
}

//
//
ASDCP::Result_t
ih__Reader::ReadFrame(ui32_t FrameNum, JXS::FrameBuffer& FrameBuf,
		      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEGXSEssence), Ctx, HMAC);
}

//
Result_t ih__Reader::CalcFrameBufferSize(ui64_t &size)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ASDCP::h__ASDCPReader::CalcFrameBufferSize(size);
}

//
class ASDCP::JXS::MXFReader::h__Reader : public ih__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  h__Reader(const Dictionary *d) : ih__Reader(d) {}
};


//------------------------------------------------------------------------------------------

ASDCP::JXS::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(&DefaultCompositeDict());
}


ASDCP::JXS::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JXS::MXFReader::OP1aHeader()
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
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::JXS::MXFReader::OPAtomIndexFooter()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::JXS::MXFReader::RIP()
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
ASDCP::Result_t
ASDCP::JXS::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename, ASDCP::ESS_JPEG_XS);
}

//
ASDCP::Result_t
ASDCP::JXS::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

//
ASDCP::Result_t
ASDCP::JXS::MXFReader::CalcFrameBufferSize(ui64_t &size) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->CalcFrameBufferSize(size);

  return RESULT_INIT;
}

ASDCP::Result_t
ASDCP::JXS::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JXS::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::JXS::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::JXS::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::JXS::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------

//
class ih__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP_NO_COPY_CONSTRUCT(ih__Writer);
  ih__Writer();

  JPEGXSPictureSubDescriptor* m_EssenceSubDescriptor;

public:
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  ih__Writer(const Dictionary *d) : ASDCP::h__ASDCPWriter(d), m_EssenceSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~ih__Writer(){}

  Result_t OpenWrite(const std::string&, EssenceType_t type, ui32_t HeaderSize);
  Result_t SetSourceStream(ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
                           ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_sub_descriptor,
                           const std::string& label,
			   const ASDCP::Rational& edit_rate);
  Result_t WriteFrame(const JXS::FrameBuffer&, bool add_index, AESEncContext*, HMACContext*);
  Result_t Finalize();
};

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ih__Writer::OpenWrite(const std::string& filename, EssenceType_t type, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      RGBAEssenceDescriptor* tmp_rgba = new RGBAEssenceDescriptor(m_Dict);
      tmp_rgba->ComponentMaxRef = 4095;
      tmp_rgba->ComponentMinRef = 0;

      m_EssenceDescriptor = tmp_rgba;
      m_EssenceSubDescriptor = new JPEGXSPictureSubDescriptor(m_Dict);
      m_EssenceSubDescriptorList.push_back((InterchangeObject*)m_EssenceSubDescriptor);

      GenRandomValue(m_EssenceSubDescriptor->InstanceUID);
      m_EssenceDescriptor->SubDescriptors.push_back(m_EssenceSubDescriptor->InstanceUID);

      /*
      ** thor: stereoskopic images are currently disabled.
      if ( type == ASDCP::ESS_JPEG_XS_S && m_Info.LabelSetType == LS_MXF_SMPTE )
	{
	  InterchangeObject* StereoSubDesc = new StereoscopicPictureSubDescriptor(m_Dict);
	  m_EssenceSubDescriptorList.push_back(StereoSubDesc);
	  GenRandomValue(StereoSubDesc->InstanceUID);
	  m_EssenceDescriptor->SubDescriptors.push_back(StereoSubDesc->InstanceUID);
	}
      **
      */

      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
ASDCP::Result_t
ih__Writer::SetSourceStream(
    ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
    ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_sub_descriptor,
    const std::string& label,
    const ASDCP::Rational& edit_rate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( edit_rate == ASDCP::Rational(0,0) )
    {
      DefaultLogSink().Error("Edit rate not set before call to ih__Writer::SetSourceStream.\n");
      return RESULT_PARAM;
    }

  assert(m_Dict);

  m_EssenceDescriptor = new ASDCP::MXF::GenericPictureEssenceDescriptor(m_Dict);
  m_EssenceDescriptor->Copy(picture_descriptor);
  assert(m_EssenceDescriptor);

  m_EssenceSubDescriptor = new ASDCP::MXF::JPEGXSPictureSubDescriptor(m_Dict);
  m_EssenceSubDescriptor->Copy(jxs_sub_descriptor);
  assert(m_EssenceSubDescriptor);

  memcpy(m_EssenceUL, m_Dict->ul(MDD_JPEGXSEssence), SMPTE_UL_LENGTH);
  m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container

  Result_t result = m_State.Goto_READY();

  if ( ASDCP_SUCCESS(result) )
    {
      result = WriteASDCPHeader(label, UL(m_Dict->ul(MDD_MXFGCFUFrameWrappedPictureElement)),
				PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
				edit_rate, derive_timecode_rate_from_edit_rate(edit_rate));
    }

  return result;
}

//
bool
ASDCP::JXS::lookup_PictureEssenceCoding(int value, ASDCP::UL& ul)
{
  const ASDCP::Dictionary& dict = DefaultSMPTEDict();
  switch ( value )
    {
    case 0: // Profile_Unrestricted
      ul = dict.ul(MDD_JPEGXSUnrestrictedCodestream);
      break;
    case 0x1500: // Profile_Light422
      ul = dict.ul(MDD_JPEGXSLight422_10Profile);
      break;
    case 0x1a00: // Profile_Light444
      ul = dict.ul(MDD_JPEGXSLight444_12Profile);
      break;
    case 0x2500: // Profile_LightSubline
      ul = dict.ul(MDD_JPEGXSLightSubline422_10Profile);
      break;
    case 0x3540: // Profile_Main422
      ul = dict.ul(MDD_JPEGXSMain422_10Profile);
      break;
    case 0x3a40: // Profile_Main444
      ul = dict.ul(MDD_JPEGXSMain444_12Profile);
      break;
    case 0x3e40: // Profile_Main4444
      ul = dict.ul(MDD_JPEGXSMain4444_12Profile);
      break;
    case 0x4a40: // Profile_High444
      ul = dict.ul(MDD_JPEGXSHigh444_12Profile);
      break;
    case 0x4e40: // Profile_High4444
      ul = dict.ul(MDD_JPEGXSHigh4444_12Profile);
      break;

    default:
      return false;
      break;
    }

  return true;
}


// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
ASDCP::Result_t
ih__Writer::WriteFrame(const JXS::FrameBuffer& FrameBuf, bool add_index,
		       AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through
 
  ui64_t StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, MXF_BER_LENGTH, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) && add_index )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = StreamOffset;
      m_FooterPart.PushIndexEntry(Entry);
    }

  m_FramesWritten++;
  return result;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ih__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}


//
class ASDCP::JXS::MXFWriter::h__Writer : public ih__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  h__Writer(const Dictionary *d) : ih__Writer(d) {}
};


//------------------------------------------------------------------------------------------



ASDCP::JXS::MXFWriter::MXFWriter()
{
}

ASDCP::JXS::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JXS::MXFWriter::OP1aHeader()
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
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::JXS::MXFWriter::OPAtomIndexFooter()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Writer->m_FooterPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::JXS::MXFWriter::RIP()
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
ASDCP::JXS::MXFWriter::OpenWrite(
    const std::string& filename, const WriterInfo& Info,
    ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
    ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_sub_descriptor,
    const ASDCP::Rational& edit_rate, ui32_t HeaderSize)
{
  if ( Info.LabelSetType == LS_MXF_SMPTE )
    m_Writer = new h__Writer(&DefaultSMPTEDict());
  else
    m_Writer = new h__Writer(&DefaultInteropDict());

  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, ASDCP::ESS_JPEG_XS, HeaderSize);
  
  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(picture_descriptor, jxs_sub_descriptor, JXS_PACKAGE_LABEL, edit_rate);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}


// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::JXS::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, true, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::JXS::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}

//
// end AS_DCP_JXS.cpp
//
