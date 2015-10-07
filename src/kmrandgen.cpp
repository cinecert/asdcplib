/*
Copyright (c) 2005-2009, John Hurst
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
  /*! \file    kmrandgen.cpp
    \version $Id$
    \brief   psuedo-random number generation utility
  */

#include "AS_DCP.h"
#include <KM_fileio.h>
#include <KM_prng.h>
#include <ctype.h>

using namespace Kumu;

const ui32_t RandBlockSize = 16;
const char* PROGRAM_NAME = "kmrandgen";

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
Copyright (c) 2003-2009 John Hurst\n\n\
%s is part of the asdcp DCP tools package.\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PROGRAM_NAME, Kumu::Version(), PROGRAM_NAME, PROGRAM_NAME);
}

//
void
usage(FILE* stream = stdout)
{
  fprintf(stream, "\
USAGE: %s [-b|-B|-c|-x] [-n] [-s <size>] [-v]\n\
\n\
       %s [-h|-help] [-V]\n\
\n\
  -b          - Output a stream of binary data\n\
  -B          - Output a Base64 string\n\
  -c          - Output a C-language struct containing the values\n\
  -h | -help  - Show help\n\
  -n          - Suppress newlines\n\
  -s <size>   - Number of random bytes to generate (default 32)\n\
  -v          - Verbose. Prints informative messages to stderr\n\
  -V          - Show version information\n\
  -x          - Output hexadecimal (default)\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME, PROGRAM_NAME);
}

enum OutputFormat_t {
  OF_HEX,
  OF_BINARY,
  OF_BASE64,
  OF_CSTRUCT
};

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;      // true if the given options are in error or not complete
  bool   no_newline_flag; // 
  bool   verbose_flag;    // true if the verbose option was selected
  bool   version_flag;    // true if the version display option was selected
  bool   help_flag;       // true if the help display option was selected
  OutputFormat_t format;  // 
  ui32_t request_size;

 //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), no_newline_flag(false), verbose_flag(false),
    version_flag(false), help_flag(false), format(OF_HEX), request_size(RandBlockSize*2)
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
	      case 'b': format = OF_BINARY; break;
	      case 'B': format = OF_BASE64; break;
	      case 'c': format = OF_CSTRUCT; break;
	      case 'n': no_newline_flag = true; break;
	      case 'h': help_flag = true; break;

	      case 's':
		TEST_EXTRA_ARG(i, 's');
		request_size = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'v': verbose_flag = true; break;
	      case 'V': version_flag = true; break;
	      case 'x': format = OF_HEX; break;

	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
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

    if ( request_size == 0 )
      {
	fprintf(stderr, "Please use a non-zero request size\n");
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

  FortunaRNG    RandGen;
  ByteString    Buf(Kumu::Kilobyte);

  if ( Options.verbose_flag )
    fprintf(stderr, "Generating %d random byte%s.\n", Options.request_size, (Options.request_size == 1 ? "" : "s"));

  if ( Options.format == OF_BINARY )
    {
      if ( KM_FAILURE(Buf.Capacity(Options.request_size)) )
	{
	  fprintf(stderr, "randbuf: %s\n", RESULT_ALLOC.Label());
	  return 1;
	}

      RandGen.FillRandom(Buf.Data(), Options.request_size);
      fwrite((byte_t*)Buf.Data(), 1, Options.request_size, stdout);
    }
  else if ( Options.format == OF_CSTRUCT )
    {
      ui32_t line_count = 0;
      byte_t* p = Buf.Data();
      printf("byte_t rand_buf[%u] = {\n", Options.request_size);

      if ( Options.request_size > 128 )
	fputs("  // 0x00000000\n", stdout);

      while ( Options.request_size > 0 )
	{
	  if ( line_count > 0 && (line_count % (RandBlockSize*8)) == 0 )
	    fprintf(stdout, "  // 0x%08x\n", line_count);

	  RandGen.FillRandom(p, RandBlockSize);
	  fputc(' ', stdout);

	  for ( ui32_t i = 0; i < RandBlockSize && Options.request_size > 0; i++, Options.request_size-- )
	    printf(" 0x%02x,", p[i]);

	  fputc('\n', stdout);
	  line_count += RandBlockSize;
	}

      fputs("};", stdout);

      if ( ! Options.no_newline_flag )
	fputc('\n', stdout);
    }
  else if ( Options.format == OF_BASE64 )
    {
      if ( KM_FAILURE(Buf.Capacity(Options.request_size)) )
	{
	  fprintf(stderr, "randbuf: %s\n", RESULT_ALLOC.Label());
	  return 1;
	}

      ByteString Strbuf;
      ui32_t e_len = base64_encode_length(Options.request_size) + 1;

      if ( KM_FAILURE(Strbuf.Capacity(e_len)) )
        {
          fprintf(stderr, "strbuf: %s\n", RESULT_ALLOC.Label());
          return 1;
        }

      RandGen.FillRandom(Buf.Data(), Options.request_size);

      if ( base64encode(Buf.RoData(), Options.request_size, (char*)Strbuf.Data(), Strbuf.Capacity()) == 0 )
	{
          fprintf(stderr, "encode error\n");
          return 2;
        } 

      fputs((const char*)Strbuf.RoData(), stdout);

      if ( ! Options.no_newline_flag )
	fputs("\n", stdout);
    }
  else // OF_HEX
    {
      byte_t* p = Buf.Data();
      char hex_buf[64];

      while ( Options.request_size > 0 )
	{
	  ui32_t x_len = xmin(Options.request_size, RandBlockSize);
	  RandGen.FillRandom(p, RandBlockSize);
	  bin2hex(p, x_len, hex_buf, 64);
          fputs(hex_buf, stdout);

          if ( ! Options.no_newline_flag )
            fputc('\n', stdout);

	  Options.request_size -= x_len;
        }

    }

  return 0;
}


//
// end kmrandgen.cpp
//
