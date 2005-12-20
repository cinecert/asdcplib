//
// Index.cpp
//

#include "MDD.h"
#include "MXF.h"

//
ASDCP::MXF::IndexTableSegment::IndexTableSegment() :
  IndexStartPosition(0), IndexDuration(0), EditUnitByteCount(0),
  IndexSID(0), BodySID(0), SliceCount(0), PosTableCount(0)
{
}

//
ASDCP::MXF::IndexTableSegment::~IndexTableSegment()
{
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::InitFromBuffer(const byte_t* p, ui32_t l)
{
  ASDCP_TEST_NULL(p);

  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_IndexTableSegment].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVReader MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(IndexTableSegmentBase, IndexEditRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexStartPosition));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexDuration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, EditUnitByteCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, IndexSID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, BodySID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, SliceCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, PosTableCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(IndexTableSegment, DeltaEntryArray));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(IndexTableSegment, IndexEntryArray));
    }

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

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::WriteToBuffer(ASDCP::FrameBuffer& Buffer)
{
  return WriteKLToBuffer(Buffer, s_MDD_Table[MDDindex_IndexTableSegment].ul, 0);
#if 0
  Result_t result = KLVPacket::InitFromBuffer(p, l, s_MDD_Table[MDDindex_IndexTableSegment].ul);

  if ( ASDCP_SUCCESS(result) )
    {
      TLVWriter MemRDR(m_ValueStart, m_ValueLength, m_Lookup);

      result = MemRDR.ReadObject(OBJ_READ_ARGS(InterchangeObject, InstanceUID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(IndexTableSegmentBase, IndexEditRate));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexStartPosition));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi64(OBJ_READ_ARGS(IndexTableSegmentBase, IndexDuration));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, EditUnitByteCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, IndexSID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi32(OBJ_READ_ARGS(IndexTableSegmentBase, BodySID));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, SliceCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadUi8(OBJ_READ_ARGS(IndexTableSegmentBase, PosTableCount));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(IndexTableSegment, DeltaEntryArray));
      if ( ASDCP_SUCCESS(result) ) result = MemRDR.ReadObject(OBJ_READ_ARGS(IndexTableSegment, IndexEntryArray));
    }

  return result;
#endif
}

//
void
ASDCP::MXF::IndexTableSegment::Dump(FILE* stream)
{
  char identbuf[IdentBufferLen];

  if ( stream == 0 )
    stream = stderr;

  KLVPacket::Dump(stream, false);
  fprintf(stream, "  InstanceUID        = %s\n",  InstanceUID.ToString(identbuf));
  fprintf(stream, "  IndexEditRate      = %s\n",  IndexEditRate.ToString(identbuf));
  fprintf(stream, "  IndexStartPosition = %s\n",  i64sz(IndexStartPosition, identbuf));
  fprintf(stream, "  IndexDuration      = %s\n",  i64sz(IndexDuration, identbuf));
  fprintf(stream, "  EditUnitByteCount  = %lu\n", EditUnitByteCount);
  fprintf(stream, "  IndexSID           = %lu\n", IndexSID);
  fprintf(stream, "  BodySID            = %lu\n", BodySID);
  fprintf(stream, "  SliceCount         = %hu\n", SliceCount);
  fprintf(stream, "  PosTableCount      = %hu\n", PosTableCount);

  fprintf(stream, "  DeltaEntryArray:\n");  DeltaEntryArray.Dump(stream);
  fprintf(stream, "  IndexEntryArray:\n");  IndexEntryArray.Dump(stream);

  fputs("==========================================================================\n", stream);
}

//
const char*
ASDCP::MXF::IndexTableSegment::DeltaEntry::ToString(char* str_buf) const
{
  sprintf(str_buf, "%3i %-3hu %-3lu", PosTableIndex, Slice, ElementData);
  return str_buf;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::DeltaEntry::ReadFrom(ASDCP::MemIOReader& Reader)
{
  Result_t result = Reader.ReadUi8((ui8_t*)&PosTableIndex);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi8(&Slice);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi32BE(&ElementData);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::DeltaEntry::WriteTo(ASDCP::MemIOWriter& Writer)
{
  Result_t result = Writer.WriteUi8((ui8_t)PosTableIndex);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi8(Slice);
  if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi32BE(ElementData);
  return result;
}

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
  txt_flags[5] = 0;

  txt_flags[0] = ( (Flags & 0x80) != 0 ) ? 'r' : ' ';
  txt_flags[1] = ( (Flags & 0x40) != 0 ) ? 's' : ' ';
  txt_flags[2] = ( (Flags & 0x20) != 0 ) ? 'f' : ' ';
  txt_flags[3] = ( (Flags & 0x10) != 0 ) ? 'b' : ' ';
  txt_flags[4] = ( (Flags & 0x0f) == 3 ) ? 'B' : ( (Flags & 0x0f) == 2 ) ? 'P' : 'I';

  sprintf(str_buf, "%3i %-3hu %s %s",
	  TemporalOffset, KeyFrameOffset, txt_flags,
	  i64sz(StreamOffset, intbuf));

  return str_buf;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::IndexEntry::ReadFrom(ASDCP::MemIOReader& Reader)
{
  Result_t result = Reader.ReadUi8((ui8_t*)&TemporalOffset);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi8((ui8_t*)&KeyFrameOffset);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi8(&Flags);
  if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi64BE(&StreamOffset);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::IndexTableSegment::IndexEntry::WriteTo(ASDCP::MemIOWriter& Writer)
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

