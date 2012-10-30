/*
  Copyright (c) 2011-2012, Robert Scheler, Heiko Sparenberg Fraunhofer IIS, John Hurst
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

#include "KM_platform.h"
#include "KM_util.h"
#include "KM_log.h"
#include "Metadata.h"
#include "AS_DCP_internal.h"
#include "AS_02.h"

using Kumu::DefaultLogSink;

using namespace ASDCP;
using namespace ASDCP::MXF;

namespace AS_02
{
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
  class h__Reader
  {
    ASDCP_NO_COPY_CONSTRUCT(h__Reader);
    h__Reader();

  public:
    const Dictionary*  m_Dict;
    Kumu::FileReader   m_File;
    OPAtomHeader       m_HeaderPart;
    //more than one Body-Partition use a list 
    std::vector<Partition*> m_BodyPartList;
    OPAtomIndexFooter  m_FooterPart;
    ui64_t             m_EssenceStart;
    WriterInfo         m_Info;
    ASDCP::FrameBuffer m_CtFrameBuf;
    Kumu::fpos_t       m_LastPosition;

    IndexStrategy_t    m_IndexStrategy; //Shim parameter index_strategy_frame/clip
    ui32_t             m_PartitionSpace;

    ////new elements for AS-02
    Partition*         m_pCurrentBodyPartition;
    AS_02::MXF::OP1aIndexBodyPartion* m_pCurrentIndexPartition;
    ui32_t	       m_start_pos;

    h__Reader(const Dictionary&);
    virtual ~h__Reader();

    Result_t InitInfo();
    virtual Result_t OpenMXFRead(const char* filename) = 0;
    Result_t InitMXFIndex();

    // positions file before reading
    virtual Result_t ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
				   const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC) = 0;

    // reads from current position
    virtual Result_t ReadEKLVPacket(ui32_t FrameNum, ui32_t SequenceNum, ASDCP::FrameBuffer& FrameBuf,
				    const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC) = 0;
    void     Close();
  };

  //
  class h__Writer
  {
    ASDCP_NO_COPY_CONSTRUCT(h__Writer);
    h__Writer();

  public:
    const Dictionary*  m_Dict;
    Kumu::FileWriter   m_File;
    ui32_t             m_HeaderSize;
    OPAtomHeader       m_HeaderPart;
    //more than one Body-Partition -> use a list of Partitions
    std::vector<Partition*> m_BodyPartList;
    //we don't use the footer like in the dcps but we need also a footer
    AS_02::MXF::OP1aIndexFooter    m_FooterPart;
    ui64_t             m_EssenceStart;

    MaterialPackage*   m_MaterialPackage;
    SourcePackage*     m_FilePackage;

    FileDescriptor*    m_EssenceDescriptor;
    std::list<InterchangeObject*> m_EssenceSubDescriptorList;

    ui32_t             m_FramesWritten;
    ui64_t             m_StreamOffset;
    ASDCP::FrameBuffer m_CtFrameBuf;
    h__WriterState     m_State;
    WriterInfo         m_Info;
    DurationElementList_t m_DurationUpdateList;

    //new elements for AS-02
    ui64_t             m_BodyOffset;
    //TODO: Currently not used, delete if not necessary
    // Counter values for BodySID and IndexSID
    ui32_t             m_CurrentBodySID;
    ui32_t             m_CurrentIndexSID;
    //TODO: maybe set this to the lf__Writer class because this is only in JP2K creation necessary
    //our computed PartitionSpace
    ui32_t             m_PartitionSpace;
    IndexStrategy_t    m_IndexStrategy; //Shim parameter index_strategy_frame/clip

    //the EditRate
    ASDCP::MXF::Rational      m_EditRate;
    //the BytesPerEditUnit
    ui32_t			   m_BytesPerEditUnit;
    //pointer to the current Body Partition(Index)
    AS_02::MXF::OP1aIndexBodyPartion*  m_CurrentIndexBodyPartition;

    h__Writer(const Dictionary&);
    virtual ~h__Writer();

    //virtual methods, implementation details for JP2K or PCM are in the MXFWriter::h__Writer classes if they are different
    virtual void InitHeader();
    /*virtual*/ void AddSourceClip(const ASDCP::MXF::Rational& EditRate, ui32_t TCFrameRate,
				   const std::string& TrackName, const UL& EssenceUL,
				   const UL& DataDefinition, const std::string& PackageLabel);
    /*virtual*/ void AddDMSegment(const ASDCP::MXF::Rational& EditRate, ui32_t TCFrameRate,
				  const std::string& TrackName, const UL& DataDefinition,
				  const std::string& PackageLabel);
    /*virtual*/ void AddEssenceDescriptor(const ASDCP::UL& WrappingUL);
    virtual Result_t CreateBodyPart(const ASDCP::MXF::Rational& EditRate, ui32_t BytesPerEditUnit = 0);

    //new method to create BodyPartition for essence and index
    virtual Result_t CreateBodyPartPair();
    //new method to finalize BodyPartion(index)
    virtual Result_t CompleteIndexBodyPart();

    // all the above for a single source clip
    virtual Result_t WriteMXFHeader(const std::string& PackageLabel, const UL& WrappingUL,
				    const std::string& TrackName, const UL& EssenceUL,
				    const UL& DataDefinition, const ASDCP::MXF::Rational& EditRate,
				    ui32_t TCFrameRate, ui32_t BytesPerEditUnit = 0);

    virtual Result_t WriteEKLVPacket(const ASDCP::FrameBuffer& FrameBuf,
				     const byte_t* EssenceUL, AESEncContext* Ctx, HMACContext* HMAC);

    virtual Result_t WriteMXFFooter() = 0;

  };

} // namespace AS_02

#endif // _AS_02_INTERNAL_H_

//
// end AS_02_internal.h
//
