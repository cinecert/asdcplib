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
/*! \file    AS_DCP_system.h
    \version $Id$
    \brief   AS-DCP portable types and byte swapping
*/


#ifndef _AS_DCP_SYSTEM_H_
#define _AS_DCP_SYSTEM_H_

#ifdef __APPLE__
#define ASDCP_BIG_ENDIAN
#endif

// 64 bit types are not used in the public interface
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#pragma warning(disable:4786)			// Ignore "identifer > 255 characters" warning
#define snprintf _snprintf

#ifndef ASDCP_NO_BASE_TYPES
typedef unsigned __int64   ui64_t;
typedef __int64            i64_t;
#define i64_C(c)  (i64_t)(c)
#define ui64_C(c) (ui64_t)(c)
#endif

#else // WIN32

#ifndef ASDCP_NO_BASE_TYPES
typedef unsigned long long ui64_t;
typedef long long          i64_t;
#define i64_C(c)  c##LL
#define ui64_C(c) c##ULL
#endif

#endif // WIN32

#include <AS_DCP.h>
#include <assert.h>

namespace ASDCP {

  inline ui16_t Swap2(ui16_t i)
    {
      return ( (i << 8) | (( i & 0xff00) >> 8) );
    }

  inline ui32_t Swap4(ui32_t i)
    {
      return
	( (i & 0x000000ffUL) << 24 ) |
	( (i & 0xff000000UL) >> 24 ) |
	( (i & 0x0000ff00UL) << 8  ) |
	( (i & 0x00ff0000UL) >> 8  );
    }

  inline ui64_t Swap8(ui64_t i)
    {
      return
	( (i & ui64_C(0x00000000000000FF)) << 56 ) |
	( (i & ui64_C(0xFF00000000000000)) >> 56 ) |
	( (i & ui64_C(0x000000000000FF00)) << 40 ) |
	( (i & ui64_C(0x00FF000000000000)) >> 40 ) |
	( (i & ui64_C(0x0000000000FF0000)) << 24 ) |
	( (i & ui64_C(0x0000FF0000000000)) >> 24 ) |
	( (i & ui64_C(0x00000000FF000000)) << 8  ) |
	( (i & ui64_C(0x000000FF00000000)) >> 8  );
    }

  //
  template<class T>
  inline T xmin(T lhs, T rhs)
    {
      return (lhs < rhs) ? lhs : rhs;
    }

  //
  template<class T>
  inline T xmax(T lhs, T rhs)
    {
      return (lhs > rhs) ? lhs : rhs;
    }

  // read an integer from byte-structured storage
  template<class T>
  inline T    cp2i(const byte_t* p) { return *(T*)p; }

  // write an integer to byte-structured storage
  template<class T>
  inline void i2p(T i, byte_t* p) { *(T*)p = i; }

#ifdef ASDCP_BIG_ENDIAN
#define ASDCP_i16_LE(i)        ASDCP::Swap2(i)
#define ASDCP_i32_LE(i)        ASDCP::Swap4(i)
#define ASDCP_i64_LE(i)        ASDCP::Swap8(i)
#define ASDCP_i16_BE(i)        (i)
#define ASDCP_i32_BE(i)        (i)
#define ASDCP_i64_BE(i)        (i)
#else
#define ASDCP_i16_LE(i)        (i)
#define ASDCP_i32_LE(i)        (i)
#define ASDCP_i64_LE(i)        (i)
#define ASDCP_i16_BE(i)        ASDCP::Swap2(i)
#define ASDCP_i32_BE(i)        ASDCP::Swap4(i)
#define ASDCP_i64_BE(i)        ASDCP::Swap8(i)
#endif // ASDCP_BIG_ENDIAN


// 64 bit integer/text conversion
//
  const ui32_t IntBufferLen = 32;

  inline i64_t atoi64(const char *str) {
#ifdef WIN32
    return _atoi64(str);
#else
    return strtoll(str, NULL, 10);
#endif
  }

  inline const char* i64sz(i64_t i, char* buf)
    { 
      assert(buf);
#ifdef WIN32
      snprintf(buf, IntBufferLen, "%I64d", i);
#else
      snprintf(buf, IntBufferLen, "%lld", i);
#endif
      return buf;
    }

  inline const char* ui64sz(ui64_t i, char* buf)
    { 
      assert(buf);
#ifdef WIN32
      snprintf(buf, IntBufferLen, "%I64u", i);
#else
      snprintf(buf, IntBufferLen, "%llu", i);
#endif
      return buf;
    }

  inline const char* i64szx(i64_t i, ui32_t digits, char* buf)
    {
      assert(buf);
      if ( digits > 30 ) digits = 30;
#ifdef WIN32
      snprintf(buf, IntBufferLen, "%0*I64x", digits, i);
#else
      snprintf(buf, IntBufferLen, "%0*llx", digits, i);
#endif
      return buf;
    }

} // namespace ASDCP



#endif // _AS_DCP_SYSTEM_H_

//
// AS_DCP_system.h
//
