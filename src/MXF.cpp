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

#include "MXF.h"
#include "hex_utils.h"


//------------------------------------------------------------------------------------------
//

const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;

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
  Result_t result = KLVFilePacket::InitFromFile(Reader, Dict::ul(MDD_RandomIndexMetadata));

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOReader MemRDR(m_ValueStart, m_ValueLength - 4);
      result =  PairArray.Unarchive(MemRDR);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize RIP\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::RIP::WriteToFile(ASDCP::FileWriter& Writer)
{
  ASDCP::FrameBuffer Buffer;
  ui32_t RIPSize = ( PairArray.size() * (sizeof(ui32_t) + sizeof(ui64_t)) ) + 4;
  Result_t result = Buffer.Capacity(RIPSize);

  if ( ASDCP_SUCCESS(result) )
    result = WriteKLToFile(Writer, Dict::ul(MDD_RandomIndexMetadata), RIPSize);

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOWriter MemWRT(Buffer.Data(), Buffer.Capacity());
      result =  PairArray.Archive(MemWRT);

      if ( ASDCP_SUCCESS(result) )
	MemWRT.WriteUi32BE(RIPSize + 20);

      if ( ASDCP_SUCCESS(result) )
	Buffer.Size(MemWRT.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    result = Writer.Write(Buffer.RoData(), Buffer.Size());

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
class ASDCP::MXF::Partition::h__PacketList
{
public:
  std::list<InterchangeObject*> m_List;
  std::map<UUID, InterchangeObject*> m_Map;

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
    m_Map.insert(std::map<UUID, InterchangeObject*>::value_type(ThePacket->InstanceUID, ThePacket));
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


ASDCP::MXF::Partition::Partition() :
  MajorVersion(1), MinorVersion(2),
  KAGSize(1), ThisPartition(0), PreviousPartition(0),
  FooterPartition(0), HeaderByteCount(0), IndexByteCount(0), IndexSID(0),
  BodyOffset(0), BodySID(0)
{
  m_PacketList = new h__PacketList;
}

ASDCP::MXF::Partition::~Partition()
{
}

//
void
ASDCP::MXF::Partition::AddChildObject(InterchangeObject* Object)
{
  assert(Object);
  UUID TmpID;
  TmpID.GenRandomValue();
  Object->InstanceUID = TmpID;
  m_PacketList->AddPacket(Object);
}

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
      if ( ASDCP_SUCCESS(result) )  result = OperationalPattern.Unarchive(MemRDR);
      if ( ASDCP_SUCCESS(result) )  result = EssenceContainers.Unarchive(MemRDR);
    }

  if ( ASDCP_FAILURE(result) )
    DefaultLogSink().Error("Failed to initialize Partition\n");

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Partition::WriteToFile(ASDCP::FileWriter& Writer, UL& PartitionLabel)
{
  ASDCP::FrameBuffer Buffer;
  Result_t result = Buffer.Capacity(1024);

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOWriter MemWRT(Buffer.Data(), Buffer.Capacity());
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
      if ( ASDCP_SUCCESS(result) )  result = OperationalPattern.Archive(MemWRT);
      if ( ASDCP_SUCCESS(result) )  result = EssenceContainers.Archive(MemWRT);
      if ( ASDCP_SUCCESS(result) )  Buffer.Size(MemWRT.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count;
      result = WriteKLToFile(Writer, PartitionLabel.Value(), Buffer.Size());

      if ( ASDCP_SUCCESS(result) )
	result = Writer.Write(Buffer.RoData(), Buffer.Size(), &write_count);
    }

  return result;
}

//
ui32_t
ASDCP::MXF::Partition::ArchiveSize()
{
  return ( kl_length
	   + sizeof(ui16_t) + sizeof(ui16_t)
	   + sizeof(ui32_t)
	   + sizeof(ui64_t) + sizeof(ui64_t) + sizeof(ui64_t) + sizeof(ui64_t) + sizeof(ui64_t)
	   + sizeof(ui32_t)
	   + sizeof(ui64_t)
	   + sizeof(ui32_t)
	   + SMPTE_UL_LENGTH
	   + sizeof(ui32_t) + sizeof(ui32_t) + ( UUIDlen * EssenceContainers.size() ) );
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
ASDCP::MXF::Primer::Primer() : m_LocalTag(0xff) {}

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
  Result_t result = KLVPacket::InitFromBuffer(p, l, Dict::ul(MDD_Primer));

  if ( ASDCP_SUCCESS(result) )
    {
      MemIOReader MemRDR(m_ValueStart, m_ValueLength);
      result = LocalTagEntryBatch.Unarchive(MemRDR);
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
ASDCP::MXF::Primer::WriteToFile(ASDCP::FileWriter& Writer)
{
  ASDCP::FrameBuffer Buffer;
  Result_t result = Buffer.Capacity(128*1024);

  if ( ASDCP_SUCCESS(result) )
    result = WriteToBuffer(Buffer);

  if ( ASDCP_SUCCESS(result) )
  result = Writer.Write(Buffer.RoData(), Buffer.Size());

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  ASDCP::FrameBuffer LocalTagBuffer;
  MemIOWriter MemWRT(Buffer.Data() + kl_length, Buffer.Capacity() - kl_length);
  Result_t result = LocalTagEntryBatch.Archive(MemWRT);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t packet_length = MemWRT.Size();
      result = WriteKLToBuffer(Buffer, Dict::ul(MDD_Primer), packet_length);

      if ( ASDCP_SUCCESS(result) )
	Buffer.Size(Buffer.Size() + packet_length);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Primer::InsertTag(const MDDEntry& Entry, ASDCP::TagValue& Tag)
{
  assert(m_Lookup);
  UL TestUL(Entry.ul);
  std::map<UL, TagValue>::iterator i = m_Lookup->find(TestUL);

  if ( i == m_Lookup->end() )
    {
      if ( Entry.tag.a == 0 && Entry.tag.b == 0 )
	{
	  Tag.a = 0xff;
	  Tag.b = m_LocalTag--;
	}
      else
	{
	  Tag.a = Entry.tag.a;
	  Tag.b = Entry.tag.b;
	}

      LocalTagEntry TmpEntry;
      TmpEntry.UL = TestUL;
      TmpEntry.Tag = Tag;

      LocalTagEntryBatch.push_back(TmpEntry);
      m_Lookup->insert(std::map<UL, TagValue>::value_type(TmpEntry.UL, TmpEntry.Tag));
    }
  else
    {
      Tag = (*i).second;
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
	  LocalTagEntryBatch.size(),
	  ( LocalTagEntryBatch.size() == 1 ? "entry" : "entries" ));
  
  Batch<LocalTagEntry>::iterator i = LocalTagEntryBatch.begin();
  for ( ; i != LocalTagEntryBatch.end(); i++ )
    {
      const MDDEntry* Entry = Dict::FindUL((*i).UL.Value());
      fprintf(stream, "  %s %s\n", (*i).ToString(identbuf), (Entry ? Entry->name : "Unknown"));
    }

  fputs("==========================================================================\n", stream);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::Preface::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, LastModifiedDate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi16(OBJ_READ_ARGS(Preface, Version));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, PrimaryPackage));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, Identifications));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, ContentStorage));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, OperationalPattern));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, EssenceContainers));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(Preface, DMSchemes));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, LastModifiedDate));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteUi16(OBJ_WRITE_ARGS(Preface, Version));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(Preface, ObjectModelVersion));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, PrimaryPackage));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, Identifications));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, ContentStorage));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, OperationalPattern));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, EssenceContainers));
  if ( ASDCP_SUCCESS(result) )  result = TLVSet.WriteObject(OBJ_WRITE_ARGS(Preface, DMSchemes));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_Preface);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::Preface::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_Preface);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
void
ASDCP::MXF::Preface::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "LastModifiedDate", LastModifiedDate.ToString(identbuf));
  fprintf(stream, "  %22s = %hu\n", "Version", Version);
  fprintf(stream, "  %22s = %lu\n", "ObjectModelVersion", ObjectModelVersion);
  fprintf(stream, "  %22s = %s\n",  "PrimaryPackage", PrimaryPackage.ToString(identbuf));
  fprintf(stream, "  %22s:\n", "Identifications");  Identifications.Dump(stream);
  fprintf(stream, "  %22s = %s\n",  "ContentStorage", ContentStorage.ToString(identbuf));
  fprintf(stream, "  %22s = %s\n",  "OperationalPattern", OperationalPattern.ToString(identbuf));
  fprintf(stream, "  %22s:\n", "EssenceContainers");  EssenceContainers.Dump(stream);
  fprintf(stream, "  %22s:\n", "DMSchemes");  DMSchemes.Dump(stream);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OPAtomHeader::OPAtomHeader() : m_Preface(0), m_HasRIP(false) {}
ASDCP::MXF::OPAtomHeader::~OPAtomHeader() {}

//
ASDCP::Result_t
ASDCP::MXF::OPAtomHeader::InitFromFile(const ASDCP::FileReader& Reader)
{
  m_HasRIP = false;
  Result_t result = SeekToRIP(Reader);

  if ( ASDCP_SUCCESS(result) )
    {
      result = m_RIP.InitFromFile(Reader);
      ui32_t test_s = m_RIP.PairArray.size();

      if ( ASDCP_FAILURE(result) )
	{
	  DefaultLogSink().Error("File contains no RIP\n");
	  result = RESULT_OK;
	}
      else if ( test_s == 0 )
	{
	  DefaultLogSink().Error("RIP contains no Pairs.\n");
	  result = RESULT_FORMAT;
	}
      else if ( test_s < 2 || test_s > 3 )
	{
	  // OP-Atom states that there will be either two or three partitions,
	  // one closed header and one closed footer with an optional body
	  DefaultLogSink().Error("RIP count is not 2 or 3: %lu\n", test_s);
	  return RESULT_FORMAT;
	}
      else
	{
	  m_HasRIP = true;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      Array<RIP::Pair>::iterator r_i = m_RIP.PairArray.begin();
      
      if ( (*r_i).ByteOffset !=  0 )
	{
	  DefaultLogSink().Error("First Partition in RIP is not at offset 0.\n");
	  result = RESULT_FORMAT;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    result = Reader.Seek(0);

  if ( ASDCP_SUCCESS(result) )
    result = Partition::InitFromFile(Reader); // test UL and OP

  Partition::Dump();

  // is it really OP-Atom?
  UL OPAtomUL(Dict::ul(MDD_OPAtom));
  UL InteropOPAtomUL(Dict::ul(MDD_MXFInterop_OPAtom));

  if ( ! ( OperationalPattern == OPAtomUL  || OperationalPattern == InteropOPAtomUL ) )
    {
      char strbuf[IntBufferLen];
      const MDDEntry* Entry = Dict::FindUL(OperationalPattern.Value());
      if ( Entry == 0 )
	DefaultLogSink().Warn("Operational pattern is not OP-Atom: %s\n", OperationalPattern.ToString(strbuf));
      else
	DefaultLogSink().Warn("Operational pattern is not OP-Atom: %s\n", Entry->name);
    }

  // slurp up the remainder of the header
  if ( ASDCP_SUCCESS(result) )
    {
      if ( HeaderByteCount < 1024 )
	DefaultLogSink().Warn("Improbably small HeaderByteCount value: %lu\n", HeaderByteCount);

      result = m_Buffer.Capacity(HeaderByteCount);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t read_count;
      result = Reader.Read(m_Buffer.Data(), m_Buffer.Capacity(), &read_count);

      if ( ASDCP_SUCCESS(result) && read_count != m_Buffer.Capacity() )
	{
	  DefaultLogSink().Error("Short read of OP-Atom header metadata; wanted %lu, got %lu\n",
				 m_Buffer.Capacity(), read_count);
	  return RESULT_FAIL;
	}
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
	  if ( object->IsA(Dict::ul(MDD_KLVFill)) )
	    {
	      delete object;
	    }
	  else if ( object->IsA(Dict::ul(MDD_Primer)) )
	    {
	      delete object;
	      result = m_Primer.InitFromBuffer(redo_p, end_p - redo_p);
	    }
	  else if ( object->IsA(Dict::ul(MDD_Preface)) )
	    {
	      assert(m_Preface == 0);
	      m_Preface = (Preface*)object;
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
  if ( m_Preface == 0 )
    return RESULT_STATE;

  if ( HeaderSize < 4096 ) 
    {
      DefaultLogSink().Error("HeaderSize %lu is too small. Must be >= 4096\n", HeaderSize);
      return RESULT_FAIL;
    }

  ASDCP::FrameBuffer HeaderBuffer;
  HeaderByteCount = HeaderSize - ArchiveSize();
  Result_t result = HeaderBuffer.Capacity(HeaderByteCount); 
  m_Preface->m_Lookup = &m_Primer;

  std::list<InterchangeObject*>::iterator pl_i = m_PacketList->m_List.begin();
  for ( ; pl_i != m_PacketList->m_List.end() && ASDCP_SUCCESS(result); pl_i++ )
    {
      InterchangeObject* object = *pl_i;
      object->m_Lookup = &m_Primer;

      ASDCP::FrameBuffer WriteWrapper;
      WriteWrapper.SetData(HeaderBuffer.Data() + HeaderBuffer.Size(),
			   HeaderBuffer.Capacity() - HeaderBuffer.Size());
      result = object->WriteToBuffer(WriteWrapper);
      HeaderBuffer.Size(HeaderBuffer.Size() + WriteWrapper.Size());
    }

  if ( ASDCP_SUCCESS(result) )
    {
      UL TmpUL(Dict::ul(MDD_ClosedCompleteHeader));
      result = Partition::WriteToFile(Writer, TmpUL);
    }

  if ( ASDCP_SUCCESS(result) )
    result = m_Primer.WriteToFile(Writer);

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

      if ( pos > (ASDCP::fpos_t)HeaderByteCount )
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
      result = WriteKLToFile(Writer, Dict::ul(MDD_KLVFill), klv_fill_length);

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

  if ( m_Preface == 0 )
    fputs("No Preface loaded\n", stream);
  else
    m_Preface->Dump(stream);

  std::list<InterchangeObject*>::iterator i = m_PacketList->m_List.begin();
  for ( ; i != m_PacketList->m_List.end(); i++ )
    (*i)->Dump(stream);
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::OPAtomIndexFooter::OPAtomIndexFooter() :
  m_CurrentSegment(0), m_BytesPerEditUnit(0), m_BodySID(0),
  m_ECOffset(0), m_Lookup(0)
{
  BodySID = 0;
  IndexSID = 129;
}

ASDCP::MXF::OPAtomIndexFooter::~OPAtomIndexFooter() {}


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
ASDCP::MXF::OPAtomIndexFooter::WriteToFile(ASDCP::FileWriter& Writer, ui64_t duration)
{
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

	      Segment->IndexDuration = duration;
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
      UL FooterUL(Dict::ul(MDD_CompleteFooter));
      result = Partition::WriteToFile(Writer, FooterUL);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count;
      Writer.Write(FooterBuffer.RoData(), FooterBuffer.Size(), &write_count);
      assert(write_count == FooterBuffer.Size());
    }

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

//
void
ASDCP::MXF::OPAtomIndexFooter::SetIndexParamsCBR(IPrimerLookup* lookup, ui32_t size, const Rational& Rate)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = size;
  m_EditRate = Rate;

  IndexTableSegment* Index = new IndexTableSegment;
  AddChildObject(Index);
  Index->EditUnitByteCount = m_BytesPerEditUnit;
  Index->IndexEditRate = Rate;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::SetIndexParamsVBR(IPrimerLookup* lookup, const Rational& Rate, fpos_t offset)
{
  assert(lookup);
  m_Lookup = lookup;
  m_BytesPerEditUnit = 0;
  m_EditRate = Rate;
  m_ECOffset = offset;
}

//
void
ASDCP::MXF::OPAtomIndexFooter::PushIndexEntry(const IndexTableSegment::IndexEntry& Entry)
{
  if ( m_BytesPerEditUnit != 0 )  // are we CBR? that's bad 
    {
      DefaultLogSink().Error("Call to PushIndexEntry() failed: index is CBR\n");
      return;
    }

  // do we have an available segment?
  if ( m_CurrentSegment == 0 )
    { // no, set up a new segment
      m_CurrentSegment = new IndexTableSegment;
      assert(m_CurrentSegment);
      AddChildObject(m_CurrentSegment);
      m_CurrentSegment->DeltaEntryArray.push_back(IndexTableSegment::DeltaEntry());
      m_CurrentSegment->IndexEditRate = m_EditRate;
      m_CurrentSegment->IndexStartPosition = 0;
    }
  else if ( m_CurrentSegment->IndexEntryArray.size() >= 1486 ) // 1486 gets us 16K packets
    { // no, this one is full, start another
      m_CurrentSegment->IndexDuration = m_CurrentSegment->IndexEntryArray.size();
      ui64_t StartPosition = m_CurrentSegment->IndexStartPosition + m_CurrentSegment->IndexDuration;

      m_CurrentSegment = new IndexTableSegment;
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
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = TLVSet.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) )
    result = TLVSet.ReadObject(OBJ_READ_ARGS(GenerationInterchangeObject, GenerationUID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = TLVSet.WriteObject(OBJ_WRITE_ARGS(InterchangeObject, InstanceUID));
  if ( ASDCP_SUCCESS(result) )
    result = TLVSet.WriteObject(OBJ_WRITE_ARGS(GenerationInterchangeObject, GenerationUID));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);
  Result_t result;

  if ( m_Typeinfo == 0 )
    {
      result = KLVPacket::InitFromBuffer(p, l);
    }
  else
    {
      result = KLVPacket::InitFromBuffer(p, l, m_Typeinfo->ul);

      if ( ASDCP_SUCCESS(result) )
	{
	  TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);
	  result = InitFromTLVSet(MemRDR);
	}
    }
  
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::InterchangeObject::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  if ( m_Typeinfo == 0 )
    return RESULT_STATE;

  TLVWriter MemWRT(Buffer.Data() + kl_length, Buffer.Capacity() - kl_length, m_Lookup);
  Result_t result = WriteToTLVSet(MemWRT);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t packet_length = MemWRT.Size();
      result = WriteKLToBuffer(Buffer, m_Typeinfo->ul, packet_length);

      if ( ASDCP_SUCCESS(result) )
	Buffer.Size(Buffer.Size() + packet_length);
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
