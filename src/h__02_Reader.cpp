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


#if 0
//
AS_02::h__AS02Reader::h__Reader(const Dictionary& d) :
  m_HeaderPart(m_Dict), m_IndexAccess(m_Dict), m_Dict(&d), m_EssenceStart(0)
{
  m_pCurrentIndexPartition = 0;
  ////	start_pos = 0;
}

AS_02::h__AS02Reader::~h__Reader()
{
  std::vector<Partition*>::iterator bli = m_BodyPartList.begin();
  for ( ; bli != m_BodyPartList.end(); bli++ ){
    delete(*bli);
    *bli = 0;
  }
  Close();
}
#endif

//------------------------------------------------------------------------------------------
//

//
// end h__02_Reader.cpp
//
