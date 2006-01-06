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
#define OBJ_READ_ARGS(s,l) s_MDD_Table[MDDindex_##s##_##l], &l
#define OBJ_READ_ARGS_R(s,l,r) s_MDD_Table[MDDindex_##s##_##l], &r

#define OBJ_WRITE_ARGS(s,l) s_MDD_Table[MDDindex_##s##_##l], &l

#define OBJ_TYPE_ARGS(t) s_MDD_Table[MDDindex_##t].ul


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
	  ui32_t ItemCount;
	  ui32_t ItemSize;

	  Batch() : ItemCount(0), ItemSize(0) { ItemSize = sizeof(T); }
	  ~Batch() {}

	  //
	  Result_t ReadFrom(ASDCP::MemIOReader& Reader) {
	    Result_t result = Reader.ReadUi32BE(&ItemCount);

	    if ( ASDCP_SUCCESS(result) )
	      result = Reader.ReadUi32BE(&ItemSize);

	    if ( ( ItemCount > 65536 ) || ( ItemSize > 1024 ) )
	      return RESULT_FAIL;

	    for ( ui32_t i = 0; i < ItemCount && ASDCP_SUCCESS(result); i++ )
	      {
		T Tmp;
		result = Tmp.ReadFrom(Reader);

		if ( ASDCP_SUCCESS(result) )
		  push_back(Tmp);
	      }

	    return result;
	  }

	  //
	  Result_t WriteTo(ASDCP::MemIOWriter& Writer) {
	    Result_t result = Writer.WriteUi32BE(size());

	    if ( ASDCP_SUCCESS(result) )
	      result = Writer.WriteUi32BE(ItemSize);

	    typename std::vector<T>::iterator l_i = begin();
	    for ( ; l_i != end() && ASDCP_SUCCESS(result); l_i++ )
	      result = (*l_i).WriteTo(Writer);

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
	  Result_t ReadFrom(ASDCP::MemIOReader& Reader)
	    {
	      while ( Reader.Remainder() > 0 )
		{
		  T Tmp;
		  Tmp.ReadFrom(Reader);
		  push_back(Tmp);
		}

	      return RESULT_OK;
	    }

	  //
	  Result_t WriteTo(ASDCP::MemIOWriter& Writer) {
	    Result_t result = RESULT_OK;
	    typename std::list<T>::iterator l_i = begin();

	    for ( ; l_i != end() && ASDCP_SUCCESS(result); l_i++ )
	      result = (*l_i).WriteTo(Writer);

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
	  ui8_t  mSec_4;

	  Timestamp() :
	    Year(0), Month(0),  Day(0),
	    Hour(0), Minute(0), Second(0), mSec_4(0) {}

	  //
	  inline const char* ToString(char* str_buf) const {
	    snprintf(str_buf, IdentBufferLen,
		     "%04hu-%02hu-%02hu %02hu:%02hu:%02hu.%03hu",
		     Year, Month, Day, Hour, Minute, Second, mSec_4);
	    return str_buf;
	  }

	  //
	  inline Result_t ReadFrom(ASDCP::MemIOReader& Reader) {
	    Result_t result = Reader.ReadUi16BE(&Year);

	    if ( ASDCP_SUCCESS(result) )
	      result = Reader.ReadRaw(&Month, 6);

	    return result;
	  }

	  //
	  inline Result_t WriteTo(ASDCP::MemIOWriter& Writer) {
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
	  
	public:
	  UTF16String() : m_length(0) { *m_buffer = 0; }
	  ~UTF16String() {}

	  //
	  const char* ToString(char* str_buf) const {
	    strncpy(str_buf, m_buffer, m_length+1);
	    return str_buf;
	  }

	  Result_t ReadFrom(ASDCP::MemIOReader& Reader);
	  Result_t WriteTo(ASDCP::MemIOWriter& Writer);
	};

      //
      class Rational : public ASDCP::Rational, public IArchive
	{
	public:
	  Rational() {}
	  ~Rational() {}

	  //
	  const char* ToString(char* str_buf) const {
	    snprintf(str_buf, IdentBufferLen, "%lu/%lu", Numerator, Denominator);
	    return str_buf;
	  }

	  Result_t ReadFrom(ASDCP::MemIOReader& Reader) {
	    Result_t result = Reader.ReadUi32BE((ui32_t*)&Numerator);

	    if ( ASDCP_SUCCESS(result) )
	      result = Reader.ReadUi32BE((ui32_t*)&Denominator);
	    
	    return result;
	  }

	  Result_t WriteTo(ASDCP::MemIOWriter& Writer) {
	    Result_t result = Writer.WriteUi32BE((ui32_t)Numerator);

	    if ( ASDCP_SUCCESS(result) )
	      result = Writer.WriteUi32BE((ui32_t)Denominator);
	    
	    return result;
	  }
	};

    } // namespace MXF
} // namespace ASDCP


#endif //_MXFTYPES_H_

//
// end MXFTypes.h
//
