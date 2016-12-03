/*
Copyright (c) 2011-2016, John Hurst

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
/*! \file    AS_02_PHDR.cpp
  \version $Id$
  \brief   AS-02 library, JPEG 2000 P-HDR essence reader and writer implementation
*/

#include "AS_02_internal.h"
#include "AS_02_PHDR.h"

#include <iostream>
#include <iomanip>

using namespace ASDCP;
using namespace ASDCP::JP2K;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string JP2K_PACKAGE_LABEL = "File Package: PROTOTYPE SMPTE ST 422 / ST 2067-5 frame wrapping of JPEG 2000 codestreams with HDR metadata";
static std::string PICT_DEF_LABEL = "PHDR Image Track";
static std::string MD_DEF_LABEL = "PHDR Metadata Track";



//------------------------------------------------------------------------------------------

//
void
AS_02::PHDR::FrameBuffer::Dump(FILE* stream, ui32_t dump_bytes) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame %d, %d bytes (metadata: %zd bytes)\n", FrameNumber(), Size(), OpaqueMetadata.size());

  if ( dump_bytes > 0 )
    {
      Kumu::hexdump(RoData(), Kumu::xmin(dump_bytes, Size()), stream);
    }
}


//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG 2000 reader


class AS_02::PHDR::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  h__Reader(const Dictionary& d) :
    AS_02::h__AS02Reader(d) {}

  virtual ~h__Reader() {}

  Result_t    OpenRead(const std::string& filename, std::string& PHDR_master_metadata);
  Result_t    ReadFrame(ui32_t, AS_02::PHDR::FrameBuffer&, AESDecContext*, HMACContext*);
};

//
Result_t
AS_02::PHDR::MXFReader::h__Reader::OpenRead(const std::string& filename, std::string& PHDR_master_metadata)
{
  Result_t result = OpenMXFRead(filename.c_str());
  ui32_t SimplePayloadSID = 0;

  if( KM_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(CDCIEssenceDescriptor), &tmp_iobj);

      if ( tmp_iobj == 0 )
	{
	  m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
	}

      if ( tmp_iobj == 0 )
	{
	  DefaultLogSink().Error("RGBAEssenceDescriptor nor CDCIEssenceDescriptor found.\n");
	  return RESULT_AS02_FORMAT;
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEG2000PictureSubDescriptor), &tmp_iobj);

      if ( tmp_iobj == 0 )
	{
	  DefaultLogSink().Error("JPEG2000PictureSubDescriptor not found.\n");
	  return RESULT_AS02_FORMAT;
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(PHDRMetadataTrackSubDescriptor), &tmp_iobj);

      if ( tmp_iobj == 0 )
	{
	  DefaultLogSink().Error("PHDRMetadataTrackSubDescriptor not found.\n");
	  return RESULT_AS02_FORMAT;
	}
      else
	{
	  PHDRMetadataTrackSubDescriptor *tmp_desc = dynamic_cast<PHDRMetadataTrackSubDescriptor*>(tmp_iobj);
	  assert(tmp_desc);
	  SimplePayloadSID = tmp_desc->SimplePayloadSID;
	}

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_AS02_FORMAT;
	}
    }

  // if PHDRSimplePayload exists, go get it
  if ( KM_SUCCESS(result) && SimplePayloadSID )
    {
      RIP::const_pair_iterator pi;
      RIP::PartitionPair TmpPair;
      
      // Look up the partition start in the RIP using the SID.
      for ( pi = m_RIP.PairArray.begin(); pi != m_RIP.PairArray.end(); ++pi )
	{
	  if ( (*pi).BodySID == SimplePayloadSID )
	    {
	      TmpPair = *pi;
	      break;
	    }
	}

      if ( TmpPair.ByteOffset == 0 )
	{
	  DefaultLogSink().Error("Body SID not found in RIP set: %d\n", SimplePayloadSID);
	  return RESULT_AS02_FORMAT;
	}

      // seek to the start of the partition
      if ( (Kumu::fpos_t)TmpPair.ByteOffset != m_LastPosition )
	{
	  m_LastPosition = TmpPair.ByteOffset;
	  result = m_File.Seek(TmpPair.ByteOffset);
	}

      // read the partition header
      ASDCP::MXF::Partition GSPart(m_Dict);
      result = GSPart.InitFromFile(m_File);

      if ( KM_SUCCESS(result) )
	{
	  // read the generic stream packet
	  if ( KM_SUCCESS(result) )
	    {
	      ASDCP::FrameBuffer tmp_buf;
	      tmp_buf.Capacity(Kumu::Megabyte);

	      result = Read_EKLV_Packet(m_File, *m_Dict, m_Info, m_LastPosition, m_CtFrameBuf,
					0, 0, tmp_buf, m_Dict->ul(MDD_GenericStream_DataElement), 0, 0);

	      if ( KM_SUCCESS(result) )
		{
		  PHDR_master_metadata.assign((const char*)tmp_buf.RoData(), tmp_buf.Size());
		}
	    }
	}
    }

  return result;
}

//
//
Result_t
AS_02::PHDR::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, AS_02::PHDR::FrameBuffer& FrameBuf,
		      ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  Result_t result = ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEG2000Essence), Ctx, HMAC);

  if ( KM_SUCCESS(result) )
    {
      ASDCP::FrameBuffer tmp_metadata_buffer;
      tmp_metadata_buffer.Capacity(8192);

      result = Read_EKLV_Packet(m_File, *m_Dict, m_Info, m_LastPosition, m_CtFrameBuf,
				FrameNum, FrameNum + 1, tmp_metadata_buffer, m_Dict->ul(MDD_PHDRImageMetadataItem), Ctx, HMAC);

      if ( KM_SUCCESS(result) )
	{
	  FrameBuf.OpaqueMetadata.assign((const char*)tmp_metadata_buffer.RoData(), tmp_metadata_buffer.Size());
	}
      else
	{
	  DefaultLogSink().Error("Metadata packet not found at frame %d.\n", FrameNum);
	  result = RESULT_OK;
	}
    }

  return result;
}

//------------------------------------------------------------------------------------------
//

AS_02::PHDR::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(DefaultCompositeDict());
}


AS_02::PHDR::MXFReader::~MXFReader()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::PHDR::MXFReader::OP1aHeader()
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
AS_02::PHDR::MXFReader::AS02IndexReader()
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
AS_02::PHDR::MXFReader::RIP()
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
AS_02::PHDR::MXFReader::OpenRead(const std::string& filename, std::string& PHDR_master_metadata) const
{
  return m_Reader->OpenRead(filename, PHDR_master_metadata);
}

//
Result_t
AS_02::PHDR::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
Result_t
AS_02::PHDR::MXFReader::ReadFrame(ui32_t FrameNum, AS_02::PHDR::FrameBuffer& FrameBuf,
					   ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
Result_t
AS_02::PHDR::MXFReader::FillWriterInfo(WriterInfo& Info) const
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
class AS_02::PHDR::MXFWriter::h__Writer : public AS_02::h__AS02WriterFrame
{
  PHDRMetadataTrackSubDescriptor *m_MetadataTrackSubDescriptor;

  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

  JPEG2000PictureSubDescriptor* m_EssenceSubDescriptor;

  Result_t WritePHDRHeader(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
			   const std::string& TrackName, const ASDCP::UL& EssenceUL,
			   const ASDCP::UL& DataDefinition, const ASDCP::Rational& EditRate,
			   const ui32_t& TCFrameRate);

public:
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];
  byte_t            m_MetadataUL[SMPTE_UL_LENGTH];

  h__Writer(const Dictionary& d) : h__AS02WriterFrame(d), m_EssenceSubDescriptor(0), m_MetadataTrackSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
    memset(m_MetadataUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer(){}

  Result_t OpenWrite(const std::string&, ASDCP::MXF::FileDescriptor* essence_descriptor,
		     ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
		     const AS_02::IndexStrategy_t& IndexStrategy,
		     const ui32_t& PartitionSpace, const ui32_t& HeaderSize);
  Result_t SetSourceStream(const std::string& label, const ASDCP::Rational& edit_rate);
  Result_t WriteFrame(const AS_02::PHDR::FrameBuffer&, ASDCP::AESEncContext*, ASDCP::HMACContext*);
  Result_t Finalize(const std::string& PHDR_master_metadata);
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t
AS_02::PHDR::MXFWriter::h__Writer::OpenWrite(const std::string& filename,
					     ASDCP::MXF::FileDescriptor* essence_descriptor,
					     ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
					     const AS_02::IndexStrategy_t& IndexStrategy,
					     const ui32_t& PartitionSpace_sec, const ui32_t& HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    {
      return RESULT_STATE;
    }

  if ( m_IndexStrategy != AS_02::IS_FOLLOW )
    {
      DefaultLogSink().Error("Only strategy IS_FOLLOW is supported at this time.\n");
      return Kumu::RESULT_NOTIMPL;
    }

  Result_t result = m_File.OpenWrite(filename);

  if ( KM_SUCCESS(result) )
    {
      m_IndexStrategy = IndexStrategy;
      m_PartitionSpace = PartitionSpace_sec; // later converted to edit units by WritePHDRHeader()
      m_HeaderSize = HeaderSize;

      if ( essence_descriptor->GetUL() != UL(m_Dict->ul(MDD_RGBAEssenceDescriptor))
	   && essence_descriptor->GetUL() != UL(m_Dict->ul(MDD_CDCIEssenceDescriptor)) )
	{
	  DefaultLogSink().Error("Essence descriptor is not a RGBAEssenceDescriptor or CDCIEssenceDescriptor.\n");
	  essence_descriptor->Dump();
	  return RESULT_AS02_FORMAT;
	}

      m_EssenceDescriptor = essence_descriptor;

      ASDCP::MXF::InterchangeObject_list_t::iterator i;
      for ( i = essence_sub_descriptor_list.begin(); i != essence_sub_descriptor_list.end(); ++i )
	{
	  if ( (*i)->GetUL() != UL(m_Dict->ul(MDD_JPEG2000PictureSubDescriptor)) )
	    {
	      DefaultLogSink().Error("Essence sub-descriptor is not a JPEG2000PictureSubDescriptor.\n");
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

// all the above for a single source clip
Result_t
AS_02::PHDR::MXFWriter::h__Writer::WritePHDRHeader(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
						   const std::string& TrackName, const ASDCP::UL& EssenceUL,
						   const ASDCP::UL& DataDefinition, const ASDCP::Rational& EditRate,
						   const ui32_t& TCFrameRate)
{
  if ( EditRate.Numerator == 0 || EditRate.Denominator == 0 )
    {
      DefaultLogSink().Error("Non-zero edit-rate reqired.\n");
      return RESULT_PARAM;
    }
  
  InitHeader(MXFVersion_2011);
  
  AddSourceClip(EditRate, EditRate/*TODO: for a moment*/, TCFrameRate, TrackName, EssenceUL, DataDefinition, PackageLabel);

  // add metadata track
  TrackSet<SourceClip> metdata_track =
    CreateTrackAndSequence<SourcePackage, SourceClip>(m_HeaderPart, *m_FilePackage,
						      MD_DEF_LABEL, EditRate,
						      UL(m_Dict->ul(MDD_PHDRImageMetadataItem)),
						      3 /* track id */, m_Dict);

  metdata_track.Sequence->Duration.set_has_value();
  m_DurationUpdateList.push_back(&(metdata_track.Sequence->Duration.get()));
  // Consult ST 379:2004 Sec. 6.3, "Element to track relationship" to see where "12" comes from.
  metdata_track.Track->TrackNumber = KM_i32_BE(Kumu::cp2i<ui32_t>((UL(m_MetadataUL).Value() + 12)));

  metdata_track.Clip = new SourceClip(m_Dict);
  m_HeaderPart.AddChildObject(metdata_track.Clip);
  metdata_track.Sequence->StructuralComponents.push_back(metdata_track.Clip->InstanceUID);
  metdata_track.Clip->DataDefinition = UL(m_Dict->ul(MDD_PHDRImageMetadataWrappingFrame));

  // for now we do not allow setting this value, so all files will be 'original'
  metdata_track.Clip->SourceTrackID = 0;
  metdata_track.Clip->SourcePackageID = NilUMID;
  
  metdata_track.Clip->Duration.set_has_value();
  m_DurationUpdateList.push_back(&(metdata_track.Clip->Duration.get()));

  // add PHDR subdescriptor
  m_MetadataTrackSubDescriptor = new PHDRMetadataTrackSubDescriptor(m_Dict);
  m_EssenceSubDescriptorList.push_back(m_MetadataTrackSubDescriptor);
  GenRandomValue(m_MetadataTrackSubDescriptor->InstanceUID);
  m_EssenceDescriptor->SubDescriptors.push_back(m_MetadataTrackSubDescriptor->InstanceUID);
  m_MetadataTrackSubDescriptor->DataDefinition = UL(m_Dict->ul(MDD_PHDRImageMetadataWrappingFrame));
  m_MetadataTrackSubDescriptor->SourceTrackID = 3;
  m_MetadataTrackSubDescriptor->SimplePayloadSID = 0;

  AddEssenceDescriptor(WrappingUL);

  m_IndexWriter.SetPrimerLookup(&m_HeaderPart.m_Primer);
  m_RIP.PairArray.push_back(RIP::PartitionPair(0, 0)); // Header partition RIP entry
  m_IndexWriter.OperationalPattern = m_HeaderPart.OperationalPattern;
  m_IndexWriter.EssenceContainers = m_HeaderPart.EssenceContainers;

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( KM_SUCCESS(result) )
    {
      m_PartitionSpace *= floor( EditRate.Quotient() + 0.5 );  // convert seconds to edit units
      m_ECStart = m_File.Tell();
      m_IndexWriter.IndexSID = 129;

      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      Partition body_part(m_Dict);
      body_part.BodySID = 1;
      body_part.MajorVersion = m_HeaderPart.MajorVersion;
      body_part.MinorVersion = m_HeaderPart.MinorVersion;
      body_part.OperationalPattern = m_HeaderPart.OperationalPattern;
      body_part.EssenceContainers = m_HeaderPart.EssenceContainers;
      body_part.ThisPartition = m_ECStart;
      result = body_part.WriteToFile(m_File, body_ul);
      m_RIP.PairArray.push_back(RIP::PartitionPair(1, body_part.ThisPartition)); // Second RIP Entry
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
Result_t
AS_02::PHDR::MXFWriter::h__Writer::SetSourceStream(const std::string& label, const ASDCP::Rational& edit_rate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    {
      return RESULT_STATE;
    }

  memcpy(m_EssenceUL, m_Dict->ul(MDD_JPEG2000Essence), SMPTE_UL_LENGTH);
  m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first track of the essence container

  memcpy(m_MetadataUL, m_Dict->ul(MDD_PHDRImageMetadataItem), SMPTE_UL_LENGTH);
  m_MetadataUL[SMPTE_UL_LENGTH-1] = 3; // third track of the essence container

  Result_t result = m_State.Goto_READY();

  if ( KM_SUCCESS(result) )
    {
      result = WritePHDRHeader(label, UL(m_Dict->ul(MDD_MXFGCFUFrameWrappedPictureElement)),
			       PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
			       edit_rate, derive_timecode_rate_from_edit_rate(edit_rate));

      if ( KM_SUCCESS(result) )
	{
	  this->m_IndexWriter.SetPrimerLookup(&this->m_HeaderPart.m_Primer);
	}
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
Result_t
AS_02::PHDR::MXFWriter::h__Writer::WriteFrame(const AS_02::PHDR::FrameBuffer& FrameBuf,
					      AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( FrameBuf.Size() == 0 )
    {
      DefaultLogSink().Error("The frame buffer size is zero.\n");
      return RESULT_PARAM;
    }

  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    {
      result = m_State.Goto_RUNNING(); // first time through
    }

  if ( KM_SUCCESS(result) )
    {
      ui64_t this_stream_offset = m_StreamOffset; // m_StreamOffset will be changed by the call to Write_EKLV_Packet

      result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				 m_StreamOffset, FrameBuf, m_EssenceUL, Ctx, HMAC);
      
      if ( KM_SUCCESS(result) )
	{
	  ASDCP::FrameBuffer metadata_buffer_wrapper;
	  metadata_buffer_wrapper.SetData((byte_t*)(FrameBuf.OpaqueMetadata.c_str()), FrameBuf.OpaqueMetadata.size());
	  metadata_buffer_wrapper.Size(FrameBuf.OpaqueMetadata.size());
	  
	  
	  result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				     m_StreamOffset, metadata_buffer_wrapper, m_MetadataUL, Ctx, HMAC);
	}
      
      if ( KM_SUCCESS(result) )
	{  
	  IndexTableSegment::IndexEntry Entry;
	  Entry.StreamOffset = this_stream_offset;
	  m_IndexWriter.PushIndexEntry(Entry);
	}

      if ( m_FramesWritten > 1 && ( ( m_FramesWritten + 1 ) % m_PartitionSpace ) == 0 )
	{
	  m_IndexWriter.ThisPartition = m_File.Tell();
	  m_IndexWriter.WriteToFile(m_File);
	  m_RIP.PairArray.push_back(RIP::PartitionPair(0, m_IndexWriter.ThisPartition));

	  UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
	  Partition body_part(m_Dict);
	  body_part.BodySID = 1;
	  body_part.MajorVersion = m_HeaderPart.MajorVersion;
	  body_part.MinorVersion = m_HeaderPart.MinorVersion;
	  body_part.OperationalPattern = m_HeaderPart.OperationalPattern;
	  body_part.EssenceContainers = m_HeaderPart.EssenceContainers;
	  body_part.ThisPartition = m_File.Tell();

	  body_part.BodyOffset = m_StreamOffset;
	  result = body_part.WriteToFile(m_File, body_ul);
	  m_RIP.PairArray.push_back(RIP::PartitionPair(1, body_part.ThisPartition));
	}
    }

  if ( KM_SUCCESS(result) )
    {
      m_FramesWritten++;
    }

  return result;
}

// Closes the MXF file, writing the index and other closing information.
//
Result_t
AS_02::PHDR::MXFWriter::h__Writer::Finalize(const std::string& PHDR_master_metadata)
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  Result_t result = m_State.Goto_FINAL();

  if ( KM_SUCCESS(result) )
    {
      if ( m_IndexWriter.GetDuration() > 0 )
	{
	  m_IndexWriter.ThisPartition = this->m_File.Tell();
	  m_IndexWriter.WriteToFile(this->m_File);
	  m_RIP.PairArray.push_back(RIP::PartitionPair(0, this->m_IndexWriter.ThisPartition));
	}

      if ( ! PHDR_master_metadata.empty() )
	{
	  // write PHDRSimplePayload
	  Kumu::fpos_t here = m_File.Tell();

	  // create generic stream partition header
	  static UL GenericStream_DataElement(m_Dict->ul(MDD_GenericStream_DataElement));
	  ASDCP::MXF::Partition GSPart(m_Dict);

	  GSPart.MajorVersion = m_HeaderPart.MajorVersion;
	  GSPart.MinorVersion = m_HeaderPart.MinorVersion;
	  GSPart.ThisPartition = here;
	  GSPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;
	  GSPart.OperationalPattern = m_HeaderPart.OperationalPattern;
	  GSPart.BodySID = 2;
  	  m_MetadataTrackSubDescriptor->SimplePayloadSID = 2;

	  m_RIP.PairArray.push_back(RIP::PartitionPair(2, here));
	  GSPart.EssenceContainers = m_HeaderPart.EssenceContainers;

	  static UL gs_part_ul(m_Dict->ul(MDD_GenericStreamPartition));
	  Result_t result = GSPart.WriteToFile(m_File, gs_part_ul);

	  if ( KM_SUCCESS(result) )
	    {
	      ASDCP::FrameBuffer tmp_buf;
	      tmp_buf.SetData((byte_t*)PHDR_master_metadata.c_str(), PHDR_master_metadata.size());
	      tmp_buf.Size(PHDR_master_metadata.size());

	      result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
					 m_StreamOffset, tmp_buf, GenericStream_DataElement.Value(), 0, 0);
	    }
	}

      result = WriteAS02Footer();
    }

  return result;
}


//------------------------------------------------------------------------------------------



AS_02::PHDR::MXFWriter::MXFWriter()
{
}

AS_02::PHDR::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
AS_02::PHDR::MXFWriter::OP1aHeader()
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
AS_02::PHDR::MXFWriter::RIP()
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
Result_t
AS_02::PHDR::MXFWriter::OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
				  ASDCP::MXF::FileDescriptor* essence_descriptor,
				  ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
				  const ASDCP::Rational& edit_rate, const ui32_t& header_size,
				  const IndexStrategy_t& strategy, const ui32_t& partition_space)
{
  if ( essence_descriptor == 0 )
    {
      DefaultLogSink().Error("Essence descriptor object required.\n");
      return RESULT_PARAM;
    }

  m_Writer = new AS_02::PHDR::MXFWriter::h__Writer(DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, essence_descriptor, essence_sub_descriptor_list,
					strategy, partition_space, header_size);

  if ( KM_SUCCESS(result) )
    result = m_Writer->SetSourceStream(JP2K_PACKAGE_LABEL, edit_rate);

  if ( KM_FAILURE(result) )
    m_Writer.release();

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
Result_t 
AS_02::PHDR::MXFWriter::WriteFrame(const AS_02::PHDR::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
Result_t
AS_02::PHDR::MXFWriter::Finalize(const std::string& PHDR_master_metadata)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize(PHDR_master_metadata);
}


//
// end AS_02_PHDR.cpp
//
