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
/*! \file    Index.h
    \version $Id$
    \brief   MXF index segment objects
*/

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
