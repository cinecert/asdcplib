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
  /*! \file    KM_log.h
    \version $Id$
    \brief   message logging API
  */


#ifndef _KM_LOG_H_
#define _KM_LOG_H_

#include <KM_platform.h>
#include <KM_mutex.h>
#include <stdarg.h>
#include <errno.h>

#define LOG_MSG_IMPL(t) va_list args; va_start(args, fmt); vLogf((t), fmt, &args); va_end(args)


namespace Kumu
{
  // no log message will exceed this length
  const ui32_t MaxLogLength = 512;

  //---------------------------------------------------------------------------------
  // message logging

  // Error and debug messages will be delivered to an object having this interface.
  // The default implementation sends only LOG_ERROR and LOG_WARN messages to stderr.
  // To receive LOG_INFO or LOG_DEBUG messages, or to send messages somewhere other
  // than stderr, implement this interface and register an instance of your new class
  // by calling SetDefaultLogSink().
  class ILogSink
    {
    public:
      enum LogType_t { LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERROR,
		       LOG_NOTICE, LOG_ALERT, LOG_CRIT };

      virtual ~ILogSink() {}

      void Critical(const char* fmt, ...) { LOG_MSG_IMPL(LOG_CRIT); }
      void Alert(const char* fmt, ...)    { LOG_MSG_IMPL(LOG_ALERT); }
      void Notice(const char* fmt, ...)   { LOG_MSG_IMPL(LOG_NOTICE); }
      void Error(const char* fmt, ...)    { LOG_MSG_IMPL(LOG_ERROR); }
      void Warn(const char* fmt, ...)     { LOG_MSG_IMPL(LOG_WARN);  }
      void Info(const char* fmt, ...)     { LOG_MSG_IMPL(LOG_INFO);  }
      void Debug(const char* fmt, ...)    { LOG_MSG_IMPL(LOG_DEBUG); }
      void Logf(ILogSink::LogType_t type, const char* fmt, ...) { LOG_MSG_IMPL(type); }
      virtual void vLogf(LogType_t, const char*, va_list*) = 0; // log a formatted string with a va_list struct
    };

  // Sets the internal default sink to the given receiver. If the given value
  // is zero, sets the default sink to the internally allocated stderr sink.
  void SetDefaultLogSink(ILogSink* = 0);

  // Returns the internal default sink.
  ILogSink& DefaultLogSink();

  //
  class StdioLogSink : public ILogSink
    {
      Mutex m_Lock;
      FILE* m_stream;
      KM_NO_COPY_CONSTRUCT(StdioLogSink);

    public:
      StdioLogSink() : m_stream(stderr) {};
      StdioLogSink(FILE* stream) : m_stream(stream) {}
      virtual ~StdioLogSink() {}
      virtual void vLogf(LogType_t, const char*, va_list*);
    };

#ifdef KM_WIN32
  //
  class WinDbgLogSink : public ILogSink
    {
      Mutex m_Lock;
      KM_NO_COPY_CONSTRUCT(WinDbgLogSink);

    public:
      WinDbgLogSink() {}
      virtual ~WinDbgLogSink() {}
      virtual void vLogf(LogType_t, const char*, va_list*);
    };

#else

  //
  class StreamLogSink : public ILogSink
    {
      Mutex m_Lock;
      int   m_fd;
      KM_NO_COPY_CONSTRUCT(StreamLogSink);
      StreamLogSink();

    public:
      StreamLogSink(int fd) : m_fd(fd) {}
      virtual ~StreamLogSink() {}
      virtual void vLogf(LogType_t, const char*, va_list*);
    };
#endif

} // namespace Kumu

#endif // _KM_LOG_H_

//
// end KM_log.h
//
