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
/*! \file    DirScanner.h
    \version $Id$
    \brief   Cross-platform, simple non-recursive directory scanner
*/

#ifndef _DIRSCANNER_H_
#define _DIRSCANNER_H_

#include <AS_DCP_system.h>
#define ASDCP_MAX_PATH 256

// Win32 directory scanner
//
#ifdef WIN32
#include <io.h>

//
//
class DirScanner
{
 public:
  __int64               m_Handle;
  struct _finddatai64_t m_FileInfo;

  DirScanner()  {};
  ~DirScanner() { Close(); }
  ASDCP::Result_t Open(const char*);
  ASDCP::Result_t Close();
  ASDCP::Result_t GetNext(char*);
};


#else // WIN32

#include <dirent.h>

// POSIX directory scanner

//
//
class DirScanner
{
 public:
  DIR*       m_Handle;

  DirScanner() : m_Handle(NULL)
  {
  }

  ~DirScanner() { Close(); }

  ASDCP::Result_t  Open(const char*);
  ASDCP::Result_t  Close();
  ASDCP::Result_t  GetNext(char*);
};


#endif // WIN32

#endif // _DIRSCANNER_H_


//
// end DirScanner.h
//
