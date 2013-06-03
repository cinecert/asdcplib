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
/*! \file    h__02_Writer.cpp
  \version $Id$
  \brief   MXF file writer base class
*/

#include "AS_02_internal.h"

using namespace ASDCP;
using namespace ASDCP::MXF;


//
AS_02::h__AS02Writer::h__AS02Writer(const ASDCP::Dictionary& d) : ASDCP::MXF::TrackFileWriter<ASDCP::MXF::OP1aHeader>(d) {}
AS_02::h__AS02Writer::~h__AS02Writer() {}

//
Result_t
AS_02::h__AS02Writer::WriteAS02Header(const std::string& PackageLabel, const ASDCP::UL& WrappingUL,
			       const std::string& TrackName, const ASDCP::UL& EssenceUL,
			       const ASDCP::UL& DataDefinition, const ASDCP::Rational& EditRate,
			       ui32_t TCFrameRate, ui32_t BytesPerEditUnit)
{
  InitHeader();
  AddSourceClip(EditRate, TCFrameRate, TrackName, EssenceUL, DataDefinition, PackageLabel);
  AddEssenceDescriptor(WrappingUL);

  Result_t result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);

  //use the FrameRate for the indexing; initialize the SID counters
  ///  m_CurrentBodySID = 0;
  ///  m_CurrentIndexSID = 0;
  m_PartitionSpace = m_PartitionSpace * TCFrameRate; ///// TODO ????

  return result;
}
    

//
// end h__02_Writer.cpp
//
