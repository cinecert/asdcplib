/*
Copyright (c) 2005-2016, John Hurst
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
/*! \file    klvwalk.cpp
    \version $Id$
    \brief   KLV+MXF test
*/

#include "AS_DCP.h"
#include "MXF.h"
#include <KM_log.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

using namespace ASDCP;
using Kumu::DefaultLogSink;


//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "klvwalk";    // program name for messages
typedef std::list<std::string> FileList_t;

// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)    if ( ++i >= argc || argv[(i)][0] == '-' ) \
                                 { \
                                   fprintf(stderr, "Argument not found for option -%c.\n", (c)); \
                                   return; \
                                 }

//
void
banner(FILE* stream = stdout)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2005-2013 John Hurst\n\
%s is part of the asdcplib DCP tools package.\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, ASDCP::Version(), PROGRAM_NAME, PROGRAM_NAME);
}

//
void
usage(FILE* stream = stdout)
{
  fprintf(stream, "\
USAGE: %s [-r|-p] [-v] <input-file> [<input-file2> ...]\n\
\n\
       %s [-h|-help] [-V]\n\
\n\
  -h | -help   - Show help\n\
  -r           - When KLV data is an MXF OPAtom or OP 1a file, display headers\n\
  -p           - Display partition headers by walking the RIP\n\
  -v           - Verbose. Prints informative messages to stderr\n\
  -V           - Show version information\n\
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
   bool   error_flag;               // true if the given options are in error or not complete
   bool   version_flag;             // true if the version display option was selected
   bool   help_flag;                // true if the help display option was selected
   bool   verbose_flag;             // true if the informative messages option was selected
   bool   read_mxf_flag;            // true if the -r option was selected
   bool   walk_parts_flag;          // true if the -p option was selected
   FileList_t inFileList;           // File to operate on

   CommandOptions(int argc, const char** argv) :
     error_flag(true), version_flag(false), help_flag(false),
     verbose_flag(false), read_mxf_flag(false), walk_parts_flag(false)
   {
     for ( int i = 1; i < argc; i++ )
       {

	 if ( (strcmp( argv[i], "-help") == 0) )
	   {
	     help_flag = true;
	     continue;
	   }
         
	 if ( argv[i][0] == '-' && isalpha(argv[i][1]) && argv[i][2] == 0 )
	   {
	     switch ( argv[i][1] )
	       {
	       case 'h': help_flag = true; break;
	       case 'r': read_mxf_flag = true; break;
	       case 'p': walk_parts_flag = true; break;
	       case 'V': version_flag = true; break;
	       case 'v': verbose_flag = true; break;

	       default:
		 fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		 return;
	       }
	   }
	 else
	   {
	     if ( argv[i][0] != '-' )
	       inFileList.push_back(argv[i]);

	     else
	       {
       	         fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		 return;
	       }
	   }
       }

     if ( help_flag || version_flag )
       return;
     
     if ( inFileList.empty() )
       {
	 fputs("Input filename(s) required.\n", stderr);
	 return;
       }
     
     error_flag = false;
   }
 };


//---------------------------------------------------------------------------------------------------
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

  FileList_t::iterator fi;
  Result_t result = RESULT_OK;

  for ( fi = Options.inFileList.begin(); ASDCP_SUCCESS(result) && fi != Options.inFileList.end(); fi++ )
    {
      if (Options.verbose_flag)
	fprintf(stderr, "Opening file %s\n", ((*fi).c_str()));
      
      if ( Options.read_mxf_flag ) // dump MXF
	{
	  Kumu::FileReader        Reader;
	  const Dictionary* Dict = &DefaultCompositeDict();
	  ASDCP::MXF::OP1aHeader Header(Dict);
	  ASDCP::MXF::RIP RIP(Dict);
	  
	  result = Reader.OpenRead(*fi);
	  
	  if ( ASDCP_SUCCESS(result) )
	    {
	      result = MXF::SeekToRIP(Reader);
	      
	      if ( ASDCP_SUCCESS(result) )
		{
		  result = RIP.InitFromFile(Reader);
		  ui32_t test_s = RIP.PairArray.size();

		  if ( ASDCP_FAILURE(result) )
		    {
		      DefaultLogSink().Error("File contains no RIP\n");
		      result = RESULT_OK;
		    }
		  else if ( RIP.PairArray.empty() )
		    {
		      DefaultLogSink().Error("RIP contains no Pairs.\n");
		    }

		  Reader.Seek(0);
		}
	      else
		{
		  DefaultLogSink().Error("read_mxf SeekToRIP failed: %s\n", result.Label());
		}
	    }

	  if ( ASDCP_SUCCESS(result) )
	    result = Header.InitFromFile(Reader);
	  
	  if ( ASDCP_SUCCESS(result) )
	    Header.Dump(stdout);
	  
	  if ( ASDCP_SUCCESS(result) && RIP.PairArray.size() > 2 )
	    {
	      MXF::RIP::const_pair_iterator pi = RIP.PairArray.begin();

	      for ( pi++; pi != RIP.PairArray.end() && ASDCP_SUCCESS(result); pi++ )
		{
		  result = Reader.Seek((*pi).ByteOffset);

		  if ( ASDCP_SUCCESS(result) )
		    {
		      MXF::Partition TmpPart(Dict);
		      result = TmpPart.InitFromFile(Reader);

		      if ( ASDCP_SUCCESS(result) && TmpPart.BodySID > 0 )
			TmpPart.Dump(stdout);
		    }
		}
	    }

	  if ( ASDCP_SUCCESS(result) )
	    {
	      ASDCP::MXF::OPAtomIndexFooter Index(Dict);
	      result = Reader.Seek(Header.FooterPartition);
	      
	      if ( ASDCP_SUCCESS(result) )
		{
		  Index.m_Lookup = &Header.m_Primer;
		  result = Index.InitFromFile(Reader);
		}
	      
	      if ( ASDCP_SUCCESS(result) )
		Index.Dump(stdout);
	    }

	  if ( ASDCP_SUCCESS(result) )
	    RIP.Dump(stdout);
	}
      else if ( Options.walk_parts_flag )
	{
	  Kumu::FileReader        Reader;
	  const Dictionary* Dict = &DefaultCompositeDict();
	  ASDCP::MXF::OP1aHeader Header(Dict);
	  ASDCP::MXF::RIP RIP(Dict);
	  
	  result = Reader.OpenRead((*fi).c_str());
	  
	  if ( ASDCP_SUCCESS(result) )
	    result = MXF::SeekToRIP(Reader);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      result = RIP.InitFromFile(Reader);
	      ui32_t test_s = RIP.PairArray.size();

	      if ( ASDCP_FAILURE(result) )
		{
		  DefaultLogSink().Error("File contains no RIP\n");
		  result = RESULT_OK;
		}
	      else if ( RIP.PairArray.empty() )
		{
		  DefaultLogSink().Error("RIP contains no Pairs.\n");
		}

	      Reader.Seek(0);
	    }
	  else
	    {
	      DefaultLogSink().Error("walk_parts SeekToRIP failed: %s\n", result.Label());
	    }

	  if ( ASDCP_SUCCESS(result) )
	    {
	      RIP.Dump();

	      MXF::RIP::const_pair_iterator i;
	      for ( i = RIP.PairArray.begin(); i != RIP.PairArray.end(); ++i )
		{
		  Reader.Seek(i->ByteOffset);
		  MXF::Partition plain_part(Dict);
		  plain_part.InitFromFile(Reader);

		  if ( plain_part.ThisPartition != i->ByteOffset )
		    {
		      DefaultLogSink().Error("ThisPartition value error: wanted=%qu, got=%qu\n",
					     plain_part.ThisPartition, i->ByteOffset);
		    }

		  plain_part.Dump();
		}
	    }
	}
      else // dump klv
	{
	  Kumu::FileReader Reader;
	  KLVFilePacket KP;
	  ui64_t pos = 0;

	  result = Reader.OpenRead((*fi).c_str());
	  
	  if ( ASDCP_SUCCESS(result) )
	    result = KP.InitFromFile(Reader);
	  
	  while ( ASDCP_SUCCESS(result) )
	    {
	      fprintf(stdout, "@0x%08qx: ", pos);
	      KP.Dump(stdout, DefaultCompositeDict(), true);
	      pos = Reader.Tell();
	      result = KP.InitFromFile(Reader);
	    }
	  
	  if( result == RESULT_ENDOFFILE )
	    result = RESULT_OK;
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
// end klvwalk.cpp
//
