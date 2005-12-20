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
/*! \file    FileIO.h
    \version $Id$
    \brief   Cross-platform, simple file accessors
*/


#ifndef _FILEIO_H_
#define _FILEIO_H_

#include <AS_DCP_system.h>
#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#include <sys/types.h>
#endif

namespace ASDCP {
#ifdef WIN32
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

  bool    PathIsFile(const char* pathname);
  bool    PathIsDirectory(const char* pathname);
  fsize_t FileSize(const char* pathname);

  //
  class FileReader
    {
      ASDCP_NO_COPY_CONSTRUCT(FileReader);

    protected:
      std::string m_Filename;
      HANDLE      m_Handle;

    public:
      FileReader() : m_Handle(INVALID_HANDLE_VALUE) {}
      virtual ~FileReader() { Close(); }

      Result_t OpenRead(const char*) const;                          // open the file for reading
      Result_t Close() const;                                        // close the file
      fsize_t  Size() const;                                         // returns the file's current size
      Result_t Seek(ASDCP::fpos_t = 0, SeekPos_t = SP_BEGIN) const;  // move the file pointer
      Result_t Tell(ASDCP::fpos_t* pos) const;                       // report the file pointer's location
      Result_t Read(byte_t*, ui32_t, ui32_t* = 0) const;             // read a buffer of data

      inline ASDCP::fpos_t Tell() const                              // report the file pointer's location
	{
	  ASDCP::fpos_t tmp_pos;
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
      ASDCP_NO_COPY_CONSTRUCT(FileWriter);

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
      // the iovec list will be written to disk before the givn buffer,as though
      // you had called Writev() first.
      Result_t Write(const byte_t*, ui32_t, ui32_t* = 0);            // write buffer to disk


   };

} // namespace ASDCP


#endif // _FILEREADER_H_


//
// end FileReader.h
//
