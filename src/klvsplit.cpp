/*
Copyright (c) 2005-2013, John Hurst
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

#include "MXF.h"
#include <KM_log.h>


using namespace ASDCP;
using Kumu::DefaultLogSink;


//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "klvsplit";    // program name for messages
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
USAGE: %s [-l <limit>] [-p <prefix>] [-s <suffix>] [-u|-U] [-v] \n\
           (<type-name>|<type-ul>) <mxf-filename>+\n\
\n\
       %s -d\n\
\n\
       %s [-h|-help] [-V]\n\
\n\
  -d           - List the valid packet type names\n\
  -h | -help   - Show help\n\
  -l <limit>   - Stop processing after <limit> matching packets\n\
  -p <prefix>  - Use <prefix> to start output filenames (default\n\
                   uses the input filename minus any extension\n\
  -s <suffix>  - Append <suffix> to output filenames\n\
  -u           - Unwrap the packet value (i.e., do not output KL)\n\
  -U           - Do not unwrap (default)\n\
  -v           - Verbose. Prints informative messages to stderr\n\
  -V           - Show version information\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME, PROGRAM_NAME, PROGRAM_NAME);
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
   bool   unwrap_mode;              // true if we are to strip the K and L before writing
   bool   list_mode;
   ASDCP::UL target_ul;             // a UL value identifying the packets to be extracted
   ui64_t  extract_limit;           // limit extraction to the given number of packets
   std::string prefix;              // output filename prefix
   std::string suffix;              // output filename suffix
   FileList_t inFileList;           // File to operate on

   CommandOptions(int argc, const char** argv, const ASDCP::Dictionary& dict) :
     error_flag(true), version_flag(false), help_flag(false),
     verbose_flag(false), unwrap_mode(false), list_mode(false), extract_limit(ui64_C(-1))
   {
     for ( int i = 1; i < argc; ++i )
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
	       case 'd': list_mode = true; break;
	       case 'h': help_flag = true; break;

	       case 'l':
		 TEST_EXTRA_ARG(i, 'l');
		 extract_limit = Kumu::xabs(strtoll(argv[i], 0, 10));
		 break;

	       case 'p':
		 TEST_EXTRA_ARG(i, 'p');
		 prefix = argv[i];
		 break;

	       case 's':
		 TEST_EXTRA_ARG(i, 's');
		 suffix = argv[i];
		 break;

	       case 'u': unwrap_mode = true; break;
	       case 'U': unwrap_mode = false; break;
	       case 'V': version_flag = true; break;
	       case 'v': verbose_flag = true; break;

	       default:
		 fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		 return;
	       }
	   }
	 else if ( argv[i][0] != '-' )
	   {
	     if ( ! target_ul.HasValue() )
	       {
		 if ( ! target_ul.DecodeHex(argv[i]) )
		   {
		     const ASDCP::MDDEntry *e = dict.FindSymbol(argv[i]);

		     if ( e != 0 )
		       target_ul = e->ul;
		   }

		 if ( ! target_ul.HasValue() )
		   {
		     fprintf(stderr, "Value is not a UL or valid object name: %s\n", argv[i]);
		     return;
		   }
	       }
	     else
	       {
		 inFileList.push_back(argv[i]);
	       }
	   }
	 else
	   {
	     fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
	     return;
	   }
       }

     if ( help_flag || version_flag )
       return;
     
     if ( ! list_mode )
       {
	 if ( inFileList.empty() )
	   {
	     fputs("Input filename(s) required.\n", stderr);
	     return;
	   }
     
	 if ( ! target_ul.HasValue() )
	   {
	     fputs("Packet UL not set.  Use %s -u <ul> or keyword.\n", stderr);
	     return;
	   }
       }

     error_flag = false;
   }
 };


//---------------------------------------------------------------------------------------------------
//

int
main(int argc, const char** argv)
{
  const Dictionary *dict = &DefaultCompositeDict();
  CommandOptions Options(argc, argv, *dict);

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

  if ( Options.list_mode )
    {
      DefaultLogSink().UnsetFilterFlag(Kumu::LOG_ALLOW_WARN);
      char buf[64];

      MDD_t di = (MDD_t)0;
      while ( di < MDD_Max )
	{
	  const MDDEntry& e = dict->Type(di);

	  if ( e.name != 0  && ( e.ul[4] == 1 || e.ul[4] == 2 ) )
	    {
	      if ( Options.verbose_flag )
		{
		  UL tmp_ul(e.ul);
		  printf("%s %s\n", tmp_ul.EncodeString(buf, 64), e.name);
		}
	      else
		{
		  printf("%s\n", e.name);
		}
	    }

	  di = (MDD_t)(di + 1);
	}

      return 0;
    }

  Result_t result = RESULT_OK;
  FileList_t::iterator fi;

  for ( fi = Options.inFileList.begin(); KM_SUCCESS(result) && fi != Options.inFileList.end(); ++fi )
    {
      if ( Options.verbose_flag )
	fprintf(stderr, "Opening file %s\n", (fi->c_str()));
      
      std::string this_prefix =  Options.prefix.empty() ? Kumu::PathSetExtension(*fi, "") + "_" : Options.prefix;
      Kumu::FileReader reader;
      KLVFilePacket packet;
      char filename_buf[1024];
      ui64_t item_counter = 0;

      result = reader.OpenRead(fi->c_str());
	  
      if ( KM_SUCCESS(result) )
	result = packet.InitFromFile(reader);
	  
      while ( KM_SUCCESS(result) && item_counter < Options.extract_limit )
	{
	  if ( packet.GetUL() == Options.target_ul
	       || packet.GetUL().MatchIgnoreStream(Options.target_ul) )
	    {
	      snprintf(filename_buf, 1024, "%s%010qu%s", this_prefix.c_str(), item_counter, Options.suffix.c_str());

	      if ( Options.verbose_flag )
		fprintf(stderr, "%s (%llu bytes)\n", filename_buf, packet.ValueLength());

	      Kumu::FileWriter writer;
	      writer.OpenWrite(filename_buf);

	      if ( KM_SUCCESS(result) )
		{
		  if ( Options.unwrap_mode )
		    {
		      result = writer.Write(packet.m_Buffer.RoData() + packet.KLLength(), packet.ValueLength());
		    }
		  else
		    {
		      result = writer.Write(packet.m_Buffer.RoData(), packet.m_Buffer.Size());
		    }

		  ++item_counter;
		}
	    }

	  if ( KM_SUCCESS(result) )
	    result = packet.InitFromFile(reader);
	}
	  
      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

  if ( KM_FAILURE(result) )
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
