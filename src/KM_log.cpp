/*
Copyright (c) 2004-2009, John Hurst
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
#include <KM_mutex.h>
#include <sys/types.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>
#include <sstream>

#ifdef KM_WIN32
#define getpid GetCurrentProcessId
#else
#include <unistd.h>
#endif

//------------------------------------------------------------------------------------------
//

void
Kumu::ILogSink::vLogf(LogType_t type, const char* fmt, va_list* list)
{
  char buf[MaxLogLength];
  vsnprintf(buf, MaxLogLength, fmt, *list);

  WriteEntry(LogEntry(getpid(), type, buf));
}

//------------------------------------------------------------------------------------------
//

static Kumu::Mutex     s_DefaultLogSinkLock;
static Kumu::ILogSink* s_DefaultLogSink = 0;
static Kumu::StdioLogSink s_StderrLogSink;

//
void
Kumu::SetDefaultLogSink(ILogSink* Sink)
{
  AutoMutex L(s_DefaultLogSinkLock);
  s_DefaultLogSink = Sink;
}

// Returns the internal default sink.
Kumu::ILogSink&
Kumu::DefaultLogSink()
{
  AutoMutex L(s_DefaultLogSinkLock);

  if ( s_DefaultLogSink == 0 )
    s_DefaultLogSink = &s_StderrLogSink;

  return *s_DefaultLogSink;
}


//------------------------------------------------------------------------------------------
//

void
Kumu::EntryListLogSink::WriteEntry(const LogEntry& Entry)
{
  AutoMutex L(m_Lock);

  if ( Entry.TestFilter(m_filter) )
    m_Target.push_back(Entry);
}

//------------------------------------------------------------------------------------------
//

void
Kumu::StdioLogSink::WriteEntry(const LogEntry& Entry)
{
  AutoMutex L(m_Lock);
  std::string buf;

  if ( Entry.TestFilter(m_filter) )
    {
      Entry.CreateStringWithOptions(buf, m_options);
      fputs(buf.c_str(), m_stream);
    }
}

//---------------------------------------------------------------------------------

#ifdef KM_WIN32
//
void
Kumu::WinDbgLogSink::WriteEntry(const LogEntry& Entry)
{
  AutoMutex L(m_Lock);
  std::string buf;

  if ( Entry.TestFilter(m_filter) )
    {
      Entry.CreateStringWithOptions(buf, m_options);
      ::OutputDebugString(buf.c_str());
    }
}
#endif

//------------------------------------------------------------------------------------------
//

#ifndef KM_WIN32
//
void
Kumu::StreamLogSink::WriteEntry(const LogEntry& Entry)
{
  AutoMutex L(m_Lock);
  std::string buf;

  if ( Entry.TestFilter(m_filter) )
    {
      Entry.CreateStringWithOptions(buf, m_options);
      write(m_fd, buf.c_str(), buf.size());
    }
}
#endif

//------------------------------------------------------------------------------------------

//
std::basic_ostream<char, std::char_traits<char> >&
Kumu::operator<<(std::basic_ostream<char, std::char_traits<char> >& strm, LogEntry const& Entry)
{
  std::basic_ostringstream<char, std::char_traits<char> > s;
  s.copyfmt(strm);
  s.width(0);
  std::string buf;

  s << Entry.CreateStringWithOptions(buf, LOG_OPTION_ALL);

  strm << s.str();
  return strm;
}

//------------------------------------------------------------------------------------------


//
bool
Kumu::LogEntry::TestFilter(i32_t filter) const
{
  switch ( Type )
    {
    case LOG_CRIT:
      if ( (filter & LOG_ALLOW_CRIT) == 0 )
	return false;
      break;

    case LOG_ALERT:
      if ( (filter & LOG_ALLOW_ALERT) == 0 )
	return false;
      break;

    case LOG_NOTICE:
      if ( (filter & LOG_ALLOW_NOTICE) == 0 )
	return false;
      break;

    case LOG_ERROR:
      if ( (filter & LOG_ALLOW_ERROR) == 0 )
	return false;
      break;

    case LOG_WARN:
      if ( (filter & LOG_ALLOW_WARN) == 0 )
	return false;
      break;

    case LOG_INFO:
      if ( (filter & LOG_ALLOW_INFO) == 0 )
	return false;
      break;

    case LOG_DEBUG:
      if ( (filter & LOG_ALLOW_DEBUG) == 0 )
	return false;
      break;

    }

 return true;
}

//
std::string&
Kumu::LogEntry::CreateStringWithOptions(std::string& out_buf, i32_t opt) const
{
  out_buf.erase();

  if ( opt != 0 )
    {
      char buf[64];

      if ( (opt & LOG_OPTION_TIMESTAMP) != 0 )
	{
	  Timestamp Now;
	  out_buf += Now.EncodeString(buf, 64);
	}

      if ( (opt & LOG_OPTION_PID) != 0 )
	{
	  if ( ! out_buf.empty() )  out_buf += " ";
	  snprintf(buf, 64, "%d", PID);
	  out_buf += buf;
	}

      if ( (opt & LOG_OPTION_TYPE) != 0 )
	{
	  if ( ! out_buf.empty() )  out_buf += " ";
	  
	  switch ( Type )
	    {
	    case LOG_CRIT:   out_buf += "CRT";      break;
	    case LOG_ALERT:  out_buf += "ALR";      break;
	    case LOG_NOTICE: out_buf += "NTC";      break;
	    case LOG_ERROR:  out_buf += "ERR";      break;
	    case LOG_WARN:   out_buf += "WRN";      break;
	    case LOG_INFO:   out_buf += "INF";      break;
	    case LOG_DEBUG:  out_buf += "DBG";      break;
	    default:	     out_buf += "DFL";      break;
	    }
	}

      out_buf.insert(0, "[");
      out_buf += "]: ";
    }

  out_buf += Msg;
  return out_buf;
}


//
ui32_t
Kumu::LogEntry::ArchiveLength() const
{
  return sizeof(ui32_t)
    + EventTime.ArchiveLength()
    + sizeof(ui32_t)
    + sizeof(ui32_t) + Msg.size();
}

//
bool
Kumu::LogEntry::Archive(Kumu::MemIOWriter* Writer) const
{
  if ( ! Writer->WriteUi32BE(PID) ) return false;
  if ( ! EventTime.Archive(Writer) ) return false;
  if ( ! Writer->WriteUi32BE(Type) ) return false;
  if ( ! ArchiveString(*Writer, Msg) ) return false;
  return true;
}

//
bool
Kumu::LogEntry::Unarchive(Kumu::MemIOReader* Reader)
{
  if ( ! Reader->ReadUi32BE(&PID) ) return false;
  if ( ! EventTime.Unarchive(Reader) ) return false;
  if ( ! Reader->ReadUi32BE((ui32_t*)&Type) ) return false;
  if ( ! UnarchiveString(*Reader, Msg) ) return false;
  return true;
}

//
// end
//
