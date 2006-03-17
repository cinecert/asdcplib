/*
Copyright (c) 2005-2006, John Hurst
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
/*! \file    MXFTypes.h
    \version $Id$
    \brief   MXF objects
*/

#ifndef _MXFTYPES_H_
#define _MXFTYPES_H_

#include "KLV.h"
#include <list>
#include <vector>
#include <map>
#include <wchar.h>

// used with TLVReader::Read*
//
// these are used below to manufacture arguments
#define OBJ_READ_ARGS(s,l) Dict::Type(MDD_##s##_##l), &l
#define OBJ_WRITE_ARGS(s,l) Dict::Type(MDD_##s##_##l), &l
#define OBJ_TYPE_ARGS(t) Dict::Type(MDD_##t).ul


namespace ASDCP
{
  namespace MXF
    {
      typedef std::pair<ui32_t, ui32_t> ItemInfo;
      typedef std::map<TagValue, ItemInfo> TagMap;

      //      
      class TLVReader : public ASDCP::MemIOReader
	{

	  TagMap         m_ElementMap;
	  IPrimerLookup* m_Lookup;

	  TLVReader();
	  ASDCP_NO_COPY_CONSTRUCT(TLVReader);
	  bool FindTL(const MDDEntry&);

	public:
	  TLVReader(const byte_t* p, ui32_t c, IPrimerLookup* = 0);
	  Result_t ReadObject(const MDDEntry&, IArchive*);
	  Result_t ReadUi8(const MDDEntry&, ui8_t*);
	  Result_t ReadUi16(const MDDEntry&, ui16_t*);
	  Result_t ReadUi32(const MDDEntry&, ui32_t*);
	  Result_t ReadUi64(const MDDEntry&, ui64_t*);
	};

      //      
      class TLVWriter : public ASDCP::MemIOWriter
	{

	  TagMap         m_ElementMap;
	  IPrimerLookup* m_Lookup;

	  TLVWriter();
	  ASDCP_NO_COPY_CONSTRUCT(TLVWriter);
	  Result_t WriteTag(const MDDEntry&);

	public:
	  TLVWriter(byte_t* p, ui32_t c, IPrimerLookup* = 0);
	  Result_t WriteObject(const MDDEntry&, IArchive*);
	  Result_t WriteUi8(const MDDEntry&, ui8_t*);
	  Result_t WriteUi16(const MDDEntry&, ui16_t*);
	  Result_t WriteUi32(const MDDEntry&, ui32_t*);
	  Result_t WriteUi64(const MDDEntry&, ui64_t*);
	};

      //
      template <class T>
	class Batch : public std::vector<T>, public IArchive
	{
	public:
	  Batch() {}
	  ~Batch() {}

	  //
	  Result_t Unarchive(ASDCP::MemIOReader& Reader) {
	    ui32_t ItemCount, ItemSize;
	    Result_t result = Reader.ReadUi32BE(&ItemCount);

	    if ( ASDCP_SUCCESS(result) )
	      result = Reader.ReadUi32BE(&ItemSize);

	    if ( ( ItemCount > 65536 ) || ( ItemSize > 1024 ) )
	      return RESULT_FAIL;

	    for ( ui32_t i = 0; i < ItemCount && ASDCP_SUCCESS(result); i++ )
	      {
		T Tmp;
		result = Tmp.Unarchive(Reader);

		if ( ASDCP_SUCCESS(result) )
		  push_back(Tmp);
	      }

	    return result;
	  }

	  inline bool HasValue() const { return ! empty(); }

	  //
	  Result_t Archive(ASDCP::MemIOWriter& Writer) const {
	    Result_t result = Writer.WriteUi32BE(size());
	    byte_t* p = Writer.CurrentData();

	    if ( ASDCP_SUCCESS(result) )
	      result = Writer.WriteUi32BE(0);

	    if ( ASDCP_FAILURE(result) || empty() )
	      return result;
	    
	    typename std::vector<T>::const_iterator l_i = begin();
	    assert(l_i != end());

	    ui32_t ItemSize = Writer.Remainder();
	    result = (*l_i).Archive(Writer);
	    ItemSize -= Writer.Remainder();
	    i2p<ui32_t>(ASDCP_i32_BE(ItemSize), p);
	    l_i++;

	    for ( ; l_i != end() && ASDCP_SUCCESS(result); l_i++ )
	      result = (*l_i).Archive(Writer);

	    return result;
	  }

	  //
	  void Dump(FILE* stream = 0, ui32_t depth = 0)
	    {
	      char identbuf[IdentBufferLen];

	      if ( stream == 0 )
		stream = stderr;

	      typename std::vector<T>::iterator i = this->begin();
	      for ( ; i != this->end(); i++ )
		fprintf(stream, "  %s\n", (*i).ToString(identbuf));
	    }
	};

      //
      template <class T>
	class Array : public std::list<T>, public IArchive
	{
	public:
	  Array() {}
	  ~Array() {}

	  //
	  Result_t Unarchive(ASDCP::MemIOReader& Reader)
	    {
	      while ( Reader.Remainder() > 0 )
		{
		  T Tmp;
		  Tmp.Unarchive(Reader);
		  push_back(Tmp);
		}

	      return RESULT_OK;
	    }

	  inline bool HasValue() const { return ! empty(); }

	  //
	  Result_t Archive(ASDCP::MemIOWriter& Writer) const {
	    Result_t result = RESULT_OK;
	    typename std::list<T>::const_iterator l_i = begin();

	    for ( ; l_i != end() && ASDCP_SUCCESS(result); l_i++ )
	      result = (*l_i).Archive(Writer);

	    return result;
	  }

	  //
	  void Dump(FILE* stream = 0, ui32_t depth = 0)
	    {
	      char identbuf[IdentBufferLen];

	      if ( stream == 0 )
		stream = stderr;

	      typename std::list<T>::iterator i = this->begin();
	      for ( ; i != this->end(); i++ )
		fprintf(stream, "  %s\n", (*i).ToString(identbuf));
	    }
	};

      //
      class Timestamp : public IArchive
	{
	public:
	  ui16_t Year;
	  ui8_t  Month;
	  ui8_t  Day;
	  ui8_t  Hour;
	  ui8_t  Minute;
	  ui8_t  Second;
	  ui8_t  Tick;

	  Timestamp();
	  Timestamp(const Timestamp& rhs);
	  Timestamp(const char* datestr);
	  virtual ~Timestamp();

	  const Timestamp& operator=(const Timestamp& rhs);
	  bool operator<(const Timestamp& rhs) const;
	  bool operator==(const Timestamp& rhs) const;
	  bool operator!=(const Timestamp& rhs) const;

	  // decode and set value from string formatted by EncodeAsString
	  Result_t    SetFromString(const char* datestr);
	  
	  // add the given number of days or hours to the timestamp value. Values less than zero
	  // will cause the value to decrease
	  void AddDays(i32_t);
	  void AddHours(i32_t);

	  // Write the timestamp value to the given buffer in the form 2004-05-01 13:20:00.000
	  // returns 0 if the buffer is smaller than DateTimeLen
	  const char* ToString(char* str_buf) const;

	  //
	  inline Result_t Unarchive(ASDCP::MemIOReader& Reader) {
	    Result_t result = Reader.ReadUi16BE(&Year);

	    if ( ASDCP_SUCCESS(result) )
	      result = Reader.ReadRaw(&Month, 6);

	    return result;
	  }

	  inline bool HasValue() const { return true; }

	  //
	  inline Result_t Archive(ASDCP::MemIOWriter& Writer) const {
	    Result_t result = Writer.WriteUi16BE(Year);

	    if ( ASDCP_SUCCESS(result) )
	      result = Writer.WriteRaw(&Month, 6);

	    return result;
	  }
	};

      //
      class UTF16String : public IArchive
	{
	  ui16_t m_length;
	  char   m_buffer[IdentBufferLen];
	  ASDCP_NO_COPY_CONSTRUCT(UTF16String);
	  
	public:
	  UTF16String() : m_length(0) { *m_buffer = 0; }
	  ~UTF16String() {}

	  const UTF16String& operator=(const char*);

	  //
	  const char* ToString(char* str_buf) const {
	    strncpy(str_buf, m_buffer, m_length+1);
	    return str_buf;
	  }

	  Result_t Unarchive(ASDCP::MemIOReader& Reader);
	  inline bool HasValue() const { return m_length > 0; }
	  Result_t Archive(ASDCP::MemIOWriter& Writer) const;
	};

      //
      class Rational : public ASDCP::Rational, public IArchive
	{
	public:
	  Rational() {}
	  ~Rational() {}

	  Rational(const Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	  }

	  const Rational& operator=(const Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	    return *this;
	  }

	  Rational(const ASDCP::Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	  }

	  const Rational& operator=(const ASDCP::Rational& rhs) {
	    Numerator = rhs.Numerator;
	    Denominator = rhs.Denominator;
	    return *this;
	  }

	  //
	  const char* ToString(char* str_buf) const {
	    snprintf(str_buf, IdentBufferLen, "%lu/%lu", Numerator, Denominator);
	    return str_buf;
	  }

	  Result_t Unarchive(ASDCP::MemIOReader& Reader) {
	    Result_t result = Reader.ReadUi32BE((ui32_t*)&Numerator);

	    if ( ASDCP_SUCCESS(result) )
	      result = Reader.ReadUi32BE((ui32_t*)&Denominator);
	    
	    return result;
	  }

	  inline bool HasValue() const { return true; }

	  Result_t Archive(ASDCP::MemIOWriter& Writer) const {
	    Result_t result = Writer.WriteUi32BE((ui32_t)Numerator);

	    if ( ASDCP_SUCCESS(result) )
	      result = Writer.WriteUi32BE((ui32_t)Denominator);
	    
	    return result;
	  }
	};

      //
      class VersionType : public IArchive
	{
	  ASDCP_NO_COPY_CONSTRUCT(VersionType);

	public:
	  enum Release_t { RL_UNKNOWN, RM_RELEASE, RL_DEVELOPMENT, RL_PATCHED, RL_BETA, RL_PRIVATE };
	  ui16_t Major;
	  ui16_t Minor;
	  ui16_t Patch;
	  ui16_t Build;
	  ui16_t Release;

	  VersionType() : Major(0), Minor(0), Patch(0), Build(0), Release(RL_UNKNOWN) {}
	  ~VersionType() {}
	  void Dump(FILE* = 0);

	  const char* ToString(char* str_buf) const {
	    snprintf(str_buf, IdentBufferLen, "%hu.%hu.%hu.%hur%hu", Major, Minor, Patch, Build, Release);
	    return str_buf;
	  }

	  Result_t Unarchive(ASDCP::MemIOReader& Reader) {
	    Result_t result = Reader.ReadUi16BE(&Major);
	    if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi16BE(&Minor);
	    if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi16BE(&Patch);
	    if ( ASDCP_SUCCESS(result) ) result = Reader.ReadUi16BE(&Build);
	    if ( ASDCP_SUCCESS(result) )
	      {
		ui16_t tmp_release;
		result = Reader.ReadUi16BE(&tmp_release);
		Release = (Release_t)tmp_release;
	      }

	    return result;
	  }

	  inline bool HasValue() const { return true; }

	  Result_t Archive(ASDCP::MemIOWriter& Writer) const {
	    Result_t result = Writer.WriteUi16BE(Major);
	    if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi16BE(Minor);
	    if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi16BE(Patch);
	    if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi16BE(Build);
	    if ( ASDCP_SUCCESS(result) ) result = Writer.WriteUi16BE((ui16_t)(Release & 0x0000ffffL));
	    return result;
	  }
	};

      //
      class Raw : public ASDCP::FrameBuffer, public IArchive
	{
	  ASDCP_NO_COPY_CONSTRUCT(Raw);

	public:
	  Raw();
	  ~Raw();

	  //
          Result_t    Unarchive(ASDCP::MemIOReader& Reader);
	  inline bool HasValue() const { return Size() > 0; }
	  Result_t    Archive(ASDCP::MemIOWriter& Writer) const;
	  const char* ToString(char* str_buf) const;
	};

    } // namespace MXF
} // namespace ASDCP


#endif //_MXFTYPES_H_

//
// end MXFTypes.h
//
