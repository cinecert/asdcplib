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
/*! \file    wavesplit.cpp
    \version $Id$
    \brief   WAV file splitter
*/

#include <AS_DCP.h>
#include <WavFileWriter.h>
#include <assert.h>

using namespace ASDCP;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "wavesplit";    // program name for messages

// Macros used to test command option data state.

// True if a major mode has already been selected.
#define TEST_MAJOR_MODE()     ( create_flag || info_flag )

// Causes the caller to return if a major mode has already been selected,
// otherwise sets the given flag.
#define TEST_SET_MAJOR_MODE(f) if ( TEST_MAJOR_MODE() ) \
                                 { \
                                   fputs("Conflicting major mode, choose one of -(ic).\n", stderr); \
                                   return; \
                                 } \
                                 (f) = true;

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
Copyright (c) 2005-2009 John Hurst\n\n\
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
USAGE: %s [-v] -V\n\
       %s [-v] -c <root-name> [-d <duration>] [-f <start-frame>] <filename>\n\
       %s [-v] -i <filename>\n\
\n\
Major modes:\n\
  -c <root-name>  - Create a WAV file for each channel in the input file\n\
                    (default is two-channel files)\n\
  -d <duration>   - Number of frames to process, default all\n\
  -f <frame-num>  - Starting frame number, default 0\n\
  -h              - Show help\n\
  -i              - Show input file metadata (no output created)\n\
  -V              - Show version\n\
  -v              - Print extra info while processing\n\
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
  bool   error_flag;     // true if the given options are in error or not complete
  bool   create_flag;    // true if the file create mode was selected
  bool   info_flag;      // true if the file info mode was selected
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   verbose_flag;   // true for extra info during procesing
  ui32_t start_frame;    // frame number to begin processing
  ui32_t duration;       // number of frames to be processed
  const char* file_root; // filename prefix for files written by the extract mode
  const char* filename;  // filename to be processed

  CommandOptions(int argc, const char** argv) :
    error_flag(true), create_flag(false), info_flag(false),
    version_flag(false), help_flag(false), start_frame(0),
    duration(0xffffffff), file_root(0), filename(0)
  {
    for ( int i = 1; i < argc; i++ )
      {
	if ( argv[i][0] == '-' && isalpha(argv[i][1]) && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'c':
		TEST_SET_MAJOR_MODE(create_flag);
		TEST_EXTRA_ARG(i, 'c');
		file_root = argv[i];
		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'f':
		TEST_EXTRA_ARG(i, 'f');
		start_frame = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'h': help_flag = true; break;
	      case 'i': TEST_SET_MAJOR_MODE(info_flag); break;
	      case 'V': version_flag = true; break;

	      default:
		fprintf(stderr, "Unrecognized option: %c\n", argv[i][1]);
		return;
	      }
	  }
	else
	  {
	    if ( filename )
	      {
		fprintf(stderr, "Unexpected extra filename.\n");
		return;
	      }

	    filename = argv[i];
	  }
      }

    if ( TEST_MAJOR_MODE() )
      {
	if ( filename == 0 )
	  {
	    fputs("Input filename required.\n", stderr);
	    return;
	  }
      }

    if ( ! TEST_MAJOR_MODE() && ! help_flag && ! version_flag )
      {
	fputs("No operation selected (use one of -(ic) or -h for help).\n", stderr);
	return;
      }

    error_flag = false;
  }
};

//
Result_t
wav_file_info(CommandOptions& Options)
{
  PCM::AudioDescriptor ADesc;
  Rational         PictureRate = EditRate_24;
  PCM::WAVParser Parser;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filename, PictureRate);

  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillAudioDescriptor(ADesc);
      ADesc.EditRate = PictureRate;
      fprintf(stderr, "48Khz PCM Audio, %s fps (%u spf)\n", "24",
	      PCM::CalcSamplesPerFrame(ADesc));
      fputs("AudioDescriptor:\n", stderr);
      PCM::AudioDescriptorDump(ADesc);
    }

  return result;
}

//
void
split_buffer(ui32_t sample_size, PCM::FrameBuffer& FrameBuffer,
	     PCM::FrameBuffer& L_FrameBuffer, PCM::FrameBuffer& R_FrameBuffer)
{
  assert((FrameBuffer.Size() % 2) == 0);
  byte_t* p = FrameBuffer.Data();
  byte_t* end_p = p + FrameBuffer.Size();
  byte_t* lp = L_FrameBuffer.Data();
  byte_t* rp = R_FrameBuffer.Data();

  for ( ; p < end_p; )
    {
      memcpy(lp, p, sample_size);
      lp += sample_size;
      p += sample_size;
      memcpy(rp, p, sample_size);
      rp += sample_size;
      p += sample_size;
    }

  L_FrameBuffer.Size(L_FrameBuffer.Capacity());
  R_FrameBuffer.Size(R_FrameBuffer.Capacity());
}

//
Result_t
split_wav_file(CommandOptions& Options)
{
  PCM::FrameBuffer FrameBuffer;
  PCM::FrameBuffer L_FrameBuffer;
  PCM::FrameBuffer R_FrameBuffer;
  PCM::AudioDescriptor ADesc;
  Rational         PictureRate = EditRate_24;
  PCM::WAVParser Parser;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filename, PictureRate);

  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillAudioDescriptor(ADesc);

      ADesc.EditRate = PictureRate;
      ui32_t fb_size = PCM::CalcFrameBufferSize(ADesc);
      assert((fb_size % 2) == 0);
      FrameBuffer.Capacity(fb_size);
      L_FrameBuffer.Capacity(fb_size/2);
      R_FrameBuffer.Capacity(fb_size/2);

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "48Khz PCM Audio, %s fps (%u spf)\n", "24",
		  PCM::CalcSamplesPerFrame(ADesc));
	  fputs("AudioDescriptor:\n", stderr);
	  PCM::AudioDescriptorDump(ADesc);
	}

      ADesc.ChannelCount = 1;
    }

  // set up output files
  Kumu::FileWriter L_OutFile;
  Kumu::FileWriter R_OutFile;

  if ( ASDCP_SUCCESS(result) )
    {
      char filename[256];
      snprintf(filename, 256, "%s_l.wav", Options.file_root);
      result = L_OutFile.OpenWrite(filename);

      if ( ASDCP_SUCCESS(result) )
	{
	  snprintf(filename, 256, "%s_r.wav", Options.file_root);
	  result = R_OutFile.OpenWrite(filename);
	}
    }


  if ( ASDCP_SUCCESS(result) )
    {
      Wav::SimpleWaveHeader WavHeader(ADesc);
      result = WavHeader.WriteToFile(L_OutFile);

      if ( ASDCP_SUCCESS(result) )
	result = WavHeader.WriteToFile(R_OutFile);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      ui32_t duration = 0;

      while ( ASDCP_SUCCESS(result) && (duration++ < Options.duration) )
	{
	  result = Parser.ReadFrame(FrameBuffer);

	  if ( FrameBuffer.Size() != FrameBuffer.Capacity() )
	    {
	      fprintf(stderr, "WARNING: Last frame read was short, PCM input is possibly not frame aligned.\n");
	      fprintf(stderr, "Expecting %u bytes, got %u.\n", FrameBuffer.Capacity(), FrameBuffer.Size());
	      result = RESULT_ENDOFFILE;
	      continue;
	    }

	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      split_buffer(PCM::CalcSampleSize(ADesc), FrameBuffer, L_FrameBuffer, R_FrameBuffer);
	      result = L_OutFile.Write(L_FrameBuffer.Data(), L_FrameBuffer.Size(), &write_count);

	      if ( ASDCP_SUCCESS(result) )
		result = R_OutFile.Write(R_FrameBuffer.Data(), R_FrameBuffer.Size(), &write_count);
	    }
	}

      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;

      if ( ASDCP_SUCCESS(result) )
	{
	  ADesc.ContainerDuration = duration;
	  Wav::SimpleWaveHeader WavHeader(ADesc);
	  L_OutFile.Seek();

	  if ( ASDCP_SUCCESS(result) )
	    result = R_OutFile.Seek();

	  if ( ASDCP_SUCCESS(result) )
	    result = WavHeader.WriteToFile(L_OutFile);

	  if ( ASDCP_SUCCESS(result) )
	    result = WavHeader.WriteToFile(R_OutFile);
	}
    }

  return RESULT_OK;
}


//
int
main(int argc, const char** argv)
{
  Result_t result = RESULT_OK;
  CommandOptions Options(argc, argv);

  if ( Options.help_flag )
    {
      usage();
      return 0;
    }

  if ( Options.error_flag )
    return 3;

  if ( Options.version_flag )
    banner();

  if ( Options.info_flag )
    result = wav_file_info(Options);

  else if ( Options.create_flag )
    result = split_wav_file(Options);

  if ( result != RESULT_OK )
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
