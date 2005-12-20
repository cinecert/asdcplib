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
/*! \file    DirScanner.cpp
    \version $Id$
    \brief   Cross-platform, simple non-recursive directory scanner
*/

#include <DirScanner.h>
#include <errno.h>
#include <assert.h>

// Win32 directory scanner
//
#ifdef WIN32

//
//
ASDCP::Result_t
DirScanner::Open(const char* filename)
{
  ASDCP_TEST_NULL_STR(filename);

  // we need to append a '*' to read the entire directory
  ui32_t fn_len = strlen(filename); 
  char* tmp_file = (char*)malloc(fn_len + 8);

  if ( tmp_file == 0 )
    return ASDCP::RESULT_ALLOC;

  strcpy(tmp_file, filename);
  char* p = &tmp_file[fn_len] - 1;

  if ( *p != '/' && *p != '\\' )
    {
      p++;
      *p++ = '/';
    }

  *p++ = '*';
  *p = 0;
  // whew...

  m_Handle = _findfirsti64(tmp_file, &m_FileInfo);
  ASDCP::Result_t result = ASDCP::RESULT_OK;

  if ( m_Handle == -1 )
    result = ASDCP::RESULT_NOT_FOUND;

  return result;
}


//
//
ASDCP::Result_t
DirScanner::Close()
{
  if ( m_Handle == -1 )
    return ASDCP::RESULT_FILEOPEN;

  if ( _findclose((long)m_Handle) == -1 )
    return ASDCP::RESULT_FAIL;

  m_Handle = -1;
  return ASDCP::RESULT_OK;
}


// This sets filename param to the same per-instance buffer every time, so
// the value will change on the next call
ASDCP::Result_t
DirScanner::GetNext(char* filename)
{
  ASDCP_TEST_NULL(filename);

  if ( m_Handle == -1 )
    return ASDCP::RESULT_FILEOPEN;

  if ( m_FileInfo.name[0] == '\0' )
    return ASDCP::RESULT_ENDOFFILE;

  strncpy(filename, m_FileInfo.name, ASDCP_MAX_PATH);
  ASDCP::Result_t result = ASDCP::RESULT_OK;

  if ( _findnexti64((long)m_Handle, &m_FileInfo) == -1 )
    {
      m_FileInfo.name[0] = '\0';
	  
      if ( errno != ENOENT )
	result = ASDCP::RESULT_FAIL;
    }

  return result;
}


#else // WIN32

// POSIX directory scanner

//
ASDCP::Result_t
DirScanner::Open(const char* filename)
{
  ASDCP_TEST_NULL_STR(filename);

  ASDCP::Result_t result = ASDCP::RESULT_OK;

  if ( ( m_Handle = opendir(filename) ) == NULL )
    {
      if ( errno == ENOENT )
	result = ASDCP::RESULT_ENDOFFILE;

      else
	result = ASDCP::RESULT_FAIL;
    }

  return result;
}


//
ASDCP::Result_t
DirScanner::Close()
{
  if ( m_Handle == NULL )
    return ASDCP::RESULT_FILEOPEN;

  if ( closedir(m_Handle) == -1 )
    return ASDCP::RESULT_FAIL;

  m_Handle = NULL;
  return ASDCP::RESULT_OK;
}


//
ASDCP::Result_t
DirScanner::GetNext(char* filename)
{
  ASDCP_TEST_NULL(filename);

  if ( m_Handle == NULL )
    return ASDCP::RESULT_FILEOPEN;

  struct dirent* entry;

  for (;;)
    {
      if ( ( entry = readdir(m_Handle)) == NULL )
	return ASDCP::RESULT_ENDOFFILE;

      break;
    }

  strncpy(filename, entry->d_name, ASDCP_MAX_PATH);
  return ASDCP::RESULT_OK;
}


#endif // WIN32

//
// end DirScanner.cpp
//
