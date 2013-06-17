/*
Copyright (c) 2011-2013, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
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
/*! \file    h__02_Writer.cpp
  \version $Id$
  \brief   MXF file writer base class
*/

#include "AS_02_internal.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

static const ui32_t CBRIndexEntriesPerSegment = 5000;


//------------------------------------------------------------------------------------------
//

AS_02::MXF::AS02IndexWriter::AS02IndexWriter(const ASDCP::Dictionary*& d) :
  Partition(d), m_CurrentSegment(0), m_BytesPerEditUnit(0), m_Dict(d), m_Lookup(0)
{
  BodySID = 0;
  IndexSID = 129;
}

AS_02::MXF::AS02IndexWriter::~AS02IndexWriter() {}

//
Result_t
AS_02::MXF::AS02IndexWriter::WriteToFile(Kumu::FileWriter& Writer)
{
  assert(m_Dict);
  ASDCP::FrameBuffer index_body_buffer;
  ui32_t   index_body_size = m_PacketList->m_List.size() * MaxIndexSegmentSize; // segment-count * max-segment-size
  Result_t result = index_body_buffer.Capacity(index_body_size); 
  ui64_t start_position = 0;

  if ( m_CurrentSegment != 0 )
    {
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      start_position = m_CurrentSegment->IndexStartPosition + m_CurrentSegment->IndexDuration;
      m_CurrentSegment = 0;
    }

  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && KM_SUCCESS(result); pl_i++ )
    {
      InterchangeObject* object = *pl_i;
      object->m_Lookup = m_Lookup;

      ASDCP::FrameBuffer WriteWrapper;
      WriteWrapper.SetData(index_body_buffer.Data() + index_body_buffer.Size(),
			   index_body_buffer.Capacity() - index_body_buffer.Size());
      result = object->WriteToBuffer(WriteWrapper);
      index_body_buffer.Size(index_body_buffer.Size() + WriteWrapper.Size());
      delete *pl_i;
      *pl_i = 0;
    }

  m_PacketList->m_List.clear();

  if ( KM_SUCCESS(result) )
    {
      IndexByteCount = index_body_buffer.Size();
      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = Partition::WriteToFile(Writer, body_ul);
    }

  if ( KM_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      result = Writer.Write(index_body_buffer.RoData(), index_body_buffer.Size(), &write_count);
      assert(write_count == index_body_buffer.Size());
    }

  if ( KM_SUCCESS(result) )
    {
      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = start_position;
    }

  return result;
}

//
void
AS_02::MXF::AS02IndexWriter::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  Partition::Dump(stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); i++ )
    (*i)->Dump(stream);
}

//
ui32_t
AS_02::MXF::AS02IndexWriter::GetDuration() const
{
  ui32_t duration;
  std::list<InterchangeObject*>::const_iterator i;

  for ( i = m_PacketList->m_List.begin(); i != m_PacketList->m_List.end(); ++i )
    {
      if ( (*i)->IsA(OBJ_TYPE_ARGS(IndexTableSegment)) )
	{
	  IndexTableSegment& Segment = *(IndexTableSegment*)*i;
	  duration += Segment.IndexDuration;
	}
    }

  return duration;
}

//
void
AS_02::MXF::AS02IndexWriter::SetIndexParamsCBR(IPrimerLookup* lookup, ui32_t size, const ASDCP::Rational& Rate)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = size;
  m_EditRate = Rate;

  IndexTableSegment* Index = new IndexTableSegment(m_Dict);
  AddChildObject(Index);
  Index->EditUnitByteCount = m_BytesPerEditUnit;
  Index->IndexEditRate = Rate;
}

//
void
AS_02::MXF::AS02IndexWriter::SetIndexParamsVBR(IPrimerLookup* lookup, const ASDCP::Rational& Rate, Kumu::fpos_t offset)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = 0;
  m_EditRate = Rate;
}

//
void
AS_02::MXF::AS02IndexWriter::PushIndexEntry(const IndexTableSegment::IndexEntry& Entry)
{
  if ( m_BytesPerEditUnit != 0 )  // are we CBR? that's bad 
    {
      DefaultLogSink().Error("Call to PushIndexEntry() failed: index is CBR\n");
      return;
    }

  // do we have an available segment?
  if ( m_CurrentSegment == 0 )
    { // no, set up a new segment
      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = 0;
    }
  else if ( m_CurrentSegment->IndexEntryArray.size() >= CBRIndexEntriesPerSegment )
    { // no, this one is full, start another
      DefaultLogSink().Warn("%s, line %d: This has never been tested.\n", __FILE__, __LINE__);
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      ui64_t start_position = m_CurrentSegment->IndexStartPosition + m_CurrentSegment->IndexDuration;

      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = start_position;
    }

  m_CurrentSegment->IndexEntryArray.push_back(Entry);
}


//------------------------------------------------------------------------------------------
//

//
AS_02::h__AS02Writer::h__AS02Writer(const ASDCP::Dictionary& d) : ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>(d),
								  m_ECStart(0), m_ClipStart(0), m_IndexWriter(m_Dict), m_PartitionSpace(0),
								  m_IndexStrategy(AS_02::IS_FOLLOW) {}

AS_02::h__AS02Writer::~h__AS02Writer() {}

//
Result_t
AS_02::h__AS02Writer::WriteAS02Header(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
				      const std::string& TrackName, const ASDCP::UL& EssenceUL, const ASDCP::UL& DataDefinition,
				      const ASDCP::Rational& EditRate, ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  if ( EditRate.Numerator == 0 || EditRate.Denominator == 0 )
    {
      DefaultLogSink().Error("Non-zero edit-rate reqired.\n");
      return RESULT_PARAM;
    }

  InitHeader();

  AddSourceClip(EditRate, EditRate/*TODO: for a moment*/, TCFrameRate, TrackName, EssenceUL, DataDefinition, PackageLabel);
  AddEssenceDescriptor(WrappingUL);
  m_RIP.PairArray.push_back(RIP::Pair(0, 0)); // Header partition RIP entry
  m_IndexWriter.OperationalPattern = m_HeaderPart.OperationalPattern;
  m_IndexWriter.EssenceContainers = m_HeaderPart.EssenceContainers;

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      m_PartitionSpace *= ceil(EditRate.Quotient());  // convert seconds to edit units
      m_ECStart = m_File.Tell();
      m_IndexWriter.IndexSID = 129;

      if ( BytesPerEditUnit == 0 )
	{
	  m_IndexWriter.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, m_ECStart);
	}
      else
	{
	  m_IndexWriter.SetIndexParamsCBR(&m_HeaderPart.m_Primer, BytesPerEditUnit, EditRate);
	}

      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      Partition body_part(m_Dict);
      body_part.BodySID = 1;
      body_part.OperationalPattern = m_HeaderPart.OperationalPattern;
      body_part.EssenceContainers = m_HeaderPart.EssenceContainers;
      body_part.ThisPartition = m_ECStart;
      result = body_part.WriteToFile(m_File, body_ul);
      m_RIP.PairArray.push_back(RIP::Pair(1, body_part.ThisPartition)); // Second RIP Entry
    }

  return result;
}

//	
Result_t
AS_02::h__AS02Writer::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC)
{
  ui64_t this_stream_offset = m_StreamOffset; // m_StreamOffset will be changed by the call to Write_EKLV_Packet

  Result_t result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				      m_StreamOffset, FrameBuf, EssenceUL, Ctx, HMAC);

  if ( KM_SUCCESS(result) )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = this_stream_offset;
      m_IndexWriter.PushIndexEntry(Entry);
    }

  if ( m_FramesWritten > 0 && ( m_FramesWritten % m_PartitionSpace ) == 0 )
    {
      m_IndexWriter.ThisPartition = m_File.Tell();
      m_IndexWriter.WriteToFile(m_File);
      m_RIP.PairArray.push_back(RIP::Pair(0, m_IndexWriter.ThisPartition));

      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      Partition body_part(m_Dict);
      body_part.BodySID = 1;
      body_part.OperationalPattern = m_HeaderPart.OperationalPattern;
      body_part.EssenceContainers = m_HeaderPart.EssenceContainers;
      body_part.ThisPartition = m_File.Tell();

      body_part.BodyOffset = m_StreamOffset;
      result = body_part.WriteToFile(m_File, body_ul);
      m_RIP.PairArray.push_back(RIP::Pair(1, body_part.ThisPartition));
    }

  return result;
}

//
bool
AS_02::h__AS02Writer::HasOpenClip() const
{
  return m_ClipStart != 0;
}

//
Result_t
AS_02::h__AS02Writer::StartClip(const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( Ctx != 0 )
    {
      DefaultLogSink().Error("Encryption not yet supported for PCM clip-wrap.\n");
      return RESULT_STATE;
    }

  if ( m_ClipStart != 0 )
    {
      DefaultLogSink().Error("Cannot open clip, clip already open.\n");
      return RESULT_STATE;
    }

  m_ClipStart = m_File.Tell();
  byte_t clip_buffer[24] = {0};
  memcpy(clip_buffer, EssenceUL, 16);
  bool check = Kumu::write_BER(clip_buffer+16, 0, 8);
  assert(check);
  return m_File.Write(clip_buffer, 24);
}

//
Result_t
AS_02::h__AS02Writer::WriteClipBlock(const ASDCP::FrameBuffer& FrameBuf)
{
  if ( m_ClipStart == 0 )
    {
      DefaultLogSink().Error("Cannot write clip block, no clip open.\n");
      return RESULT_STATE;
    }

  return m_File.Write(FrameBuf.RoData(), FrameBuf.Size());
}

//
Result_t
AS_02::h__AS02Writer::FinalizeClip(ui32_t bytes_per_frame)
{
  if ( m_ClipStart == 0 )
    {
      DefaultLogSink().Error("Cannot close clip, clip not open.\n");
      return RESULT_STATE;
    }

  ui64_t current_position = m_File.Tell();
  Result_t result = m_File.Seek(m_ClipStart+16);

  if ( ASDCP_SUCCESS(result) )
    {
      byte_t clip_buffer[8] = {0};
      bool check = Kumu::write_BER(clip_buffer, m_FramesWritten * bytes_per_frame, 8);
      assert(check);
      result = m_File.Write(clip_buffer, 8);
    }

  m_File.Seek(current_position);
  m_ClipStart = 0;
  return result;
}

// standard method of writing the header and footer of a completed AS-02 file
//
Result_t
AS_02::h__AS02Writer::WriteAS02Footer()
{

  if ( m_IndexWriter.GetDuration() > 0 )
    {
      m_IndexWriter.ThisPartition = m_File.Tell();
      m_IndexWriter.WriteToFile(m_File);
      m_RIP.PairArray.push_back(RIP::Pair(0, m_IndexWriter.ThisPartition));
    }

  // update all Duration properties
  ASDCP::MXF::Partition footer_part(m_Dict);
  DurationElementList_t::iterator dli = m_DurationUpdateList.begin();

  for (; dli != m_DurationUpdateList.end(); ++dli )
    {
      **dli = m_FramesWritten;
    }

  m_EssenceDescriptor->ContainerDuration = m_FramesWritten;
  footer_part.PreviousPartition = m_RIP.PairArray.back().ByteOffset;

  Kumu::fpos_t here = m_File.Tell();
  m_RIP.PairArray.push_back(RIP::Pair(0, here)); // Last RIP Entry
  m_HeaderPart.FooterPartition = here;

  assert(m_Dict);
  footer_part.OperationalPattern = m_HeaderPart.OperationalPattern;
  footer_part.EssenceContainers = m_HeaderPart.EssenceContainers;
  footer_part.FooterPartition = here;
  footer_part.ThisPartition = here;

  UL footer_ul(m_Dict->ul(MDD_CompleteFooter));
  Result_t result = footer_part.WriteToFile(m_File, footer_ul);

  if ( ASDCP_SUCCESS(result) )
    result = m_RIP.WriteToFile(m_File);

  if ( ASDCP_SUCCESS(result) )
    result = m_File.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      ASDCP::MXF::Array<ASDCP::MXF::RIP::Pair>::const_iterator i = m_RIP.PairArray.begin();
      ui64_t header_byte_count = m_HeaderPart.HeaderByteCount;
      ui64_t previous_partition = 0;

      for ( i = m_RIP.PairArray.begin(); ASDCP_SUCCESS(result) && i != m_RIP.PairArray.end(); ++i )
	{
	  ASDCP::MXF::Partition plain_part(m_Dict);
	  result = m_File.Seek(i->ByteOffset);

	  if ( ASDCP_SUCCESS(result) )
	    result = plain_part.InitFromFile(m_File);
      
	  if ( KM_SUCCESS(result)
	       && ( plain_part.IndexSID > 0 || plain_part.BodySID > 0 ) )
	    {
	      plain_part.PreviousPartition = previous_partition;
	      plain_part.FooterPartition = footer_part.ThisPartition;
	      previous_partition = plain_part.ThisPartition;
	      result = m_File.Seek(i->ByteOffset);

	      if ( ASDCP_SUCCESS(result) )
		{
		  UL tmp_ul = plain_part.GetUL();
		  result = plain_part.WriteToFile(m_File, tmp_ul);
		}
	    }
	}
    }

  m_File.Close();
  return result;
}


//
// end h__02_Writer.cpp
//
