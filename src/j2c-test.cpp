/*
Copyright (c) 2005-2014, John Hurst
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
/*! \file    j2c-test.cpp
    \version $Id$
    \brief   JP2K parser test
*/

#include <AS_DCP.h>
#include <KM_fileio.h>
#include <KM_util.h>
#include <JP2K.h>

using namespace Kumu;
using namespace ASDCP;
using namespace ASDCP::JP2K;



//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "j2c-test";    // program name for messages

// Macros used to test command option data state.

// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)    if ( ++i >= argc || argv[(i)][0] == '-' ) \
                                 { \
                                   fprintf(stderr, "Argument not found for option %c.\n", (c)); \
                                   return; \
                                 }
//
void
banner(FILE* stream = stderr)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2005-2010 John Hurst\n\n\
%s is part of asdcplib.\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, ASDCP::Version(), PROGRAM_NAME, PROGRAM_NAME);
}

//
void
usage(FILE* stream = stderr)
{
  fprintf(stream, "\
USAGE: %s [-h|-help] [-V]\n\
\n\
       %s [-r] [-v] <filename> [...]\n\
\n\
  -V           - Show version\n\
  -h           - Show help\n\
  -r           - Show raw data\n\
  -v           - Print extra detail\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME, PROGRAM_NAME);
}

//
//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   version_flag;   // true if the version display option was selected
  bool   verbose_flag;   // true if the verbose option was selected
  bool   detail_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  std::list<std::string> filename_list;

  CommandOptions(int argc, const char** argv) :
    error_flag(true), version_flag(false), verbose_flag(false),
    detail_flag(false), help_flag(false)
  {
    for ( int i = 1; i < argc; i++ )
      {
	if ( argv[i][0] == '-' && isalpha(argv[i][1]) && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'V': version_flag = true; break;
	      case 'h': help_flag = true; break;
	      case 'r': detail_flag = true; break;
	      case 'v': verbose_flag = true; break;

	      default:
		fprintf(stderr, "Unrecognized option: %c\n", argv[i][1]);
		return;
	      }
	  }
	else
	  {
	    filename_list.push_back(argv[i]);
	  }
      }

    if ( filename_list.empty() )
      {
	fputs("Input j2c filename(s) required.\n", stderr);
	return;
      }

    error_flag = false;
  }
};




//
int
main(int argc, const char** argv)
{
  CommandOptions Options(argc, argv);

  if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.version_flag || Options.help_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  ASDCP::JP2K::FrameBuffer FB;
  Marker        MyMarker;
  CodestreamParser Parser;
  std::list<std::string>::iterator i;
  bool has_soc = false;

  Result_t result = FB.Capacity(1024*1024*4);
  
  for ( i = Options.filename_list.begin(); ASDCP_SUCCESS(result) && i != Options.filename_list.end(); i++ )
    {
      result = Parser.OpenReadFrame(i->c_str(), FB);

      if ( ASDCP_SUCCESS(result) )
	{
	  const byte_t* p = FB.RoData();
	  const byte_t* end_p = p + FB.Size();

	  while ( p < end_p && ASDCP_SUCCESS(GetNextMarker(&p, MyMarker)) )
	    {
	      if ( MyMarker.m_Type == MRK_SOC )
		{
		  if ( has_soc )
		    {
		      fprintf(stderr, "Duplicate SOC detected.\n");
		      result = RESULT_FAIL;
		      break;
		    }
		  else
		    {
		      has_soc = true;
		      continue;
		    }

		  if  ( ! has_soc )
		    {
		      fprintf(stderr, "Markers detected before SOC.\n");
		      result = RESULT_FAIL;
		      break;
		    }
		}

	      if ( Options.verbose_flag )
		{
		  MyMarker.Dump(stdout);

		  if ( Options.detail_flag )
		    hexdump(MyMarker.m_Data - 2, MyMarker.m_DataSize + 2, stdout);
		}

	      if ( MyMarker.m_Type == MRK_SOD )
		{
		  p = end_p;
		}
	      else if ( MyMarker.m_Type == MRK_SIZ )
		{
		  Accessor::SIZ SIZ_(MyMarker);
		  SIZ_.Dump(stdout);
		}
	      else if ( MyMarker.m_Type == MRK_COD )
		{
		  Accessor::COD COD_(MyMarker);
		  COD_.Dump(stdout);
		}
	      else if ( MyMarker.m_Type == MRK_COM )
		{
		  Accessor::COM COM_(MyMarker);
		  COM_.Dump(stdout);
		}
	      else if ( MyMarker.m_Type == MRK_QCD )
		{
		  Accessor::QCD QCD_(MyMarker);
		  QCD_.Dump(stdout);
		}
	      else
		{
		  fprintf(stderr, "Unprocessed marker - %s\n", GetMarkerString(MyMarker.m_Type));
		}
	    }

	  /*
	  while ( p < end_p )
	    {
	      if ( *p == 0xff )
		{
		  fprintf(stdout, "0x%02x 0x%02x 0x%02x\n", *(p+1), *(p+2), *(p+3));
		  p += 4;
		}
	      else
		{
		  ++p;
		}
	    }
	  */
	}
    }

  if ( ASDCP_FAILURE(result) )
    {
      fputs("Program stopped on error.\n", stderr);

      if ( result != RESULT_FAIL )
	{
	  fputs(result, stderr);
	  fputc('\n', stderr);
	}

      return 1;
    }

  return 0;
}

//
// end j2c-test.cpp
//
