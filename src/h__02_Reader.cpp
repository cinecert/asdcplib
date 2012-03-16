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
/*! \file    h__02_Reader.cpp
  \version $Id$
  \brief   MXF file reader base class
*/

#include "AS_02_internal.h"

using namespace ASDCP;
using namespace ASDCP::MXF;

//
AS_02::h__Reader::h__Reader(const Dictionary& d) :
  m_HeaderPart(m_Dict), m_FooterPart(m_Dict), m_Dict(&d), m_EssenceStart(0)
{
  m_pCurrentIndexPartition = 0;
  ////	start_pos = 0;
}

AS_02::h__Reader::~h__Reader()
{
  std::vector<Partition*>::iterator bli = m_BodyPartList.begin();
  for ( ; bli != m_BodyPartList.end(); bli++ ){
    delete(*bli);
    *bli = 0;
  }
  Close();
}

void
AS_02::h__Reader::Close()
{
  m_File.Close();
}

//------------------------------------------------------------------------------------------
//

//
Result_t
AS_02::h__Reader::InitInfo()
{
  assert(m_Dict);
  InterchangeObject* Object;

  UL OPAtomUL(SMPTE_390_OPAtom_Entry().ul);
  UL Interop_OPAtomUL(MXFInterop_OPAtom_Entry().ul);
  m_Info.LabelSetType = LS_MXF_SMPTE;

  // Identification
  Result_t result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(Identification), &Object);

  if( ASDCP_SUCCESS(result) )
    MD_to_WriterInfo((Identification*)Object, m_Info);

  // SourcePackage
  if( ASDCP_SUCCESS(result) )
    result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(SourcePackage), &Object);

  if( ASDCP_SUCCESS(result) )
    {
      SourcePackage* SP = (SourcePackage*)Object;
      memcpy(m_Info.AssetUUID, SP->PackageUID.Value() + 16, UUIDlen);
    }

  // optional CryptographicContext
  if( ASDCP_SUCCESS(result) )
    {
      Result_t cr_result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(CryptographicContext), &Object);

      if( ASDCP_SUCCESS(cr_result) )
	MD_to_CryptoInfo((CryptographicContext*)Object, m_Info, *m_Dict);
    }

  return result;
}

// standard method of populating the in-memory index
Result_t
AS_02::h__Reader::InitMXFIndex()
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  Result_t result = m_File.Seek(m_HeaderPart.FooterPartition);

  if ( ASDCP_SUCCESS(result) )
    {
      m_FooterPart.m_Lookup = &m_HeaderPart.m_Primer;
      //Footer don't need to be initialized because there is no index table in the footer
      //result = m_FooterPart.InitFromFile(m_File);
    }

  if ( ASDCP_SUCCESS(result) )
    m_File.Seek(m_EssenceStart);

  return result;
}


//
// end h__02_Reader.cpp
//
