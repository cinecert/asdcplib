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

static byte_t SNDFMT_CFG_1_UL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0b,
				      0x04, 0x02, 0x02, 0x10, 0x03, 0x01, 0x01, 0x00 };

static byte_t SNDFMT_CFG_2_UL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0b,
				      0x04, 0x02, 0x02, 0x10, 0x03, 0x01, 0x02, 0x00 };

static byte_t SNDFMT_CFG_3_UL[16] = { 0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0b,
				      0x04, 0x02, 0x02, 0x10, 0x03, 0x01, 0x03, 0x00 };

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


class AS_02::PCM::MXFReader::h__Reader : public AS_02::h__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  ASDCP::PCM::AudioDescriptor m_ADesc;

  h__Reader(const Dictionary& d) : AS_02::h__Reader(d) {}
  ~h__Reader() {}
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
      InterchangeObject* Object;
      if ( ASDCP_SUCCESS(m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(WaveAudioDescriptor), &Object)) )
	{
	  assert(Object);
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
ASDCP::MXF::OPAtomHeader&
AS_02::PCM::MXFReader::OPAtomHeader()
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
AS_02::PCM::MXFReader::OPAtomIndexFooter()
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
    m_Reader->m_FooterPart.Dump(stream);
}


//------------------------------------------------------------------------------------------

//
class AS_02::PCM::MXFWriter::h__Writer : public AS_02::h__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  ASDCP::PCM::AudioDescriptor m_ADesc;
  byte_t          m_EssenceUL[SMPTE_UL_LENGTH];
  ui64_t			m_KLV_start;

  h__Writer(const Dictionary& d) : AS_02::h__Writer(d), m_KLV_start(0){
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);

  }

  ~h__Writer(){}

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
  Result_t WriteMXFFooter();

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

      result = WriteMXFHeader(PCM_PACKAGE_LABEL, UL(m_Dict->ul(MDD_WAVWrapping)),
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

  return WriteMXFFooter();
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
ASDCP::MXF::OPAtomHeader&
AS_02::PCM::MXFWriter::OPAtomHeader()
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
AS_02::PCM::MXFWriter::OPAtomIndexFooter()
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

// standard method of opening an MXF file for read
Result_t
AS_02::PCM::MXFReader::h__Reader::OpenMXFRead(const char* filename)
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

	  while ( r_i != m_HeaderPart.m_RIP.PairArray.end() && i < partition_size )
	    {
	      m_File.Seek((*r_i).ByteOffset);
	      pPart = new Partition(this->m_Dict);
	      result = pPart->InitFromFile(m_File);

	      // TODO:: expect Index partition
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

	      if ( ASDCP_FAILURE(result) )
		{
		  break;
		}

	      if(i==3)
		{
		  m_EssenceStart = EssenceStart;
		  m_pCurrentBodyPartition = pPart;
		  m_pCurrentIndexPartition = pCurrentBodyPartIndex;
		}
	  
	      m_BodyPartList.push_back(pCurrentBodyPartIndex);
	      m_BodyPartList.push_back(pPart);
	      r_i++; i++;
	    }
	}
    }

  return result;
}

// standard method of reading a plaintext or encrypted frame
Result_t
AS_02::PCM::MXFReader::h__Reader::ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
						const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;
  // look up frame index node
  IndexTableSegment::IndexEntry TmpEntry;
  ui32_t i = 0;

  if(m_pCurrentIndexPartition == NULL)
    {
      m_pCurrentIndexPartition = dynamic_cast<AS_02::MXF::OP1aIndexBodyPartion*> (m_BodyPartList.at(i));
      m_pCurrentBodyPartition = m_BodyPartList.at(i+1);
    }		
  else
    {
      return RESULT_FORMAT; //return error
    }
				
  if(m_pCurrentIndexPartition == NULL)
    {
      return RESULT_FORMAT; //return error
    }

  m_pCurrentIndexPartition->PCMIndexLookup(FrameNum,TmpEntry);
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

Result_t
AS_02::PCM::MXFReader::h__Reader::ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
						 const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  KLReader Reader;

  //save position to read KLV packet
  Kumu::fpos_t SaveFilePosition = m_File.Tell();
  //Seek backward to KLV start
  m_File.Seek(this->m_EssenceStart);
  Result_t result = Reader.ReadKLFromFile(m_File);
  //set old position
  m_File.Seek(SaveFilePosition);

  if ( ASDCP_FAILURE(result) )
    return result;

  UL Key(Reader.Key());
  ui64_t PacketLength = Reader.Length();
  m_LastPosition = m_LastPosition + PacketLength;
  if(FrameNum = 0){
    m_LastPosition+= Reader.KLLength();
  }
  assert(m_Dict);

  //TODO: for AS_02 PCM - not in the dictionary

  static const byte_t WaveClipEssenceUL_Data[SMPTE_UL_LENGTH] =
    { 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
      0x0d, 0x01, 0x03, 0x01, 0x16, 0x01, 0x02, 0x01 };

  
    if( memcmp(Key.Value(), WaveClipEssenceUL_Data, SMPTE_UL_LENGTH) == 0 ){
      byte_t WaveFrameEssenceUL_Data[SMPTE_UL_LENGTH] =
	{ 0x06, 0x0e, 0x2b, 0x34, 0x01, 0x02, 0x01, 0x01,
	  0x0d, 0x01, 0x03, 0x01, 0x16, 0x01, 0x01, 0x01 };

      Key.Set(WaveFrameEssenceUL_Data);
    }


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
	    FrameBuffer TmpWrapper;
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
AS_02::PCM::MXFWriter::h__Writer::WriteMXFFooter()
{
  // Set top-level file package correctly for OP-Atom

  //  m_MPTCSequence->Duration = m_MPTimecode->Duration = m_MPClSequence->Duration = m_MPClip->Duration = 
  //    m_FPTCSequence->Duration = m_FPTimecode->Duration = m_FPClSequence->Duration = m_FPClip->Duration = 

  //
  m_CurrentIndexBodyPartition->m_FramesWritten = m_FramesWritten;
  m_CurrentIndexBodyPartition->PCMSetIndexParamsCBR(m_CurrentIndexBodyPartition->m_BytesPerEditUnit+24,m_CurrentIndexBodyPartition->m_BytesPerEditUnit);
  CompleteIndexBodyPart();

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
  UL OP1aUL(m_Dict->ul(MDD_OP1a));
  m_HeaderPart.OperationalPattern = OP1aUL;
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
  std::vector<Partition*>::iterator bl_i = this->m_BodyPartList.begin();
  for (; bl_i != m_BodyPartList.end(); bl_i++ ){
    (*bl_i)->FooterPartition =  m_FooterPart.ThisPartition;
    if ( ASDCP_SUCCESS(result) )
      result = m_File.Seek((*bl_i)->ThisPartition);
    if ( ASDCP_SUCCESS(result) ){
      UL BodyUL(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = (*bl_i)->WriteToFile(m_File, BodyUL);
    }
  } 

  m_File.Close();
  return result;
}

// standard method of writing a plaintext or encrypted frame
Result_t
AS_02::PCM::MXFWriter::h__Writer::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf, const byte_t* EssenceUL,
						  AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;
  IntegrityPack IntPack;
	
  const ui32_t AS_02_PCM_MXF_BER_LENGTH = 8;
  //TODO: AS_02_PCM_MXF_BER_LENGTH - customize for EncryptedEssence 

  byte_t overhead[128];
  Kumu::MemIOWriter Overhead(overhead, 128);
  assert(m_Dict);

  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("Cannot write empty frame buffer\n");
      return RESULT_EMPTY_FB;
    }

  if ( m_Info.EncryptedEssence )
    {
      if ( ! Ctx )
	return RESULT_CRYPT_CTX;

      if ( m_Info.UsesHMAC && ! HMAC )
	return RESULT_HMAC_CTX;

      if ( FrameBuf.PlaintextOffset() > FrameBuf.Size() )
	return RESULT_LARGE_PTO;

      // encrypt the essence data (create encrypted source value)
      result = EncryptFrameBuffer(FrameBuf, m_CtFrameBuf, Ctx);

      // create HMAC
      if ( ASDCP_SUCCESS(result) && m_Info.UsesHMAC )
	result = IntPack.CalcValues(m_CtFrameBuf, m_Info.AssetUUID, m_FramesWritten + 1, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{ // write UL
	  Overhead.WriteRaw(m_Dict->ul(MDD_CryptEssence), SMPTE_UL_LENGTH);

	  // construct encrypted triplet header
	  ui32_t ETLength = klv_cryptinfo_size + m_CtFrameBuf.Size();
	  ui32_t BER_length = MXF_BER_LENGTH;

	  if ( m_Info.UsesHMAC )
	    ETLength += klv_intpack_size;
	  else
	    ETLength += (MXF_BER_LENGTH * 3); // for empty intpack

	  if ( ETLength > 0x00ffffff ) // Need BER integer longer than MXF_BER_LENGTH bytes
	    {
	      BER_length = Kumu::get_BER_length_for_value(ETLength);

	      // the packet is longer by the difference in expected vs. actual BER length
	      ETLength += BER_length - MXF_BER_LENGTH;

	      if ( BER_length == 0 )
		result = RESULT_KLV_CODING;
	    }

	  if ( ASDCP_SUCCESS(result) )
	    {
	      if ( ! ( Overhead.WriteBER(ETLength, BER_length)                      // write encrypted triplet length
		       && Overhead.WriteBER(UUIDlen, MXF_BER_LENGTH)                // write ContextID length
		       && Overhead.WriteRaw(m_Info.ContextID, UUIDlen)              // write ContextID
		       && Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH)         // write PlaintextOffset length
		       && Overhead.WriteUi64BE(FrameBuf.PlaintextOffset())          // write PlaintextOffset
		       && Overhead.WriteBER(SMPTE_UL_LENGTH, MXF_BER_LENGTH)        // write essence UL length
		       && Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH)    // write the essence UL
		       && Overhead.WriteBER(sizeof(ui64_t), MXF_BER_LENGTH)         // write SourceLength length
		       && Overhead.WriteUi64BE(FrameBuf.Size())                     // write SourceLength
		       && Overhead.WriteBER(m_CtFrameBuf.Size(), BER_length) ) )    // write ESV length
		{
		  result = RESULT_KLV_CODING;
		}
	    }

	  if ( ASDCP_SUCCESS(result) )
	    result = m_File.Writev(Overhead.Data(), Overhead.Length());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += Overhead.Length();
	  // write encrypted source value
	  result = m_File.Writev((byte_t*)m_CtFrameBuf.RoData(), m_CtFrameBuf.Size());
	}

      if ( ASDCP_SUCCESS(result) )
	{
	  m_StreamOffset += m_CtFrameBuf.Size();

	  byte_t hmoverhead[512];
	  Kumu::MemIOWriter HMACOverhead(hmoverhead, 512);

	  // write the HMAC
	  if ( m_Info.UsesHMAC )
	    {
	      HMACOverhead.WriteRaw(IntPack.Data, klv_intpack_size);
	    }
	  else
	    { // we still need the var-pack length values if the intpack is empty
	      for ( ui32_t i = 0; i < 3 ; i++ )
		HMACOverhead.WriteBER(0, MXF_BER_LENGTH);
	    }

	  // write HMAC
	  result = m_File.Writev(HMACOverhead.Data(), HMACOverhead.Length());
	  m_StreamOffset += HMACOverhead.Length();
	}
    }
  else
    {
      if(m_FramesWritten == 0){
	ui32_t BER_length = AS_02_PCM_MXF_BER_LENGTH; //MXF_BER_LENGTH;

	if ( FrameBuf.Size() > 0x00ffffff ) // Need BER integer longer than MXF_BER_LENGTH bytes
	  {
	    BER_length = Kumu::get_BER_length_for_value(FrameBuf.Size());

	    if ( BER_length == 0 )
	      result = RESULT_KLV_CODING;
	  }

	Overhead.WriteRaw((byte_t*)EssenceUL, SMPTE_UL_LENGTH);
	Overhead.WriteBER(FrameBuf.Size(), BER_length);

	//position of the KLV start
	m_KLV_start = m_File.Tell();

	if ( ASDCP_SUCCESS(result) )
	  result = m_File.Writev(Overhead.Data(), Overhead.Length());

	if ( ASDCP_SUCCESS(result) )
	  result = m_File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

	if ( ASDCP_SUCCESS(result) )
	  m_StreamOffset += Overhead.Length() + FrameBuf.Size();
      }
      else{
	//update the KLV - length; new value plus old value from length field
	//necessary to know position of length field -> bodyPartition + 8
	//update every time when writing new essence or at the end of writing

	if ( ASDCP_SUCCESS(result) )
	  result = m_File.Writev((byte_t*)FrameBuf.RoData(), FrameBuf.Size());

	if ( ASDCP_SUCCESS(result) )
	  m_StreamOffset += FrameBuf.Size();
      }
    }

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Writev();

  return result;
}

//
// end AS_02_PCM.cpp
//

