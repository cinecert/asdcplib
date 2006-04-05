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
/*! \file    AS_DCP_PCM.cpp
    \version $Id$       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include <map>

//------------------------------------------------------------------------------------------

static std::string PCM_PACKAGE_LABEL = "File Package: SMPTE 382M frame wrapping of wave audio";
static std::string SOUND_DEF_LABEL = "Sound Track";

//
Result_t
PCM_ADesc_to_MD(PCM::AudioDescriptor& ADesc, MXF::WaveAudioDescriptor* ADescObj)
{
  ASDCP_TEST_NULL(ADescObj);
  ADescObj->SampleRate = ADesc.SampleRate;
  ADescObj->AudioSamplingRate = ADesc.AudioSamplingRate;
  ADescObj->Locked = ADesc.Locked;
  ADescObj->ChannelCount = ADesc.ChannelCount;
  ADescObj->QuantizationBits = ADesc.QuantizationBits;
  ADescObj->BlockAlign = ADesc.BlockAlign;
  ADescObj->AvgBps = ADesc.AvgBps;
  ADescObj->LinkedTrackID = ADesc.LinkedTrackID;
  ADescObj->ContainerDuration = ADesc.ContainerDuration;
  return RESULT_OK;
}

//
ASDCP::Result_t
MD_to_PCM_ADesc(MXF::WaveAudioDescriptor* ADescObj, PCM::AudioDescriptor& ADesc)
{
  ASDCP_TEST_NULL(ADescObj);
  ADesc.SampleRate = ADescObj->SampleRate;
  ADesc.AudioSamplingRate = ADescObj->AudioSamplingRate;
  ADesc.Locked = ADescObj->Locked;
  ADesc.ChannelCount = ADescObj->ChannelCount;
  ADesc.QuantizationBits = ADescObj->QuantizationBits;
  ADesc.BlockAlign = ADescObj->BlockAlign;
  ADesc.AvgBps = ADescObj->AvgBps;
  ADesc.LinkedTrackID = ADescObj->LinkedTrackID;
  ADesc.ContainerDuration = ADescObj->ContainerDuration;
  return RESULT_OK;
}

void
ASDCP::PCM::AudioDescriptorDump(const AudioDescriptor& ADesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
        SampleRate: %lu/%lu\n\
 AudioSamplingRate: %lu/%lu\n\
            Locked: %lu\n\
      ChannelCount: %lu\n\
  QuantizationBits: %lu\n\
        BlockAlign: %lu\n\
            AvgBps: %lu\n\
     LinkedTrackID: %lu\n\
 ContainerDuration: %lu\n",
	  ADesc.SampleRate.Numerator ,ADesc.SampleRate.Denominator,
	  ADesc.AudioSamplingRate.Numerator ,ADesc.AudioSamplingRate.Denominator,
	  ADesc.Locked,
	  ADesc.ChannelCount,
	  ADesc.QuantizationBits,
	  ADesc.BlockAlign,
	  ADesc.AvgBps,
	  ADesc.LinkedTrackID,
	  ADesc.ContainerDuration
	  );
}


//
//
static ui32_t
calc_CBR_frame_size(ASDCP::WriterInfo& Info, const ASDCP::PCM::AudioDescriptor& ADesc)
{
  ui32_t CBR_frame_size = 0;

  if ( Info.EncryptedEssence )
    {
      CBR_frame_size =
	SMPTE_UL_LENGTH
	+ MXF_BER_LENGTH
	+ klv_cryptinfo_size
	+ calc_esv_length(ASDCP::PCM::CalcFrameBufferSize(ADesc), 0)
	+ ( Info.UsesHMAC ? klv_intpack_size : (MXF_BER_LENGTH * 3) );
    }
  else
    {
      CBR_frame_size = ASDCP::PCM::CalcFrameBufferSize(ADesc) + SMPTE_UL_LENGTH + MXF_BER_LENGTH;
    }

  return CBR_frame_size;
}


//------------------------------------------------------------------------------------------


class ASDCP::PCM::MXFReader::h__Reader : public ASDCP::h__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  AudioDescriptor m_ADesc;

  h__Reader() {}
  ~h__Reader() {}
  Result_t    OpenRead(const char*);
  Result_t    ReadFrame(ui32_t, FrameBuffer&, AESDecContext*, HMACContext*);
};


//
//
ASDCP::Result_t
ASDCP::PCM::MXFReader::h__Reader::OpenRead(const char* filename)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* Object;
      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &Object)) )
	{
	  assert(Object);
	  result = MD_to_PCM_ADesc((MXF::WaveAudioDescriptor*)Object, m_ADesc);
	}
    }

  // check for sample/frame rate sanity
  if ( m_ADesc.SampleRate != EditRate_24
       && m_ADesc.SampleRate != EditRate_48
       && m_ADesc.SampleRate != EditRate_23_98 )
    {
      DefaultLogSink().Error("PCM file SampleRate is not 24/1, 48/1 or 24000/1001: %08x/%08x\n", // lu
			     m_ADesc.SampleRate.Numerator, m_ADesc.SampleRate.Denominator);

      // oh, they gave us the audio sampling rate instead, assume 24/1
      if ( m_ADesc.SampleRate == SampleRate_48k )
	{
	  DefaultLogSink().Warn("adjusting SampleRate to 24/1\n"); 
	  m_ADesc.SampleRate.Numerator = 24;
	  m_ADesc.SampleRate.Denominator = 1;
	}
      else
	{
	  // or we just drop the hammer
	  return RESULT_FORMAT;
	}
    }

  if( ASDCP_SUCCESS(result) )
    result = InitMXFIndex();

  if( ASDCP_SUCCESS(result) )
    result = InitInfo();

  // TODO: test file for sane CBR index BytesPerEditUnit

  return result;
}


//
//
ASDCP::Result_t
ASDCP::PCM::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
					    AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  return ReadEKLVPacket(FrameNum, FrameBuf, Dict::ul(MDD_WAVEssence), Ctx, HMAC);
}

//------------------------------------------------------------------------------------------


//
void
ASDCP::PCM::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06lu, %7lu bytes\n",
	  m_FrameNumber, m_Size);

  if ( dump_len )
    Kumu::hexdump(m_Data, dump_len, stream);
}

//------------------------------------------------------------------------------------------

ASDCP::PCM::MXFReader::MXFReader()
{
  m_Reader = new h__Reader;
}


ASDCP::PCM::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::PCM::MXFReader::OpenRead(const char* filename) const
{
  return m_Reader->OpenRead(filename);
}

// Reads a frame of essence from the MXF file. If the optional AESEncContext
// argument is present, the essence is decrypted after reading. If the MXF
// file is encrypted and the AESDecContext argument is NULL, the frame buffer
// will contain the ciphertext frame data.
ASDCP::Result_t
ASDCP::PCM::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				 AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::PCM::MXFReader::FillAudioDescriptor(AudioDescriptor& ADesc) const
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
ASDCP::PCM::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
ASDCP::PCM::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::PCM::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_FooterPart.Dump(stream);
}


//------------------------------------------------------------------------------------------

//
class ASDCP::PCM::MXFWriter::h__Writer : public ASDCP::h__Writer
{
public:
  AudioDescriptor m_ADesc;
  byte_t          m_EssenceUL[SMPTE_UL_LENGTH];


  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  
  h__Writer(){
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  ~h__Writer(){}

  Result_t OpenWrite(const char*, ui32_t HeaderSize);
  Result_t SetSourceStream(const AudioDescriptor&);
  Result_t WriteFrame(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
};



// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::OpenWrite(const char* filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new WaveAudioDescriptor;
      result = m_State.Goto_INIT();
    }

  return result;
}


// Automatically sets the MXF file's metadata from the WAV parser info.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::SetSourceStream(const AudioDescriptor& ADesc)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( ADesc.SampleRate != EditRate_24
       && ADesc.SampleRate != EditRate_48
       && ADesc.SampleRate != EditRate_23_98 )
    {
      DefaultLogSink().Error("AudioDescriptor.SampleRate is not 24/1, 48/1 or 24000/1001: %lu/%lu\n",
			     ADesc.SampleRate.Numerator, ADesc.SampleRate.Denominator);
      return RESULT_RAW_FORMAT;
    }

  if ( ADesc.AudioSamplingRate != SampleRate_48k )
    {
      DefaultLogSink().Error("AudioDescriptor.AudioSamplingRate is not 48000/1: %lu/%lu\n",
			     ADesc.AudioSamplingRate.Numerator, ADesc.AudioSamplingRate.Denominator);
      return RESULT_RAW_FORMAT;
    }

  m_ADesc = ADesc;
  Result_t result = PCM_ADesc_to_MD(m_ADesc, (WaveAudioDescriptor*)m_EssenceDescriptor);
  
  if ( ASDCP_SUCCESS(result) )
      result = WriteMXFHeader(PCM_PACKAGE_LABEL, UL(Dict::ul(MDD_WAVWrapping)),
			      SOUND_DEF_LABEL,   UL(Dict::ul(MDD_SoundDataDef)),
			      m_ADesc.SampleRate, 24 /* TCFrameRate */, calc_CBR_frame_size(m_Info, m_ADesc));

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, Dict::ul(MDD_WAVEssence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  return result;
}


//
//
ASDCP::Result_t
ASDCP::PCM::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx,
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
ASDCP::PCM::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteMXFFooter();
}


//------------------------------------------------------------------------------------------
//



ASDCP::PCM::MXFWriter::MXFWriter()
{
}

ASDCP::PCM::MXFWriter::~MXFWriter()
{
}


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::OpenWrite(const char* filename, const WriterInfo& Info,
				 const AudioDescriptor& ADesc, ui32_t HeaderSize)
{
  m_Writer = new h__Writer;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      m_Writer->m_Info = Info;
      result = m_Writer->SetSourceStream(ADesc);
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
ASDCP::PCM::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::PCM::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}

//
// end AS_DCP_PCM.cpp
//

