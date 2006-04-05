/*
Copyright (c) 2006, John Hurst
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
#include <map>
 
static Kumu::Mutex s_Lock;
static bool    s_md_init = false;
static std::map<ASDCP::UL, ui32_t> s_md_lookup;

//------------------------------------------------------------------------------------------
// singleton wrapper

//
const ASDCP::MDDEntry&
ASDCP::Dict::Type(MDD_t type_id)
{
  return s_MDD_Table[type_id];
}


//
const ASDCP::MDDEntry*
ASDCP::Dict::FindUL(const byte_t* ul_buf)
{
  if ( ! s_md_init )
    {
      Kumu::AutoMutex AL(s_Lock);

      if ( ! s_md_init )
	{
	  for ( ui32_t x = 0; x < s_MDD_Table_size; x++ )
	    s_md_lookup.insert(std::map<UL, ui32_t>::value_type(UL(s_MDD_Table[x].ul), x));

	  s_md_init = true;
	}
    }

  std::map<UL, ui32_t>::iterator i = s_md_lookup.find(UL(ul_buf));
  
  if ( i == s_md_lookup.end() )
    {
      byte_t tmp_ul[SMPTE_UL_LENGTH];
      memcpy(tmp_ul, ul_buf, SMPTE_UL_LENGTH);
      tmp_ul[SMPTE_UL_LENGTH-1] = 0;

      i = s_md_lookup.find(UL(tmp_ul));

      if ( i == s_md_lookup.end() )
	return 0;
    }

  return &s_MDD_Table[(*i).second];
}


//
// end Dict.cpp
//
