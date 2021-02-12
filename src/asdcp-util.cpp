/*
Copyright (c) 2003-2012, John Hurst
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
/*! \file    asdcp-util.cpp
    \version $Id$       
    \brief   Utility functions for working with AS-DCP files

  This program provides utility features commonly useful in DCP workflows.

  For more information about asdcplib, please refer to the header file AS_DCP.h
*/

#include <KM_fileio.h>
#include <KM_prng.h>
#include <KM_sha1.h>
#include <AS_DCP.h>

using namespace Kumu;


//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "asdcp-util";  // program name for messages

// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)						\
  if ( ++i >= argc || argv[(i)][0] == '-' ) {				\
    fprintf(stderr, "Argument not found for option -%c.\n", (c));	\
    return;								\
  }

//
void
banner(FILE* stream = stdout)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2003-2012 John Hurst\n\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, ASDCP::Version(), PROGRAM_NAME);
}

//
void
usage(FILE* stream = stdout)
{
  fprintf(stream, "\
USAGE: %s [-h|-help] [-V]\n\
\n\
       %s -d <input-file>\n\
\n\
       %s -g | -u\n",
	  PROGRAM_NAME, PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Major modes:\n\
  -d                - Calculate message digest of input file\n\
  -g                - Generate a random 16 byte value to stdout\n\
  -h | -help        - Show help\n\
  -u                - Generate a random UUID value to stdout\n\
  -V                - Show version information\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\n");
}

//
enum MajorMode_t
{
  MMT_NONE,
  MMT_GEN_ID,
  MMT_GEN_KEY,
  MMT_DIGEST,
};

//
class CommandOptions
{
  CommandOptions();

public:
  MajorMode_t mode;
  bool   error_flag;     // true if the given options are in error or not complete
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  PathList_t filenames;  // list of filenames to be processed

  //
  CommandOptions(int argc, const char** argv) :
    mode(MMT_NONE), error_flag(true), version_flag(false), help_flag(false)
  {
    for ( int i = 1; i < argc; ++i )
      {

	if ( (strcmp( argv[i], "-help") == 0) )
	  {
	    help_flag = true;
	    continue;
	  }
         
	if ( argv[i][0] == '-'
	     && ( isalpha(argv[i][1]) || isdigit(argv[i][1]) )
	     && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'd': mode = MMT_DIGEST; break;
	      case 'g': mode = MMT_GEN_KEY; break;
	      case 'h': help_flag = true; break;
	      case 'u':	mode = MMT_GEN_ID; break;
	      case 'V': version_flag = true; break;

	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
	else
	  {
	    if ( argv[i][0] != '-' )
	      {
		filenames.push_back(argv[i]);
	      }
	    else
	      {
		fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
		return;
	      }
	  }
      }

    if ( help_flag || version_flag )
      return;
    
    if ( ( mode == MMT_DIGEST ) && filenames.empty() )
      {
	fputs("Option requires at least one filename argument.\n", stderr);
	return;
      }

    if ( mode == MMT_NONE && ! help_flag && ! version_flag )
      {
	fputs("No operation selected (use one of -[dgu] or -h for help).\n", stderr);
	return;
      }

    error_flag = false;
  }
};

//
Result_t
digest_file(const std::string& filename)
{
  FileReader Reader;
  SHA1_CTX Ctx;
  SHA1_Init(&Ctx);
  ByteString Buf(8192);

  Result_t result = Reader.OpenRead(filename.c_str());

  while ( ASDCP_SUCCESS(result) )
    {
      ui32_t read_count = 0;
      result = Reader.Read(Buf.Data(), Buf.Capacity(), &read_count);

      if ( result == RESULT_ENDOFFILE )
	{
	  result = RESULT_OK;
	  break;
	}

      if ( ASDCP_SUCCESS(result) )
	SHA1_Update(&Ctx, Buf.Data(), read_count);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      const ui32_t sha_len = 20;
      byte_t bin_buf[sha_len];
      char sha_buf[64];
      SHA1_Final(bin_buf, &Ctx);

      fprintf(stdout, "%s %s\n",
	      base64encode(bin_buf, sha_len, sha_buf, 64),
	      filename.c_str());
    }

  return result;
}

//
int
main(int argc, const char** argv)
{
  Result_t result = RESULT_OK;
  char     str_buf[64];
  CommandOptions Options(argc, argv);

  if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.version_flag || Options.help_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n",
	      PROGRAM_NAME);
      return 3;
    }

  if ( Options.mode == MMT_GEN_KEY )
    {
      SymmetricKey key_value;
      Kumu::GenRandomValue(key_value);
      printf("%s\n", key_value.EncodeHex(str_buf, 64));
    }
  else if ( Options.mode == MMT_GEN_ID )
    {
      UUID id_value;
      Kumu::GenRandomValue(id_value);
      printf("%s\n", id_value.EncodeHex(str_buf, 64));
    }
  else if ( Options.mode == MMT_DIGEST )
    {
      PathList_t::iterator i;

      for ( i = Options.filenames.begin();
	    i != Options.filenames.end() && ASDCP_SUCCESS(result); ++i )
	result = digest_file(*i);
    }
  else
    {
      fprintf(stderr, "Unhandled mode: %d.\n", Options.mode);
      return 6;
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
// end asdcp-util.cpp
//
