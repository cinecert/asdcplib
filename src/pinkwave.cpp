/*
Copyright (c) 2015, John Hurst
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
/*! \file    wavsplit.cpp
    \version $Id$
    \brief   Black WAV file generator
*/

#include "Wav.h"
#include "ST2095_PinkNoise.h"
#include <assert.h>

using namespace ASDCP;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "pinkwave";    // program name for messages

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
Copyright (c) 2015 John Hurst\n\n\
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
USAGE: %s [-v|-h[-d]] <filename>\n\
\n\
  -V              - Show version\n\
  -h              - Show help\n\
  -d <duration>   - Number of edit units to process, default 1440\n\
  -9              - Make a 96 kHz file (default 48 kHz)\n\
\n\
Other Options:\n\
  -v              - Verbose, show extra detail during run\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
\n", PROGRAM_NAME);
}

//
//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   verbose_flag;   // true if the verbose option was selected
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   s96_flag;       // true if the samples should be at 96 kHz
  ui32_t duration;       // number of frames to be processed
  float  HpFc;           // Highpass filter cutoff frequency in Hz
  float  LpFc;           // Lowpass filter cutoff frequency in Hz
  const char* filename;  //

  CommandOptions(int argc, const char** argv) :
    error_flag(true), verbose_flag(false), version_flag(false), help_flag(false), s96_flag(false),
    duration(1440), HpFc(PinkFilterHighPassConstant), LpFc(PinkFilterLowPassConstant), filename(0)
  {
    for ( int i = 1; i < argc; i++ )
      {
	if ( argv[i][0] == '-' && ( isalpha(argv[i][1]) || isdigit(argv[i][1]) ) && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'V': version_flag = true; break;
	      case 'h': help_flag = true; break;
	      case 'v': verbose_flag = true; break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case '9':
		s96_flag = true;
		break;

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

    if ( filename == 0 )
      {
	fputs("Output filename required.\n", stderr);
	return;
      }

    error_flag = false;
  }
};

// 
//
Result_t
make_pink_wav_file(CommandOptions& Options)
{
  PCM::FrameBuffer FrameBuffer;
  PCM::AudioDescriptor ADesc;

  ADesc.EditRate = Rational(24,1);
  ADesc.AudioSamplingRate = Options.s96_flag ? ASDCP::SampleRate_96k : ASDCP::SampleRate_48k;
  ADesc.Locked = 0;
  ADesc.ChannelCount = 1;
  ADesc.QuantizationBits = 24;
  ADesc.BlockAlign = 3;
  ADesc.AvgBps = ADesc.BlockAlign * ADesc.AudioSamplingRate.Quotient();
  ADesc.LinkedTrackID = 1;
  ADesc.ContainerDuration = Options.duration;

  // set up LCG and pink filter
  PinkFilter pink_filter(Options.s96_flag ? ASDCP::SampleRate_96k.Numerator : ASDCP::SampleRate_48k.Numerator,
			 Options.HpFc, Options.LpFc);

  LinearCongruentialGenerator lcg(Options.s96_flag ? ASDCP::SampleRate_96k.Numerator : ASDCP::SampleRate_48k.Numerator);


  FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));
  FrameBuffer.Size(FrameBuffer.Capacity());
  ui32_t samples_per_frame = PCM::CalcSamplesPerFrame(ADesc);

  if ( Options.verbose_flag )
    {
      fprintf(stderr, "%s kHz PCM Audio, 24 fps (%u spf)\n",
	      (Options.s96_flag?"96":"48"), samples_per_frame);
      fputs("AudioDescriptor:\n", stderr);
      PCM::AudioDescriptorDump(ADesc);
    }

  // set up output file
  Kumu::FileWriter OutFile;
  Result_t result = OutFile.OpenWrite(Options.filename);

  if ( ASDCP_SUCCESS(result) )
    {
       RF64::SimpleRF64Header WavHeader(ADesc);
       result = WavHeader.WriteToFile(OutFile);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t write_count = 0;
      ui32_t duration = 0;
      byte_t scaled_pink[sizeof(ui32_t)];

      while ( ASDCP_SUCCESS(result) && (duration++ < Options.duration) )
	{
	  // fill the frame buffer with a frame of pink noise
	  byte_t *p = FrameBuffer.Data();

	  for ( int i = 0; i < samples_per_frame; ++i )
	    {
	      float pink_sample = pink_filter.GetNextSample(lcg.GetNextSample());
	      ScalePackSample(pink_sample, p, ADesc.BlockAlign);
	      p += ADesc.BlockAlign;
	    }

	  result = OutFile.Write(FrameBuffer.RoData(), FrameBuffer.Size(), &write_count);
	}
    }

  return result;
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

  else
    result = make_pink_wav_file(Options);

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
// end pinkwave.cpp
//
