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
/*! \file    h__02_Reader.cpp
  \version $Id$
  \brief   MXF file reader base class
*/

#define DEFAULT_MD_DECL
#include "AS_02_internal.h"

using namespace ASDCP;
using namespace ASDCP::MXF;


static Kumu::Mutex sg_DefaultMDInitLock;
static bool        sg_DefaultMDTypesInit = false;
static const ASDCP::Dictionary *sg_dict;

//
void
AS_02::default_md_object_init()
{
  if ( ! sg_DefaultMDTypesInit )
    {
      Kumu::AutoMutex BlockLock(sg_DefaultMDInitLock);

      if ( ! sg_DefaultMDTypesInit )
	{
	  sg_dict = &DefaultSMPTEDict();
	  g_AS02IndexReader = new AS_02::MXF::AS02IndexReader(sg_dict);
	  sg_DefaultMDTypesInit = true;
	}
    }
}


//---------------------------------------------------------------------------------
//

    
AS_02::MXF::AS02IndexReader::AS02IndexReader(const ASDCP::Dictionary*& d) : m_Duration(0), ASDCP::MXF::Partition(m_Dict), m_Dict(d) {}
AS_02::MXF::AS02IndexReader::~AS02IndexReader() {}

//    
Result_t
AS_02::MXF::AS02IndexReader::InitFromFile(const Kumu::FileReader& reader, const ASDCP::MXF::RIP& rip)
{
  ASDCP::MXF::Array<ASDCP::MXF::RIP::Pair>::const_iterator i;

  Result_t result = m_IndexSegmentData.Capacity(128*Kumu::Kilobyte);

  for ( i = rip.PairArray.begin(); KM_SUCCESS(result) && i != rip.PairArray.end(); ++i )
    {
      reader.Seek(i->ByteOffset);
      ASDCP::MXF::Partition plain_part(m_Dict);
      result = plain_part.InitFromFile(reader);

      if ( KM_SUCCESS(result) && plain_part.IndexByteCount > 0 )
	{
	  // slurp up the remainder of the footer
	  ui32_t read_count = 0;

	  assert (plain_part.IndexByteCount <= 0xFFFFFFFFL);
	  ui32_t bytes_this_partition = (ui32_t)plain_part.IndexByteCount;

	  result = m_IndexSegmentData.Capacity(m_IndexSegmentData.Length() + bytes_this_partition);

	  if ( ASDCP_SUCCESS(result) )
	    result = reader.Read(m_IndexSegmentData.Data() + m_IndexSegmentData.Length(),
				 bytes_this_partition, &read_count);

	  if ( ASDCP_SUCCESS(result) && read_count != bytes_this_partition )
	    {
	      DefaultLogSink().Error("Short read of footer partition: got %u, expecting %u\n",
				     read_count, bytes_this_partition);
	      return RESULT_FAIL;
	    }

	  if ( ASDCP_SUCCESS(result) )
	    {
	      result = InitFromBuffer(m_IndexSegmentData.RoData() + m_IndexSegmentData.Length(), bytes_this_partition);
	      m_IndexSegmentData.Length(m_IndexSegmentData.Length() + bytes_this_partition);
	    }
	}
    }

  return result;
}

//
ASDCP::Result_t
AS_02::MXF::AS02IndexReader::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = RESULT_OK;
  const byte_t* end_p = p + l;

  while ( ASDCP_SUCCESS(result) && p < end_p )
    {
      // parse the packets and index them by uid, discard KLVFill items
      InterchangeObject* object = CreateObject(m_Dict, p);
      assert(object);

      object->m_Lookup = m_Lookup;
      result = object->InitFromBuffer(p, end_p - p);
      p += object->PacketLength();

      if ( ASDCP_SUCCESS(result) )
	{
	  m_PacketList->AddPacket(object); // takes ownership
	}
      else
	{
	  DefaultLogSink().Error("Error initializing packet\n");
	  delete object;
	}
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize AS02IndexReader\n");

  std::list<InterchangeObject*>::const_iterator i;
  
  for ( i = m_PacketList->m_List.begin(); i != m_PacketList->m_List.end(); ++i )
    {
      if ( (*i)->IsA(OBJ_TYPE_ARGS(IndexTableSegment)) )
	{
	  m_Duration += static_cast<IndexTableSegment*>(*i)->IndexDuration;
	}
    }

  return result;
}

//
void
AS_02::MXF::AS02IndexReader::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); ++i )
    (*i)->Dump(stream);
}

//
Result_t
AS_02::MXF::AS02IndexReader::GetMDObjectByID(const UUID& object_id, InterchangeObject** Object)
{
  return m_PacketList->GetMDObjectByID(object_id, Object);
}

//
Result_t
AS_02::MXF::AS02IndexReader::GetMDObjectByType(const byte_t* type_id, InterchangeObject** Object)
{
  InterchangeObject* TmpObject;

  if ( Object == 0 )
    Object = &TmpObject;

  return m_PacketList->GetMDObjectByType(type_id, Object);
}

//
Result_t
AS_02::MXF::AS02IndexReader::GetMDObjectsByType(const byte_t* ObjectID, std::list<ASDCP::MXF::InterchangeObject*>& ObjectList)
{
  return m_PacketList->GetMDObjectsByType(ObjectID, ObjectList);
}


//
ui32_t
AS_02::MXF::AS02IndexReader::GetDuration() const
{
  return m_Duration;
}
   

//
Result_t
AS_02::MXF::AS02IndexReader::Lookup(ui32_t frame_num, ASDCP::MXF::IndexTableSegment::IndexEntry& Entry) const
{
  std::list<InterchangeObject*>::iterator li;
  for ( li = m_PacketList->m_List.begin(); li != m_PacketList->m_List.end(); li++ )
    {
      if ( (*li)->IsA(OBJ_TYPE_ARGS(IndexTableSegment)) )
	{
	  IndexTableSegment* Segment = (IndexTableSegment*)(*li);
	  ui64_t start_pos = Segment->IndexStartPosition;

	  if ( Segment->EditUnitByteCount > 0 )
	    {
	      if ( m_PacketList->m_List.size() > 1 )
		DefaultLogSink().Error("Unexpected multiple IndexTableSegment in CBR file\n");

	      if ( ! Segment->IndexEntryArray.empty() )
		DefaultLogSink().Error("Unexpected IndexEntryArray contents in CBR file\n");

	      Entry.StreamOffset = (ui64_t)frame_num * Segment->EditUnitByteCount;
	      return RESULT_OK;
	    }
	  else if ( (ui64_t)frame_num >= start_pos
		    && (ui64_t)frame_num < (start_pos + Segment->IndexDuration) )
	    {
	      ui64_t tmp = frame_num - start_pos;
	      assert(tmp <= 0xFFFFFFFFL);
	      Entry = Segment->IndexEntryArray[(ui32_t) tmp];
	      return RESULT_OK;
	    }
	}
    }

  return RESULT_FAIL;
}


//---------------------------------------------------------------------------------
//


AS_02::h__AS02Reader::h__AS02Reader(const ASDCP::Dictionary& d) : ASDCP::MXF::TrackFileReader<ASDCP::MXF::OP1aHeader, AS_02::MXF::AS02IndexReader>(d) {}
AS_02::h__AS02Reader::~h__AS02Reader() {}


// AS-DCP method of opening an MXF file for read
Result_t
AS_02::h__AS02Reader::OpenMXFRead(const char* filename)
{
  Result_t result = ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::OpenMXFRead(filename);

  if ( KM_SUCCESS(result) )
    result = ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::InitInfo();

  if( KM_SUCCESS(result) )
    {
      //
      UL OP1a_ul(m_Dict->ul(MDD_OP1a));
      InterchangeObject* Object;
      m_Info.LabelSetType = LS_MXF_SMPTE;

      if ( m_HeaderPart.OperationalPattern != OP1a_ul )
	{
	  char strbuf[IdentBufferLen];
	  const MDDEntry* Entry = m_Dict->FindUL(m_HeaderPart.OperationalPattern.Value());

	  if ( Entry == 0 )
	    {
	      DefaultLogSink().Warn("Operational pattern is not OP-1a: %s\n",
				    m_HeaderPart.OperationalPattern.EncodeString(strbuf, IdentBufferLen));
	    }
	  else
	    {
	      DefaultLogSink().Warn("Operational pattern is not OP-1a: %s\n", Entry->name);
	    }
	}

      //
      if ( m_RIP.PairArray.front().ByteOffset != 0 )
	{
	  DefaultLogSink().Error("First Partition in RIP is not at offset 0.\n");
	  result = RESULT_FORMAT;
	}
    }

  if ( KM_SUCCESS(result) )
    {
      m_HeaderPart.BodyOffset = m_File.Tell();
      m_IndexAccess.m_Lookup = &m_HeaderPart.m_Primer;
      result = m_IndexAccess.InitFromFile(m_File, m_RIP);
    }

  m_File.Seek(m_HeaderPart.BodyOffset);
  return result;
}

// AS-DCP method of reading a plaintext or encrypted frame
Result_t
AS_02::h__AS02Reader::ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
				     const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  return ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::ReadEKLVFrame(m_HeaderPart, FrameNum, FrameBuf,
										     EssenceUL, Ctx, HMAC);
}

Result_t
AS_02::h__AS02Reader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset,
                           i8_t& temporalOffset, i8_t& keyFrameOffset)
{
  return ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::LocateFrame(m_HeaderPart, FrameNum,
                                                                                   streamOffset, temporalOffset, keyFrameOffset);
}


//
// end h__02_Reader.cpp
//
