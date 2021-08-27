/*
Copyright (c) 2011-2021, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
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
#include "AS_02_JXS.h"

using Kumu::DefaultLogSink;

namespace AS_02
{
  
  #ifdef DEFAULT_02_MD_DECL
  AS_02::MXF::AS02IndexReader *g_AS02IndexReader;
  #else
  extern AS_02::MXF::AS02IndexReader *g_AS02IndexReader;
  #endif

  void default_md_object_init();


  //
  class h__AS02Reader : public ASDCP::MXF::TrackFileReader<ASDCP::MXF::OP1aHeader, AS_02::MXF::AS02IndexReader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02Reader);
      h__AS02Reader();

    public:
      h__AS02Reader(const ASDCP::Dictionary*, const Kumu::IFileReaderFactory& fileReaderFactory);

      virtual ~h__AS02Reader();

      Result_t OpenMXFRead(const std::string& filename);

      // USE FRAME WRAPPING...
      Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
			     const byte_t* EssenceUL, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC);

     // OR CLIP WRAPPING...
      // clip wrapping is handled directly by the essence-specific classes
      //      Result_t ReadyClip(const ui32_t& FrameNum, const byte_t* EssenceUL, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC, ui64_t& position);
      ///      Result_t ReadClipBlock(ASDCP::FrameBuffer& FrameBuf, const ui32_t& read_size);

      // NOT BOTH!
    };


  namespace MXF
  {
    //
    class AS02IndexWriterVBR : public ASDCP::MXF::Partition
      {
	ASDCP::MXF::IndexTableSegment*  m_CurrentSegment;
	ASDCP::MXF::Rational m_EditRate;

	KM_NO_COPY_CONSTRUCT(AS02IndexWriterVBR);
	AS02IndexWriterVBR();

      public:
	const ASDCP::Dictionary*  m_Dict;
	ASDCP::IPrimerLookup*      m_Lookup;
      
	AS02IndexWriterVBR(const ASDCP::Dictionary*);
	virtual ~AS02IndexWriterVBR();

	//
	void SetPrimerLookup(ASDCP::IPrimerLookup* lookup) {
	  assert(lookup);
	  m_Lookup = lookup;
	}

	Result_t WriteToFile(Kumu::FileWriter& Writer);
	void     Dump(FILE* = 0);

	ui32_t GetDuration() const;
	void PushIndexEntry(const ASDCP::MXF::IndexTableSegment::IndexEntry&);
	void SetEditRate(const ASDCP::Rational& edit_rate);
      };


   //
    class AS02IndexWriterCBR : public ASDCP::MXF::Partition
      {
	ASDCP::MXF::IndexTableSegment*  m_CurrentSegment;
	ASDCP::MXF::Rational m_EditRate;

	KM_NO_COPY_CONSTRUCT(AS02IndexWriterCBR);
	AS02IndexWriterCBR();

      public:
	const ASDCP::Dictionary*  m_Dict;
	ASDCP::IPrimerLookup* m_Lookup;
	ui32_t m_Duration;
	ui32_t m_SampleSize;
      
	AS02IndexWriterCBR(const ASDCP::Dictionary*);
	virtual ~AS02IndexWriterCBR();

	//
	void SetPrimerLookup(ASDCP::IPrimerLookup* lookup) {
	  assert(lookup);
	  m_Lookup = lookup;
	}

	Result_t WriteToFile(Kumu::FileWriter& Writer);
	ui32_t GetDuration() const;
	void SetEditRate(const ASDCP::Rational& edit_rate, const ui32_t& sample_size);
      };
  }

  //
  template <class IndexWriterType>
  class h__AS02Writer : public ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02Writer);
      h__AS02Writer();

    public:
      ui32_t  m_PartitionSpace;  // edit units per partition
      IndexWriterType m_IndexWriter;
      ui64_t  m_ECStart; // offset of the first essence element

      //
      h__AS02Writer(const ASDCP::Dictionary *d) :
          ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>(d), m_IndexWriter(d), m_ECStart(0) {}

      ~h__AS02Writer() {}


      
      // all the above for a single source clip
      Result_t WriteAS02Header(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
			       const std::string& TrackName, const ASDCP::UL& EssenceUL,
			       const ASDCP::UL& DataDefinition, const ASDCP::Rational& EditRate,
			       const std::vector<ASDCP::UL>* conformsToSpecifications = NULL)
      {
	if ( EditRate.Numerator == 0 || EditRate.Denominator == 0 )
	  {
	    DefaultLogSink().Error("Non-zero edit-rate reqired.\n");
	    return RESULT_PARAM;
	  }

	InitHeader(MXFVersion_2011, conformsToSpecifications);

	AddSourceClip(EditRate, EditRate/*TODO: for a moment*/, 0 /*no timecode track*/, TrackName, EssenceUL, DataDefinition, PackageLabel);
	AddEssenceDescriptor(WrappingUL);

	this->m_IndexWriter.SetPrimerLookup(&this->m_HeaderPart.m_Primer);
	this->m_RIP.PairArray.push_back(RIP::PartitionPair(0, 0)); // Header partition RIP entry
	this->m_IndexWriter.MajorVersion = m_HeaderPart.MajorVersion;
	this->m_IndexWriter.MinorVersion = m_HeaderPart.MinorVersion;
	this->m_IndexWriter.OperationalPattern = this->m_HeaderPart.OperationalPattern;
	this->m_IndexWriter.EssenceContainers = this->m_HeaderPart.EssenceContainers;

	Result_t result = this->m_HeaderPart.WriteToFile(this->m_File, this->m_HeaderSize);

	if ( KM_SUCCESS(result) )
	  {
	    this->m_PartitionSpace *= (ui32_t)floor( EditRate.Quotient() + 0.5 );  // convert seconds to edit units
	    this->m_ECStart = this->m_File.TellPosition();
	    this->m_IndexWriter.IndexSID = 129;

	    UL body_ul(this->m_Dict->ul(MDD_ClosedCompleteBodyPartition));
	    Partition body_part(this->m_Dict);
	    body_part.BodySID = 1;
	    body_part.MajorVersion = this->m_HeaderPart.MajorVersion;
	    body_part.MinorVersion = this->m_HeaderPart.MinorVersion;
	    body_part.OperationalPattern = this->m_HeaderPart.OperationalPattern;
	    body_part.EssenceContainers = this->m_HeaderPart.EssenceContainers;
	    body_part.ThisPartition = this->m_ECStart;
	    result = body_part.WriteToFile(this->m_File, body_ul);
	    this->m_RIP.PairArray.push_back(RIP::PartitionPair(1, body_part.ThisPartition)); // Second RIP Entry
	  }

	return result;
      }

      Result_t FlushIndexPartition()
      {
          Result_t result = RESULT_OK;
	    if ( this->m_IndexWriter.GetDuration() > 0 )
	    {
	    this->m_IndexWriter.ThisPartition = this->m_File.TellPosition();
	    result = this->m_IndexWriter.WriteToFile(this->m_File);
	    this->m_RIP.PairArray.push_back(RIP::PartitionPair(0, this->m_IndexWriter.ThisPartition));
	    }
          return result;
      }
      
      // standard method of writing the header and footer of a completed AS-02 file
      //
      Result_t WriteAS02Footer()
      {
          Result_t result = this->FlushIndexPartition();
	  
	// update all Duration properties
	ASDCP::MXF::Partition footer_part(this->m_Dict);
	DurationElementList_t::iterator dli = this->m_DurationUpdateList.begin();

	for (; dli != this->m_DurationUpdateList.end(); ++dli )
	  {
	    **dli = this->m_FramesWritten;
	  }

	this->m_EssenceDescriptor->ContainerDuration = this->m_FramesWritten;
	footer_part.PreviousPartition = this->m_RIP.PairArray.back().ByteOffset;

	Kumu::fpos_t here = this->m_File.TellPosition();
	this->m_RIP.PairArray.push_back(RIP::PartitionPair(0, here)); // Last RIP Entry
	this->m_HeaderPart.FooterPartition = here;

	assert(this->m_Dict);
	footer_part.MajorVersion = this->m_HeaderPart.MajorVersion;
	footer_part.MinorVersion = this->m_HeaderPart.MinorVersion;
	footer_part.OperationalPattern = this->m_HeaderPart.OperationalPattern;
	footer_part.EssenceContainers = this->m_HeaderPart.EssenceContainers;
	footer_part.FooterPartition = here;
	footer_part.ThisPartition = here;

    if (KM_SUCCESS(result))
    {
        UL footer_ul(this->m_Dict->ul(MDD_CompleteFooter));
        result = footer_part.WriteToFile(this->m_File, footer_ul);
    }

	if ( KM_SUCCESS(result) )
	  result = this->m_RIP.WriteToFile(this->m_File);

	if ( KM_SUCCESS(result) )
	  result = this->m_File.Seek(0);
	
	if ( KM_SUCCESS(result) )
	  result = m_HeaderPart.WriteToFile(this->m_File, this->m_HeaderSize);
  
	if ( KM_SUCCESS(result) )
	  {
	    ASDCP::MXF::RIP::const_pair_iterator i = this->m_RIP.PairArray.begin();
	    ui64_t previous_partition = 0;

	    for ( i = this->m_RIP.PairArray.begin(); KM_SUCCESS(result) && i != this->m_RIP.PairArray.end(); ++i )
	      {
		ASDCP::MXF::Partition plain_part(this->m_Dict);
		result = this->m_File.Seek(i->ByteOffset);

		if ( KM_SUCCESS(result) )
		  result = plain_part.InitFromFile(this->m_File);
		
		if ( KM_SUCCESS(result)
		     && ( plain_part.IndexSID > 0 || plain_part.BodySID > 0 ) )
		  {
		    plain_part.PreviousPartition = previous_partition;
		    plain_part.FooterPartition = footer_part.ThisPartition;
		    previous_partition = plain_part.ThisPartition;
		    result = this->m_File.Seek(i->ByteOffset);

		    if ( KM_SUCCESS(result) )
		      {
			UL tmp_ul = plain_part.GetUL();
			result = plain_part.WriteToFile(this->m_File, tmp_ul);
		      }
		  }
	      }
	  }
	
	this->m_File.Close();
	return result;
      }
    };

  //
  class h__AS02WriterFrame : public h__AS02Writer<AS_02::MXF::AS02IndexWriterVBR>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02WriterFrame);
      h__AS02WriterFrame();

    public:
      IndexStrategy_t m_IndexStrategy; // per SMPTE ST 2067-5

      h__AS02WriterFrame(const Dictionary*);
      virtual ~h__AS02WriterFrame();

      Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,const byte_t* EssenceUL,
			       const ui32_t& MinEssenceElementBerLength,
			       AESEncContext* Ctx, HMACContext* HMAC);
    };

  //
  template <typename IndexWriterType>
  class h__AS02WriterClip : public h__AS02Writer<IndexWriterType>
    {
      ASDCP_NO_COPY_CONSTRUCT(h__AS02WriterClip);
      h__AS02WriterClip() {}

    public:
        ui64_t  m_ECStart; // offset of the first essence element
        ui64_t  m_ClipStart;  // state variable for clip-wrap-in-progress
        IndexStrategy_t m_IndexStrategy; // per SMPTE ST 2067-5

        h__AS02WriterClip(const Dictionary* d) :
            h__AS02Writer<IndexWriterType>(d),
            m_ECStart(0), m_ClipStart(0), m_IndexStrategy(AS_02::IS_FOLLOW)
        {}
        virtual ~h__AS02WriterClip()
        {}

        bool HasOpenClip() const { return m_ClipStart != 0; }
        Result_t StartClip(const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC)
        {
            if (Ctx != 0)
            {
                DefaultLogSink().Error("Encryption not yet supported for PCM clip-wrap.\n");
                return RESULT_STATE;
            }

            if (m_ClipStart != 0)
            {
                DefaultLogSink().Error("Cannot open clip, clip already open.\n");
                return RESULT_STATE;
            }

            m_ClipStart = h__AS02Writer<IndexWriterType>::m_File.TellPosition();
            byte_t clip_buffer[24] = { 0 };
            memcpy(clip_buffer, EssenceUL, 16);
            bool check = Kumu::write_BER(clip_buffer + 16, 0, 8);
            assert(check);
            return h__AS02Writer<IndexWriterType>::m_File.Write(clip_buffer, 24);
        }
        Result_t WriteClipBlock(const ASDCP::FrameBuffer& FrameBuf)
        {
            if (m_ClipStart == 0)
            {
                DefaultLogSink().Error("Cannot write clip block, no clip open.\n");
                return RESULT_STATE;
            }

            return h__AS02Writer<IndexWriterType>::m_File.Write(FrameBuf.RoData(), FrameBuf.Size());
        }
        Result_t FinalizeClip(ui32_t bytes_per_frame)
        {
            if (m_ClipStart == 0)
            {
                DefaultLogSink().Error("Cannot close clip, clip not open.\n");
                return RESULT_STATE;
            }

            ui64_t current_position = h__AS02Writer<IndexWriterType>::m_File.TellPosition();
            Result_t result = h__AS02Writer<IndexWriterType>::m_File.Seek(m_ClipStart + 16);

            if (KM_SUCCESS(result))
            {
                byte_t clip_buffer[8] = { 0 };
                ui64_t size = static_cast<ui64_t>(h__AS02Writer<IndexWriterType>::m_FramesWritten) * bytes_per_frame;
                bool check = Kumu::write_BER(clip_buffer, size, 8);
                assert(check);
                result = h__AS02Writer<IndexWriterType>::m_File.Write(clip_buffer, 8);
            }

            if (KM_SUCCESS(result))
            {
                result = h__AS02Writer<IndexWriterType>::m_File.Seek(current_position);
                m_ClipStart = 0;
            }

            return result;
        }
        Result_t FinalizeClip(ui64_t total_bytes_written)
        {
            if (m_ClipStart == 0)
            {
                DefaultLogSink().Error("Cannot close clip, clip not open.\n");
                return RESULT_STATE;
            }

            ui64_t current_position = h__AS02Writer<IndexWriterType>::m_File.TellPosition();
            Result_t result = h__AS02Writer<IndexWriterType>::m_File.Seek(m_ClipStart + 16);

            if (KM_SUCCESS(result))
            {
                byte_t clip_buffer[8] = { 0 };
                bool check = Kumu::write_BER(clip_buffer, total_bytes_written, 8);
                assert(check);
                result = h__AS02Writer<IndexWriterType>::m_File.Write(clip_buffer, 8);
            }

            if (KM_SUCCESS(result))
            {
                result = h__AS02Writer<IndexWriterType>::m_File.Seek(current_position);
                m_ClipStart = 0;
            }

            return result;

        }
    };

} // namespace AS_02

#endif // _AS_02_INTERNAL_H_

//
// end AS_02_internal.h
//
