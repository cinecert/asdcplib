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


#include "KLV.h"
#include "MDD.cpp"

//------------------------------------------------------------------------------------------
// singleton wrapper

const byte_t mdd_key[] = { 0x06, 0x0e, 0x2b, 0x34 };

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
  ui32_t t_idx = 0;
  ui32_t k_idx = 8;

  // must be a pointer to a SMPTE UL
  if ( ul_buf == 0 || memcmp(mdd_key, ul_buf, 4) != 0 )
    return 0;

  // advance to first matching element
  // TODO: optimize using binary search
  while ( s_MDD_Table[t_idx].ul != 0
  	  && s_MDD_Table[t_idx].ul[k_idx] != ul_buf[k_idx] )
    t_idx++;

  if ( s_MDD_Table[t_idx].ul == 0 )
    return 0;

  // match successive elements
  while ( s_MDD_Table[t_idx].ul != 0
	  && k_idx < SMPTE_UL_LENGTH - 1
	  && s_MDD_Table[t_idx].ul[k_idx] == ul_buf[k_idx] )
    {
      if ( s_MDD_Table[t_idx].ul[k_idx+1] == ul_buf[k_idx+1] )
	{
	  k_idx++;
	}
      else
	{
	  while ( s_MDD_Table[t_idx].ul != 0
		  && s_MDD_Table[t_idx].ul[k_idx] == ul_buf[k_idx]
		  && s_MDD_Table[t_idx].ul[k_idx+1] != ul_buf[k_idx+1] )
	    t_idx++;
	      
	  while ( s_MDD_Table[t_idx].ul[k_idx] != ul_buf[k_idx] )
	    k_idx--;
	}
    }

  return (s_MDD_Table[t_idx].ul == 0 ? 0 : &s_MDD_Table[t_idx]);
}


//
// implementation

ASDCP::Dict::Dict() { DefaultLogSink().Warn("new Dict\n"); }
ASDCP::Dict::~Dict() {}



//
// end Dict.cpp
//
