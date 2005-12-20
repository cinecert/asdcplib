/*
Copyright (c) 2005, John Hurst
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
/*! \file    AS_DCP_UUID.cpp
    \version $Id$
    \brief   Miscellaneous timecode functions
*/

#include <AS_DCP_UUID.h>
#include <assert.h>


static FortunaRNG s_RNG;

using namespace ASDCP;


// given a 16 byte buffer, this subroutine uses the given RNG to generate a
// random value, and sets the necessary bits to turn the value into a proper UUID per
// sec. 4.4 of http://www.ietf.org/internet-drafts/draft-mealling-uuid-urn-03.txt
// Parts of that document are reprinted here for your convenience:
//
// 0                   1                   2                   3
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                          time_low                             |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |       time_mid                |         time_hi_and_version   |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |clk_seq_hi_res |  clk_seq_low  |         node (0-1)            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |                         node (2-5)                            |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//     Msb0  Msb1  Msb2  Msb3   Version  Description
//
//      0     1     0     0        4     The randomly or pseudo-
//                                       randomly generated version
//                                       specified in this document.
//
//  o  Set the two most significant bits (bits six and seven) of the
//     clock_seq_hi_and_reserved to zero and one, respectively.
//  o  Set the four most significant bits (bits 12 through 15) of the
//     time_hi_and_version field to the four-bit version number from
//     Section 4.1.3.
//  o  Set all the other bits to randomly (or pseudo-randomly) chosen
//     values.
//

// fill buffer with random bits and set appropriate UUID flags
void
ASDCP::GenRandomUUID(FortunaRNG& RNG, byte_t* buf)
{
  assert(buf);
  RNG.FillRandom(buf, UUIDlen);
  buf[6] &= 0x0f; // clear bits 4-7
  buf[6] |= 0x40; // set UUID version
  buf[8] &= 0x3f; // clear bits 6&7
  buf[8] |= 0x80; // set bit 7
}

// add UUID hyphens to 32 character hexadecimal string
char*
ASDCP::hyphenate_UUID(char* str_buf, ui32_t buf_len)
{
  ui32_t i, j, k;

  assert(str_buf);
  assert (buf_len > 36);

  // shift the node id
  for ( k = 19, i = 12; i > 0; i-- )
    str_buf[k+i+4] = str_buf[k+i];

  // shift the time (mid+hi+clk)
  for ( k = 15, j = 3; k > 6; k -= 4, j-- )
    {
      for ( i = 4; i > 0; i-- )
	str_buf[k+i+j] = str_buf[k+i];
    }

  // add in the hyphens and trainling null
  for ( i = 8; i < 24; i += 5 )
    str_buf[i] = '-';
  
  str_buf[36] = 0;
  return str_buf;
}


//
void
ASDCP::MakeUUID(byte_t* buf)
{
  GenRandomUUID(s_RNG, buf);
}



//
// end AS_DCP_UUID.cpp
//
