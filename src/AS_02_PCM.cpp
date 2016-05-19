/*
Copyright (c) 2011-2015, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
John Hurst

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


//------------------------------------------------------------------------------------------


class AS_02::PCM::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ui64_t m_ClipEssenceBegin, m_ClipSize;
  ui32_t m_ClipDurationFrames, m_BytesPerFrame;

  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  h__Reader(const Dictionary& d) : AS_02::h__AS02Reader(d), m_ClipEssenceBegin(0), m_ClipSize(0),
				   m_ClipDurationFrames(0) {}
  virtual ~h__Reader() {}

  ASDCP::Result_t    OpenRead(const std::string&, const ASDCP::Rational& edit_rate);
  ASDCP::Result_t    ReadFrame(ui32_t, ASDCP::PCM::FrameBuffer&, ASDCP::AESDecContext*, ASDCP::HMACContext*);
};

// TODO: This will ignore any body partitions past the first
//
//
ASDCP::Result_t
AS_02::PCM::MXFReader::h__Reader::OpenRead(const std::string& filename, const ASDCP::Rational& edit_rate)
{
  ASDCP::MXF::WaveAudioDescriptor* wave_descriptor = 0;
  IndexTableSegment::IndexEntry tmp_entry;
  Result_t result = OpenMXFRead(filename.c_str());

  if( KM_SUCCESS(result) )
    {
      InterchangeObject* tmp_obj = 0;

      if ( KM_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &tmp_obj)) )
	{
	  wave_descriptor = dynamic_cast<ASDCP::MXF::WaveAudioDescriptor*>(tmp_obj);
	}
    }

  if ( wave_descriptor == 0 )
    {
      DefaultLogSink().Error("WaveAudioDescriptor object not found.\n");
      result = RESULT_AS02_FORMAT;
    }

  if ( KM_SUCCESS(result) )
    result = m_IndexAccess.Lookup(0, tmp_entry);

  if ( KM_SUCCESS(result) )
    result = m_File.Seek(tmp_entry.StreamOffset);

  if ( KM_SUCCESS(result) )
    {
      assert(wave_descriptor);
      KLReader reader;
      result = reader.ReadKLFromFile(m_File);

      if ( KM_SUCCESS(result) )
	{
	  if ( ! UL(reader.Key()).MatchIgnoreStream(m_Dict->ul(MDD_WAVEssenceClip)) )
	    {
	      const MDDEntry *entry = m_Dict->FindULAnyVersion(reader.Key());

	      if ( entry == 0 )
		{
		  char buf[64];
		  DefaultLogSink().Error("Essence wrapper key is not WAVEssenceClip: %s\n", UL(reader.Key()).EncodeString(buf, 64));
		}
	      else
		{
		  DefaultLogSink().Error("Essence wrapper key is not WAVEssenceClip: %s\n", entry->name);
		}
	      
	      return RESULT_AS02_FORMAT;
	    }

	  if ( wave_descriptor->BlockAlign == 0 )
	    {
	      DefaultLogSink().Error("EssenceDescriptor has corrupt BlockAlign value, unable to continue.\n");
	      return RESULT_AS02_FORMAT;
	    }

	  if ( reader.Length() % wave_descriptor->BlockAlign != 0 )
	    {
	      DefaultLogSink().Error("Clip length is not an even multiple of BlockAlign, unable to continue.\n");
	      return RESULT_AS02_FORMAT;
	    }

	  m_ClipEssenceBegin = m_File.Tell();
	  m_ClipSize = reader.Length();
	  m_BytesPerFrame = AS_02::MXF::CalcFrameBufferSize(*wave_descriptor, edit_rate);
	  m_ClipDurationFrames = m_ClipSize / m_BytesPerFrame;

	  if ( m_ClipSize % m_BytesPerFrame > 0 )
	    {
	      ++m_ClipDurationFrames; // there is a partial frame at the end
	    }
	}
    }

  return result;
}

//
ASDCP::Result_t
AS_02::PCM::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, ASDCP::PCM::FrameBuffer& FrameBuf,
					    ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    {
      return RESULT_INIT;
    }

  if ( ! ( FrameNum < m_ClipDurationFrames ) )
    {
      return RESULT_RANGE;
    }

  assert(m_ClipEssenceBegin);
  ui64_t offset = FrameNum * m_BytesPerFrame;
  ui64_t position = m_ClipEssenceBegin + offset;
  Result_t result = RESULT_OK;

  if ( m_File.Tell() != position )
    {
      result = m_File.Seek(position);
    }

  if ( KM_SUCCESS(result) )
    {
      ui64_t remainder = m_ClipSize - offset;
      ui32_t read_size = ( remainder < m_BytesPerFrame ) ? remainder : m_BytesPerFrame;
      result = m_File.Read(FrameBuf.Data(), read_size);

      if ( KM_SUCCESS(result) )
	{
	  FrameBuf.Size(read_size);

	  if ( read_size < FrameBuf.Capacity() )
	    {
	      memset(FrameBuf.Data() + FrameBuf.Size(), 0, FrameBuf.Capacity() - FrameBuf.Size());
	    }
	}
    }

  return result;
}


//------------------------------------------------------------------------------------------
//


AS_02::PCM::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}

AS_02::PCM::MXFReader::~MXFReader()
{
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
AS_02::PCM::MXFReader::OpenRead(const std::string& filename, const ASDCP::Rational& edit_rate) const
{
  return m_Reader->OpenRead(filename, edit_rate);
}

//
Result_t
AS_02::PCM::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
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
    {
      m_Reader->m_HeaderPart.Dump(stream);
    }
}

//
void
AS_02::PCM::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->m_IndexAccess.Dump(stream);
    }
}


//------------------------------------------------------------------------------------------

//
class AS_02::PCM::MXFWriter::h__Writer : public AS_02::h__AS02WriterClip
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  ASDCP::MXF::WaveAudioDescriptor *m_WaveAudioDescriptor;
  byte_t m_EssenceUL[SMPTE_UL_LENGTH];
  ui32_t m_BytesPerSample;
  
  h__Writer(const Dictionary& d) : AS_02::h__AS02WriterClip(d), m_WaveAudioDescriptor(0), m_BytesPerSample(0)
  {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const std::string&, ASDCP::MXF::FileDescriptor* essence_descriptor,
		     ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list, const ui32_t& header_size);
  Result_t SetSourceStream(const ASDCP::Rational&);
  Result_t WriteFrame(const FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);
  Result_t Finalize();
};

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::OpenWrite(const std::string& filename, ASDCP::MXF::FileDescriptor* essence_descriptor,
					    ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list, const ui32_t& header_size)
{
  assert(essence_descriptor);

  m_WaveAudioDescriptor = dynamic_cast<ASDCP::MXF::WaveAudioDescriptor*>(essence_descriptor);

  if ( m_WaveAudioDescriptor == 0 )
    {
      DefaultLogSink().Error("Essence descriptor is not a WaveAudioDescriptor.\n");
      essence_descriptor->Dump();
      return RESULT_AS02_FORMAT;
    }

  if ( ! m_State.Test_BEGIN() )
    {
      return RESULT_STATE;
    }

  Result_t result = m_File.OpenWrite(filename.c_str());

  if ( KM_SUCCESS(result) )
    {
      m_HeaderSize = header_size;
      m_EssenceDescriptor = essence_descriptor;
      m_WaveAudioDescriptor->SampleRate = m_WaveAudioDescriptor->AudioSamplingRate;

      ASDCP::MXF::InterchangeObject_list_t::iterator i;
      for ( i = essence_sub_descriptor_list.begin(); i != essence_sub_descriptor_list.end(); ++i )
	{
	  if ( (*i)->GetUL() != UL(m_Dict->ul(MDD_AudioChannelLabelSubDescriptor))
	       && (*i)->GetUL() != UL(m_Dict->ul(MDD_SoundfieldGroupLabelSubDescriptor))
	       && (*i)->GetUL() != UL(m_Dict->ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor)) )
	    {
	      DefaultLogSink().Error("Essence sub-descriptor is not an MCALabelSubDescriptor.\n");
	      (*i)->Dump();
	    }

	  m_EssenceSubDescriptorList.push_back(*i);
	  GenRandomValue((*i)->InstanceUID);
	  m_EssenceDescriptor->SubDescriptors.push_back((*i)->InstanceUID);
	  *i = 0; // parent will only free the ones we don't keep
	}

      result = m_State.Goto_INIT();
    }

  return result;
}


// Automatically sets the MXF file's metadata from the WAV parser info.
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::SetSourceStream(const ASDCP::Rational& edit_rate)
{
  if ( ! m_State.Test_INIT() )
    {
      return RESULT_STATE;
    }

  memcpy(m_EssenceUL, m_Dict->ul(MDD_WAVEssenceClip), SMPTE_UL_LENGTH);
  m_EssenceUL[15] = 1; // set the stream identifier
  Result_t result = m_State.Goto_READY();

  if ( KM_SUCCESS(result) )
    {
      assert(m_WaveAudioDescriptor);
      m_BytesPerSample = AS_02::MXF::CalcSampleSize(*m_WaveAudioDescriptor);
      result = WriteAS02Header(PCM_PACKAGE_LABEL, UL(m_Dict->ul(MDD_WAVWrappingClip)),
			       SOUND_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_SoundDataDef)),
			       m_EssenceDescriptor->SampleRate, derive_timecode_rate_from_edit_rate(edit_rate));

      if ( KM_SUCCESS(result) )
	{
	  this->m_IndexWriter.SetEditRate(m_WaveAudioDescriptor->AudioSamplingRate,
					  AS_02::MXF::CalcSampleSize(*m_WaveAudioDescriptor));
	}
    }

  return result;
}


//
//
ASDCP::Result_t
AS_02::PCM::MXFWriter::h__Writer::WriteFrame(const FrameBuffer& frame_buf, AESEncContext* Ctx,
					     HMACContext* HMAC)
{
  if ( frame_buf.Size() == 0 )
    {
      DefaultLogSink().Error("The frame buffer size is zero.\n");
      return RESULT_PARAM;
    }

  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    {
      result = m_State.Goto_RUNNING(); // first time through
    }

  if ( KM_SUCCESS(result) && ! HasOpenClip() )
    {
      result = StartClip(m_EssenceUL, Ctx, HMAC);
    }

  if ( KM_SUCCESS(result) )
    {
      result = WriteClipBlock(frame_buf);
    }

  if ( KM_SUCCESS(result) )
    {
      m_FramesWritten += frame_buf.Size() / m_BytesPerSample;
    }

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

  Result_t result = FinalizeClip(AS_02::MXF::CalcSampleSize(*m_WaveAudioDescriptor));

  if ( KM_SUCCESS(result) )
    {
      m_WaveAudioDescriptor->ContainerDuration = m_IndexWriter.m_Duration = m_FramesWritten;
      WriteAS02Footer();
    }

  return result;
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
AS_02::PCM::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				 ASDCP::MXF::FileDescriptor* essence_descriptor,
				 ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
				 const ASDCP::Rational& edit_rate, ui32_t header_size)
{
  if ( essence_descriptor == 0 )
    {
      DefaultLogSink().Error("Essence descriptor object required.\n");
      return RESULT_PARAM;
    }

  if ( Info.EncryptedEssence )
    {
      DefaultLogSink().Error("Encryption not supported for ST 382 clip-wrap.\n");
      return Kumu::RESULT_NOTIMPL;
    }

  m_Writer = new h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, essence_descriptor, essence_sub_descriptor_list, header_size);

  if ( KM_SUCCESS(result) )
    result = m_Writer->SetSourceStream(edit_rate);

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

