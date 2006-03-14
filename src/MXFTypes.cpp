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
/*! \file    MXFTypes.cpp
    \version $Id$
    \brief   MXF objects
*/

#include "MXFTypes.h"
#include "FortunaRNG.h"

//------------------------------------------------------------------------------------------
//


//
void
ASDCP::UMID::MakeUMID(int Type)
{
  UUID AssetID;
  AssetID.GenRandomValue();
  MakeUMID(Type, AssetID);
}

//
void
ASDCP::UMID::MakeUMID(int Type, const UUID& AssetID)
{
  // Set the non-varying base of the UMID
  static const byte_t UMIDBase[10] = { 0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01 };
  memcpy(m_Value, UMIDBase, 10);
  m_Value[10] = Type;  // Material Type
  m_Value[12] = 0x13;  // length

  // preserved for compatibility with mfxlib
  if( Type > 4 ) m_Value[7] = 5;
  m_Value[11] = 0x20; // UUID/UL method, number gen undefined

  // Instance Number
  m_Value[13] = m_Value[14] = m_Value[15] = 0;
  
  memcpy(&m_Value[16], AssetID.Value(), AssetID.Size());
}


// Write the timestamp value to the given buffer in the form 2004-05-01 13:20:00.000
// returns 0 if the buffer is smaller than DateTimeLen
const char*
ASDCP::UMID::ToString(char* str_buf) const
{
  assert(str_buf);

  snprintf(str_buf, IdentBufferLen, "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x],%02x,%02x,%02x,%02x,",
	   m_Value[0],  m_Value[1],  m_Value[2],  m_Value[3],
	   m_Value[4],  m_Value[5],  m_Value[6],  m_Value[7],
	   m_Value[8],  m_Value[9],  m_Value[10], m_Value[11],
	   m_Value[12], m_Value[13], m_Value[14], m_Value[15]
	   );

  ui32_t offset = strlen(str_buf);

  if ( ( m_Value[8] & 0x80 ) == 0 )
    {
      // half-swapped UL, use [bbaa9988.ddcc.ffee.00010203.04050607]
      snprintf(str_buf + offset, IdentBufferLen - offset,
	       "[%02x%02x%02x%02x.%02x%02x.%02x%02x.%02x%02x%02x%02x.%02x%02x%02x%02x]",
               m_Value[24], m_Value[25], m_Value[26], m_Value[27],
	       m_Value[28], m_Value[29], m_Value[30], m_Value[31],
               m_Value[16], m_Value[17], m_Value[18], m_Value[19],
	       m_Value[20], m_Value[21], m_Value[22], m_Value[23]
               );
    }
  else
    {
      // UUID, use {00112233-4455-6677-8899-aabbccddeeff}
      snprintf(str_buf + offset, IdentBufferLen - offset,
	       "{%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
               m_Value[16], m_Value[17], m_Value[18], m_Value[19],
	       m_Value[20], m_Value[21], m_Value[22], m_Value[23],
               m_Value[24], m_Value[25], m_Value[26], m_Value[27],
	       m_Value[28], m_Value[29], m_Value[30], m_Value[31]
               );
    }

  return str_buf;
}

//
void
ASDCP::UUID::GenRandomValue()
{
  FortunaRNG RNG;
  RNG.FillRandom(m_Value, UUIDlen);
  m_Value[6] &= 0x0f; // clear bits 4-7
  m_Value[6] |= 0x40; // set UUID version
  m_Value[8] &= 0x3f; // clear bits 6&7
  m_Value[8] |= 0x80; // set bit 7
  //  m_HasValue = true;
}


//------------------------------------------------------------------------------------------
//

const ASDCP::MXF::UTF16String&
ASDCP::MXF::UTF16String::operator=(const char* sz)
{
  if ( sz == 0 || *sz == 0 )
    {
      m_length = 0;
      *m_buffer = 0;
    }
  else
    {
      ui32_t len = xmin((ui32_t)strlen(sz), (IdentBufferLen - 1));
      m_length = len;
      memcpy(m_buffer, sz, m_length);
      m_buffer[m_length] = 0;
    }
  
  return *this;
}


//
ASDCP::Result_t
ASDCP::MXF::UTF16String::Unarchive(ASDCP::MemIOReader& Reader)
{
  const byte_t* p = Reader.CurrentData();
  m_length = Reader.Remainder();
  assert(m_length % 2 == 0);
  m_length /= 2;
  assert(IdentBufferLen >= m_length);
  ui32_t i = 0;

  for ( i = 0; i < m_length; i++ )
    m_buffer[i] = p[(i*2)+1];

  m_buffer[i] = 0;

  Reader.SkipOffset(m_length*2);
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MXF::UTF16String::Archive(ASDCP::MemIOWriter& Writer) const
{
  byte_t* p = Writer.Data() + Writer.Size();
  ui32_t i = 0;
  memset(p, 0, (m_length*2)+2);

  for ( i = 0; i < m_length; i++ )
    p[(i*2)+1] = m_buffer[i];

  Writer.AddOffset(m_length * 2);
  return RESULT_OK;
}


//------------------------------------------------------------------------------------------
//

#ifdef WIN32

#define TIMESTAMP_TO_SYSTIME(ts, t) \
  (t)->wYear    = (ts).Year;   /* year */ \
  (t)->wMonth   = (ts).Month;  /* month of year (1 - 12) */ \
  (t)->wDay     = (ts).Day;    /* day of month (1 - 31) */ \
  (t)->wHour    = (ts).Hour;   /* hours (0 - 23) */ \
  (t)->wMinute  = (ts).Minute; /* minutes (0 - 59) */ \
  (t)->wSecond  = (ts).Second; /* seconds (0 - 60) */ \
  (t)->wDayOfWeek = 0; \
  (t)->wMilliseconds = ((ts).Tick * 4);

#define SYSTIME_TO_TIMESTAMP(t, ts) \
  (ts).Year   = (t)->wYear;    /* year */ \
  (ts).Month  = (t)->wMonth;   /* month of year (1 - 12) */ \
  (ts).Day    = (t)->wDay;     /* day of month (1 - 31) */ \
  (ts).Hour   = (t)->wHour;    /* hours (0 - 23) */ \
  (ts).Minute = (t)->wMinute;  /* minutes (0 - 59) */ \
  (ts).Second = (t)->wSecond;  /* seconds (0 - 60) */ \
  (ts).Tick   = (t)->wMilliseconds / 4;

//
ASDCP::MXF::Timestamp::Timestamp() :
  Year(0), Month(0),  Day(0), Hour(0), Minute(0), Second(0), Tick(0)
{
  SYSTEMTIME sys_time;
  GetSystemTime(&sys_time);
  SYSTIME_TO_TIMESTAMP(&sys_time, *this);
}

//
bool
ASDCP::MXF::Timestamp::operator<(const Timestamp& rhs) const
{
  SYSTEMTIME lhst, rhst;
  FILETIME lft, rft;

  TIMESTAMP_TO_SYSTIME(*this, &lhst);
  TIMESTAMP_TO_SYSTIME(rhs, &rhst);
  SystemTimeToFileTime(&lhst, &lft);
  SystemTimeToFileTime(&rhst, &rft);
  return ( CompareFileTime(&lft, &rft) == -1 );
}

inline ui64_t
seconds_to_ns100(ui32_t seconds)
{
  return ((ui64_t)seconds * 10000000);
}

//
void
ASDCP::MXF::Timestamp::AddDays(i32_t days)
{
  SYSTEMTIME current_st;
  FILETIME current_ft;
  ULARGE_INTEGER current_ul;

  if ( days != 0 )
    {
      TIMESTAMP_TO_SYSTIME(*this, &current_st);
      SystemTimeToFileTime(&current_st, &current_ft);
      memcpy(&current_ul, &current_ft, sizeof(current_ul));
      current_ul.QuadPart += ( seconds_to_ns100(86400) * (ui64_t)days );
      memcpy(&current_ft, &current_ul, sizeof(current_ft));
      FileTimeToSystemTime(&current_ft, &current_st);
      SYSTIME_TO_TIMESTAMP(&current_st, *this);
    }
}

//
void
ASDCP::MXF::Timestamp::AddHours(i32_t hours)
{
  SYSTEMTIME current_st;
  FILETIME current_ft;
  ULARGE_INTEGER current_ul;

  if ( hours != 0 )
    {
      TIMESTAMP_TO_SYSTIME(*this, &current_st);
      SystemTimeToFileTime(&current_st, &current_ft);
      memcpy(&current_ul, &current_ft, sizeof(current_ul));
      current_ul.QuadPart += ( seconds_to_ns100(3600) * (ui64_t)hours );
      memcpy(&current_ft, &current_ul, sizeof(current_ft));
      FileTimeToSystemTime(&current_ft, &current_st);
      SYSTIME_TO_TIMESTAMP(&current_st, *this);
    }
}

#else // WM_WIN32

#include <time.h>

#define TIMESTAMP_TO_TM(ts, t) \
  (t)->tm_year = (ts).Year - 1900;   /* year - 1900 */ \
  (t)->tm_mon  = (ts).Month - 1;     /* month of year (0 - 11) */ \
  (t)->tm_mday = (ts).Day;           /* day of month (1 - 31) */ \
  (t)->tm_hour = (ts).Hour;          /* hours (0 - 23) */ \
  (t)->tm_min  = (ts).Minute;        /* minutes (0 - 59) */ \
  (t)->tm_sec  = (ts).Second;        /* seconds (0 - 60) */

#define TM_TO_TIMESTAMP(t, ts) \
  (ts).Year   = (t)->tm_year + 1900;    /* year - 1900 */ \
  (ts).Month  = (t)->tm_mon + 1;     /* month of year (0 - 11) */ \
  (ts).Day    = (t)->tm_mday;    /* day of month (1 - 31) */ \
  (ts).Hour   = (t)->tm_hour;    /* hours (0 - 23) */ \
  (ts).Minute = (t)->tm_min;     /* minutes (0 - 59) */ \
  (ts).Second = (t)->tm_sec;     /* seconds (0 - 60) */

//
ASDCP::MXF::Timestamp::Timestamp() :
  Year(0), Month(0),  Day(0), Hour(0), Minute(0), Second(0)
{
  time_t t_now = time(0);
  struct tm*  now = gmtime(&t_now);
  TM_TO_TIMESTAMP(now, *this);
}

//
bool
ASDCP::MXF::Timestamp::operator<(const Timestamp& rhs) const
{
  struct tm lhtm, rhtm;
  TIMESTAMP_TO_TM(*this, &lhtm);
  TIMESTAMP_TO_TM(rhs, &rhtm);
  return ( timegm(&lhtm) < timegm(&rhtm) );
}

//
void
ASDCP::MXF::Timestamp::AddDays(i32_t days)
{
  struct tm current;

  if ( days != 0 )
    {
      TIMESTAMP_TO_TM(*this, &current);
      time_t adj_time = timegm(&current);
      adj_time += 86400 * days;
      struct tm*  now = gmtime(&adj_time);
      TM_TO_TIMESTAMP(now, *this);
    }
}

//
void
ASDCP::MXF::Timestamp::AddHours(i32_t hours)
{
  struct tm current;

  if ( hours != 0 )
    {
      TIMESTAMP_TO_TM(*this, &current);
      time_t adj_time = timegm(&current);
      adj_time += 3600 * hours;
      struct tm*  now = gmtime(&adj_time);
      TM_TO_TIMESTAMP(now, *this);
    }
}

#endif // WM_WIN32


ASDCP::MXF::Timestamp::Timestamp(const Timestamp& rhs)
{
  Year   = rhs.Year;
  Month  = rhs.Month;
  Day    = rhs.Day;
  Hour   = rhs.Hour;
  Minute = rhs.Minute;
  Second = rhs.Second;
}

ASDCP::MXF::Timestamp::~Timestamp()
{
}

//
const ASDCP::MXF::Timestamp&
ASDCP::MXF::Timestamp::operator=(const Timestamp& rhs)
{
  Year   = rhs.Year;
  Month  = rhs.Month;
  Day    = rhs.Day;
  Hour   = rhs.Hour;
  Minute = rhs.Minute;
  Second = rhs.Second;
  return *this;
}

//
bool
ASDCP::MXF::Timestamp::operator==(const Timestamp& rhs) const
{
  if ( Year == rhs.Year
       && Month  == rhs.Month
       && Day    == rhs.Day
       && Hour   == rhs.Hour
       && Minute == rhs.Minute
       && Second == rhs.Second )
    return true;

  return false;
}

//
bool
ASDCP::MXF::Timestamp::operator!=(const Timestamp& rhs) const
{
  if ( Year != rhs.Year
       || Month  != rhs.Month
       || Day    != rhs.Day
       || Hour   != rhs.Hour
       || Minute != rhs.Minute
       || Second != rhs.Second )
    return true;

  return false;
}

//
const char*
ASDCP::MXF::Timestamp::ToString(char* str_buf) const
{
  // 2004-05-01 13:20:00.000
  snprintf(str_buf, IntBufferLen,
           "%04hu-%02hu-%02hu %02hu:%02hu:%02hu.000",
           Year, Month, Day, Hour, Minute, Second, Tick);
  
  return str_buf;
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::TLVReader::TLVReader(const byte_t* p, ui32_t c, IPrimerLookup* PrimerLookup) :
  MemIOReader(p, c), m_Lookup(PrimerLookup)
{
  Result_t result = RESULT_OK;

  while ( Remainder() > 0 && ASDCP_SUCCESS(result) )
    {
      TagValue Tag;
      ui16_t pkt_len = 0;

      result = MemIOReader::ReadUi8(&Tag.a);

      if ( ASDCP_SUCCESS(result) )
	result = MemIOReader::ReadUi8(&Tag.b);

      if ( ASDCP_SUCCESS(result) )
	result = MemIOReader::ReadUi16BE(&pkt_len);

      if ( ASDCP_SUCCESS(result) )
	{
	  m_ElementMap.insert(TagMap::value_type(Tag, ItemInfo(m_size, pkt_len)));
	  result = SkipOffset(pkt_len);
	}
      
      if ( ASDCP_FAILURE(result) )
	{
	  DefaultLogSink().Error("Malformed Set\n");
	  m_ElementMap.clear();
	  break;
	}
    }
}

//
bool
ASDCP::MXF::TLVReader::FindTL(const MDDEntry& Entry)
{
  if ( m_Lookup == 0 )
    {
      DefaultLogSink().Error("No Lookup service\n");
      return false;
    }
  
  TagValue TmpTag;

  if ( m_Lookup->TagForKey(Entry.ul, TmpTag) != RESULT_OK )
    {
      if ( Entry.tag.a == 0 )
	{
	  DefaultLogSink().Info("No such UL in this TL list: %s (%02x %02x)\n",
				Entry.name, Entry.tag.a, Entry.tag.b);
	  return false;
	}

      TmpTag = Entry.tag;
    }

  TagMap::iterator e_i = m_ElementMap.find(TmpTag);

  if ( e_i != m_ElementMap.end() )
    {
      m_size = (*e_i).second.first;
      m_capacity = m_size + (*e_i).second.second;
      return true;
    }

  DefaultLogSink().Info("Not Found (%02x %02x): %s\n", TmpTag.a, TmpTag.b, Entry.name);
  return false;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadObject(const MDDEntry& Entry, IArchive* Object)
{
  ASDCP_TEST_NULL(Object);

  if ( FindTL(Entry) )
    {
      if ( m_size < m_capacity ) // don't try to unarchive an empty item
	return Object->Unarchive(*this);
    }

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi8(const MDDEntry& Entry, ui8_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi8(value);

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi16(const MDDEntry& Entry, ui16_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi16BE(value);

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi32(const MDDEntry& Entry, ui32_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi32BE(value);

  return RESULT_FALSE;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVReader::ReadUi64(const MDDEntry& Entry, ui64_t* value)
{
  ASDCP_TEST_NULL(value);

  if ( FindTL(Entry) )
    return MemIOReader::ReadUi64BE(value);

  return RESULT_FALSE;
}

//------------------------------------------------------------------------------------------
//

ASDCP::MXF::TLVWriter::TLVWriter(byte_t* p, ui32_t c, IPrimerLookup* PrimerLookup) :
  MemIOWriter(p, c), m_Lookup(PrimerLookup)
{
  assert(c > 3);
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteTag(const MDDEntry& Entry)
{
  if ( m_Lookup == 0 )
    {
      DefaultLogSink().Error("No Primer object available\n");
      return RESULT_FAIL;
    }

  TagValue TmpTag;

  if ( m_Lookup->InsertTag(Entry, TmpTag) != RESULT_OK )
    {
      DefaultLogSink().Error("No tag for entry %s\n", Entry.name);
      return RESULT_FAIL;
    }

  Result_t result = MemIOWriter::WriteUi8(TmpTag.a);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi8(TmpTag.b);

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteObject(const MDDEntry& Entry, IArchive* Object)
{
  ASDCP_TEST_NULL(Object);

  if ( Entry.optional && ! Object->HasValue() )
    return RESULT_OK;

  Result_t result = WriteTag(Entry);

  // write a temp length
  byte_t* l_p = CurrentData();

  if ( ASDCP_SUCCESS(result) )
    MemIOWriter::WriteUi16BE(0);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t before = Size();
      result = Object->Archive(*this);

      if ( ASDCP_SUCCESS(result) ) 
	i2p<ui16_t>(ASDCP_i16_BE( Size() - before), l_p);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi8(const MDDEntry& Entry, ui8_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(sizeof(ui8_t));
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi8(*value);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi16(const MDDEntry& Entry, ui16_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(sizeof(ui16_t));
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(*value);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi32(const MDDEntry& Entry, ui32_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(sizeof(ui32_t));
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi32BE(*value);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi64(const MDDEntry& Entry, ui64_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(sizeof(ui64_t));
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi64BE(*value);
  return result;
}


//----------------------------------------------------------------------------------------------------
//

ASDCP::MXF::Raw::Raw()
{
  Capacity(256);
}

ASDCP::MXF::Raw::~Raw()
{
}

//
ASDCP::Result_t
ASDCP::MXF::Raw::Unarchive(ASDCP::MemIOReader& Reader)
{
  ui32_t payload_size = Reader.Remainder();

  if ( payload_size == 0 )
    return RESULT_OK;

  Result_t result = Capacity(payload_size);

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(Data(), Reader.CurrentData(), payload_size);
      Size(payload_size);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::Raw::Archive(ASDCP::MemIOWriter& Writer) const
{
  return Writer.WriteRaw(RoData(), Size());
}

//
const char*
ASDCP::MXF::Raw::ToString(char* str_buf) const
{
  *str_buf = 0;
  bin2hex(RoData(), Size(), str_buf, IdentBufferLen);
  snprintf(str_buf, IdentBufferLen, "%s\n", str_buf);
  return str_buf;
}

//
// end MXFTypes.cpp
//
