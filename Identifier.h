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
/*! \file    Identifier.h
    \version $Id$
    \brief   strings of bytes that identify things
*/

#ifndef _IDENTIFIER_H_
#define _IDENTIFIER_H_

namespace ASDCP
{
  // the base of all identifier classes
  template <ui32_t SIZE>
    class Identifier : public IArchive
    {
    protected:
      byte_t m_Value[SIZE];
      bool   m_HasValue;

    public:
      Identifier() : m_HasValue(false) {
	memset(m_Value, 0, SIZE);
      }

      //
      inline Result_t Set(const byte_t* value) {
	ASDCP_TEST_NULL(value);
	memcpy(m_Value, value, SIZE);
	m_HasValue = true;
	return RESULT_OK;
      }

      //
      inline Identifier& operator=(const Identifier& rhs)
      {
	Set(rhs.m_Value);
	return *this;
      }

      //
      inline Result_t Unarchive(ASDCP::MemIOReader& Reader) {
	Result_t result = Reader.ReadRaw(m_Value, SIZE);
	if ( ASDCP_SUCCESS(result) ) m_HasValue = true;
	return result;
      }

      inline bool HasValue() const { return m_HasValue; }

      //
      inline Result_t Archive(ASDCP::MemIOWriter& Writer) const {
	return Writer.WriteRaw(m_Value, SIZE);
      }

      inline const byte_t* Value() const { return m_Value; }

      inline ui32_t Size() const { return SIZE; }

      //
      inline bool operator<(const Identifier& rhs) const
	{
	  for ( ui32_t i = 0; i < SIZE; i++ )
	    {
	      if ( m_Value[i] != rhs.m_Value[i] )
		return m_Value[i] < rhs.m_Value[i];
	    }

	  return false;
	}

      //
      inline bool operator==(const Identifier& rhs) const
      {
	if ( rhs.Size() != SIZE )
	  return false;

	return ( memcmp(m_Value, rhs.m_Value, SIZE) == 0 );
      }

      //
      // todo: refactor characrer insertion back to bin2hex()
      const char* ToString(char* str_buf) const
	{
	  char* p = str_buf;

	  for ( ui32_t i = 0; i < SIZE; i++ )
	    {
	      *p = (m_Value[i] >> 4) & 0x0f;
	      *p += *p < 10 ? 0x30 : 0x61 - 10;
	      p++;

	      *p = m_Value[i] & 0x0f;
	      *p += *p < 10 ? 0x30 : 0x61 - 10;
	      p++;

	      *p = ' ';
	      p++;
	    }

	  *p = 0;
	  return str_buf;
	}
    };


  class UL;
  class UUID;
  class UMID;

  // Universal Label
  class UL : public Identifier<SMPTE_UL_LENGTH>
    {
    public:
      UL() {}
      UL(const byte_t* rhs) {
	assert(rhs);
	Set(rhs);
      }

      UL(const UL& rhs) {
	Set(rhs.m_Value);
      }

      bool operator==(const UL& rhs) const {
	return ( memcmp(m_Value, rhs.m_Value, SMPTE_UL_LENGTH) == 0 ) ? true : false;
      }
    };

  // UUID
  class UUID : public Identifier<UUIDlen>
    {
    public:
      UUID() {}
      UUID(const byte_t* rhs) {
	assert(rhs);
	Set(rhs);
      }

      UUID(const UUID& rhs) {
	Set(rhs.m_Value);
      }

      void GenRandomValue();
    };

  // UMID
  class UMID : public Identifier<SMPTE_UMID_LENGTH>
    {
    public:
      UMID() {}
      UMID(const UMID &rhs) {
	Set(rhs.m_Value);
      };

      void MakeUMID(int Type);
      void MakeUMID(int Type, const UUID& ID);
      const char* ToString(char* str_buf) const;
    };

} // namespace mxflib

#endif // _IDENTIFIER_H_

//
// end Identifier.h
//
