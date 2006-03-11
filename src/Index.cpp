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
/*! \file    Index.cpp
    \version $Id$
    \brief   MXF index segment objects
*/

#include "MXF.h"
const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;


//
ASDCP::MXF::IndexTableSegment::IndexTableSegment() :
  IndexStartPosition(0), IndexDuration(0), EditUnitByteCount(0),
  IndexSID(129), BodySID(1), SliceCount(0), PosTableCount(0)
{
}

//
ASDCP::MXF::IndexTableSegment::~IndexTableSegment()
{
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::InitFromTLVSet(TLVReader& TLVSet)
{
  Result_t result = InterchangeObject::InitFromTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(IndexTableSegmentBase, IndexEditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, EditUnitByteCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, BodySID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, SliceCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, PosTableCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(IndexTableSegment, DeltaEntryArray));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.ReadObject(OBJ_READ_ARGS(IndexTableSegment, IndexEntryArray));

#if 0
  if ( ASDCP_SUCCESS(result) )
    {
      Batch<IndexEntry>::iterator i;
      ui32_t offset = 0;
      for ( i = IndexEntryArray.begin(); i != IndexEntryArray.end(); i++ )
	{
	  if ( (*i).Flags == 0x40 )
	    offset = 0;

	  (*i).KeyFrameOffset = offset++;
	}
    }
#endif

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::WriteToTLVSet(TLVWriter& TLVSet)
{
  Result_t result = InterchangeObject::WriteToTLVSet(TLVSet);
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexEditRate));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexStartPosition));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi64(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexDuration));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(IndexTableSegmentBase, EditUnitByteCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(IndexTableSegmentBase, IndexSID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi32(OBJ_WRITE_ARGS(IndexTableSegmentBase, BodySID));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(IndexTableSegmentBase, SliceCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteUi8(OBJ_WRITE_ARGS(IndexTableSegmentBase, PosTableCount));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(IndexTableSegment, DeltaEntryArray));
  if ( ASDCP_SUCCESS(result) ) result = TLVSet.WriteObject(OBJ_WRITE_ARGS(IndexTableSegment, IndexEntryArray));
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::InitFromBuffer(const byte_t* p, ui32_t l)
{
  m_Typeinfo = &Dict::Type(MDD_IndexTableSegment);
  return InterchangeObject::InitFromBuffer(p, l);
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  m_Typeinfo = &Dict::Type(MDD_IndexTableSegment);
  return InterchangeObject::WriteToBuffer(Buffer);
}

//
void
ASDCP::MXF::IndexTableSegment::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  InterchangeObject::Dump(stream);
  fprintf(stream, "  IndexEditRate      = %s\n",  IndexEditRate.ToString(identbuf));
  fprintf(stream, "  IndexStartPosition = %s\n",  i64sz(IndexStartPosition, identbuf));
  fprintf(stream, "  IndexDuration      = %s\n",  i64sz(IndexDuration, identbuf));
  fprintf(stream, "  EditUnitByteCount  = %lu\n", EditUnitByteCount);
  fprintf(stream, "  IndexSID           = %lu\n", IndexSID);
  fprintf(stream, "  BodySID            = %lu\n", BodySID);
  fprintf(stream, "  SliceCount         = %hu\n", SliceCount);
  fprintf(stream, "  PosTableCount      = %hu\n", PosTableCount);

  fprintf(stream, "  DeltaEntryArray:\n");  DeltaEntryArray.Dump(stream);

  if ( IndexEntryArray.size() < 100 )
    {
      fprintf(stream, "  IndexEntryArray:\n");
      IndexEntryArray.Dump(stream);
    }
  else
    {
      fprintf(stream, "  IndexEntryArray: %lu entries\n", IndexEntryArray.size());
    }
}

//------------------------------------------------------------------------------------------
//

//
const char*
ASDCP::MXF::IndexTableSegment::DeltaEntry::ToString(char* str_buf) const
{
  snprintf(str_buf, IdentBufferLen, "%3i %-3hu %-3lu", PosTableIndex, Slice, ElementData);
  return str_buf;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::DeltaEntry::Unarchive(ASDCP::MemIOReader& Reader)
{
  Result_t result = Reader.ReadUi8((ui8_t*)&PosTableIndex);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi8(&Slice);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi32BE(&ElementData);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::DeltaEntry::Archive(ASDCP::MemIOWriter& Writer)
{
  Result_t result = Writer.WriteUi8((ui8_t)PosTableIndex);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi8(Slice);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi32BE(ElementData);
  return result;
}

//------------------------------------------------------------------------------------------
//

// Flags:
// Bit 7: Random Access
// Bit 6: Sequence Header
// Bit 5: forward prediction flag
// Bit 4: backward prediction flag 
//   e.g.
//   00== I frame (no prediction)
//   10== P frame(forward prediction from previous  frame)
//   01== B frame (backward prediction from future  frame)
//   11== B frame (forward & backward prediction)
// Bits 0-3: reserved  [RP210 Flags to indicate coding of elements in this  edit unit]

//
const char*
ASDCP::MXF::IndexTableSegment::IndexEntry::ToString(char* str_buf) const
{
  char intbuf[IntBufferLen];
  char txt_flags[6];

  txt_flags[0] = ( (Flags & 0x80) != 0 ) ? 'r' : ' ';
  txt_flags[1] = ( (Flags & 0x40) != 0 ) ? 's' : ' ';
  txt_flags[2] = ( (Flags & 0x20) != 0 ) ? 'f' : ' ';
  txt_flags[3] = ( (Flags & 0x10) != 0 ) ? 'b' : ' ';
  txt_flags[4] = ( (Flags & 0x0f) == 3 ) ? 'B' : ( (Flags & 0x0f) == 2 ) ? 'P' : 'I';
  txt_flags[5] = 0;

  snprintf(str_buf, IdentBufferLen, "%3i %-3hu %s %s",
	  TemporalOffset, KeyFrameOffset, txt_flags,
	  i64sz(StreamOffset, intbuf));

  return str_buf;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::IndexEntry::Unarchive(ASDCP::MemIOReader& Reader)
{
  Result_t result = Reader.ReadUi8((ui8_t*)&TemporalOffset);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi8((ui8_t*)&KeyFrameOffset);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi8(&Flags);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi64BE(&StreamOffset);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::IndexEntry::Archive(ASDCP::MemIOWriter& Writer)
{
  Result_t result = Writer.WriteUi8((ui8_t)TemporalOffset);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi8((ui8_t)KeyFrameOffset);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi8(Flags);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi64BE(StreamOffset);
  return result;
}


//
// end Index.cpp
//

