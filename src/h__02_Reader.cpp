/*
Copyright (c) 2011-2016, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
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
/*! \file    h__02_Reader.cpp
  \version $Id$
  \brief   MXF file reader base class
*/

#define DEFAULT_02_MD_DECL
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

    
AS_02::MXF::AS02IndexReader::AS02IndexReader(const ASDCP::Dictionary*& d) :
  m_Duration(0), m_BytesPerEditUnit(0),
  ASDCP::MXF::Partition(d), m_Dict(d) {}

AS_02::MXF::AS02IndexReader::~AS02IndexReader() {}

//    
Result_t
AS_02::MXF::AS02IndexReader::InitFromFile(const Kumu::FileReader& reader, const ASDCP::MXF::RIP& rip, const bool has_header_essence)
{
  typedef std::list<Kumu::mem_ptr<ASDCP::MXF::Partition> > body_part_array_t;
  body_part_array_t body_part_array;
  body_part_array_t::const_iterator body_part_iter;

  RIP::const_pair_iterator i;
  Result_t result = m_IndexSegmentData.Capacity(128*Kumu::Kilobyte); // will be grown if needed
  ui32_t first_body_sid = 0;

  // create a list of body parts and index parts
  for ( i = rip.PairArray.begin(); KM_SUCCESS(result) && i != rip.PairArray.end(); ++i )
    {
      if ( i->BodySID == 0 )
	continue;

      if ( first_body_sid == 0 )
	{
	  first_body_sid = i->BodySID;
	}
      else if ( i->BodySID != first_body_sid )
	{
	  DefaultLogSink().Debug("The index assembler is ignoring BodySID %d.\n", i->BodySID);
	  continue;
	}

      reader.Seek(i->ByteOffset);
      ASDCP::MXF::Partition *this_partition = new ASDCP::MXF::Partition(m_Dict);
      assert(this_partition);

      result = this_partition->InitFromFile(reader);

      if ( KM_FAILURE(result) )
	{
	  delete this_partition;
	  return result;
	}

      if ( this_partition->BodySID != i->BodySID )
	{
	  DefaultLogSink().Error("Partition BodySID %d does not match RIP BodySID %d.\n",
				 this_partition->BodySID, i->BodySID);
	}

      body_part_array.push_back(0);
      body_part_array.back().set(this_partition);
    }

  if ( body_part_array.empty() )
    {
      DefaultLogSink().Error("File has no partitions with essence data.\n");
      return RESULT_AS02_FORMAT;
    }

  body_part_iter = body_part_array.begin();

  for ( i = rip.PairArray.begin(); KM_SUCCESS(result) && i != rip.PairArray.end(); ++i )
    {
      reader.Seek(i->ByteOffset);
      ASDCP::MXF::Partition plain_part(m_Dict);
      result = plain_part.InitFromFile(reader);

      if ( KM_FAILURE(result) )
	return result;

      if ( plain_part.IndexByteCount > 0 )
	{
	  if ( body_part_iter == body_part_array.end() )
	    {
	      DefaultLogSink().Error("Index and Body partitions do not match.\n");
	      break;
	    }

	  if ( plain_part.ThisPartition == plain_part.FooterPartition )
	    {
	      DefaultLogSink().Warn("File footer partition contains index data.\n");
	    }

	  // slurp up the remainder of the partition
	  ui32_t read_count = 0;

	  assert (plain_part.IndexByteCount <= 0xFFFFFFFFL);
	  ui32_t bytes_this_partition = (ui32_t)plain_part.IndexByteCount;

	  result = m_IndexSegmentData.Capacity(m_IndexSegmentData.Length() + bytes_this_partition);

	  if ( KM_SUCCESS(result) )
	    result = reader.Read(m_IndexSegmentData.Data() + m_IndexSegmentData.Length(),
				 bytes_this_partition, &read_count);

	  if ( KM_SUCCESS(result) && read_count != bytes_this_partition )
	    {
	      DefaultLogSink().Error("Short read of index partition: got %u, expecting %u\n",
				     read_count, bytes_this_partition);
	      return RESULT_AS02_FORMAT;
	    }

	  if ( KM_SUCCESS(result) )
	    {
	      ui64_t current_body_offset = 0;
	      ui64_t current_ec_offset = 0;
	      assert(body_part_iter != body_part_array.end());

	      assert(!body_part_iter->empty());
	      ASDCP::MXF::Partition *tmp_partition = body_part_iter->get();

	      if ( has_header_essence && tmp_partition->ThisPartition == 0 )
		{
		  current_body_offset = 0;
		  current_ec_offset = tmp_partition->HeaderByteCount + tmp_partition->ArchiveSize();
		}
	      else
		{
		  current_body_offset = tmp_partition->BodyOffset;
		  current_ec_offset += tmp_partition->ThisPartition + tmp_partition->ArchiveSize();
		}

	      result = InitFromBuffer(m_IndexSegmentData.RoData() + m_IndexSegmentData.Length(), bytes_this_partition, current_body_offset, current_ec_offset);
	      m_IndexSegmentData.Length(m_IndexSegmentData.Length() + bytes_this_partition);
	      ++body_part_iter;
	    }
	}
    }

  if ( KM_SUCCESS(result) )
    {
      std::list<InterchangeObject*>::const_iterator ii;
  
      for ( ii = m_PacketList->m_List.begin(); ii != m_PacketList->m_List.end(); ++ii )
	{
	  IndexTableSegment *segment = dynamic_cast<IndexTableSegment*>(*ii);

	  if ( segment != 0 )
	    {
	      m_Duration += segment->IndexDuration;
	    }
	}
    }

#if 0
  char identbuf[IdentBufferLen];
  std::list<InterchangeObject*>::iterator j;
  std::vector<ASDCP::MXF::IndexTableSegment::IndexEntry>::iterator k;
  ui32_t entry_count = 0;

  for ( j = m_PacketList->m_List.begin(); j != m_PacketList->m_List.end(); ++j )
    {
      assert(*j);
      ASDCP::MXF::IndexTableSegment* segment = static_cast<ASDCP::MXF::IndexTableSegment*>(*j);

      fprintf(stderr, "  --------------------------------------\n");
      fprintf(stderr, "  IndexEditRate      = %d/%d\n",  segment->IndexEditRate.Numerator, segment->IndexEditRate.Denominator);
      fprintf(stderr, "  IndexStartPosition = %s\n",  i64sz(segment->IndexStartPosition, identbuf));
      fprintf(stderr, "  IndexDuration      = %s\n",  i64sz(segment->IndexDuration, identbuf));
      fprintf(stderr, "  EditUnitByteCount  = %u\n",  segment->EditUnitByteCount);
      fprintf(stderr, "  IndexSID           = %u\n",  segment->IndexSID);
      fprintf(stderr, "  BodySID            = %u\n",  segment->BodySID);
      fprintf(stderr, "  SliceCount         = %hhu\n", segment->SliceCount);
      fprintf(stderr, "  PosTableCount      = %hhu\n", segment->PosTableCount);
      fprintf(stderr, "  RtFileOffset       = %s\n",  i64sz(segment->RtFileOffset, identbuf));
      fprintf(stderr, "  RtEntryOffset      = %s\n",  i64sz(segment->RtEntryOffset, identbuf));
      fprintf(stderr, "  IndexEntryArray:\n");

      for ( k = segment->IndexEntryArray.begin(); k != segment->IndexEntryArray.end(); ++k )
	{
	  fprintf(stderr, "  0x%010qx\n", k->StreamOffset);
	  ++entry_count;
	}
    }

  fprintf(stderr, "Actual entries: %d\n", entry_count);
#endif

  return result;
}

//
ASDCP::Result_t
AS_02::MXF::AS02IndexReader::InitFromBuffer(const byte_t* p, ui32_t l, const ui64_t& body_offset, const ui64_t& essence_container_offset)
{
  Result_t result = RESULT_OK;
  const byte_t* end_p = p + l;

  while ( KM_SUCCESS(result) && p < end_p )
    {
      // parse the packets and index them by uid, discard KLVFill items
      InterchangeObject* object = CreateObject(m_Dict, p);
      assert(object);

      object->m_Lookup = m_Lookup;
      result = object->InitFromBuffer(p, end_p - p);
      p += object->PacketLength();

      if ( KM_SUCCESS(result) )
	{
	  IndexTableSegment *segment = dynamic_cast<IndexTableSegment*>(object);

	  if ( segment != 0 )
	    {
	      segment->RtFileOffset = essence_container_offset;
	      segment->RtEntryOffset = body_offset;
	      m_PacketList->AddPacket(object); // takes ownership
	    }
	  else
	    {
	      delete object;
	    }
	}
      else
	{
	  DefaultLogSink().Error("Error initializing index segment packet.\n");
	  delete object;
	}
    }

  if ( KM_FAILURE(result) )
    {
      DefaultLogSink().Error("Failed to initialize AS02IndexReader.\n");
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
  std::list<InterchangeObject*>::iterator i;
  for ( i = m_PacketList->m_List.begin(); i != m_PacketList->m_List.end(); ++i )
    {
      IndexTableSegment *segment = dynamic_cast<IndexTableSegment*>(*i);

      if ( segment != 0 )
	{
	  ui64_t start_pos = segment->IndexStartPosition;

	  if ( segment->EditUnitByteCount > 0 ) // CBR
	    {
	      if ( m_PacketList->m_List.size() > 1 )
		DefaultLogSink().Error("Unexpected multiple IndexTableSegment in CBR file\n");

	      if ( ! segment->IndexEntryArray.empty() )
		DefaultLogSink().Error("Unexpected IndexEntryArray contents in CBR file\n");

	      Entry.StreamOffset = ((ui64_t)frame_num * segment->EditUnitByteCount) + segment->RtFileOffset;
	      return RESULT_OK;
	    }
	  else if ( (ui64_t)frame_num >= start_pos
		    && (ui64_t)frame_num < (start_pos + segment->IndexDuration) ) // VBR in segments
	    {
	      ui64_t tmp = frame_num - start_pos;
	      assert(tmp <= 0xFFFFFFFFL);

	      if ( tmp < segment->IndexEntryArray.size() )
		{
		  Entry = segment->IndexEntryArray[(ui32_t) tmp];
		  Entry.StreamOffset = Entry.StreamOffset - segment->RtEntryOffset + segment->RtFileOffset;
		  return RESULT_OK;
		}
	      else
		{
		  DefaultLogSink().Error("Malformed index table segment, IndexDuration does not match entries.\n");
		}
	    }
	}
    }

  DefaultLogSink().Error("AS_02::MXF::AS02IndexReader::Lookup FAILED: frame_num=%d\n", frame_num);
  return RESULT_FAIL;
}


//---------------------------------------------------------------------------------
//


AS_02::h__AS02Reader::h__AS02Reader(const ASDCP::Dictionary& d) : ASDCP::MXF::TrackFileReader<ASDCP::MXF::OP1aHeader, AS_02::MXF::AS02IndexReader>(d) {}
AS_02::h__AS02Reader::~h__AS02Reader() {}


// AS-DCP method of opening an MXF file for read
Result_t
AS_02::h__AS02Reader::OpenMXFRead(const std::string& filename)
{
  bool has_header_essence = false;
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
	  const MDDEntry* Entry = m_Dict->FindULAnyVersion(m_HeaderPart.OperationalPattern.Value());

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
	  return RESULT_AS02_FORMAT;
	}

      Kumu::fpos_t first_partition_after_header = 0;
      bool has_body_sid = false;
      RIP::pair_iterator r_i;

      for ( r_i = m_RIP.PairArray.begin(); r_i != m_RIP.PairArray.end(); ++r_i )
	{
	  if ( r_i->BodySID != 0 )
	    {
	      has_body_sid = true;
	    }

	  if ( first_partition_after_header == 0 && r_i->ByteOffset != 0 )
	    {
	      first_partition_after_header = r_i->ByteOffset;
	    }
	}

      // essence in header partition?
      Kumu::fpos_t header_end = m_HeaderPart.HeaderByteCount + m_HeaderPart.ArchiveSize();
      has_header_essence = header_end < first_partition_after_header;

      if ( has_header_essence )
	{
	  DefaultLogSink().Warn("File header partition contains essence data.\n");
	}

      if ( ! has_body_sid )
	{
	  DefaultLogSink().Error("File contains no essence.\n");
	  return RESULT_AS02_FORMAT;
	}
    }

  if ( KM_SUCCESS(result) )
    {
      m_IndexAccess.m_Lookup = &m_HeaderPart.m_Primer;
      result = m_IndexAccess.InitFromFile(m_File, m_RIP, has_header_essence);
    }

  return result;
}

// AS-DCP method of reading a plaintext or encrypted frame
Result_t
AS_02::h__AS02Reader::ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
				     const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  return ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::ReadEKLVFrame(FrameNum, FrameBuf, EssenceUL, Ctx, HMAC);
}

//
// end h__02_Reader.cpp
//
