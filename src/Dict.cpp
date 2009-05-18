/*
Copyright (c) 2006-2009, John Hurst
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
/*! \file    Dict.cpp
  \version $Id$
  \brief   MXF dictionary
*/


#include "KM_mutex.h"
#include "KLV.h"
#include "MDD.cpp"

//------------------------------------------------------------------------------------------

//static ASDCP::Dictionary s_SMPTEDict;
//static ASDCP::Dictionary s_InteropDict;

const ASDCP::Dictionary&
ASDCP::DefaultSMPTEDict() {
  // return s_SMPTEDict;
  return DefaultCompositeDict();
}

const ASDCP::Dictionary&
ASDCP::DefaultInteropDict() {
  // return s_InteropDict;
  return DefaultCompositeDict();
}

//
//
static ASDCP::Dictionary s_CompositeDict;
static Kumu::Mutex s_Lock;
static bool s_md_init = false;

//
const ASDCP::Dictionary&
ASDCP::DefaultCompositeDict()
{
  if ( ! s_md_init )
    {
      Kumu::AutoMutex AL(s_Lock);

      if ( ! s_md_init )
	{
	  for ( ui32_t x = 0; x < ASDCP::MDD_Table_size; x++ )
	    s_CompositeDict.AddEntry(s_MDD_Table[x], x);
	  //	    s_md_lookup.insert(std::map<UL, ui32_t>::value_type(UL(s_MDD_Table[x].ul), x));

	  s_md_init = true;
	}
    }

  return s_CompositeDict;
}

//------------------------------------------------------------------------------------------
//

ASDCP::Dictionary::Dictionary() {}
ASDCP::Dictionary::~Dictionary() {}


//
bool
ASDCP::Dictionary::AddEntry(const MDDEntry& Entry, ui32_t index)
{
  m_MDD_Table[index] = Entry;
  m_md_lookup.insert(std::map<UL, ui32_t>::value_type(UL(Entry.ul), index));
  return true;
}

//
const ASDCP::MDDEntry&
ASDCP::Dictionary::Type(MDD_t type_id) const
{
  return m_MDD_Table[type_id];
}

//
const ASDCP::MDDEntry*
ASDCP::Dictionary::FindUL(const byte_t* ul_buf) const
{
  std::map<UL, ui32_t>::const_iterator i = m_md_lookup.find(UL(ul_buf));
  
  if ( i == m_md_lookup.end() )
    {
      byte_t tmp_ul[SMPTE_UL_LENGTH];
      memcpy(tmp_ul, ul_buf, SMPTE_UL_LENGTH);
      tmp_ul[SMPTE_UL_LENGTH-1] = 0;

      i = m_md_lookup.find(UL(tmp_ul));

      if ( i == m_md_lookup.end() )
	return 0;
    }

  return &m_MDD_Table[(*i).second];
}

//
void
ASDCP::Dictionary::Dump(FILE* stream) const
{
  if ( stream == 0 )
    stream = stderr;

  MDD_t di = (MDD_t)0;
  char     str_buf[64];

  while ( di < MDD_Max )
    {
      MDDEntry TmpType = m_MDD_Table[di];
      UL TmpUL(TmpType.ul);
      fprintf(stream, "%s: %s\n", TmpUL.EncodeString(str_buf, 64), TmpType.name);
      di = (MDD_t)(di + 1);
    }
}

//
// end Dict.cpp
//
