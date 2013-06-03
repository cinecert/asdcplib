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
/*! \file    AS_02_internal.h
  \version $Id: AS_02_internal.h ***       
  \brief   AS-02 library, non-public common elements
*/

#ifndef _AS_02_INTERNAL_H_
#define _AS_02_INTERNAL_H_

#include "KM_log.h"
#include "AS_DCP_internal.h"
#include "AS_02.h"

using Kumu::DefaultLogSink;

#ifdef DEFAULT_MD_DECL
AS_02::MXF::AS02IndexReader *g_AS02IndexReader;
#else
extern AS_02::MXF::AS02IndexReader *g_AS02IndexReader;
#endif


namespace AS_02
{

  void default_md_object_init();

  static void CalculateIndexPartitionSize(ui32_t& size,ui32_t numberOfIndexEntries)
  {
    if(numberOfIndexEntries){
      //Partition::ArchiveSize(); HeaderSize = 124 bytes
      //KLV-Item = 20 bytes
      //ItemSize IndexEntry = 11 bytes
      //number of IndexEntries - parameter
      //IndexEntryArray = 12 bytes
      //size for other Elements(PosTableCount etc.) = 108 bytes
      //see Index.cpp ASDCP::MXF::IndexTableSegment::WriteToTLVSet(TLVWriter& TLVSet) how this is computed 
      size = 124 + 20 + 11 * numberOfIndexEntries + 12 +108;
      size += 20;//because maybe we must fill the last partition, minimum required space for KLV-Item
    }
    else{
      //Partition HeaderSize = 124 bytes
      //KLV-Item = 20 bytes
      //244 for 2 IndexTableSegments
      size = 124 + 20 + 244;
    }
  }

  //
  class h__AS02Reader : public ASDCP::MXF::TrackFileReader<ASDCP::MXF::OP1aHeader, AS_02::MXF::AS02IndexReader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02Reader);
      h__AS02Reader();

    public:
      Partition *m_pCurrentBodyPartition;
      AS_02::MXF::OP1aIndexBodyPartion* m_pCurrentIndexPartition;
      ui64_t     m_EssenceStart;
      std::vector<Partition*> m_BodyPartList;
      ui32_t     m_start_pos;
      ui32_t     m_PartitionSpace;
      IndexStrategy_t    m_IndexStrategy; //Shim parameter index_strategy_frame/clip

      h__AS02Reader(const ASDCP::Dictionary&);
      virtual ~h__AS02Reader();

      Result_t OpenMXFRead(const char* filename);
      Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			     const byte_t* EssenceUL, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC);
      Result_t LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset,
			   i8_t& temporalOffset, i8_t& keyFrameOffset);

    };

  //
  class h__AS02Writer : public ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02Writer);
      h__AS02Writer();

    public:
      AS_02::MXF::OP1aIndexBodyPartion*  m_CurrentIndexBodyPartition;
      ui64_t     m_BodyOffset;
      ui32_t     m_PartitionSpace;
      IndexStrategy_t    m_IndexStrategy; //Shim parameter index_strategy_frame/clip
      std::vector<Partition*> m_BodyPartList;

      h__AS02Writer(const Dictionary&);
      virtual ~h__AS02Writer();

      // all the above for a single source clip
      Result_t WriteAS02Header(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
			       const std::string& TrackName, const ASDCP::UL& EssenceUL,
			       const ASDCP::UL& DataDefinition, const ASDCP::Rational& EditRate,
			       ui32_t TCFrameRate, ui32_t BytesPerEditUnit = 0);

      Result_t CreateBodyPart(const ASDCP::MXF::Rational& EditRate, ui32_t BytesPerEditUnit);
      Result_t CreateBodyPartPair();
      Result_t CompleteIndexBodyPart();

      Result_t WriteAS02Footer();
    };

} // namespace AS_02

#endif // _AS_02_INTERNAL_H_

//
// end AS_02_internal.h
//
