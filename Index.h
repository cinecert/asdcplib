//
//
//

#ifndef _INDEX_H_
#define _INDEX_H_

#include "MXF.h"

namespace ASDCP
{
  namespace MXF
    {
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
	      i8_t            TemporalOffset;
	      i8_t            KeyFrameOffset;
	      ui8_t           Flags;
	      ui64_t          StreamOffset;
	      std::list<ui32_t>  SliceOffset;
	      Array<Rational> PosTable;

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

    } // namespace MXF
} // namespace ASDCP


#endif // _INDEX_H_

//
// end MXF.h
//
