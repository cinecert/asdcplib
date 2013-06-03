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
/*! \file    h__02_Reader.cpp
  \version $Id$
  \brief   MXF file reader base class
*/

#define DEFAULT_MD_DECL
#include "AS_02_internal.h"

using namespace ASDCP;
using namespace ASDCP::MXF;


static Kumu::Mutex sg_DefaultMDInitLock;
static bool        sg_DefaultMDTypesInit = false;
static const ASDCP::Dictionary *sg_dict;

//
void
AS_02::default_md_object_init()
{
  if ( ! sg_DefaultMDTypesInit )
    {
      Kumu::AutoMutex BlockLock(sg_DefaultMDInitLock);

      if ( ! sg_DefaultMDTypesInit )
	{
	  sg_dict = &DefaultSMPTEDict();
	  g_AS02IndexReader = new AS_02::MXF::AS02IndexReader(sg_dict);
	  sg_DefaultMDTypesInit = true;
	}
    }
}




AS_02::h__AS02Reader::h__AS02Reader(const ASDCP::Dictionary& d) : ASDCP::MXF::TrackFileReader<ASDCP::MXF::OP1aHeader, AS_02::MXF::AS02IndexReader>(d) {}
AS_02::h__AS02Reader::~h__AS02Reader() {}



// AS-DCP method of opening an MXF file for read
Result_t
AS_02::h__AS02Reader::OpenMXFRead(const char* filename)
{
  Result_t result = ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::OpenMXFRead(filename);

  if ( KM_SUCCESS(result) )
    result = ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::InitInfo();

  if( KM_SUCCESS(result) )
    {
      //
      InterchangeObject* Object;
      m_Info.LabelSetType = LS_MXF_SMPTE;

      if ( ! m_HeaderPart.OperationalPattern.ExactMatch(SMPTE_390_OPAtom_Entry().ul) )
	{
	  char strbuf[IdentBufferLen];
	  const MDDEntry* Entry = m_Dict->FindUL(m_HeaderPart.OperationalPattern.Value());

	  if ( Entry == 0 )
	    {
	      DefaultLogSink().Warn("Operational pattern is not OP-1a: %s\n",
				    m_HeaderPart.OperationalPattern.EncodeString(strbuf, IdentBufferLen));
	    }
	  else
	    {
	      DefaultLogSink().Warn("Operational pattern is not OP-1a: %s\n", Entry->name);
	    }
	}

      //
      if ( m_RIP.PairArray.front().ByteOffset != 0 )
	{
	  DefaultLogSink().Error("First Partition in RIP is not at offset 0.\n");
	  result = RESULT_FORMAT;
	}
    }

  if ( KM_SUCCESS(result) )
    {
      m_HeaderPart.BodyOffset = m_File.Tell();

      result = m_File.Seek(m_HeaderPart.FooterPartition);

      if ( ASDCP_SUCCESS(result) )
	{
	  m_IndexAccess.m_Lookup = &m_HeaderPart.m_Primer;
	  result = m_IndexAccess.InitFromFile(m_File);
	}
    }

  m_File.Seek(m_HeaderPart.BodyOffset);
  return result;
}

// AS-DCP method of reading a plaintext or encrypted frame
Result_t
AS_02::h__AS02Reader::ReadEKLVFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf,
				     const byte_t* EssenceUL, AESDecContext* Ctx, HMACContext* HMAC)
{
  return ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::ReadEKLVFrame(m_HeaderPart, FrameNum, FrameBuf,
										     EssenceUL, Ctx, HMAC);
}

Result_t
AS_02::h__AS02Reader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset,
                           i8_t& temporalOffset, i8_t& keyFrameOffset)
{
  return ASDCP::MXF::TrackFileReader<OP1aHeader, AS_02::MXF::AS02IndexReader>::LocateFrame(m_HeaderPart, FrameNum,
                                                                                   streamOffset, temporalOffset, keyFrameOffset);
}


//
// end h__02_Reader.cpp
//
