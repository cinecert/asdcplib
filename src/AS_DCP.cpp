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
/*! \file    AS_DCP.cpp
    \version $Id$       
    \brief   AS-DCP library, misc classes and subroutines
*/

#include <AS_DCP_system.h>
#include "hex_utils.h"
#include <assert.h>


static const ui32_t s_MessageCount = 27;
static const char* s_ErrorMessages[] =
{
  "An undefined error was detected.",
  "An unexpected NULL pointer was given.",
  "An unexpected empty string was given.",
  "The given frame buffer is too small.",
  "The object is not yet initialized.",

  "The requested file does not exist on the system.",
  "Insufficient privilege exists to perform the operation.",
  "File open error.",
  "The file contains errors or is not OP-Atom/AS-DCP.",
  "An invalid file location was requested.",

  "File read error.",
  "File write error.",
  "Unknown raw essence file type.",
  "Raw essence format invalid.",
  "Object state error.",

  "Attempt to read past end of file.",
  "Invalid configuration option detected.",
  "Frame number out of range.",
  "AESEncContext required when writing to encrypted file",
  "Plaintext offset exceeds frame buffer size",

  "Error allocating memory",
  "Cannot resize externally allocated memory",
  "The check value did not decrypt correctly",
  "HMAC authentication failure",
  "HMAC context required",

  "Error initializing block cipher context",
  "Attempted to write an empty frame buffer"
};


//------------------------------------------------------------------------------------------

//
class StderrLogSink : public ASDCP::ILogSink
{
public:
  bool show_info;
  bool show_debug;

  StderrLogSink() : show_info(false), show_debug(false) {}
  ~StderrLogSink() {}

  void Error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vLogf(LOG_ERROR, fmt, &args);
    va_end(args);
  }

  void Warn(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vLogf(LOG_WARN, fmt, &args);
    va_end(args);
  }

  void Info(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vLogf(LOG_INFO, fmt, &args);
    va_end(args);
  }

  void Debug(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vLogf(LOG_DEBUG, fmt, &args);
    va_end(args);
  }

  void Logf(ASDCP::ILogSink::LogType_t type, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vLogf(type, fmt, &args);
    va_end(args);
  }

  void vLogf(ASDCP::ILogSink::LogType_t type, const char* fmt, va_list* list) {
    FILE* stream = stderr;

    switch ( type )
      {
      case LOG_ERROR: fputs("Error: ", stream); break;
      case LOG_WARN:  fputs("Warning: ", stream); break;
      case LOG_INFO:
	if ( ! show_info ) return;
	fputs("Info: ", stream);
	break;
      case LOG_DEBUG:
	if ( ! show_debug ) return;
	fputs("Debug: ", stream);
	break;
      }
    
    vfprintf(stream, fmt, *list);
  }

} s_StderrLogSink;

//
static ASDCP::ILogSink* s_DefaultLogSink = 0;

//
void
ASDCP::SetDefaultLogSink(ILogSink* Sink)
{
    s_DefaultLogSink = Sink;
}

// bootleg entry for debug enthusiasts
void
set_debug_mode(bool info_mode, bool debug_mode)
{
  s_StderrLogSink.show_info = info_mode;
  s_StderrLogSink.show_debug = debug_mode;
}

// Returns the internal default sink.
ASDCP::ILogSink&
ASDCP::DefaultLogSink()
{
  if ( s_DefaultLogSink == 0 )
    s_DefaultLogSink = &s_StderrLogSink;

  return *s_DefaultLogSink;
}

const char*
ASDCP::Version()
{
  static char ver[16];
  sprintf(ver, "%lu.%lu.%lu", VERSION_MAJOR, VERSION_APIMINOR, VERSION_IMPMINOR);
  return ver;
}


// Returns a pointer to an English language string describing the given result code.
// If the result code is not a valid member of the Result_t enum, the string
// "**UNKNOWN**" will be returned.
const char*
ASDCP::GetResultString(Result_t result)
{
  if ( result >= 0 )
    return "No error.";

  ui32_t idx = (- result);

  if ( idx > s_MessageCount )
    return "**UNKNOWN**";

  return s_ErrorMessages[--idx];
}


// convert utf-8 hext string to bin
i32_t
ASDCP::hex2bin(const char* str, byte_t* buf, ui32_t buf_len, ui32_t* conv_size)
{
  ASDCP_TEST_NULL(str);
  ASDCP_TEST_NULL(buf);
  ASDCP_TEST_NULL(conv_size);

  *conv_size = 0;

  if ( str[0] == 0 ) // nothing to convert
    return 0;

  for ( int j = 0; str[j]; j++ )
    {
      if ( isxdigit(str[j]) )
	(*conv_size)++;
    }

  if ( *conv_size & 0x01 ) (*conv_size)++;
  *conv_size /= 2;

  if ( *conv_size > buf_len )// maximum possible data size
    return -1;

  *conv_size = 0;

  int phase = 0; // track high/low nybble

  // for each character, fill in the high nybble then the low
  for ( int i = 0; str[i]; i++ )
    {
      if ( ! isxdigit(str[i]) )
        continue;

      byte_t val = str[i] - ( isdigit(str[i]) ? 0x30 : ( isupper(str[i]) ? 0x37 : 0x57 ) );

      if ( phase == 0 )
        {
          buf[*conv_size] = val << 4;
          phase++;
        }
      else
        {
          buf[*conv_size] |= val;
          phase = 0;
          (*conv_size)++;
        }
    }

  return 0;
}


// convert a memory region to a NULL-terminated hexadecimal string
//
const char*
ASDCP::bin2hex(const byte_t* bin_buf, ui32_t bin_len, char* str_buf, ui32_t str_len)
{
  if ( bin_buf == 0
       || str_buf == 0
       || ((bin_len * 2) + 1) > str_len )
    return 0;

  char* p = str_buf;

  for ( ui32_t i = 0; i < bin_len; i++ )
    {
      *p = (bin_buf[i] >> 4) & 0x0f;
      *p += *p < 10 ? 0x30 : 0x61 - 10;
      p++;

      *p = bin_buf[i] & 0x0f;
      *p += *p < 10 ? 0x30 : 0x61 - 10;
      p++;
    }

  *p = '\0';
  return str_buf;
}


// spew a range of bin data as hex
void
ASDCP::hexdump(const byte_t* buf, ui32_t dump_len, FILE* stream)
{
  if ( buf == 0 )
    return;

  if ( stream == 0 )
    stream = stderr;

  static ui32_t row_len = 16;
  const byte_t* p = buf;
  const byte_t* end_p = p + dump_len;

  for ( ui32_t line = 0; p < end_p; line++ )
    {
      fprintf(stream, "  %06x: ", line);
      ui32_t i;
      const byte_t* pp;

      for ( pp = p, i = 0; i < row_len && pp < end_p; i++, pp++ )
	fprintf(stream, "%02x ", *pp);

      while ( i++ < row_len )
	fputs("   ", stream);

      for ( pp = p, i = 0; i < row_len && pp < end_p; i++, pp++ )
	fputc((isprint(*pp) ? *pp : '.'), stream);

      fputc('\n', stream);
      p += row_len;
    }
}

//------------------------------------------------------------------------------------------

// read a ber value from the buffer and compare with test value.
// Advances buffer to first character after BER value.
//
bool
ASDCP::read_test_BER(byte_t **buf, ui64_t test_value)
{
  if ( buf == 0 )
    return false;

  if ( ( **buf & 0x80 ) == 0 )
    return false;

  ui64_t val = 0;
  ui8_t ber_size = ( **buf & 0x0f ) + 1;

  if ( ber_size > 9 )
    return false;

  for ( ui8_t i = 1; i < ber_size; i++ )
    {
      if ( (*buf)[i] > 0 )
        val |= (ui64_t)((*buf)[i]) << ( ( ( ber_size - 1 ) - i ) * 8 );
    }

  *buf += ber_size;
  return ( val == test_value );
}


//
bool
ASDCP::read_BER(const byte_t* buf, ui64_t* val)
{
  ui8_t ber_size, i;
  
  if ( buf == 0 || val == 0 )
    return false;

  if ( ( *buf & 0x80 ) == 0 )
    return false;

  *val = 0;
  ber_size = ( *buf & 0x0f ) + 1;

  if ( ber_size > 9 )
    return false;

  for ( i = 1; i < ber_size; i++ )
    {
      if ( buf[i] > 0 )
	*val |= (ui64_t)buf[i] << ( ( ( ber_size - 1 ) - i ) * 8 );
    }

  return true;
}


static const ui64_t ber_masks[9] =
  { ui64_C(0xffffffffffffffff), ui64_C(0xffffffffffffff00), 
    ui64_C(0xffffffffffff0000), ui64_C(0xffffffffff000000),
    ui64_C(0xffffffff00000000), ui64_C(0xffffff0000000000),
    ui64_C(0xffff000000000000), ui64_C(0xff00000000000000),
    0
  };


//
bool
ASDCP::write_BER(byte_t* buf, ui64_t val, ui32_t ber_len)
{
  if ( buf == 0 )
    return false;

  if ( ber_len == 0 )
    { // calculate default length
      if ( val < 0x01000000L )
        ber_len = 4;
      else if ( val < ui64_C(0x0100000000000000) )
        ber_len = 8;
      else
        ber_len = 9;
    }
  else
    { // sanity check BER length
      if ( ber_len > 9 )
        {
          DefaultLogSink().Error("BER size %lu exceeds maximum size of 9\n", ber_len);
          return false;
        }
      
      if ( val & ber_masks[ber_len - 1] )
        {
	  char intbuf[IntBufferLen];
          DefaultLogSink().Error("BER size %lu too small for value %s\n",
				 ber_len, ui64sz(val, intbuf));
          return false;
        }
    }

  buf[0] = 0x80 + ( ber_len - 1 );

  for ( ui32_t i = ber_len - 1; i > 0; i-- )
    {
      buf[i] = (ui8_t)(val & 0xff);
      val >>= 8;
    }

  return true;
}

//------------------------------------------------------------------------------------------
//
// frame buffer base class implementation

ASDCP::FrameBuffer::FrameBuffer() :
  m_Data(0), m_Capacity(0), m_OwnMem(false), m_Size(0),
  m_FrameNumber(0), m_SourceLength(0), m_PlaintextOffset(0)
{
}

ASDCP::FrameBuffer::~FrameBuffer()
{
  if ( m_OwnMem && m_Data != 0 )
    free(m_Data);
}

// Instructs the object to use an externally allocated buffer. The external
// buffer will not be cleaned up by the frame buffer when it is destroyed.
// Call with (0,0) to revert to internally allocated buffer.
// Returns error if the buf_addr argument is NULL and either buf_size is
// non-zero or internally allocated memory is in use.
ASDCP::Result_t
ASDCP::FrameBuffer::SetData(byte_t* buf_addr, ui32_t buf_size)
{
  // if buf_addr is null and we have an external memory reference,
  // drop the reference and place the object in the initialized-
  // but-no-buffer-allocated state
  if ( buf_addr == 0 )
    {
      if ( buf_size > 0 || m_OwnMem )
	return RESULT_PTR;

      m_OwnMem = false;
      m_Capacity = m_Size = 0;
      m_Data = 0;
      return RESULT_OK;
    }

  if ( m_OwnMem && m_Data != 0 )
    free(m_Data);

  m_OwnMem = false;
  m_Capacity = buf_size;
  m_Data = buf_addr;
  m_Size = 0;

  return RESULT_OK;
}

// Sets the size of the internally allocate buffer. Returns RESULT_CAPEXTMEM
// if the object is using an externally allocated buffer via SetData();
// Resets content size to zero.
ASDCP::Result_t
ASDCP::FrameBuffer::Capacity(ui32_t cap_size)
{
  if ( ! m_OwnMem && m_Data != 0 )
    return RESULT_CAPEXTMEM; // cannot resize external memory

  if ( m_Capacity < cap_size )
    {
      if ( m_Data != 0 )
	{
	  assert(m_OwnMem);
	  free(m_Data);
	}

      m_Data = (byte_t*)malloc(cap_size);

      if ( m_Data == 0 )
	return RESULT_ALLOC;

      m_Capacity = cap_size;
      m_OwnMem = true;
      m_Size = 0;
    }

  return RESULT_OK;
}


//
// end AS_DCP.cpp
//
