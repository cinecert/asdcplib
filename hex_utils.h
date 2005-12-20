/*
Copyright (c) 2004-2005, John Hurst
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
/*! \file    hex_utils.h
    \version $Id$       
    \brief   AS-DCP library, printable hex utilities
*/

#ifndef _AS_DCP_HEX_UTILS_H__
#define _AS_DCP_HEX_UTILS_H__

namespace ASDCP
{
  // Convert NULL-terminated UTF-8 hexadecimal string to binary, returns 0 if
  // the binary buffer was large enough to hold the result. The output parameter
  // 'char_count' will contain the length of the converted string. If the output
  // buffer is too small or any of the pointer arguments are NULL, the subroutine
  // will return -1 and set 'char_count' to the required buffer size. No data will
  // be written to 'buf' if the subroutine fails.
  i32_t       hex2bin(const char* str, byte_t* buf, ui32_t buf_len, ui32_t* char_count);

  // Convert a binary string to NULL-terminated UTF-8 hexadecimal, returns the buffer
  // if the binary buffer was large enough to hold the result. If the output buffer
  // is too small or any of the pointer arguments are NULL, the subroutine will
  // return 0.
  //
  const char* bin2hex(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len);


  // print to a stream the contents of the buffer as hexadecimal numbers in numbered
  // rows of 16-bytes each.
  //
  void hexdump(const byte_t* buf, ui32_t dump_len, FILE* stream = 0);

  // Return the length in bytes of a BER encoded value
  inline ui32_t BER_length(const byte_t* buf)
    {
      if ( buf == 0 || (*buf & 0xf0) != 0x80 )
	return 0;

      return (*buf & 0x0f) + 1;
    }

  // read a BER value
  bool read_BER(const byte_t* buf, ui64_t* val);

  // decode a ber value and compare it to a test value
  bool read_test_BER(byte_t **buf, ui64_t test_value);

  // create BER encoding of integer value
  bool write_BER(byte_t* buf, ui64_t val, ui32_t ber_len = 0);

} // namespace ASDCP

#endif // _AS_DCP_HEX_UTILS_H__

//
// end hex_utils.cpp
//

