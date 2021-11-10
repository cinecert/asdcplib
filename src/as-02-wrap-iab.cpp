/*
Copyright (c) 2021 John Hurst

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
/*! \file    as-02-wrap-iab.cpp
    \version $Id$       
    \brief   AS-02 file manipulation utility

  This program wraps IAB sound essence into an AS-02 MXF file.

  For more information about AS-02, please refer to the header file AS_02.h
  For more information about asdcplib, please refer to the header file AS_DCP.h
*/
#include <KM_fileio.h>
#include <AS_02.h>
#include "AS_02_IAB.h"
#include <Metadata.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;
const ASDCP::Dictionary *g_dict = 0;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "as-02-wrap";  // program name for messages

// local program identification info written to file headers
class MyInfo : public WriterInfo
{
public:
  MyInfo()
  {
      static byte_t default_ProductUUID_Data[UUIDlen] =
      { 0x13, 0x54, 0x4d, 0xec, 0x47, 0xf5, 0x49, 0xf4,
        0xb7, 0xb7, 0x83, 0x49, 0x61, 0xa5, 0x49, 0x4e
      };
      
      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      CompanyName = "WidgetCo";
      ProductName = "as-02-wrap-iab";
      ProductVersion = ASDCP::Version();
  }
} s_MyInfo;



// Increment the iterator, test for an additional non-option command line argument.
// Causes the caller to return if there are no remaining arguments or if the next
// argument begins with '-'.
#define TEST_EXTRA_ARG(i,c)						\
  if ( ++i >= argc || argv[(i)][0] == '-' ) {				\
    fprintf(stderr, "Argument not found for option -%c.\n", (c));	\
    return;								\
  }

//
static void
create_random_uuid(byte_t* uuidbuf)
{
  Kumu::UUID tmp_id;
  GenRandomValue(tmp_id);
  memcpy(uuidbuf, tmp_id.Value(), tmp_id.Size());
}

//
void
banner(FILE* stream = stdout)
{
  fprintf(stream, "\n\
%s (asdcplib %s)\n\n\
Copyright (c) 2021, John Hurst\n\n\
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
       %s [options] <input-directory> <output-file>\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -h | -help        - Show help\n\
  -V                - Show version information\n\
  -a <uuid>         - Specify the Asset ID of the file\n\
  -g <rfc-5646-code>\n\
                    - Create an MCA label having the given RFC 5646 language code\n\
  -r <n>/<d>        - Edit Rate of the output file.  24/1 is the default\n\
  -v                - Verbose, prints informative messages to stderr\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\n");
}

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   asset_id_flag;  // true if an asset ID was given
  bool   verbose_flag;   // true if the verbose option was selected
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  Rational edit_rate;    // edit rate of JP2K sequence
  ui32_t fb_size;        // size of picture frame buffer
  byte_t asset_id_value[UUIDlen];// value of asset ID (when asset_id_flag is true)
  Kumu::PathList_t filenames;  // list of filenames to be processed
  std::string language;
  std::string out_file;

  CommandOptions(int argc, const char** argv) :
    error_flag(true), asset_id_flag(false),
    verbose_flag(false), version_flag(false), help_flag(false),
    edit_rate(24,1), fb_size(FRAME_BUFFER_SIZE)
  {
    for ( int i = 1; i < argc; i++ )
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
	      case 'a':
		asset_id_flag = true;
		TEST_EXTRA_ARG(i, 'a');
		{
		  ui32_t length;
		  Kumu::hex2bin(argv[i], asset_id_value, UUIDlen, &length);

		  if ( length != UUIDlen )
		    {
		      fprintf(stderr, "Unexpected asset ID length: %u, expecting %u characters.\n", length, UUIDlen);
		      return;
		    }
		}
		break;

	      case 'g':
		TEST_EXTRA_ARG(i, 'g');
		language = argv[i];
		break;

	      case 'h': help_flag = true; break;
	    
	      case 'r':
		TEST_EXTRA_ARG(i, 'r');
		if ( ! DecodeRational(argv[i], edit_rate) )
		  {
		    fprintf(stderr, "Error decoding edit rate value: %s\n", argv[i]);
		    return;
		  }
		
		break;

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
      {
	return;
      }

    if ( filenames.size() != 2 )
      {
	fputs("Two arguments are required: <input-directory> <output-file>\n", stderr);
	return;
      }

    out_file = filenames.back();
    filenames.pop_back();

    error_flag = false;
  }
};


// Write an IAB audio stream into an AS-02 file.
//
Result_t
write_IAB_file(CommandOptions& Options)
{
  DCData::SequenceParser Parser;
  AS_02::IAB::MXFWriter Writer;
  DCData::FrameBuffer FrameBuffer(Options.fb_size);
  std::vector<ASDCP::UL> conforms_to_spec;
  conforms_to_spec.push_back(g_dict->ul(MDD_IMF_IABTrackFileLevel0));
  ASDCP::MXF::IABSoundfieldLabelSubDescriptor iab_subdescr(g_dict);
  iab_subdescr.RFC5646SpokenLanguage = Options.language;
 
  // set up essence parser
  assert(Options.filenames.size() == 1);
  Result_t result = Parser.OpenRead(Options.filenames.front());

  Kumu::PathList_t::const_iterator i;

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      Info.LabelSetType = LS_MXF_SMPTE;

      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
	Kumu::GenRandomUUID(Info.AssetUUID);

      if ( ASDCP_SUCCESS(result) )
	{
	  result = Writer.OpenWrite(Options.out_file,
                                    Info,
                                    iab_subdescr,
				    conforms_to_spec,
                                    Options.edit_rate);
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      result = Parser.Reset();

      while ( ASDCP_SUCCESS(result) )
	{
	  result = Parser.ReadFrame(FrameBuffer);

	  if ( ASDCP_SUCCESS(result) )
	    {
              result = Writer.WriteFrame(FrameBuffer);
            }
	}

      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      result = Writer.Finalize();
    }

  return result;
}


//
int
main(int argc, const char** argv)
{
  g_dict = &ASDCP::DefaultSMPTEDict();
  assert(g_dict);

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

  Result_t result = write_IAB_file(Options);      

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
// end as-02-wrap-iab.cpp
//
