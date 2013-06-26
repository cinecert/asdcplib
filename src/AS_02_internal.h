/*
Copyright (c) 2011-2013, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
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

#ifdef DEFAULT_02_MD_DECL
AS_02::MXF::AS02IndexReader *g_AS02IndexReader;
#else
extern AS_02::MXF::AS02IndexReader *g_AS02IndexReader;
#endif


namespace AS_02
{

  void default_md_object_init();

  namespace MXF
  {
    //
    class AS02IndexWriter : public ASDCP::MXF::Partition
      {
	ASDCP::MXF::IndexTableSegment*  m_CurrentSegment;
	ui32_t  m_BytesPerEditUnit;
	ASDCP::MXF::Rational m_EditRate;

	KM_NO_COPY_CONSTRUCT(AS02IndexWriter);
	AS02IndexWriter();

      public:
	const ASDCP::Dictionary*&  m_Dict;
	ASDCP::IPrimerLookup*      m_Lookup;
      
	AS02IndexWriter(const ASDCP::Dictionary*&);
	virtual ~AS02IndexWriter();

	Result_t WriteToFile(Kumu::FileWriter& Writer);
	void     ResetCBR(Kumu::fpos_t offset);
	void     Dump(FILE* = 0);

	ui32_t GetDuration() const;
	void PushIndexEntry(const ASDCP::MXF::IndexTableSegment::IndexEntry&);
	void SetIndexParamsCBR(ASDCP::IPrimerLookup* lookup, ui32_t size, const ASDCP::Rational& Rate);
	void SetIndexParamsVBR(ASDCP::IPrimerLookup* lookup, const ASDCP::Rational& Rate, Kumu::fpos_t offset);
      };
  }

  //
  class h__AS02Reader : public ASDCP::MXF::TrackFileReader<ASDCP::MXF::OP1aHeader, AS_02::MXF::AS02IndexReader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02Reader);
      h__AS02Reader();

    public:
      h__AS02Reader(const ASDCP::Dictionary&);
      virtual ~h__AS02Reader();

      Result_t OpenMXFRead(const char* filename);

      // USE FRAME WRAPPING...
      Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			     const byte_t* EssenceUL, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC);

     // OR CLIP WRAPPING...
      // clip wrapping is handled directly by the essence-specific classes
      //      Result_t ReadyClip(const ui32_t& FrameNum, const byte_t* EssenceUL, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC, ui64_t& position);
      ///      Result_t ReadClipBlock(ASDCP::FrameBuffer& FrameBuf, const ui32_t& read_size);

      // NOT BOTH!
    };

  //
  class h__AS02Writer : public ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02Writer);
      h__AS02Writer();

    public:
      ui64_t  m_ECStart; // offset of the first essence element
      ui64_t  m_ClipStart;  // state variable for clip-wrap-in-progress
      ui32_t  m_PartitionSpace;  // edit units per partition
      AS_02::MXF::AS02IndexWriter m_IndexWriter;
      IndexStrategy_t m_IndexStrategy; // per SMPTE ST 2067-5

      h__AS02Writer(const Dictionary&);
      virtual ~h__AS02Writer();

      // all the above for a single source clip
      Result_t WriteAS02Header(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
			       const std::string& TrackName, const ASDCP::UL& EssenceUL,
			       const ASDCP::UL& DataDefinition, const ASDCP::Rational& EditRate,
			       ui32_t TCFrameRate, ui32_t BytesPerEditUnit = 0);

      // USE FRAME WRAPPING...
      Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL,
			       AESEncContext* Ctx, HMACContext* HMAC);

      // OR CLIP WRAPPING...
      bool HasOpenClip() const;
      Result_t StartClip(const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC);
      Result_t WriteClipBlock(const ASDCP::FrameBuffer& FrameBuf);
      Result_t FinalizeClip(ui32_t bytes_per_frame);

      // NOT BOTH!

      Result_t WriteAS02Footer();
    };

} // namespace AS_02

#endif // _AS_02_INTERNAL_H_

//
// end AS_02_internal.h
//
