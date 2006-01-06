/*
Copyright (c) 2005-2006, John Hurst
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
/*! \file    MXF.cpp
    \version $Id$
    \brief   MXF objects
*/

#define ASDCP_DECLARE_MDD
#include "MDD.h"
#include "MXF.h"
#include <hex_utils.h>

//------------------------------------------------------------------------------------------
//

const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;

const byte_t mdd_key[] = { 0x06, 0x0e, 0x2b, 0x34 };

//
const ASDCP::MDDEntry*
ASDCP::GetMDDEntry(const byte_t* ul_buf)
{
  ui32_t t_idx = 0;
  ui32_t k_idx = 8;

  // must be a pointer to a SMPTE UL
  if ( ul_buf == 0 || memcmp(mdd_key, ul_buf, 4) != 0 )
    return 0;

  // advance to first matching element
  // TODO: optimize using binary search
  while ( s_MDD_Table[t_idx].ul != 0
  	  && s_MDD_Table[t_idx].ul[k_idx] != ul_buf[k_idx] )
    t_idx++;

  if ( s_MDD_Table[t_idx].ul == 0 )
    return 0;

  // match successive elements
  while ( s_MDD_Table[t_idx].ul != 0
	  && k_idx < SMPTE_UL_LENGTH - 1
	  && s_MDD_Table[t_idx].ul[k_idx] == ul_buf[k_idx] )
    {
      if ( s_MDD_Table[t_idx].ul[k_idx+1] == ul_buf[k_idx+1] )
	{
	  k_idx++;
	}
      else
	{
	  while ( s_MDD_Table[t_idx].ul != 0
		  && s_MDD_Table[t_idx].ul[k_idx] == ul_buf[k_idx]
		  && s_MDD_Table[t_idx].ul[k_idx+1] != ul_buf[k_idx+1] )
	    t_idx++;
	      
	  while ( s_MDD_Table[t_idx].ul[k_idx] != ul_buf[k_idx] )
	    k_idx--;
	}
    }

  return (s_MDD_Table[t_idx].ul == 0 ? 0 : &s_MDD_Table[t_idx]);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::SeekToRIP(const ASDCP::FileReader& Reader)
{
  ASDCP::fpos_t end_pos;

  // go to the end - 4 bytes
  Result_t result = Reader.Seek(0, ASDCP::SP_END);

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Tell(&end_pos);

  if ( ASDCP_SUCCESS(result)
       && end_pos < (SMPTE_UL_LENGTH+MXF_BER_LENGTH) )
    result = RESULT_FAIL;  // File is smaller than an empty packet!

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(end_pos - 4);

  // get the ui32_t RIP length
  ui32_t read_count;
  byte_t intbuf[MXF_BER_LENGTH];
  ui32_t rip_size = 0;

  if ( ASDCP_SUCCESS(result) )
    {
      result = Reader.Read(intbuf, MXF_BER_LENGTH, &read_count);

      if ( ASDCP_SUCCESS(result) && read_count != 4 )
	result = RESULT_FAIL;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      rip_size = ASDCP_i32_BE(cp2i<ui32_t>(intbuf));

      if ( rip_size > end_pos ) // RIP can't be bigger than the file
	return RESULT_FAIL;
    }

  // reposition to start of RIP
  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(end_pos - rip_size);

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RIP::InitFromFile(const ASDCP::FileReader& Reader)
{
  Result_t result = KLVFilePacket::InitFromFile(Reader, s_MDD_Table[MDDindex_RandomIndexMetadata].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOReader MemRDR(m_ValueStart, m_ValueLength - 4);
      result =  PairArray.ReadFrom(MemRDR);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize RIP\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RIP::WriteToFile(ASDCP::FileWriter& Writer)
{
  Result_t result = WriteKLToFile(Writer, s_MDD_Table[MDDindex_RandomIndexMetadata].ul, 0);
  return result;
}

//
void
ASDCP::MXF::RIP::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  KLVFilePacket::Dump(stream, false);
  PairArray.Dump(stream, false);

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::Partition::InitFromFile(const ASDCP::FileReader& Reader)
{
  Result_t result = KLVFilePacket::InitFromFile(Reader);
  // test the UL
  // could be one of several values

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOReader MemRDR(m_ValueStart, m_ValueLength);
      result = MemRDR.ReadUi16BE(&MajorVersion);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi16BE(&MinorVersion);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi32BE(&KAGSize);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi64BE(&ThisPartition);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi64BE(&PreviousPartition);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi64BE(&FooterPartition);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi64BE(&HeaderByteCount);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi64BE(&IndexByteCount);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi32BE(&IndexSID);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi64BE(&BodyOffset);
      if ( ASDCP_SUCCESS(result) )  result = MemRDR.ReadUi32BE(&BodySID);
      if ( ASDCP_SUCCESS(result) )  result = OperationalPattern.ReadFrom(MemRDR);
      if ( ASDCP_SUCCESS(result) )  result = EssenceContainers.ReadFrom(MemRDR);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Partition\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::WriteToFile(ASDCP::FileWriter& Writer)
{
  Result_t result = m_Buffer.Capacity(1024);

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOWriter MemWRT(m_Buffer.Data(), m_Buffer.Capacity());
      result = MemWRT.WriteUi16BE(MajorVersion);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi16BE(MinorVersion);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi32BE(KAGSize);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi64BE(ThisPartition);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi64BE(PreviousPartition);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi64BE(FooterPartition);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi64BE(HeaderByteCount);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi64BE(IndexByteCount);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi32BE(IndexSID);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi64BE(BodyOffset);
      if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi32BE(BodySID);
      if ( ASDCP_SUCCESS(result) )  result = OperationalPattern.WriteTo(MemWRT);
      if ( ASDCP_SUCCESS(result) )  result = EssenceContainers.WriteTo(MemWRT);
      if ( ASDCP_SUCCESS(result) )  m_Buffer.Size(MemWRT.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count; // this is subclassed, so the UL is only right some of the time
      result = WriteKLToFile(Writer, s_MDD_Table[MDDindex_ClosedCompleteHeader].ul, m_Buffer.Size());

      if ( ASDCP_SUCCESS(result) )
	result = Writer.Write(m_Buffer.RoData(), m_Buffer.Size(), &write_count);
    }

  return result;
}

//
void
ASDCP::MXF::Partition::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];
  char intbuf[IntBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVFilePacket::Dump(stream, false);
  fprintf(stream, "  MajorVersion       = %hu\n", MajorVersion);
  fprintf(stream, "  MinorVersion       = %hu\n", MinorVersion);
  fprintf(stream, "  KAGSize            = %lu\n", KAGSize);
  fprintf(stream, "  ThisPartition      = %s\n",  ui64sz(ThisPartition, intbuf));
  fprintf(stream, "  PreviousPartition  = %s\n",  ui64sz(PreviousPartition, intbuf));
  fprintf(stream, "  FooterPartition    = %s\n",  ui64sz(FooterPartition, intbuf));
  fprintf(stream, "  HeaderByteCount    = %s\n",  ui64sz(HeaderByteCount, intbuf));
  fprintf(stream, "  IndexByteCount     = %s\n",  ui64sz(IndexByteCount, intbuf));
  fprintf(stream, "  IndexSID           = %lu\n", IndexSID);
  fprintf(stream, "  BodyOffset         = %s\n",  ui64sz(BodyOffset, intbuf));
  fprintf(stream, "  BodySID            = %lu\n", BodySID);
  fprintf(stream, "  OperationalPattern = %s\n",  OperationalPattern.ToString(identbuf));
  fputs("Essence Containers:\n", stream); EssenceContainers.Dump(stream, false);

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

class ASDCP::MXF::Primer::h__PrimerLookup : public std::map<UL, TagValue>
{
public:
  void InitWithBatch(ASDCP::MXF::Batch<ASDCP::MXF::Primer::LocalTagEntry>& Batch)
  {
    ASDCP::MXF::Batch<ASDCP::MXF::Primer::LocalTagEntry>::iterator i = Batch.begin();

    for ( ; i != Batch.end(); i++ )
      insert(std::map<UL, TagValue>::value_type((*i).UL, (*i).Tag));
  }
};


//
ASDCP::MXF::Primer::Primer() {}

//
ASDCP::MXF::Primer::~Primer() {}

//
void
ASDCP::MXF::Primer::ClearTagList()
{
  LocalTagEntryBatch.clear();
  m_Lookup = new h__PrimerLookup;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::InitFromBuffer(const byte_t* p, ui32_t l)
{
  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Primer].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOReader MemRDR(m_ValueStart, m_ValueLength);
      result = LocalTagEntryBatch.ReadFrom(MemRDR);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      m_Lookup = new h__PrimerLookup;
      m_Lookup->InitWithBatch(LocalTagEntryBatch);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Primer\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  MemIOWriter MemWRT(Buffer.Data(), Buffer.Capacity());
  Result_t result = LocalTagEntryBatch.WriteTo(MemWRT);
  Buffer.Size(MemWRT.Size());
#if 0
  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count;
      result = WriteKLToFile(Writer, s_MDD_Table[MDDindex_Primer].ul, Buffer.Size());

      if ( ASDCP_SUCCESS(result) )
	result = Writer.Write(Buffer.RoData(), Buffer.Size(), &write_count);
    }
#endif

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::InsertTag(const ASDCP::UL& Key, ASDCP::TagValue& Tag)
{
  assert(m_Lookup);

  std::map<UL, TagValue>::iterator i = m_Lookup->find(Key);

  if ( i == m_Lookup->end() )
    {
      const MDDEntry* mdde = GetMDDEntry(Key.Value());
      assert(mdde);

      LocalTagEntry TmpEntry;
      TmpEntry.UL = Key;
      TmpEntry.Tag = mdde->tag;

      LocalTagEntryBatch.push_back(TmpEntry);
      m_Lookup->insert(std::map<UL, TagValue>::value_type(TmpEntry.UL, TmpEntry.Tag));
    }
   
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::TagForKey(const ASDCP::UL& Key, ASDCP::TagValue& Tag)
{
  assert(m_Lookup);
  if ( m_Lookup.empty() )
    {
      DefaultLogSink().Error("Primer lookup is empty\n");
      return RESULT_FAIL;
    }

  std::map<UL, TagValue>::iterator i = m_Lookup->find(Key);

  if ( i == m_Lookup->end() )
    return RESULT_FALSE;

  Tag = (*i).second;
  return RESULT_OK;
}

//
void
ASDCP::MXF::Primer::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "Primer: %lu %s\n",
	  LocalTagEntryBatch.ItemCount,
	  ( LocalTagEntryBatch.ItemCount == 1 ? "entry" : "entries" ));
  
  Batch<LocalTagEntry>::iterator i = LocalTagEntryBatch.begin();
  for ( ; i != LocalTagEntryBatch.end(); i++ )
    {
      const MDDEntry* Entry = GetMDDEntry((*i).UL.Value());
      fprintf(stream, "  %s %s\n", (*i).ToString(identbuf), (Entry ? Entry->name : "Unknown"));
    }

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::Preface::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_Preface].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, LastModifiedDate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi16(OBJ_READ_ARGS(Preface, Version));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(Preface, ObjectModelVersion));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, PrimaryPackage));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, Identifications));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, ContentStorage));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, OperationalPattern));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, EssenceContainers));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(Preface, DMSchemes));
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Preface\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  TLVWriter MemWRT(Buffer.Data() + kl_length, Buffer.Capacity() - kl_length, m_Lookup);
  Result_t result = MemWRT.WriteObject(OBJ_WRITE_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(GenerationInterchangeObject, GenerationUID));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, LastModifiedDate));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi16(OBJ_WRITE_ARGS(Preface, Version));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteUi32(OBJ_WRITE_ARGS(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, PrimaryPackage));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, Identifications));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, ContentStorage));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, OperationalPattern));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, EssenceContainers));
  if ( ASDCP_SUCCESS(result) )  result = MemWRT.WriteObject(OBJ_WRITE_ARGS(Preface, DMSchemes));

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t packet_length = MemWRT.Size();
      result = WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_Preface].ul, packet_length);

      if ( ASDCP_SUCCESS(result) )
	Buffer.Size(Buffer.Size() + packet_length);
    }

  return result;
}

//
void
ASDCP::MXF::Preface::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "  GenerationUID      = %s\n",  GenerationUID.ToString(identbuf));
  fprintf(stream, "  LastModifiedDate   = %s\n",  LastModifiedDate.ToString(identbuf));
  fprintf(stream, "  Version            = %hu\n", Version);
  fprintf(stream, "  ObjectModelVersion = %lu\n", ObjectModelVersion);
  fprintf(stream, "  PrimaryPackage     = %s\n",  PrimaryPackage.ToString(identbuf));
  fprintf(stream, "  Identifications:\n");  Identifications.Dump(stream);
  fprintf(stream, "  ContentStorage     = %s\n",  ContentStorage.ToString(identbuf));
  fprintf(stream, "  OperationalPattern = %s\n",  OperationalPattern.ToString(identbuf));
  fprintf(stream, "  EssenceContainers:\n");  EssenceContainers.Dump(stream);
  fprintf(stream, "  DMSchemes:\n");  DMSchemes.Dump(stream);

  fputs("==========================================================================\n", stream);
}

//------------------------------------------------------------------------------------------
//

//
class ASDCP::MXF::h__PacketList
{
public:
  std::list<InterchangeObject*> m_List;
  std::map<UL, InterchangeObject*> m_Map;

  ~h__PacketList() {
    while ( ! m_List.empty() )
      {
	delete m_List.back();
	m_List.pop_back();
      }
  }

  //
  void AddPacket(InterchangeObject* ThePacket)
  {
    assert(ThePacket);
    m_Map.insert(std::map<UID, InterchangeObject*>::value_type(ThePacket->InstanceUID, ThePacket));
    m_List.push_back(ThePacket);
  }

  //
  Result_t GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object)
  {
    ASDCP_TEST_NULL(ObjectID);
    ASDCP_TEST_NULL(Object);
    std::list<InterchangeObject*>::iterator li;
    *Object = 0;

    for ( li = m_List.begin(); li != m_List.end(); li++ )
      {
	if ( (*li)->HasUL(ObjectID) )
	  {
	    *Object = *li;
	    return RESULT_OK;
	  }
      }

    return RESULT_FAIL;
  }
};

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OPAtomHeader::OPAtomHeader() : m_Preface(0), m_HasRIP(false)
{
  m_PacketList = new h__PacketList;
}


ASDCP::MXF::OPAtomHeader::~OPAtomHeader()
{
}


ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::InitFromFile(const ASDCP::FileReader& Reader)
{
  m_HasRIP = false;
  Result_t result = SeekToRIP(Reader);

  if ( ASDCP_SUCCESS(result) )
    {
      result = m_RIP.InitFromFile(Reader);

      if ( ASDCP_FAILURE(result) )
	{
	  DefaultLogSink().Error("File contains no RIP\n");
	  result = RESULT_OK;
	}
      else
	{
	  m_HasRIP = true;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = Partition::InitFromFile(Reader); // test UL and OP

  // slurp up the remainder of the header
  ui32_t read_count;

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t buf_len = HeaderByteCount;
      result = m_Buffer.Capacity(buf_len);
    }

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Read(m_Buffer.Data(), m_Buffer.Capacity(), &read_count);

  if ( ASDCP_SUCCESS(result) && read_count != m_Buffer.Capacity() )
    {
      DefaultLogSink().Error("Short read of OP-Atom header metadata; wanted %lu, got %lu\n",
			     m_Buffer.Capacity(), read_count);
      return RESULT_FAIL;
    }

  const byte_t* p = m_Buffer.RoData();
  const byte_t* end_p = p + m_Buffer.Capacity();

  while ( ASDCP_SUCCESS(result) && p < end_p )
    {
      // parse the packets and index them by uid, discard KLVFill items
      InterchangeObject* object = CreateObject(p);
      assert(object);

      object->m_Lookup = &m_Primer;
      result = object->InitFromBuffer(p, end_p - p);
      const byte_t* redo_p = p;
      p += object->PacketLength();
      //      hexdump(p, object->PacketLength());

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( object->IsA(s_MDD_Table[MDDindex_KLVFill].ul) )
	    {
	      delete object;
	    }
	  else if ( object->IsA(s_MDD_Table[MDDindex_Primer].ul) )
	    {
	      delete object;
	      result = m_Primer.InitFromBuffer(redo_p, end_p - redo_p);
	    }
	  else if ( object->IsA(s_MDD_Table[MDDindex_Preface].ul) )
	    {
	      m_Preface = object;
	    }
	  else  
	    {
	      m_PacketList->AddPacket(object);
	    }
	}
      else
	{
	  DefaultLogSink().Error("Error initializing packet\n");
	  delete object;
	}
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::GetMDObjectByType(const byte_t* ObjectID, InterchangeObject** Object)
{
  InterchangeObject* TmpObject;

  if ( Object == 0 )
    Object = &TmpObject;

  return m_PacketList->GetMDObjectByType(ObjectID, Object);
}

//
ASDCP::MXF::Identification*
ASDCP::MXF::OPAtomHeader::GetIdentification()
{
  InterchangeObject* Object;

  if ( ASDCP_SUCCESS(GetMDObjectByType(OBJ_TYPE_ARGS(Identification), &Object)) )
    return (Identification*)Object;

  return 0;
}

//
ASDCP::MXF::SourcePackage*
ASDCP::MXF::OPAtomHeader::GetSourcePackage()
{
  InterchangeObject* Object;

  if ( ASDCP_SUCCESS(GetMDObjectByType(OBJ_TYPE_ARGS(SourcePackage), &Object)) )
    return (SourcePackage*)Object;

  return 0;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::WriteToFile(ASDCP::FileWriter& Writer, ui32_t HeaderSize)
{
  if ( HeaderSize < 4096 ) 
    {
      DefaultLogSink().Error("HeaderSize %lu is too small. Must be >= 4096\n");
      return RESULT_FAIL;
    }

  ASDCP::FrameBuffer HeaderBuffer;

  Result_t result = HeaderBuffer.Capacity(HeaderSize);
  HeaderByteCount = HeaderSize;

  if ( ASDCP_SUCCESS(result) )
    {
      assert(m_Preface);
      m_Preface->m_Lookup = &m_Primer;
      result = m_Preface->WriteToBuffer(HeaderBuffer);
    }
#if 0
  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && ASDCP_SUCCESS(result); pl_i++ )
    {
      InterchangeObject* object = *pl_i;
      object->m_Lookup = &m_Primer;
      result = object->WriteToBuffer(HeaderBuffer);
    }
#endif
  if ( ASDCP_SUCCESS(result) )
    result = Partition::WriteToFile(Writer);

  //  if ( ASDCP_SUCCESS(result) )
    //    result = m_Primer.WriteToFile(Writer);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count;
      Writer.Write(HeaderBuffer.RoData(), HeaderBuffer.Size(), &write_count);
      assert(write_count == HeaderBuffer.Size());
    }

  // KLV Fill
  if ( ASDCP_SUCCESS(result) )
    {
      ASDCP::fpos_t pos = Writer.Tell();

      if ( pos > HeaderSize )
	{
	  char intbuf[IntBufferLen];
	  DefaultLogSink().Error("Header size %s exceeds specified value %lu\n",
				 ui64sz(pos, intbuf),
				 HeaderSize);
	  return RESULT_FAIL;
	}

      ASDCP::FrameBuffer NilBuf;
      ui32_t klv_fill_length = HeaderSize - (ui32_t)pos;

      if ( klv_fill_length < kl_length )
	{
	  DefaultLogSink().Error("Remaining region too small for KLV Fill header\n");
	  return RESULT_FAIL;
	}

      klv_fill_length -= kl_length;
      result = WriteKLToFile(Writer, s_MDD_Table[MDDindex_KLVFill].ul, klv_fill_length);

      if ( ASDCP_SUCCESS(result) )
	result = NilBuf.Capacity(klv_fill_length);

      if ( ASDCP_SUCCESS(result) )
	{
	  memset(NilBuf.Data(), 0, klv_fill_length);
	  ui32_t write_count;
	  Writer.Write(NilBuf.RoData(), klv_fill_length, &write_count);
	  assert(write_count == klv_fill_length);
	}
    }

  return result;
}

//
void
ASDCP::MXF::OPAtomHeader::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  if ( m_HasRIP )
    m_RIP.Dump(stream);

  Partition::Dump(stream);
  m_Primer.Dump(stream);
  assert(m_Preface);
  m_Preface->Dump(stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); i++ )
    (*i)->Dump(stream);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OPAtomIndexFooter::OPAtomIndexFooter() : m_Lookup(0)
{
  m_PacketList = new h__PacketList;
}


ASDCP::MXF::OPAtomIndexFooter::~OPAtomIndexFooter()
{
}


ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::InitFromFile(const ASDCP::FileReader& Reader)
{
  Result_t result = Partition::InitFromFile(Reader); // test UL and OP

  // slurp up the remainder of the footer
  ui32_t read_count;

  if ( ASDCP_SUCCESS(result) )
    result = m_Buffer.Capacity(IndexByteCount);

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Read(m_Buffer.Data(), m_Buffer.Capacity(), &read_count);

  if ( ASDCP_SUCCESS(result) && read_count != m_Buffer.Capacity() )
    {
      DefaultLogSink().Error("Short read of footer partition: got %lu, expecting %lu\n",
			     read_count, m_Buffer.Capacity());
      return RESULT_FAIL;
    }

  const byte_t* p = m_Buffer.RoData();
  const byte_t* end_p = p + m_Buffer.Capacity();
  
  while ( ASDCP_SUCCESS(result) && p < end_p )
    {
      // parse the packets and index them by uid, discard KLVFill items
      InterchangeObject* object = CreateObject(p);
      assert(object);

      object->m_Lookup = m_Lookup;
      result = object->InitFromBuffer(p, end_p - p);
      p += object->PacketLength();

      if ( ASDCP_SUCCESS(result) )
	{
	  m_PacketList->AddPacket(object);
	}
      else
	{
	  DefaultLogSink().Error("Error initializing packet\n");
	  delete object;
	}
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize OPAtomIndexFooter\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::WriteToFile(ASDCP::FileWriter& Writer)
{
  Result_t result = WriteKLToFile(Writer, s_MDD_Table[MDDindex_CompleteFooter].ul, 0);
  return result;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::Dump(FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  Partition::Dump(stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); i++ )
    (*i)->Dump(stream);
}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomIndexFooter::Lookup(ui32_t frame_num, IndexTableSegment::IndexEntry& Entry)
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
	      Entry = Segment->IndexEntryArray[frame_num-start_pos];
	      return RESULT_OK;
	    }
	}
    }

  return RESULT_FAIL;
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = TLVSet.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  if ( Buffer.Capacity() < (Buffer.Size() + m_KLLength + m_ValueLength) )
    {
      DefaultLogSink().Error("InterchangeObject::WriteToBuffer: Buffer too small\n");
      Dump();
      return RESULT_READFAIL;
    }

  Result_t result = WriteKLToBuffer(Buffer, m_KeyStart, m_ValueLength);

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(Buffer.Data() + Buffer.Size(), m_ValueStart, m_ValueLength);
      Buffer.Size(Buffer.Size() + m_ValueLength);
    }

  return result;
}

//
void
ASDCP::MXF::InterchangeObject::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  fputc('\n', stream);
  KLVPacket::Dump(stream, false);
  fprintf(stream, "             InstanceUID = %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "           GenerationUID = %s\n",  GenerationUID.ToString(identbuf));
}

//
bool
ASDCP::MXF::InterchangeObject::IsA(const byte_t* label)
{
  if ( m_KLLength == 0 )
    return false;

  return ( memcmp(label, m_KeyStart, SMPTE_UL_LENGTH) == 0 );
}


//
// end MXF.cpp
//
