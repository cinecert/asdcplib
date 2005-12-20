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
/*! \file    KLV.cpp
  \version $Id$
  \brief   KLV objects
*/

#include "KLV.h"
#include <hex_utils.h>


// This is how much we read when we're reading from a file and we don't know
// how long the packet is. This gives us the K (16 bytes) and L (4-9 bytes)
// and the remaining bytes for an even read (tmp_read_size % 16 == 0)
const ui32_t kl_length = ASDCP::SMPTE_UL_LENGTH + ASDCP::MXF_BER_LENGTH;
const ui32_t tmp_read_size = 32;

//------------------------------------------------------------------------------------------
//

// 
ASDCP::Result_t
ASDCP::KLVPacket::InitFromBuffer(const byte_t* buf, ui32_t buf_len, const byte_t* label)
{
  Result_t result = KLVPacket::InitFromBuffer(buf, buf_len);

  if ( ASDCP_SUCCESS(result) )
    result = ( memcmp(m_KeyStart, label, SMPTE_UL_LENGTH) == 0 ) ?
      RESULT_OK : RESULT_FAIL;

  return result;
}

//
ASDCP::Result_t
ASDCP::KLVPacket::InitFromBuffer(const byte_t* buf, ui32_t buf_len)
{
  m_KeyStart = m_ValueStart = 0;
  m_KLLength = m_ValueLength = 0;

  if ( memcmp(buf, SMPTE_UL_START, 4) != 0 )
    {
      DefaultLogSink().Error("Unexpected UL preamble: %02x.%02x.%02x.%02x\n",
			     buf[0], buf[1], buf[2], buf[3]);
      return RESULT_FAIL;
    }

  ui64_t tmp_size;
  if ( ! read_BER(buf + SMPTE_UL_LENGTH, &tmp_size) )
       return RESULT_FAIL;

  m_ValueLength = tmp_size;
  m_KLLength = SMPTE_UL_LENGTH + BER_length(buf + SMPTE_UL_LENGTH);
  m_KeyStart = buf;
  m_ValueStart = buf + m_KLLength;
  return RESULT_OK;
}

//
bool
ASDCP::KLVPacket::HasUL(const byte_t* ul)
{
  if ( m_KeyStart == 0 )
    return false;

  return ( memcmp(ul, m_KeyStart, SMPTE_UL_LENGTH) == 0 ) ? true : false;
}

//
ASDCP::Result_t
ASDCP::KLVPacket::WriteKLToBuffer(ASDCP::FrameBuffer& Buffer, const byte_t* label, ui32_t length)
{
  if ( Buffer.Size() + kl_length > Buffer.Capacity() )
    {
      DefaultLogSink().Error("Small write buffer\n");
      return RESULT_FAIL;
    }
  
  memcpy(Buffer.Data() + Buffer.Size(), label, SMPTE_UL_LENGTH);

  if ( ! write_BER(Buffer.Data() + Buffer.Size() + SMPTE_UL_LENGTH, length, MXF_BER_LENGTH) )
    return RESULT_FAIL;

  Buffer.Size(Buffer.Size() + kl_length);
  return RESULT_OK;
}

//
void
ASDCP::KLVPacket::Dump(FILE* stream, bool show_hex)
{
  if ( stream == 0 )
    stream = stderr;

  if ( m_KeyStart != 0 )
    {
      assert(m_ValueStart);

      for ( ui32_t i = 0; i < SMPTE_UL_LENGTH; i++ )
	fprintf(stream, "%02x.", m_KeyStart[i]);

      const MDDEntry* Entry = GetMDDEntry(m_KeyStart);
      fprintf(stream, "\b  len: %7lu (%s)\n", m_ValueLength, (Entry ? Entry->name : "Unknown"));

      if ( show_hex && m_ValueLength < 1000 )
	hexdump(m_ValueStart, ASDCP::xmin(m_ValueLength, (ui32_t)64), stream);
    }
  else
    {
      fprintf(stream, "*** Malformed packet ***\n");
    }
}

// 
ASDCP::Result_t
ASDCP::KLVFilePacket::InitFromFile(const FileReader& Reader, const byte_t* label)
{
  Result_t result = KLVFilePacket::InitFromFile(Reader);

  if ( ASDCP_SUCCESS(result) )
    result = ( memcmp(m_KeyStart, label, SMPTE_UL_LENGTH) == 0 ) ?
      RESULT_OK : RESULT_FAIL;

  return result;
}

//
ASDCP::Result_t
ASDCP::KLVFilePacket::InitFromFile(const FileReader& Reader)
{
  ui32_t read_count;
  byte_t tmp_data[tmp_read_size];
  ui64_t tmp_size;
  m_KeyStart = m_ValueStart = 0;
  m_KLLength = m_ValueLength = 0;
  m_Buffer.Size(0);

  Result_t result = Reader.Read(tmp_data, tmp_read_size, &read_count);

  if ( ASDCP_FAILURE(result) )
    return result;

  if ( read_count < (SMPTE_UL_LENGTH + 1) )
    {
      DefaultLogSink().Error("Short read of Key and Length got %lu\n", read_count);
      return RESULT_READFAIL;
    }

  if ( memcmp(tmp_data, SMPTE_UL_START, 4) != 0 )
    {
      DefaultLogSink().Error("Unexpected UL preamble: %02x.%02x.%02x.%02x\n",
			     tmp_data[0], tmp_data[1], tmp_data[2], tmp_data[3]);
      return RESULT_FAIL;
    }

  if ( ! read_BER(tmp_data + SMPTE_UL_LENGTH, &tmp_size) )
    {
      DefaultLogSink().Error("BER Length decoding error\n");
      return RESULT_FAIL;
    }

  if ( tmp_size > MAX_KLV_PACKET_LENGTH )
    {
      char intbuf[IntBufferLen];
      DefaultLogSink().Error("Packet length %s exceeds internal limit\n",
			     ui64sz(tmp_size, intbuf));
      return RESULT_FAIL;
    }

  ui32_t remainder = 0;
  ui32_t ber_len = BER_length(tmp_data + SMPTE_UL_LENGTH);
  m_KLLength = SMPTE_UL_LENGTH + ber_len;
  m_ValueLength = tmp_size;
  ui32_t packet_length = m_ValueLength + m_KLLength;

  result = m_Buffer.Capacity(packet_length);

  if ( ASDCP_FAILURE(result) )
    return result;

  m_KeyStart = m_Buffer.Data();
  m_ValueStart = m_Buffer.Data() + m_KLLength;
  m_Buffer.Size(packet_length);

  // is the whole packet in the tmp buf?
  if ( packet_length <= tmp_read_size )
    {
      assert(packet_length <= read_count);
      memcpy(m_Buffer.Data(), tmp_data, packet_length);

      if ( (remainder = read_count - packet_length) != 0 )
	{
	  DefaultLogSink().Warn("Repositioning pointer for short packet\n");
	  ASDCP::fpos_t pos = Reader.Tell();
	  assert(pos > remainder);
	  result = Reader.Seek(pos - remainder);
	}
    }
  else
    {
      if ( read_count < tmp_read_size )
	{
	  DefaultLogSink().Error("Short read of packet body, expecting %lu, got %lu\n",
				 m_Buffer.Size(), read_count);
	  return RESULT_READFAIL;
	}

      memcpy(m_Buffer.Data(), tmp_data, tmp_read_size);
      remainder = m_Buffer.Size() - tmp_read_size;

      if ( remainder > 0 )
	{
	  result = Reader.Read(m_Buffer.Data() + tmp_read_size, remainder, &read_count);
      
	  if ( read_count != remainder )
	    {
	      DefaultLogSink().Error("Short read of packet body, expecting %lu, got %lu\n",
				     remainder+tmp_read_size, read_count+tmp_read_size);
	      result = RESULT_READFAIL;
	    }
	}
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::KLVFilePacket::WriteKLToFile(FileWriter& Writer, const byte_t* label, ui32_t length)
{
  byte_t buffer[kl_length];
  memcpy(buffer, label, SMPTE_UL_LENGTH);

  if ( ! write_BER(buffer+SMPTE_UL_LENGTH, length, MXF_BER_LENGTH) )
    return RESULT_FAIL;

  ui32_t write_count;
  Writer.Write(buffer, kl_length, &write_count);
  assert(write_count == kl_length);
  return RESULT_OK;
}


//
// end KLV.cpp
//
