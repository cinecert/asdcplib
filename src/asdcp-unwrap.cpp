/*
Copyright (c) 2003-2016, John Hurst
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
/*! \file    asdcp-unwrap.cpp
    \version $Id$
    \brief   AS-DCP file manipulation utility

  This program extracts picture, sound and text essence from AS-DCP files.

  For more information about asdcplib, please refer to the header file AS_DCP.h
*/

#include <KM_fileio.h>
#include <WavFileWriter.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "asdcp-unwrap";  // program name for messages

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
Copyright (c) 2003-2015 John Hurst\n\n\
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
       %s -G [-v] <input-file>\n\
\n\
       %s [-1|-2] [-3] [-b <buffer-size>] [-d <duration>]\n\
       [-f <starting-frame>] [-m] [-p <frame-rate>] [-R] [-s <size>] [-v] [-W]\n\
       [-w] <input-file> [<file-prefix>]\n\n",
	  PROGRAM_NAME, PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -1                - Split Wave essence to mono WAV files during extract.\n\
                      Default is multichannel WAV\n\
  -2                - Split Wave essence to stereo WAV files during extract.\n\
                      Default is multichannel WAV\n\
  -3                - Force stereoscopic interpretation of a JP2K file.\n\
  -b <buffer-size>  - Specify size in bytes of picture frame buffer\n\
                      Defaults to 4,194,304 (4MB)\n\
  -d <duration>     - Number of frames to process, default all\n\
  -e <extension>    - Extension to use for Unknown D-Cinema Data files. default dcdata\n\
  -f <start-frame>  - Starting frame number, default 0\n                \
  -G                - Perform GOP start lookup test on MXF+Interop MPEG file\n\
  -h | -help        - Show help\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -m                - verify HMAC values when reading\n\
  -p <rate>         - Alternative picture rate when unwrapping PCM:\n\
                      Use one of [23|24|25|30|48|50|60], 24 is default\n\
  -s <size>         - Number of bytes to dump to output when -v is given\n\
  -V                - Show version information\n\
  -v                - Verbose, prints informative messages to stderr\n\
  -W                - Read input file only, do not write destination file\n\
  -w <width>        - Width of numeric element in a series of frame file names\n\
                      (default 6)\n\
  -z                - Fail if j2c inputs have unequal parameters (default)\n\
  -Z                - Ignore unequal parameters in j2c inputs\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
         o An argument of \"23\" to the -p option will be interpreted\n\
           as 24000/1001 fps.\n\n");
}

//
enum MajorMode_t
{
  MMT_NONE,
  MMT_EXTRACT,
  MMT_GOP_START,
};

//
class CommandOptions
{
  CommandOptions();

public:
  MajorMode_t mode;
  bool   error_flag;     // true if the given options are in error or not complete
  bool   key_flag;       // true if an encryption key was given
  bool   read_hmac;      // true if HMAC values are to be validated
  bool   split_wav;      // true if PCM is to be extracted to stereo WAV files
  bool   mono_wav;       // true if PCM is to be extracted to mono WAV files
  bool   verbose_flag;   // true if the verbose option was selected
  ui32_t fb_dump_size;   // number of bytes of frame buffer to dump
  bool   no_write_flag;  // true if no output files are to be written
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   stereo_image_flag; // if true, expect stereoscopic JP2K input (left eye first)
  ui32_t number_width;   // number of digits in a serialized filename (for JPEG extract)
  ui32_t start_frame;    // frame number to begin processing
  ui32_t duration;       // number of frames to be processed
  bool   duration_flag;  // true if duration argument given
  bool   j2c_pedantic;   // passed to JP2K::SequenceParser::OpenRead
  ui32_t picture_rate;   // fps of picture when wrapping PCM
  ui32_t fb_size;        // size of picture frame buffer
  const char* file_prefix; // filename pre for files written by the extract mode
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  PCM::ChannelFormat_t channel_fmt; // audio channel arrangement
  const char* input_filename;
  std::string prefix_buffer;
  const char* extension; // file extension to use for unknown D-Cinema Data track files.

  //
  Rational PictureRate()
  {
    if ( picture_rate == 16 ) return EditRate_16;
    if ( picture_rate == 18 ) return EditRate_18;
    if ( picture_rate == 20 ) return EditRate_20;
    if ( picture_rate == 22 ) return EditRate_22;
    if ( picture_rate == 23 ) return EditRate_23_98;
    if ( picture_rate == 24 ) return EditRate_24;
    if ( picture_rate == 25 ) return EditRate_25;
    if ( picture_rate == 30 ) return EditRate_30;
    if ( picture_rate == 48 ) return EditRate_48;
    if ( picture_rate == 50 ) return EditRate_50;
    if ( picture_rate == 60 ) return EditRate_60;
    if ( picture_rate == 96 ) return EditRate_96;
    if ( picture_rate == 100 ) return EditRate_100;
    if ( picture_rate == 120 ) return EditRate_120;
    if ( picture_rate == 192 ) return EditRate_192;
    if ( picture_rate == 200 ) return EditRate_200;
    if ( picture_rate == 240 ) return EditRate_240;
    return EditRate_24;
  }

  //
  CommandOptions(int argc, const char** argv) :
    mode(MMT_EXTRACT), error_flag(true), key_flag(false), read_hmac(false), split_wav(false),
    mono_wav(false), verbose_flag(false), fb_dump_size(0), no_write_flag(false),
    version_flag(false), help_flag(false), stereo_image_flag(false), number_width(6),
    start_frame(0), duration(0xffffffff), duration_flag(false), j2c_pedantic(true),
    picture_rate(24), fb_size(FRAME_BUFFER_SIZE), file_prefix(0),
    channel_fmt(PCM::CF_NONE), input_filename(0), extension("dcdata")
  {
    memset(key_value, 0, KeyLen);
    memset(key_id_value, 0, UUIDlen);

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
	      case '1': mono_wav = true; break;
	      case '2': split_wav = true; break;
	      case '3': stereo_image_flag = true; break;

	      case 'b':
		TEST_EXTRA_ARG(i, 'b');
		fb_size = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration_flag = true;
		duration = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'e':
		TEST_EXTRA_ARG(i, 'e');
		extension = argv[i];
		break;

	      case 'f':
		TEST_EXTRA_ARG(i, 'f');
		start_frame = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'G': mode = MMT_GOP_START; break;
	      case 'h': help_flag = true; break;

	      case 'k': key_flag = true;
		TEST_EXTRA_ARG(i, 'k');
		{
		  ui32_t length;
		  Kumu::hex2bin(argv[i], key_value, KeyLen, &length);

		  if ( length != KeyLen )
		    {
		      fprintf(stderr, "Unexpected key length: %u, expecting %u characters.\n", length, KeyLen);
		      return;
		    }
		}
		break;

	      case 'm': read_hmac = true; break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		picture_rate = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 's':
		TEST_EXTRA_ARG(i, 's');
		fb_dump_size = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'V': version_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'W': no_write_flag = true; break;

	      case 'w':
		TEST_EXTRA_ARG(i, 'w');
		number_width = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'Z': j2c_pedantic = false; break;
	      case 'z': j2c_pedantic = true; break;

	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
	else
	  {
	    if ( argv[i][0] != '-' )
	      {
		if ( input_filename == 0 )
		  {
		    input_filename = argv[i];
		  }
		else if ( file_prefix == 0 )
		  {
		    file_prefix = argv[i];
		  }
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

    if ( (  mode == MMT_EXTRACT || mode == MMT_GOP_START ) && input_filename == 0 )
      {
	fputs("Option requires at least one filename argument.\n", stderr);
	return;
      }

    if ( mode == MMT_EXTRACT  && file_prefix == 0 )
      {
	prefix_buffer = Kumu::PathSetExtension(input_filename, "") + "_";
	file_prefix = prefix_buffer.c_str();
      }

    error_flag = false;
  }
};

//------------------------------------------------------------------------------------------
// MPEG2 essence

// Read a plaintext MPEG2 Video Elementary Stream from a plaintext ASDCP file
// Read a plaintext MPEG2 Video Elementary Stream from a ciphertext ASDCP file
// Read a ciphertext MPEG2 Video Elementary Stream from a ciphertext ASDCP file
//
Result_t
read_MPEG2_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  MPEG2::MXFReader   Reader;
  MPEG2::FrameBuffer FrameBuffer(Options.fb_size);
  Kumu::FileWriter   OutFile;
  ui32_t             frame_count = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      MPEG2::VideoDescriptor VDesc;
      Reader.FillVideoDescriptor(VDesc);
      frame_count = VDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  MPEG2::VideoDescriptorDump(VDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ( ! Options.no_write_flag ) )
    {
      char filename[256];
      snprintf(filename, 256, "%s.ves", Options.file_prefix);
      result = OutFile.OpenWrite(filename);
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  ui32_t last_frame = Options.start_frame + ( Options.duration ? Options.duration : frame_count);
  if ( last_frame > frame_count )
    last_frame = frame_count;

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);

	  if ( ! Options.no_write_flag )
	    {
	  ui32_t write_count = 0;
	  result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
	}
    }
    }

  return result;
}


//
Result_t
gop_start_test(CommandOptions& Options)
{
  using namespace ASDCP::MPEG2;

  MXFReader   Reader;
  MPEG2::FrameBuffer FrameBuffer(Options.fb_size);
  ui32_t      frame_count = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      MPEG2::VideoDescriptor VDesc;
      Reader.FillVideoDescriptor(VDesc);
      frame_count = VDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  MPEG2::VideoDescriptorDump(VDesc);
	}
    }

  ui32_t last_frame = Options.start_frame + ( Options.duration ? Options.duration : frame_count);
  if ( last_frame > frame_count )
    last_frame = frame_count;

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrameGOPStart(i, FrameBuffer);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);

	  if ( FrameBuffer.FrameType() != FRAME_I )
	    fprintf(stderr, "Expecting an I frame, got %c\n", FrameTypeChar(FrameBuffer.FrameType()));

	  fprintf(stderr, "Requested frame %u, got %u\n", i, FrameBuffer.FrameNumber());
	}
    }

  return result;
}

//------------------------------------------------------------------------------------------
// JPEG 2000 essence


// Read one or more plaintext JPEG 2000 stereoscopic codestream pairs from a plaintext ASDCP file
// Read one or more plaintext JPEG 2000 stereoscopic codestream pairs from a ciphertext ASDCP file
// Read one or more ciphertext JPEG 2000 stereoscopic codestream pairs from a ciphertext ASDCP file
Result_t
read_JP2K_S_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  JP2K::MXFSReader    Reader;
  JP2K::FrameBuffer  FrameBuffer(Options.fb_size);
  ui32_t             frame_count = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      JP2K::PictureDescriptor PDesc;
      Reader.FillPictureDescriptor(PDesc);

      frame_count = PDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  JP2K::PictureDescriptorDump(PDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  const int filename_max = 1024;
  char filename[filename_max];
  ui32_t last_frame = Options.start_frame + ( Options.duration ? Options.duration : frame_count);
  if ( last_frame > frame_count )
    last_frame = frame_count;

  char left_format[64];  char right_format[64];
  snprintf(left_format,  64, "%%s%%0%duL.j2c", Options.number_width);
  snprintf(right_format, 64, "%%s%%0%duR.j2c", Options.number_width);

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, JP2K::SP_LEFT, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( ! Options.no_write_flag )
	    {
	  Kumu::FileWriter OutFile;
	  ui32_t write_count;
	  snprintf(filename, filename_max, left_format, Options.file_prefix, i);
	  result = OutFile.OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
	    }

	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);
	}

      if ( ASDCP_SUCCESS(result) )
	result = Reader.ReadFrame(i, JP2K::SP_RIGHT, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( ! Options.no_write_flag )
	    {
	  Kumu::FileWriter OutFile;
	  ui32_t write_count;
	  snprintf(filename, filename_max, right_format, Options.file_prefix, i);
	  result = OutFile.OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
	}

	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);
    }
    }

  return result;
}

// Read one or more plaintext JPEG 2000 codestreams from a plaintext ASDCP file
// Read one or more plaintext JPEG 2000 codestreams from a ciphertext ASDCP file
// Read one or more ciphertext JPEG 2000 codestreams from a ciphertext ASDCP file
//
Result_t
read_JP2K_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  JP2K::MXFReader    Reader;
  JP2K::FrameBuffer  FrameBuffer(Options.fb_size);
  ui32_t             frame_count = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      JP2K::PictureDescriptor PDesc;
      Reader.FillPictureDescriptor(PDesc);

      frame_count = PDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  JP2K::PictureDescriptorDump(PDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  ui32_t last_frame = Options.start_frame + ( Options.duration ? Options.duration : frame_count);
  if ( last_frame > frame_count )
    last_frame = frame_count;

  char name_format[64];
  snprintf(name_format,  64, "%%s%%0%du.j2c", Options.number_width);

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( ! Options.no_write_flag )
	    {
	  Kumu::FileWriter OutFile;
	  char filename[256];
	  ui32_t write_count;
	  snprintf(filename, 256, name_format, Options.file_prefix, i);
	  result = OutFile.OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
	    }

	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);
	}
    }

  return result;
}

//------------------------------------------------------------------------------------------
// PCM essence

// Read one or more plaintext PCM audio streams from a plaintext ASDCP file
// Read one or more plaintext PCM audio streams from a ciphertext ASDCP file
// Read one or more ciphertext PCM audio streams from a ciphertext ASDCP file
//
Result_t
read_PCM_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  PCM::MXFReader     Reader;
  PCM::FrameBuffer   FrameBuffer;
  WavFileWriter      OutWave;
  PCM::AudioDescriptor ADesc;
  ui32_t last_frame = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      Reader.FillAudioDescriptor(ADesc);

      if ( ADesc.EditRate != EditRate_23_98
	   && ADesc.EditRate != EditRate_24
	   && ADesc.EditRate != EditRate_25
	   && ADesc.EditRate != EditRate_30
	   && ADesc.EditRate != EditRate_48
	   && ADesc.EditRate != EditRate_50
	   && ADesc.EditRate != EditRate_60 )
	ADesc.EditRate = Options.PictureRate();

      if ( Options.fb_size != FRAME_BUFFER_SIZE )
	{
	  FrameBuffer.Capacity(Options.fb_size);
	}
      else
	{
      FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));
	}

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	PCM::AudioDescriptorDump(ADesc);
    }
    }

  if ( ASDCP_SUCCESS(result) )
    {
      last_frame = ADesc.ContainerDuration;

      if ( Options.duration > 0 && Options.duration < last_frame )
	last_frame = Options.duration;

      if ( Options.start_frame > 0 )
	{
	  if ( Options.start_frame > ADesc.ContainerDuration )
	    {
	      fprintf(stderr, "Start value greater than file duration.\n");
	      return RESULT_FAIL;
	    }

	  last_frame = Kumu::xmin(Options.start_frame + last_frame, ADesc.ContainerDuration);
	}

      ADesc.ContainerDuration = last_frame - Options.start_frame;

      if ( ! Options.no_write_flag )
	{
      OutWave.OpenWrite(ADesc, Options.file_prefix,
			( Options.split_wav ? WavFileWriter::ST_STEREO :
			  ( Options.mono_wav ? WavFileWriter::ST_MONO : WavFileWriter::ST_NONE ) ));
    }
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);

	  if ( ! Options.no_write_flag )
	    {
	  result = OutWave.WriteFrame(FrameBuffer);
	}
    }
    }

  return result;
}


//------------------------------------------------------------------------------------------
// TimedText essence

// Read one or more timed text streams from a plaintext ASDCP file
// Read one or more timed text streams from a ciphertext ASDCP file
// Read one or more timed text streams from a ciphertext ASDCP file
//
Result_t
read_timed_text_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  TimedText::MXFReader     Reader;
  TimedText::FrameBuffer   FrameBuffer;
  TimedText::TimedTextDescriptor TDesc;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      Reader.FillTimedTextDescriptor(TDesc);
      FrameBuffer.Capacity(Options.fb_size);

      if ( Options.verbose_flag )
	TimedText::DescriptorDump(TDesc);
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  if ( ASDCP_FAILURE(result) )
    return result;

  std::string XMLDoc;
  std::string out_path = Kumu::PathDirname(Options.file_prefix);
  ui32_t write_count;
  char buf[64];
  TimedText::ResourceList_t::const_iterator ri;

  result = Reader.ReadTimedTextResource(XMLDoc, Context, HMAC);

  if ( ASDCP_SUCCESS(result) && ( ! Options.no_write_flag ) )
    {
      Kumu::FileWriter Writer;
      result = Writer.OpenWrite(Options.file_prefix);

      if ( ASDCP_SUCCESS(result) )
	result = Writer.Write(reinterpret_cast<const byte_t*>(XMLDoc.c_str()), XMLDoc.size(), &write_count);
    }

  for ( ri = TDesc.ResourceList.begin() ; ri != TDesc.ResourceList.end() && ASDCP_SUCCESS(result); ri++ )
    {
      result = Reader.ReadAncillaryResource(ri->ResourceID, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) && ( ! Options.no_write_flag ) )
	{
	  Kumu::FileWriter Writer;
	  result = Writer.OpenWrite(Kumu::PathJoin(out_path, Kumu::UUID(ri->ResourceID).EncodeHex(buf, 64)).c_str());

	  if ( ASDCP_SUCCESS(result) )
	    result = Writer.Write(FrameBuffer.RoData(), FrameBuffer.Size(), &write_count);

	      if ( Options.verbose_flag )
		FrameBuffer.Dump(stderr, Options.fb_dump_size);
	}
    }

  return result;
}

// Read one or more plaintext DCData bytestreams from a plaintext ASDCP file
// Read one or more plaintext DCData bytestreams from a ciphertext ASDCP file
// Read one or more ciphertext DCData byestreams from a ciphertext ASDCP file
//
Result_t
read_DCData_file(CommandOptions& Options)
{
  AESDecContext*     Context = 0;
  HMACContext*       HMAC = 0;
  DCData::MXFReader    Reader;
  DCData::FrameBuffer  FrameBuffer(Options.fb_size);
  ui32_t             frame_count = 0;

  Result_t result = Reader.OpenRead(Options.input_filename);

  if ( ASDCP_SUCCESS(result) )
    {
      DCData::DCDataDescriptor DDesc;
      Reader.FillDCDataDescriptor(DDesc);

      frame_count = DDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  DCData::DCDataDescriptorDump(DDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && Options.key_flag )
    {
      Context = new AESDecContext;
      result = Context->InitKey(Options.key_value);

      if ( ASDCP_SUCCESS(result) && Options.read_hmac )
	{
	  WriterInfo Info;
	  Reader.FillWriterInfo(Info);

	  if ( Info.UsesHMAC )
	    {
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	  else
	    {
	      fputs("File does not contain HMAC values, ignoring -m option.\n", stderr);
	    }
	}
    }

  ui32_t last_frame = Options.start_frame + ( Options.duration ? Options.duration : frame_count);
  if ( last_frame > frame_count )
    last_frame = frame_count;

  char name_format[64];
  snprintf(name_format,  64, "%%s%%0%du.%s", Options.number_width, Options.extension);

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( ! Options.no_write_flag )
	    {
	  Kumu::FileWriter OutFile;
	  char filename[256];
	  ui32_t write_count;
	  snprintf(filename, 256, name_format, Options.file_prefix, i);
	  result = OutFile.OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
        }

	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);
	}
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
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  if ( Options.mode == MMT_GOP_START )
    {
      result = gop_start_test(Options);
    }
  else if ( Options.mode == MMT_EXTRACT )
    {
      EssenceType_t EssenceType;
      result = ASDCP::EssenceType(Options.input_filename, EssenceType);

      if ( ASDCP_SUCCESS(result) )
	{
	  switch ( EssenceType )
	    {
	    case ESS_MPEG2_VES:
	      result = read_MPEG2_file(Options);
	      break;

	    case ESS_JPEG_2000:
	      if ( Options.stereo_image_flag )
		result = read_JP2K_S_file(Options);
	      else
		result = read_JP2K_file(Options);
	      break;

	    case ESS_JPEG_2000_S:
	      result = read_JP2K_S_file(Options);
	      break;

	    case ESS_PCM_24b_48k:
	    case ESS_PCM_24b_96k:
	      result = read_PCM_file(Options);
	      break;

	    case ESS_TIMED_TEXT:
	      result = read_timed_text_file(Options);
	      break;

        case ESS_DCDATA_UNKNOWN:
          result = read_DCData_file(Options);
          break;

        case ESS_DCDATA_DOLBY_ATMOS:
          Options.extension = "atmos";
          result = read_DCData_file(Options);
          break;

	    default:
	      fprintf(stderr, "%s: Unknown file type, not ASDCP essence.\n", Options.input_filename);
	      return 5;
	    }
	}
    }
  else
    {
      fprintf(stderr, "Unhandled mode: %d.\n", Options.mode);
      return 6;
    }

  if ( ASDCP_FAILURE(result) )
    {
      fputs("Program stopped on error.\n", stderr);

      if ( result == RESULT_SFORMAT )
	{
	  fputs("Use option '-3' to force stereoscopic mode.\n", stderr);
	}
      else if ( result != RESULT_FAIL )
	{
	  fputs(result, stderr);
	  fputc('\n', stderr);
	}

      return 1;
    }

  return 0;
}


//
// end asdcp-unwrap.cpp
//
