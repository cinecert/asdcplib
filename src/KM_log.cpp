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
  /*! \file    KM_log.cpp
    \version $Id$
    \brief   message logging API
  */

#include <KM_util.h>
#include <KM_log.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>

#ifdef KM_WIN32
#define getpid GetCurrentProcessId
#else
#include <unistd.h>
#endif


void
Kumu::StdioLogSink::vLogf(ILogSink::LogType_t type, const char* fmt, va_list* list)
{
  AutoMutex L(m_Lock);

  switch ( type )
    {
    case LOG_CRIT:   fprintf(m_stream, "[%d CRT]: ", getpid()); break;
    case LOG_ALERT:  fprintf(m_stream, "[%d ALR]: ", getpid()); break;
    case LOG_NOTICE: fprintf(m_stream, "[%d NTC]: ", getpid()); break;
    case LOG_ERROR:  fprintf(m_stream, "[%d ERR]: ", getpid()); break;
    case LOG_WARN:   fprintf(m_stream, "[%d WRN]: ", getpid()); break;
    case LOG_INFO:   fprintf(m_stream, "[%d INF]: ", getpid()); break;
    case LOG_DEBUG:  fprintf(m_stream, "[%d DBG]: ", getpid()); break;
    default:         fprintf(m_stream, "[%d DFL]: ", getpid());
    }

  vfprintf(m_stream, fmt, *list);
}

static Kumu::ILogSink* s_DefaultLogSink;
static Kumu::StdioLogSink s_StderrLogSink;

//
void
Kumu::SetDefaultLogSink(ILogSink* Sink)
{
    s_DefaultLogSink = Sink;
}

// Returns the internal default sink.
Kumu::ILogSink&
Kumu::DefaultLogSink()
{
  if ( s_DefaultLogSink == 0 )
    s_DefaultLogSink = &s_StderrLogSink;

  return *s_DefaultLogSink;
}

//---------------------------------------------------------------------------------
#ifdef KM_WIN32

//
void
Kumu::WinDbgLogSink::vLogf(ILogSink::LogType_t type, const char* fmt, va_list* list)
{
  AutoMutex L(m_Lock);
  char msg_buf[MaxLogLength];

  DWORD pid = GetCurrentProcessId();

  switch ( type )
    {
    case LOG_CRIT:   snprintf(msg_buf, MaxLogLength, "[%d CRT]: ", pid); break;
    case LOG_ALERT:  snprintf(msg_buf, MaxLogLength, "[%d ALR]: ", pid); break;
    case LOG_NOTICE: snprintf(msg_buf, MaxLogLength, "[%d NTC]: ", pid); break;
    case LOG_ERROR:  snprintf(msg_buf, MaxLogLength, "[%d ERR]: ", pid); break;
    case LOG_WARN:   snprintf(msg_buf, MaxLogLength, "[%d WRN]: ", pid); break;
    case LOG_INFO:   snprintf(msg_buf, MaxLogLength, "[%d INF]: ", pid); break;
    case LOG_DEBUG:  snprintf(msg_buf, MaxLogLength, "[%d DBG]: ", pid); break;
    default:         snprintf(msg_buf, MaxLogLength, "[%d DFL]: ", pid);
    }
  
  ui32_t len = strlen(msg_buf);
  vsnprintf(msg_buf + len, MaxLogLength - len, fmt, *list);
  msg_buf[MaxLogLength-1] = 0;
  ::OutputDebugString(msg_buf);
}

#else

void
Kumu::StreamLogSink::vLogf(ILogSink::LogType_t type, const char* fmt, va_list* list)
{
  AutoMutex L(m_Lock);
  char msg_buf[MaxLogLength];
  char ts_buf[MaxLogLength];
  Timestamp Now;

  switch ( type )
    {
    case LOG_CRIT:   snprintf(msg_buf, MaxLogLength, "[%s %d CRT]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    case LOG_ALERT:  snprintf(msg_buf, MaxLogLength, "[%s %d ALR]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    case LOG_NOTICE: snprintf(msg_buf, MaxLogLength, "[%s %d NTC]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    case LOG_ERROR:  snprintf(msg_buf, MaxLogLength, "[%s %d ERR]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    case LOG_WARN:   snprintf(msg_buf, MaxLogLength, "[%s %d WRN]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    case LOG_INFO:   snprintf(msg_buf, MaxLogLength, "[%s %d INF]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    case LOG_DEBUG:  snprintf(msg_buf, MaxLogLength, "[%s %d DBG]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid()); break;
    default:         snprintf(msg_buf, MaxLogLength, "[%s %d DFL]: ",
			      Now.EncodeString(ts_buf, MaxLogLength), getpid());
    }
  
  ui32_t len = strlen(msg_buf);
  vsnprintf(msg_buf + len, MaxLogLength - len, fmt, *list);
  msg_buf[MaxLogLength-1] = 0;
  write(m_fd, msg_buf, strlen(msg_buf));
}
#endif


//
// end
//
