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

AS_02::AS02IndexWriter::AS02IndexWriter(const ASDCP::Dictionary*& d) :
  Partition(d), m_CurrentSegment(0), m_BytesPerEditUnit(0), m_Dict(d), m_ECOffset(0), m_Lookup(0)
{
  BodySID = 0;
  IndexSID = 129;
}

AS_02::AS02IndexWriter::~AS02IndexWriter() {}

//
Result_t
AS_02::AS02IndexWriter::WriteToFile(Kumu::FileWriter& Writer)
{
  //      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));

  assert(m_Dict);
  ASDCP::FrameBuffer FooterBuffer;
  ui32_t   footer_size = m_PacketList->m_List.size() * MaxIndexSegmentSize; // segment-count * max-segment-size
  Result_t result = FooterBuffer.Capacity(footer_size); 
  ui32_t   iseg_count = 0;

  if ( m_CurrentSegment != 0 )
    {
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      m_CurrentSegment = 0;
    }

  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && ASDCP_SUCCESS(result); pl_i++ )
    {
      if ( (*pl_i)->IsA(OBJ_TYPE_ARGS(IndexTableSegment)) )
	{
	  iseg_count++;
	  IndexTableSegment* Segment = (IndexTableSegment*)(*pl_i);

	  if ( m_BytesPerEditUnit != 0 )
	    {
	      if ( iseg_count != 1 )
		return RESULT_STATE;

	      ///	      Segment->IndexDuration = duration;
	    }
	}

      InterchangeObject* object = *pl_i;
      object->m_Lookup = m_Lookup;

      ASDCP::FrameBuffer WriteWrapper;
      WriteWrapper.SetData(FooterBuffer.Data() + FooterBuffer.Size(),
			   FooterBuffer.Capacity() - FooterBuffer.Size());
      result = object->WriteToBuffer(WriteWrapper);
      FooterBuffer.Size(FooterBuffer.Size() + WriteWrapper.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    {
      IndexByteCount = FooterBuffer.Size();
      UL body_ul(m_Dict->ul(MDD_ClosedCompleteBodyPartition));
      result = Partition::WriteToFile(Writer, body_ul);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      result = Writer.Write(FooterBuffer.RoData(), FooterBuffer.Size(), &write_count);
      assert(write_count == FooterBuffer.Size());
    }

  return result;
}

//
void
AS_02::AS02IndexWriter::ResetCBR(Kumu::fpos_t offset)
{
  m_ECOffset = offset;

  std::list<InterchangeObject*>::iterator i;

  for ( i = m_PacketList->m_List.begin(); i != m_PacketList->m_List.end(); ++i )
    {
      delete *i;
    }

  m_PacketList->m_List.clear();
}

//
void
AS_02::AS02IndexWriter::Dump(FILE* stream)
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
AS_02::AS02IndexWriter::GetDuration() const
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
AS_02::AS02IndexWriter::SetIndexParamsCBR(IPrimerLookup* lookup, ui32_t size, const ASDCP::Rational& Rate)
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
AS_02::AS02IndexWriter::SetIndexParamsVBR(IPrimerLookup* lookup, const ASDCP::Rational& Rate, Kumu::fpos_t offset)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = 0;
  m_EditRate = Rate;
  m_ECOffset = offset;
}

//
void
AS_02::AS02IndexWriter::PushIndexEntry(const IndexTableSegment::IndexEntry& Entry)
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
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      ui64_t StartPosition = m_CurrentSegment->IndexStartPosition + m_CurrentSegment->IndexDuration;

      m_CurrentSegment = new IndexTableSegment(m_Dict);
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = StartPosition;
    }

  m_CurrentSegment->IndexEntryArray.push_back(Entry);
}


//------------------------------------------------------------------------------------------
//

//
AS_02::h__AS02Writer::h__AS02Writer(const ASDCP::Dictionary& d) : ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>(d),
								  m_IndexWriter(m_Dict), m_PartitionSpace(0),
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

  AddSourceClip(EditRate, TCFrameRate, TrackName, EssenceUL, DataDefinition, PackageLabel);
  AddEssenceDescriptor(WrappingUL);
  m_RIP.PairArray.push_back(RIP::Pair(0, 0)); // Header partition RIP entry
  m_IndexWriter.OperationalPattern = m_HeaderPart.OperationalPattern;
  m_IndexWriter.EssenceContainers = m_HeaderPart.EssenceContainers;

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    {
      m_PartitionSpace *= ceil(EditRate.Quotient());  // convert seconds to edit units
      Kumu::fpos_t ECoffset = m_File.Tell();
      m_IndexWriter.IndexSID = 129;

      if ( BytesPerEditUnit == 0 )
	{
	  m_IndexWriter.SetIndexParamsVBR(&m_HeaderPart.m_Primer, EditRate, ECoffset);
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
      body_part.ThisPartition = m_File.Tell();
      result = body_part.WriteToFile(m_File, body_ul);
      m_RIP.PairArray.push_back(RIP::Pair(1, body_part.ThisPartition)); // Second RIP Entry
    }

  return result;
}

//	
Result_t
AS_02::h__AS02Writer::WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
				      m_StreamOffset, FrameBuf, EssenceUL, Ctx, HMAC);

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
      result = body_part.WriteToFile(m_File, body_ul);
      m_RIP.PairArray.push_back(RIP::Pair(1, body_part.ThisPartition));
      m_IndexWriter.ResetCBR(m_File.Tell());
    }

  return result;
}

// standard method of writing the header and footer of a completed MXF file
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
