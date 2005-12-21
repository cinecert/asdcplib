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
/*! \file    MXF.h
    \version $Id$
    \brief   MXF objects
*/

#ifndef _MXF_H_
#define _MXF_H_

#include "MXFTypes.h"

namespace ASDCP
{
  namespace MXF
    {
      // seek an open file handle to the start of the RIP KLV packet
      Result_t SeekToRIP(const FileReader&);
      
      //
      class RIP : public ASDCP::KLVFilePacket
	{
	  ASDCP_NO_COPY_CONSTRUCT(RIP);

	public:
	  //
	  class Pair {
	  public:
	    ui32_t BodySID;
	    ui64_t ByteOffset;

	    ui32_t Size() { return sizeof(ui32_t) + sizeof(ui64_t); }

	    inline const char* ToString(char* str_buf) const {
	      char intbuf[IntBufferLen];
	      sprintf(str_buf, "%-6lu: %s", BodySID, ui64sz(ByteOffset, intbuf));
	      return str_buf;
	    }

	    inline Result_t ReadFrom(ASDCP::MemIOReader& Reader) {
	      Result_t result = Reader.ReadUi32BE(&BodySID);

	      if ( ASDCP_SUCCESS(result) )
		result = Reader.ReadUi64BE(&ByteOffset);

	      return result;
	    }

	    inline Result_t WriteTo(ASDCP::MemIOWriter& Writer) {
	      Result_t result = Writer.WriteUi32BE(BodySID);

	      if ( ASDCP_SUCCESS(result) )
		result = Writer.WriteUi64BE(ByteOffset);

	      return result;
	    }
	  };

	  Array<Pair> PairArray;

	  RIP() {}
	  virtual ~RIP() {}
	  virtual Result_t InitFromFile(const ASDCP::FileReader& Reader);
	  virtual Result_t WriteToFile(ASDCP::FileWriter& Writer);
	  virtual void     Dump(FILE* = 0);
	};


      //
      class Partition : public ASDCP::KLVFilePacket
	{	
	  ASDCP_NO_COPY_CONSTRUCT(Partition);

	public:
	  ui16_t    MajorVersion;
	  ui16_t    MinorVersion;
	  ui32_t    KAGSize;
	  ui64_t    ThisPartition;
	  ui64_t    PreviousPartition;
	  ui64_t    FooterPartition;
	  ui64_t    HeaderByteCount;
	  ui64_t    IndexByteCount;
	  ui32_t    IndexSID;
	  ui64_t    BodyOffset;
	  ui32_t    BodySID;
	  UL        OperationalPattern;
	  Batch<UL> EssenceContainers;

	  Partition() {}
	  virtual ~Partition() {}
	  virtual Result_t InitFromFile(const ASDCP::FileReader& Reader);
	  virtual Result_t WriteToFile(ASDCP::FileWriter& Writer);
	  virtual void     Dump(FILE* = 0);
	};


      //
      class Primer : public ASDCP::KLVPacket, public ASDCP::IPrimerLookup
	{
	  class h__PrimerLookup;
	  mem_ptr<h__PrimerLookup> m_Lookup;
	  ASDCP_NO_COPY_CONSTRUCT(Primer);

	public:
	  //
	  class LocalTagEntry
	    {
	    public:
	      TagValue    Tag;
	      ASDCP::UL   UL;

	      inline const char* ToString(char* str_buf) const {
		sprintf(str_buf, "%02x %02x: ", Tag.a, Tag.b);
		UL.ToString(str_buf + strlen(str_buf));
		return str_buf;
	      }

	      inline Result_t ReadFrom(ASDCP::MemIOReader& Reader) {
		Result_t result = Reader.ReadUi8(&Tag.a);
		
		if ( ASDCP_SUCCESS(result) )
		  result = Reader.ReadUi8(&Tag.b);

		if ( ASDCP_SUCCESS(result) )
		  result = UL.ReadFrom(Reader);

		return result;
	      }

	      inline Result_t WriteTo(ASDCP::MemIOWriter& Writer) {
		Result_t result = Writer.WriteUi8(Tag.a);
		
		if ( ASDCP_SUCCESS(result) )
		  result = Writer.WriteUi8(Tag.b);

		if ( ASDCP_SUCCESS(result) )
		  result = UL.WriteTo(Writer);

		return result;
	      }
	    };

	  Batch<LocalTagEntry> LocalTagEntryBatch;

	  Primer();
	  virtual ~Primer();

	  virtual void     ClearTagList();
	  virtual Result_t InsertTag(const ASDCP::UL& Key, ASDCP::TagValue& Tag);
	  virtual Result_t TagForKey(const ASDCP::UL& Key, ASDCP::TagValue& Tag);

          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
          virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};


      //
      class InterchangeObject : public ASDCP::KLVPacket
	{
	public:
	  IPrimerLookup* m_Lookup;
	  UID            InstanceUID;
	  UUID           GenerationUID;

	  InterchangeObject() : m_Lookup(0) {}
	  virtual ~InterchangeObject() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual bool     IsA(const byte_t* label);

	  virtual void Dump(FILE* stream = 0) {
	    KLVPacket::Dump(stream, true);
	  }
	};

      //
      InterchangeObject* CreateObject(const byte_t* label);


      //
      class Preface : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Preface);

	public:
	  UUID         GenerationUID;
	  Timestamp    LastModifiedDate;
	  ui16_t       Version;
	  ui32_t       ObjectModelVersion;
	  UID          PrimaryPackage;
	  Batch<UID>   Identifications;
	  UID          ContentStorage;
	  UL           OperationalPattern;
	  Batch<UL>    EssenceContainers;
	  Batch<UL>    DMSchemes;

	  Preface() {}
	  virtual ~Preface() {}
          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer& Buffer);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class IndexTableSegment : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(IndexTableSegment);

	public:
	  //
	  class DeltaEntry
	    {
	    public:
	      i8_t    PosTableIndex;
	      ui8_t   Slice;
	      ui32_t  ElementData;

	      Result_t ReadFrom(ASDCP::MemIOReader& Reader);
	      Result_t WriteTo(ASDCP::MemIOWriter& Writer);
	      inline const char* ToString(char* str_buf) const;
	    };

	  //
	  class IndexEntry
	    {
	    public:
	      i8_t               TemporalOffset;
	      i8_t               KeyFrameOffset;
	      ui8_t              Flags;
	      ui64_t             StreamOffset;
	      std::list<ui32_t>  SliceOffset;
	      Array<Rational>    PosTable;

	      Result_t ReadFrom(ASDCP::MemIOReader& Reader);
	      Result_t WriteTo(ASDCP::MemIOWriter& Writer);
	      inline const char* ToString(char* str_buf) const;
	    };

	  Rational    IndexEditRate;
	  ui64_t      IndexStartPosition;
	  ui64_t      IndexDuration;
	  ui32_t      EditUnitByteCount;
	  ui32_t      IndexSID;
	  ui32_t      BodySID;
	  ui8_t       SliceCount;
	  ui8_t       PosTableCount;
	  Batch<DeltaEntry> DeltaEntryArray;
	  Batch<IndexEntry> IndexEntryArray;

	  IndexTableSegment();
	  virtual ~IndexTableSegment();
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer& Buffer);
	  virtual void     Dump(FILE* = 0);
	};

      //---------------------------------------------------------------------------------
      //
      class h__PacketList; // See MXF.cpp
      class Identification;
      class SourcePackage;

      //
      class OPAtomHeader : public Partition
	{
	  mem_ptr<h__PacketList>   m_PacketList;
	  ASDCP_NO_COPY_CONSTRUCT(OPAtomHeader);

	public:
	  ASDCP::MXF::RIP          m_RIP;
	  ASDCP::MXF::Primer       m_Primer;
	  InterchangeObject*       m_Preface;
	  ASDCP::FrameBuffer       m_Buffer;
	  bool                     m_HasRIP;

	  OPAtomHeader();
	  virtual ~OPAtomHeader();
	  virtual Result_t InitFromFile(const ASDCP::FileReader& Reader);
	  virtual Result_t WriteToFile(ASDCP::FileWriter& Writer, ui32_t HeaderLength = 16384);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t GetMDObjectByType(const byte_t*, InterchangeObject** = 0);
	  Identification*  GetIdentification();
	  SourcePackage*   GetSourcePackage();
	};

      //
      class OPAtomIndexFooter : public Partition
	{
	  mem_ptr<h__PacketList>   m_PacketList;
	  ASDCP::FrameBuffer       m_Buffer;
	  ASDCP_NO_COPY_CONSTRUCT(OPAtomIndexFooter);

	public:
	  IPrimerLookup* m_Lookup;
	 
	  OPAtomIndexFooter();
	  virtual ~OPAtomIndexFooter();
	  virtual Result_t InitFromFile(const ASDCP::FileReader& Reader);
	  virtual Result_t WriteToFile(ASDCP::FileWriter& Writer);
	  virtual void     Dump(FILE* = 0);

	  virtual Result_t Lookup(ui32_t frame_num, IndexTableSegment::IndexEntry&);
	};

    } // namespace MXF
} // namespace ASDCP


#endif // _MXF_H_

//
// end MXF.h
//
