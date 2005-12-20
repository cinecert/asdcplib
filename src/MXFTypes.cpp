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

//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::MXF::UTF16String::ReadFrom(ASDCP::MemIOReader& Reader)
{
  const byte_t* p = Reader.Data() + Reader.Offset();
  /// cheating - for all use cases, we know the previous two bytes are the length
  m_length = ASDCP_i16_BE(cp2i<ui16_t>(p-2));
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
ASDCP::MXF::UTF16String::WriteTo(ASDCP::MemIOWriter& Writer)
{
  byte_t* p = Writer.Data() + Writer.Size();
  ui32_t i = 0;
  m_length = strlen(m_buffer);
  memset(p, 0, m_length*2);

  for ( i = 0; i < m_length; i++ )
    p[(i*2)+1] = m_buffer[i];

  m_buffer[i] = 0;

  Writer.AddOffset(m_length*2);
  return RESULT_OK;
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
      fprintf(stderr, "No Lookup service\n");
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
    return Object->ReadFrom(*this);

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
  TagValue TmpTag;

  if ( m_Lookup == 0 )
    {
      DefaultLogSink().Error("No Primer object available\n");
      return RESULT_FAIL;
    }

  if ( m_Lookup->InsertTag(Entry.ul, TmpTag) != RESULT_OK )
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
  Result_t result = WriteTag(Entry);

  // write a temp length
  byte_t* l_p = CurrentData();

  if ( ASDCP_SUCCESS(result) )
    MemIOWriter::WriteUi16BE(0);

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t before = Size();
      result = Object->WriteTo(*this);

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
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi8(*value);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi32(const MDDEntry& Entry, ui32_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(sizeof(ui32_t));
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi8(*value);
  return result;
}

//
ASDCP::Result_t
ASDCP::MXF::TLVWriter::WriteUi64(const MDDEntry& Entry, ui64_t* value)
{
  ASDCP_TEST_NULL(value);
  Result_t result = WriteTag(Entry);
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi16BE(sizeof(ui64_t));
  if ( ASDCP_SUCCESS(result) ) MemIOWriter::WriteUi8(*value);
  return result;
}

//
// end 
//
