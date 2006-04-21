/*
Copyright (c) 2004-2006, John Hurst
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
  /*! \file    KM_fileio.h
    \version $Id$
    \brief   portable file i/o
  */

#ifndef _KM_FILEIO_H_
#define _KM_FILEIO_H_

#include <KM_util.h>
#include <string>

#ifdef KM_WIN32
# include <io.h>
#else
# include <dirent.h>
# include <unistd.h>
# include <time.h>
# include <sys/types.h>
#endif

#include <sys/stat.h>



namespace Kumu
{
#ifdef KM_WIN32
  //
  class DirScanner
    {
    public:
      __int64               m_Handle;
      struct _finddatai64_t m_FileInfo;

      DirScanner()  {};
      ~DirScanner() { Close(); }
      Result_t Open(const char*);
      Result_t Close();
      Result_t GetNext(char*);
    };
#else // KM_WIN32
  // POSIX directory scanner
  //
  class DirScanner
    {
    public:
      DIR*       m_Handle;

      DirScanner() : m_Handle(NULL) {}
      ~DirScanner() { Close(); }
      
      Result_t  Open(const char*);
      Result_t  Close();
      Result_t  GetNext(char*);
    };
#endif // KM_WIN32

#ifdef KM_WIN32
  typedef __int64  fsize_t;
  typedef __int64  fpos_t;

  enum SeekPos_t {
    SP_BEGIN = FILE_BEGIN,
    SP_POS   = FILE_CURRENT,
    SP_END   = FILE_END
  };
#else
  typedef off_t    fsize_t;
  typedef off_t    fpos_t;
  typedef int      HANDLE;
  const HANDLE INVALID_HANDLE_VALUE = -1L;

  enum SeekPos_t {
    SP_BEGIN = SEEK_SET,
    SP_POS   = SEEK_CUR,
    SP_END   = SEEK_END
  };
#endif

  const ui32_t Kilobyte = 1024;
  const ui32_t Megabyte = Kilobyte * Kilobyte;
  const ui32_t Gigabyte = Megabyte * Kilobyte;

  const ui32_t MaxFilePath = Kilobyte;

  bool     PathIsFile(const char* pathname);
  bool     PathIsDirectory(const char* pathname);
  fsize_t  FileSize(const char* pathname);

  // Reads an entire file into a string.
  Result_t ReadFileIntoString(const char* filename, std::string& outString, ui32_t max_size = 256 * Kilobyte);

  // Writes a string to a file, overwrites the existing file if present.
  Result_t WriteStringIntoFile(const char* filename, const std::string& inString);

  //
  class FileReader
    {
      KM_NO_COPY_CONSTRUCT(FileReader);

    protected:
      std::string m_Filename;
      HANDLE      m_Handle;

    public:
      FileReader() : m_Handle(INVALID_HANDLE_VALUE) {}
      virtual ~FileReader() { Close(); }

      Result_t OpenRead(const char*) const;                          // open the file for reading
      Result_t Close() const;                                        // close the file
      fsize_t  Size() const;                                         // returns the file's current size
      Result_t Seek(Kumu::fpos_t = 0, SeekPos_t = SP_BEGIN) const;   // move the file pointer
      Result_t Tell(Kumu::fpos_t* pos) const;                        // report the file pointer's location
      Result_t Read(byte_t*, ui32_t, ui32_t* = 0) const;             // read a buffer of data

      inline Kumu::fpos_t Tell() const                               // report the file pointer's location
	{
	  Kumu::fpos_t tmp_pos;
	  Tell(&tmp_pos);
	  return tmp_pos;
	}

      inline bool IsOpen() {                                         // returns true if the file is open
	return (m_Handle != INVALID_HANDLE_VALUE);
      }
    };

  //
  class FileWriter : public FileReader
    {
      class h__iovec;
      mem_ptr<h__iovec>  m_IOVec;
      KM_NO_COPY_CONSTRUCT(FileWriter);

    public:
      FileWriter();
      virtual ~FileWriter();

      Result_t OpenWrite(const char*);                               // open a new file, overwrites existing
      Result_t OpenModify(const char*);                              // open a file for read/write

      // this part of the interface takes advantage of the iovec structure on
      // platforms that support it. For each call to Writev(const byte_t*, ui32_t, ui32_t*),
      // the given buffer is added to an internal iovec struct. All items on the list
      // are written to disk by a call to Writev();
      Result_t Writev(const byte_t*, ui32_t);                       // queue buffer for "gather" write
      Result_t Writev(ui32_t* = 0);                                 // write all queued buffers

      // if you call this while there are unwritten items on the iovec list,
      // the iovec list will be written to disk before the given buffer,as though
      // you had called Writev() first.
      Result_t Write(const byte_t*, ui32_t, ui32_t* = 0);            // write buffer to disk
   };

} // namespace Kumu


#endif // _KM_FILEIO_H_


//
// end KM_fileio.h
//
