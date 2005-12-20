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
/*! \file    MemIO.h
    \version $Id$
    \brief   Interface for reading and writing typed objects to a byte-oriented buffer
*/

#ifndef _MEMIO_H_
#define _MEMIO_H_

#include <AS_DCP_system.h>
#include <hex_utils.h>

namespace ASDCP
{
  //
  class MemIOWriter
    {
      ASDCP_NO_COPY_CONSTRUCT(MemIOWriter);
      MemIOWriter();
      
    protected:
      byte_t* m_p;
      ui32_t  m_capacity;
      ui32_t  m_size;

    public:
      MemIOWriter(byte_t* p, ui32_t c) :
	m_p(p), m_capacity(c), m_size(0) {
	assert(m_p); assert(m_capacity);
      }

      ~MemIOWriter() {}

      inline byte_t* Data() { return m_p; }
      inline byte_t* CurrentData() { return m_p + m_size; }
      inline ui32_t Size() { return m_size; }
      inline ui32_t Remainder() { return m_capacity - m_size; }

      inline Result_t AddOffset(ui32_t offset) {
	if ( ( m_size + offset ) > m_capacity )
	  return RESULT_FAIL;

	m_size += offset;
	return RESULT_OK;

      }

      inline Result_t WriteRaw(const byte_t* p, ui32_t buf_len) {
	if ( ( m_size + buf_len ) > m_capacity )
	  return RESULT_FAIL;

	memcpy(m_p + m_size, p, buf_len);
	m_size += buf_len;
	return RESULT_OK;
      }

      inline Result_t WriteBER(ui64_t i, ui32_t ber_len) {
	if ( ( m_size + ber_len ) > m_capacity )
	  return RESULT_FAIL;

	if ( ! write_BER(m_p + m_size, i, ber_len) )
	  return RESULT_FAIL;

	m_size += ber_len;
	return RESULT_OK;
      }

      inline Result_t WriteUi8(ui8_t i) {
	if ( ( m_size + 1 ) > m_capacity )
	  return RESULT_FAIL;

	*(m_p + m_size) = i;
	m_size++;
	return RESULT_OK;
      }

      inline Result_t WriteUi16BE(ui16_t i) {
	if ( ( m_size + sizeof(ui16_t) ) > m_capacity )
	  return RESULT_FAIL;
	
	i2p<ui16_t>(ASDCP_i16_BE(i), m_p + m_size);
	m_size += sizeof(ui16_t);
	return RESULT_OK;
      }

      inline Result_t WriteUi32BE(ui32_t i) {
	if ( ( m_size + sizeof(ui32_t) ) > m_capacity )
	  return RESULT_FAIL;
	
	i2p<ui32_t>(ASDCP_i32_BE(i), m_p + m_size);
	m_size += sizeof(ui32_t);
	return RESULT_OK;
      }

      inline Result_t WriteUi64BE(ui64_t i) {
	if ( ( m_size + sizeof(ui64_t) ) > m_capacity )
	  return RESULT_FAIL;
	
	i2p<ui64_t>(ASDCP_i64_BE(i), m_p + m_size);
	m_size += sizeof(ui64_t);
	return RESULT_OK;
      }
    };

  //
  class MemIOReader
    {
      ASDCP_NO_COPY_CONSTRUCT(MemIOReader);
      MemIOReader();
      
    protected:
      const byte_t* m_p;
      ui32_t  m_capacity;
      ui32_t  m_size; // this is sort of a misnomer, when we are reading it measures offset

    public:
      MemIOReader(const byte_t* p, ui32_t c) :
	m_p(p), m_capacity(c), m_size(0) {
	assert(m_p); assert(m_capacity);
      }

      ~MemIOReader() {}

      inline const byte_t* Data() { return m_p; }
      inline const byte_t* CurrentData() { return m_p + m_size; }
      inline ui32_t Offset() { return m_size; }
      inline ui32_t Remainder() { return m_capacity - m_size; }

      inline Result_t SkipOffset(ui32_t offset) {
	if ( ( m_size + offset ) > m_capacity )
	  return RESULT_FAIL;

	m_size += offset;
	return RESULT_OK;
      }

      inline Result_t ReadRaw(byte_t* p, ui32_t buf_len) {
	if ( ( m_size + buf_len ) > m_capacity )
	  return RESULT_FAIL;

	memcpy(p, m_p + m_size, buf_len);
	m_size += buf_len;
	return RESULT_OK;
      }

      Result_t ReadBER(ui64_t* i, ui32_t* ber_len) {
	ASDCP_TEST_NULL(i);
	ASDCP_TEST_NULL(ber_len);

	if ( ( *ber_len = BER_length(m_p + m_size) ) == 0 )
	  return RESULT_FAIL;

	if ( ( m_size + *ber_len ) > m_capacity )
	  return RESULT_FAIL;

	if ( ! read_BER(m_p + m_size, i) )
	  return RESULT_FAIL;

	m_size += *ber_len;
	return RESULT_OK;
      }

      inline Result_t ReadUi8(ui8_t* i) {
	ASDCP_TEST_NULL(i);
	if ( ( m_size + 1 ) > m_capacity )
	  return RESULT_FAIL;

	*i = *(m_p + m_size);
	m_size++;
	return RESULT_OK;
      }

      inline Result_t ReadUi16BE(ui16_t* i) {
	ASDCP_TEST_NULL(i);
	if ( ( m_size + sizeof(ui16_t) ) > m_capacity )
	  return RESULT_FAIL;

	*i = ASDCP_i16_BE(cp2i<ui16_t>(m_p + m_size));
	m_size += sizeof(ui16_t);
	return RESULT_OK;
      }

      inline Result_t ReadUi32BE(ui32_t* i) {
	ASDCP_TEST_NULL(i);
	if ( ( m_size + sizeof(ui32_t) ) > m_capacity )
	  return RESULT_FAIL;

	*i = ASDCP_i32_BE(cp2i<ui32_t>(m_p + m_size));
	m_size += sizeof(ui32_t);
	return RESULT_OK;
      }

      inline Result_t ReadUi64BE(ui64_t* i) {
	ASDCP_TEST_NULL(i);
	if ( ( m_size + sizeof(ui64_t) ) > m_capacity )
	  return RESULT_FAIL;

	*i = ASDCP_i64_BE(cp2i<ui64_t>(m_p + m_size));
	m_size += sizeof(ui64_t);
	return RESULT_OK;
      }
    };


} // namespace ASDCP

#endif // _MEMIO_H_

//
// end MemIO.h
//
