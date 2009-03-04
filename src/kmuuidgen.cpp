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
  /*! \file    kmuuidgen.cpp
    \version $Id$
    \brief   UUID generation utility
  */

#include "AS_DCP.h"
#include <KM_util.h>
#include <ctype.h>


const char* PROGRAM_NAME = "kmuuidgen";

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
USAGE: %s [-c] [-n] [-v]\n\
\n\
       %s [-h|-help] [-V]\n\
\n\
  -c          - Output a C-language struct containing the value\n\
  -h | -help  - Show help\n\
  -n          - Suppress the newline\n\
  -v          - Verbose. Prints informative messages to stderr\n\
  -V          - Show version information\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME, PROGRAM_NAME);
}

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;      // true if the given options are in error or not complete
  bool   no_newline_flag; //
  bool   c_array_flag;    //
  bool   version_flag;    // true if the version display option was selected
  bool   help_flag;       // true if the help display option was selected
  bool   verbose_flag;    // true if the verbose flag was selected

 //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), no_newline_flag(false), c_array_flag(false), version_flag(false),
    help_flag(false), verbose_flag(false)
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
	      case 'c': c_array_flag = true; break;
	      case 'n': no_newline_flag = true; break;
	      case 'h': help_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'V': version_flag = true; break;

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

  Kumu::UUID UUID;
  Kumu::GenRandomValue(UUID);
  char uuid_buf[40];

  if ( Options.c_array_flag )
    {
      const byte_t* p = UUID.Value();

      printf("\
byte_t uuid_buf[] = {\n\
  // %s\n ",
	     UUID.EncodeHex(uuid_buf, 40));
	  
      for ( ui32_t i = 0; i < 16; i++ )
	printf(" 0x%02x,", p[i]);

      printf("\n");
      printf("};\n");
      return 0;
    }
  else
    {
      fputs(UUID.EncodeHex(uuid_buf, 40), stdout);
    }

  if ( Options.no_newline_flag == 0 )
    printf("\n");

  return 0;
}


//
// end kmuuidgen.cpp
//
