/*
Copyright (c) 2003-2005, John Hurst
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
/*! \file    FileIO.cpp
    \version $Id$
    \brief   Cross-platform, simple file accessors
*/


#include <FileIO.h>
#include <fcntl.h>
#include <assert.h>

#ifdef WIN32
typedef struct _stati64 fstat_t;

// AFAIK, there is no iovec equivalent in the win32 API
struct iovec {
  char* iov_base; // stupid iovec uses char*
  int   iov_len;
};
#else
#include <sys/uio.h>
typedef struct stat     fstat_t;
#endif

//
//
static ASDCP::Result_t
do_stat(const char* path, fstat_t* stat_info)
{
  ASDCP_TEST_NULL_STR(path);
  ASDCP_TEST_NULL(stat_info);

  ASDCP::Result_t result = ASDCP::RESULT_OK;

#ifdef WIN32
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  if ( _stati64(path, stat_info) == (__int64)-1 )
    result = ASDCP::RESULT_FILEOPEN;

  ::SetErrorMode( prev );
#else
  if ( stat(path, stat_info) == -1L )
    result = ASDCP::RESULT_FILEOPEN;

  if ( stat_info->st_mode & (S_IFREG|S_IFLNK|S_IFDIR) == 0 )
    result = ASDCP::RESULT_FILEOPEN;
#endif

  return result;
}



//
bool
ASDCP::PathIsFile(const char* pathname)
{
  assert(pathname);
  fstat_t info;

  if ( ASDCP_SUCCESS(do_stat(pathname, &info)) )
    {
      if ( info.st_mode & S_IFREG )
        return true;
    }

  return false;
}


//
bool
ASDCP::PathIsDirectory(const char* pathname)
{
  assert(pathname);
  fstat_t info;

  if ( ASDCP_SUCCESS(do_stat(pathname, &info)) )
    {
      if ( info.st_mode & S_IFDIR )
        return true;
    }

  return false;
}


//
ASDCP::fsize_t
ASDCP::FileSize(const char* pathname)
{
  assert(pathname);
  fstat_t info;

  if ( ASDCP_SUCCESS(do_stat(pathname, &info)) )
    {
      if ( info.st_mode & S_IFREG )
        return(info.st_size);
    }

  return 0;
}

//------------------------------------------------------------------------------------------
// portable aspects of the file classes

const int IOVecMaxEntries = 32; // we never use more that 3, but that number seems somehow small...

//
class ASDCP::FileWriter::h__iovec
{
public:
  int            m_Count;
  struct iovec   m_iovec[IOVecMaxEntries];
  h__iovec() : m_Count(0) {}
};



//
ASDCP::fsize_t
ASDCP::FileReader::Size() const
{
  return ASDCP::FileSize(m_Filename.c_str());
}

// these are declared here instead of in the header file
// because we have a mem_ptr that is managing a hidden class
ASDCP::FileWriter::FileWriter() {}
ASDCP::FileWriter::~FileWriter() {}

//
ASDCP::Result_t
ASDCP::FileWriter::Writev(const byte_t* buf, ui32_t buf_len)
{
  assert( ! m_IOVec.empty() );
  register h__iovec* iov = m_IOVec;
  ASDCP_TEST_NULL(buf);

  if ( iov->m_Count >= IOVecMaxEntries )
    {
      DefaultLogSink().Error("The iovec is full! Only %lu entries allowed before a flush.\n",
			     IOVecMaxEntries);
      return RESULT_FAIL;
    }

  iov->m_iovec[iov->m_Count].iov_base = (char*)buf; // stupid iovec uses char*
  iov->m_iovec[iov->m_Count].iov_len = buf_len;
  iov->m_Count++;

  return RESULT_OK;
}


#ifdef WIN32
//------------------------------------------------------------------------------------------
//

ASDCP::Result_t
ASDCP::FileReader::OpenRead(const char* filename) const
{
  ASDCP_TEST_NULL_STR(filename);
  const_cast<FileReader*>(this)->m_Filename = filename;
  
  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  const_cast<FileReader*>(this)->m_Handle = ::CreateFile(filename,
			  (GENERIC_READ),                // open for reading
			  FILE_SHARE_READ,               // share for reading
			  NULL,                          // no security
			  OPEN_EXISTING,                 // read
			  FILE_ATTRIBUTE_NORMAL,         // normal file
			  NULL                           // no template file
			  );

  ::SetErrorMode(prev);

  return ( m_Handle == INVALID_HANDLE_VALUE ) ?
    ASDCP::RESULT_FILEOPEN : ASDCP::RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Close() const
{
  if ( m_Handle == INVALID_HANDLE_VALUE )
    return ASDCP::RESULT_FILEOPEN;

  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  BOOL result = ::CloseHandle(m_Handle);
  ::SetErrorMode(prev);
  const_cast<FileReader*>(this)->m_Handle = INVALID_HANDLE_VALUE;

  return ( result == 0 ) ? ASDCP::RESULT_FAIL : ASDCP::RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Seek(ASDCP::fpos_t position, SeekPos_t whence) const
{
  if ( m_Handle == INVALID_HANDLE_VALUE )
    return ASDCP::RESULT_STATE;

  LARGE_INTEGER in;
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  in.QuadPart = position;
  in.LowPart = ::SetFilePointer(m_Handle, in.LowPart, &in.HighPart, whence);
  HRESULT LastError = GetLastError();
  ::SetErrorMode(prev);

  if ( (LastError != NO_ERROR
	&& (in.LowPart == INVALID_SET_FILE_POINTER
	    || in.LowPart == ERROR_NEGATIVE_SEEK )) )
    return ASDCP::RESULT_READFAIL;
  
  return ASDCP::RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Tell(ASDCP::fpos_t* pos) const
{
  ASDCP_TEST_NULL(pos);

  if ( m_Handle == (HANDLE)-1L )
    return ASDCP::RESULT_FILEOPEN;

  LARGE_INTEGER in;
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  in.QuadPart = (__int64)0;
  in.LowPart = ::SetFilePointer(m_Handle, in.LowPart, &in.HighPart, FILE_CURRENT);
  HRESULT LastError = GetLastError();
  ::SetErrorMode(prev);

  if ( (LastError != NO_ERROR
	&& (in.LowPart == INVALID_SET_FILE_POINTER
	    || in.LowPart == ERROR_NEGATIVE_SEEK )) )
    return ASDCP::RESULT_READFAIL;

  *pos = (ASDCP::fpos_t)in.QuadPart;
  return ASDCP::RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Read(byte_t* buf, ui32_t buf_len, ui32_t* read_count) const
{
  ASDCP_TEST_NULL(buf);
  Result_t result = ASDCP::RESULT_OK;
  DWORD    tmp_count;
  ui32_t tmp_int;

  if ( read_count == 0 )
    read_count = &tmp_int;

  *read_count = 0;

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return ASDCP::RESULT_FILEOPEN;
  
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  if ( ::ReadFile(m_Handle, buf, buf_len, &tmp_count, NULL) == 0 )
    result = ASDCP::RESULT_READFAIL;

  ::SetErrorMode(prev);

  if ( tmp_count == 0 ) /* EOF */
    result = ASDCP::RESULT_ENDOFFILE;

  if ( ASDCP_SUCCESS(result) )
    *read_count = tmp_count;

  return result;
}



//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::FileWriter::OpenWrite(const char* filename)
{
  ASDCP_TEST_NULL_STR(filename);
  m_Filename = filename;
  
  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);

  m_Handle = ::CreateFile(filename,
			  (GENERIC_WRITE|GENERIC_READ),  // open for reading
			  FILE_SHARE_READ,               // share for reading
			  NULL,                          // no security
			  CREATE_ALWAYS,                 // overwrite (beware!)
			  FILE_ATTRIBUTE_NORMAL,         // normal file
			  NULL                           // no template file
			  );

  ::SetErrorMode(prev);

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return ASDCP::RESULT_FILEOPEN;
  
  m_IOVec = new h__iovec;
  return ASDCP::RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileWriter::Writev(ui32_t* bytes_written)
{
  assert( ! m_IOVec.empty() );
  register h__iovec* iov = m_IOVec;
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return ASDCP::RESULT_STATE;

  *bytes_written = 0;
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  Result_t result = ASDCP::RESULT_OK;

  // AFAIK, there is no writev() equivalent in the win32 API
  for ( register int i = 0; i < iov->m_Count; i++ )
    {
      ui32_t tmp_count = 0;
      BOOL wr_result = ::WriteFile(m_Handle,
				   iov->m_iovec[i].iov_base,
				   iov->m_iovec[i].iov_len,
				   (DWORD*)&tmp_count,
				   NULL);

      if ( wr_result == 0 || iov->m_iovec[i].iov_len != tmp_count)
	{
	  result = ASDCP::RESULT_WRITEFAIL;
	  break;
	}

      *bytes_written += tmp_count;
    }

  ::SetErrorMode(prev);
  iov->m_Count = 0; // error nor not, all is lost

  return result;
}

//
ASDCP::Result_t
ASDCP::FileWriter::Write(const byte_t* buf, ui32_t buf_len, ui32_t* bytes_written)
{
  ASDCP_TEST_NULL(buf);
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == INVALID_HANDLE_VALUE )
    return ASDCP::RESULT_STATE;

  // suppress popup window on error
  UINT prev = ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
  BOOL result = ::WriteFile(m_Handle, buf, buf_len, (DWORD*)bytes_written, NULL);
  ::SetErrorMode(prev);

  return ( result == 0 ) ? ASDCP::RESULT_WRITEFAIL : ASDCP::RESULT_OK;
}

#else // WIN32
//------------------------------------------------------------------------------------------
// POSIX

//
ASDCP::Result_t
ASDCP::FileReader::OpenRead(const char* filename) const
{
  ASDCP_TEST_NULL_STR(filename);
  const_cast<FileReader*>(this)->m_Filename = filename;
  const_cast<FileReader*>(this)->m_Handle = open(filename, O_RDONLY, 0);
  return ( m_Handle == -1L ) ? RESULT_FILEOPEN : RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Close() const
{
  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  close(m_Handle);
  const_cast<FileReader*>(this)->m_Handle = -1L;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Seek(ASDCP::fpos_t position, SeekPos_t whence) const
{
  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  if ( lseek(m_Handle, position, whence) == -1L )
    return RESULT_BADSEEK;

  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Tell(ASDCP::fpos_t* pos) const
{
  ASDCP_TEST_NULL(pos);

  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  ASDCP::fpos_t tmp_pos;

  if (  (tmp_pos = lseek(m_Handle, 0, SEEK_CUR)) == -1 )
    return RESULT_READFAIL;

  *pos = tmp_pos;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileReader::Read(byte_t* buf, ui32_t buf_len, ui32_t* read_count) const
{
  ASDCP_TEST_NULL(buf);
  i32_t  tmp_count = 0;
  ui32_t tmp_int = 0;

  if ( read_count == 0 )
    read_count = &tmp_int;

  *read_count = 0;

  if ( m_Handle == -1L )
    return RESULT_FILEOPEN;

  if ( (tmp_count = read(m_Handle, buf, buf_len)) == -1L )
    return RESULT_READFAIL;

  *read_count = tmp_count;
  return (tmp_count == 0 ? RESULT_ENDOFFILE : RESULT_OK);
}


//------------------------------------------------------------------------------------------
//

//
ASDCP::Result_t
ASDCP::FileWriter::OpenWrite(const char* filename)
{
  ASDCP_TEST_NULL_STR(filename);
  m_Filename = filename;
  m_Handle = open(filename, O_RDWR|O_CREAT|O_TRUNC, 0644);

  if ( m_Handle == -1L )
    {
      perror(filename);
      return RESULT_FILEOPEN;
    }

  m_IOVec = new h__iovec;
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileWriter::Writev(ui32_t* bytes_written)
{
  assert( ! m_IOVec.empty() );
  register h__iovec* iov = m_IOVec;
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == -1L )
    return RESULT_STATE;

  int read_size = writev(m_Handle, iov->m_iovec, iov->m_Count);
  
  if ( read_size == -1L )
    return RESULT_WRITEFAIL;

  iov->m_Count = 0;
  *bytes_written = read_size;  
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::FileWriter::Write(const byte_t* buf, ui32_t buf_len, ui32_t* bytes_written)
{
  ASDCP_TEST_NULL(buf);
  ui32_t tmp_int;

  if ( bytes_written == 0 )
    bytes_written = &tmp_int;

  if ( m_Handle == -1L )
    return RESULT_STATE;

  int read_size = write(m_Handle, buf, buf_len);
  
  if ( read_size == -1L )
    return RESULT_WRITEFAIL;

  *bytes_written = read_size;  
  return RESULT_OK;
}


#endif // WIN32

//
// end FileIO.cpp
//
