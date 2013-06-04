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
/*! \file    AS_02_PCM.cpp
  \version $Id$       
  \brief   AS-02 library, PCM essence reader and writer implementation
*/

#include "AS_02_internal.h"

#include <map>
#include <iostream>
#include <iomanip>

//------------------------------------------------------------------------------------------

static std::string PCM_PACKAGE_LABEL = "File Package: SMPTE 382M clip wrapping of wave audio";
static std::string SOUND_DEF_LABEL = "Sound Track";

//this must be changed because the CBR_frame_size is only   
//
static ui32_t
calc_CBR_frame_size(ASDCP::WriterInfo& Info, const ASDCP::PCM::AudioDescriptor& ADesc)
{
  ui32_t CBR_frame_size = 0;

  if ( Info.EncryptedEssence )
    {
      CBR_frame_size =
	//TODO: correct?
	/*SMPTE_UL_LENGTH  
	  + MXF_BER_LENGTH
	  + */klv_cryptinfo_size
	+ calc_esv_length(ASDCP::PCM::CalcFrameBufferSize(ADesc), 0)
	+ ( Info.UsesHMAC ? klv_intpack_size : (MXF_BER_LENGTH * 3) );
    }
  else
    {
      CBR_frame_size = ASDCP::PCM::CalcFrameBufferSize(ADesc);
    }

  return CBR_frame_size;
}


//------------------------------------------------------------------------------------------


class AS_02::PCM::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  ASDCP::PCM::AudioDescriptor m_ADesc;

  h__Reader(const Dictionary& d) : AS_02::h__AS02Reader(d) {}
  virtual ~h__Reader() {}

  ASDCP::Result_t    OpenRead(const char*);
  ASDCP::Result_t    ReadFrame(ui32_t, ASDCP::PCM::FrameBuffer&, ASDCP::AESDecContext*, ASDCP::HMACContext*);

  
  Result_t OpenMXFRead(const char* filename);
  // positions file before reading
  Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			 const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);

  // reads from current position
  Result_t ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
			  const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC);


};


//
//
ASDCP::Result_t
AS_02::PCM::MXFReader::h__Reader::OpenRead(const char* filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* Object = 0;
      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &Object)) )
	{
	  if ( Object == 0 )
	    {
	      DefaultLogSink().Error("WaveAudioDescriptor object not found.\n");
	      return RESULT_FORMAT;
	    }

	  result = MD_to_PCM_ADesc((ASDCP::MXF::WaveAudioDescriptor*)Object, m_ADesc);
	}
    }

  // check for sample/frame rate sanity
  if ( ASDCP_SUCCESS(result)
       && m_ADesc.EditRate != EditRate_24
       && m_ADesc.EditRate != EditRate_25
       && m_ADesc.EditRate != EditRate_30
       && m_ADesc.EditRate != EditRate_48
       && m_ADesc.EditRate != EditRate_50
       && m_ADesc.EditRate != EditRate_60
       && m_ADesc.EditRate != EditRate_23_98 )
    {
      DefaultLogSink().Error("PCM file EditRate is not a supported value: %d/%d\n", // lu
			     m_ADesc.EditRate.Numerator, m_ADesc.EditRate.Denominator);

      // oh, they gave us the audio sampling rate instead, assume 24/1
      if ( m_ADesc.EditRate == SampleRate_48k )
	{
	  DefaultLogSink().Warn("adjusting EditRate to 24/1\n"); 
	  m_ADesc.EditRate = EditRate_24;
	}
      else
	{
	  // or we just drop the hammer
	  return RESULT_FORMAT;
	}
    }

  // TODO: test file for sane CBR index BytesPerEditUnit

  return result;
}


//
//
ASDCP::Result_t
AS_02::PCM::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, ASDCP::PCM::FrameBuffer& FrameBuf,
					    ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_WAVEssence), Ctx, HMAC);
}


AS_02::PCM::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


AS_02::PCM::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::PCM::MXFReader::OP1aHeader()
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
AS_02::PCM::MXFReader::AS02IndexReader()
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
AS_02::PCM::MXFReader::RIP()
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
AS_02::PCM::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename);
}

// Reads a frame of essence from the MXF file. If the optional AESEncContext
// argument is present, the essence is decrypted after reading. If the MXF
// file is encrypted and the AESDecContext argument is NULL, the frame buffer
// will contain the ciphertext frame data.
ASDCP::Result_t
AS_02::PCM::MXFReader::ReadFrame(ui32_t FrameNum, ASDCP::PCM::FrameBuffer& FrameBuf,
				 ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
AS_02::PCM::MXFReader::FillAudioDescriptor(ASDCP::PCM::AudioDescriptor& ADesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      ADesc = m_Reader->m_ADesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
AS_02::PCM::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
AS_02::PCM::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
AS_02::PCM::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}


//------------------------------------------------------------------------------------------

//
class AS_02::PCM::MXFWriter::h__Writer : public AS_02::h__AS02Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  ASDCP::PCM::AudioDescriptor m_ADesc;
  byte_t          m_EssenceUL[SMPTE_UL_LENGTH];
  ui64_t			m_KLV_start;

  h__Writer(const Dictionary& d) : AS_02::h__AS02Writer(d), m_KLV_start(0){
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);

  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const char*, ui32_t HeaderSize);
  Result_t SetSourceStream(const ASDCP::PCM::AudioDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

  Result_t Finalize();

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
  Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,
			   const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC);
  Result_t WriteAS02Footer();

};

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::OpenWrite(const char* filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new WaveAudioDescriptor(m_Dict);
      result = m_State.Goto_INIT();
    }

  return result;
}


// Automatically sets the MXF file's metadata from the WAV parser info.
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::SetSourceStream(const ASDCP::PCM::AudioDescriptor& ADesc)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( ADesc.EditRate != EditRate_24
       && ADesc.EditRate != EditRate_25
       && ADesc.EditRate != EditRate_30
       && ADesc.EditRate != EditRate_48
       && ADesc.EditRate != EditRate_50
       && ADesc.EditRate != EditRate_60
       && ADesc.EditRate != EditRate_23_98 )
    {
      DefaultLogSink().Error("AudioDescriptor.EditRate is not a supported value: %d/%d\n",
			     ADesc.EditRate.Numerator, ADesc.EditRate.Denominator);
      return RESULT_RAW_FORMAT;
    }

  if ( ADesc.AudioSamplingRate != SampleRate_48k && ADesc.AudioSamplingRate != SampleRate_96k )
    {
      DefaultLogSink().Error("AudioDescriptor.AudioSamplingRate is not 48000/1 or 96000/1: %d/%d\n",
			     ADesc.AudioSamplingRate.Numerator, ADesc.AudioSamplingRate.Denominator);
      return RESULT_RAW_FORMAT;
    }

  assert(m_Dict);
  m_ADesc = ADesc;

  Result_t result = PCM_ADesc_to_MD(m_ADesc, (WaveAudioDescriptor*)m_EssenceDescriptor);

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, m_Dict->ul(MDD_WAVEssence), SMPTE_UL_LENGTH);
      //SMPTE 382M-2007: ByteNo. 15 - 02h - Wave Clip-Wrapped Element
      m_EssenceUL[SMPTE_UL_LENGTH-2] = 2; // 02h - Wave Clip-Wrapped Element
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t TCFrameRate = ( m_ADesc.EditRate == EditRate_23_98  ) ? 24 : m_ADesc.EditRate.Numerator;

      result = WriteAS02Header(PCM_PACKAGE_LABEL, UL(m_Dict->ul(MDD_WAVWrapping)),
			       SOUND_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_SoundDataDef)),
			       m_ADesc.EditRate, TCFrameRate, calc_CBR_frame_size(m_Info, m_ADesc));
    }

  return result;
}


//
//
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx,
					     HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    m_FramesWritten++;

  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteAS02Footer();
}


//------------------------------------------------------------------------------------------
//



AS_02::PCM::MXFWriter::MXFWriter()
{
}

AS_02::PCM::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::PCM::MXFWriter::OP1aHeader()
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
AS_02::PCM::MXFWriter::RIP()
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
AS_02::PCM::MXFWriter::OpenWrite(const char* filename, const WriterInfo& Info,
				 const ASDCP::PCM::AudioDescriptor& ADesc, ui32_t HeaderSize)
{
  m_Writer = new h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(ADesc);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
AS_02::PCM::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
AS_02::PCM::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}



//
// end AS_02_PCM.cpp
//

