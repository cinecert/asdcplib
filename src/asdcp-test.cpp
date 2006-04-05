/*
Copyright (c) 2003-2006, John Hurst
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
/*! \file    asdcp-test.cpp
    \version $Id$       
    \brief   AS-DCP file manipulation utility

  This program provides command line access to the major features of the asdcplib
  library, and serves as a library unit test which provides the functionality of
  the supported use cases.

  For more information about asdcplib, please refer to the header file AS_DCP.h

  WARNING: While the asdcplib library attempts to provide a complete and secure
  implementation of the cryptographic features of the AS-DCP file formats, this
  unit test program is NOT secure and is therefore NOT SUITABLE FOR USE in a
  production environment without some modification.

  In particular, this program uses weak IV generation and externally generated
  plaintext keys. These shortcomings exist because cryptographic-quality
  random number generation and key management are outside the scope of the
  asdcplib library. Developers using asdcplib for commercial implementations
  claiming SMPTE conformance are expected to provide proper implementations of
  these features.
*/

#include <iostream>
#include <assert.h>

#include <KM_fileio.h>
#include <KM_prng.h>
#include <PCMParserList.h>
#include <WavFileWriter.h>
#include <MXF.h>
#include <Metadata.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4*1024*1024;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PACKAGE = "asdcp-test";  // program name for messages
const ui32_t MAX_IN_FILES = 16;             // maximum number of input files handled by
                                            //   the command option parser

// local program identification info written to file headers
class MyInfo : public WriterInfo
{
public:
  MyInfo()
  {
      static byte_t default_ProductUUID_Data[UUIDlen] =
      { 0x7d, 0x83, 0x6e, 0x16, 0x37, 0xc7, 0x4c, 0x22,
	0xb2, 0xe0, 0x46, 0xa7, 0x17, 0xe8, 0x4f, 0x42 };
      
      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      CompanyName = "WidgetCo";
      ProductName = "asdcp-test";

      char s_buf[128];
      snprintf(s_buf, 128, "%lu.%lu.%lu", VERSION_MAJOR, VERSION_APIMINOR, VERSION_IMPMINOR);
      ProductVersion = s_buf;
  }
} s_MyInfo;


// Macros used to test command option data state.

// True if a major mode has already been selected.
#define TEST_MAJOR_MODE()     ( info_flag||create_flag||extract_flag||genkey_flag||genid_flag||gop_start_flag )

// Causes the caller to return if a major mode has already been selected,
// otherwise sets the given flag.
#define TEST_SET_MAJOR_MODE(f) if ( TEST_MAJOR_MODE() ) \
                                 { \
                                   fputs("Conflicting major mode, choose one of -(gcixG)).\n", stderr); \
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
Copyright (c) 2003-2005 John Hurst\n\n\
asdcplib may be copied only under the terms of the license found at\n\
the top of every file in the asdcplib distribution kit.\n\n\
Specify the -h (help) option for further information about %s\n\n",
	  PACKAGE, ASDCP::Version(), PACKAGE);
}

//
void
usage(FILE* stream = stderr)
{
  fprintf(stream, "\
USAGE: %s [-i [-H, -n]|-c <filename> [-p <rate>, -e, -M, -R]|-x <root-name> [-m][-S]|-g|-u|-G|-V|-h]\n\
       [-k <key-string>] [-j <key-id-string>] [-f <start-frame-num>] [-d <duration>]\n\
       [-b <buf-size>] [-W] [-v [-s]] [<filename>, ...]\n\
\n", PACKAGE);

  fprintf(stream, "\
Major modes:\n\
  -i              - Show file info\n\
  -c <filename>   - Create AS-DCP file from input(s)\n\
  -x <root-name>  - Extract essence from AS-DCP file to named file(s)\n\
  -g              - Generate a random 16 byte value to stdout\n\
  -u              - Generate a random UUID value to stdout\n\
  -G              - Perform GOP start lookup test on MPEG file\n\
  -V              - Show version\n\
  -h              - Show help\n\
\n");

  fprintf(stream, "\
Security Options:\n\
  -j <key-id-str> - Write key ID instead of creating a random value\n\
  -k <key-string> - Use key for ciphertext operations\n\
  -e              - Encrypt MPEG or JP2K headers (default)\n\
  -E              - Do not encrypt MPEG or JP2K headers\n\
  -M              - Do not create HMAC values when writing\n\
  -m              - verify HMAC values when reading\n\
\n");

  fprintf(stream, "\
Read/Write Options:\n\
  -b <buf-size>   - Size (in bytes) of the picture frame buffer, default: 2097152 (2MB)\n\
  -f <frame-num>  - Starting frame number, default 0\n\
  -d <duration>   - Number of frames to process, default all\n\
  -p <rate>       - fps of picture when wrapping PCM or JP2K:, use one of [23|24|48], 24 is default\n\
  -L              - Write SMPTE UL values instead of MXF Interop\n\
  -R              - Repeat the first frame over the entire file (picture essence only, requires -c, -d)\n\
  -S              - Split Wave essence to stereo WAV files during extract (default = multichannel WAV)\n\
  -W              - Read input file only, do not write source file\n\
\n");

  fprintf(stream, "\
Info Options:\n\
  -H              - Show MXF header metadata, used with option -i\n\
  -n              - Show index, used with option -i\n\
\n\
Other Options:\n\
  -s <number>     - Number of bytes of frame buffer to be dumped as hex to stderr (use with -v)\n\
  -v              - Verbose, show extra detail during run\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
         o An argument of \"23\" to the -p option will be interpreted as 23000/1001 fps.\n\
\n");
}

//
//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   info_flag;      // true if the file info mode was selected
  bool   create_flag;    // true if the file create mode was selected
  bool   extract_flag;   // true if the file extract mode was selected
  bool   genkey_flag;    // true if we are to generate a new key value
  bool   genid_flag;     // true if we are to generate a new UUID value
  bool   gop_start_flag; // true if we are to perform a GOP start lookup test
  bool   key_flag;       // true if an encryption key was given
  bool   key_id_flag;    // true if a key ID was given
  bool   encrypt_header_flag; // true if mpeg headers are to be encrypted
  bool   write_hmac;     // true if HMAC values are to be generated and written
  bool   read_hmac;      // true if HMAC values are to be validated
  bool   split_wav;      // true if PCM is to be extracted to stereo WAV files
  bool   verbose_flag;   // true if the verbose option was selected
  ui32_t fb_dump_size;   // number of bytes of frame buffer to dump
  bool   showindex_flag; // true if index is to be displayed
  bool   showheader_flag; // true if MXF file header is to be displayed
  bool   no_write_flag;  // true if no output files are to be written
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  ui32_t start_frame;    // frame number to begin processing
  ui32_t duration;       // number of frames to be processed
  bool   duration_flag;  // true if duration argument given
  bool   do_repeat;      // if true and -c -d, repeat first input frame
  bool   use_smpte_labels; // if true, SMPTE UL values will be written instead of MXF Interop values
  ui32_t picture_rate;   // fps of picture when wrapping PCM
  ui32_t fb_size;        // size of picture frame buffer
  ui32_t file_count;     // number of elements in filenames[]
  const char* file_root; // filename pre for files written by the extract mode
  const char* out_file;  // name of mxf file created by create mode
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  const char* filenames[MAX_IN_FILES]; // list of filenames to be processed

  //
  Rational PictureRate()
  {
    if ( picture_rate == 23 ) return EditRate_23_98;
    if ( picture_rate == 48 ) return EditRate_48;
    return EditRate_24;
  }

  //
  const char* szPictureRate()
  {
    if ( picture_rate == 23 ) return "23.976";
    if ( picture_rate == 48 ) return "48";
    return "24";
  }

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), info_flag(false), create_flag(false),
    extract_flag(false), genkey_flag(false), genid_flag(false), gop_start_flag(false),
    key_flag(false), key_id_flag(false), encrypt_header_flag(true),
    write_hmac(true), read_hmac(false), split_wav(false),
    verbose_flag(false), fb_dump_size(0), showindex_flag(false), showheader_flag(false),
    no_write_flag(false), version_flag(false), help_flag(false), start_frame(0),
    duration(0xffffffff), duration_flag(false), do_repeat(false), use_smpte_labels(false),
    picture_rate(24), fb_size(FRAME_BUFFER_SIZE), file_count(0), file_root(0), out_file(0)
  {
    memset(key_value, 0, KeyLen);
    memset(key_id_value, 0, UUIDlen);

    for ( int i = 1; i < argc; i++ )
      {
	if ( argv[i][0] == '-' && isalpha(argv[i][1]) && argv[i][2] == 0 )
	  {
	    switch ( argv[i][1] )
	      {
	      case 'i': TEST_SET_MAJOR_MODE(info_flag);	break;
	      case 'G': TEST_SET_MAJOR_MODE(gop_start_flag); break;
	      case 'W': no_write_flag = true; break;
	      case 'n': showindex_flag = true; break;
	      case 'H': showheader_flag = true; break;
	      case 'R': do_repeat = true; break;
	      case 'S': split_wav = true; break;
	      case 'V': version_flag = true; break;
	      case 'h': help_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'g': genkey_flag = true; break;
	      case 'u':	genid_flag = true; break;
	      case 'e': encrypt_header_flag = true; break;
	      case 'E': encrypt_header_flag = false; break;
	      case 'M': write_hmac = false; break;
	      case 'm': read_hmac = true; break;
	      case 'L': use_smpte_labels = true; break;

	      case 'c':
		TEST_SET_MAJOR_MODE(create_flag);
		TEST_EXTRA_ARG(i, 'c');
		out_file = argv[i];
		break;

	      case 'x':
		TEST_SET_MAJOR_MODE(extract_flag);
		TEST_EXTRA_ARG(i, 'x');
		file_root = argv[i];
		break;

	      case 'k': key_flag = true;
		TEST_EXTRA_ARG(i, 'k');
		{
		  ui32_t length;
		  Kumu::hex2bin(argv[i], key_value, KeyLen, &length);

		  if ( length != KeyLen )
		    {
		      fprintf(stderr, "Unexpected key length: %lu, expecting %lu characters.\n", KeyLen, length);
		      return;
		    }
		}
		break;

	      case 'j': key_id_flag = true;
		TEST_EXTRA_ARG(i, 'j');
		{
		  ui32_t length;
		  Kumu::hex2bin(argv[i], key_id_value, UUIDlen, &length);

		  if ( length != UUIDlen )
		    {
		      fprintf(stderr, "Unexpected key ID length: %lu, expecting %lu characters.\n", UUIDlen, length);
		      return;
		    }
		}
		break;

	      case 'f':
		TEST_EXTRA_ARG(i, 'f');
		start_frame = atoi(argv[i]); // TODO: test for negative value, should use strtol()
		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration_flag = true;
		duration = atoi(argv[i]); // TODO: test for negative value, should use strtol()
		break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		picture_rate = atoi(argv[i]);
		break;

	      case 's':
		TEST_EXTRA_ARG(i, 's');
		fb_dump_size = atoi(argv[i]);
		break;

	      case 'b':
		TEST_EXTRA_ARG(i, 'b');
		fb_size = atoi(argv[i]);

		if ( verbose_flag )
		  fprintf(stderr, "Frame Buffer size: %lu bytes.\n", fb_size);

		break;

	      default:
		fprintf(stderr, "Unrecognized option: %c\n", argv[i][1]);
		return;
	      }
	  }
	else
	  {
	    filenames[file_count++] = argv[i];

	    if ( file_count >= MAX_IN_FILES )
	      {
		fprintf(stderr, "Filename lists exceeds maximum list size: %lu\n", MAX_IN_FILES);
		return;
	      }
	  }
      }

    if ( TEST_MAJOR_MODE() )
      {
	if ( ! genkey_flag && ! genid_flag && file_count == 0 )
	  {
	    fputs("Option requires at least one filename argument.\n", stderr);
	    return;
	  }
      }

    if ( ! TEST_MAJOR_MODE() && ! help_flag && ! version_flag )
      {
	fputs("No operation selected (use one of -(gcixG) or -h for help).\n", stderr);
	return;
      }

    error_flag = false;
  }
};

//------------------------------------------------------------------------------------------
// MPEG2 essence

// Write a plaintext MPEG2 Video Elementary Stream to a plaintext ASDCP file
// Write a plaintext MPEG2 Video Elementary Stream to a ciphertext ASDCP file
//
Result_t
write_MPEG2_file(CommandOptions& Options)
{
  AESEncContext*     Context = 0;
  HMACContext*       HMAC = 0;
  MPEG2::FrameBuffer FrameBuffer(Options.fb_size);
  MPEG2::Parser      Parser;
  MPEG2::MXFWriter   Writer;
  MPEG2::VideoDescriptor VDesc;
  byte_t             IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG   RNG;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames[0]);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillVideoDescriptor(VDesc);

      if ( Options.verbose_flag )
	{
	  fputs("MPEG-2 Pictures\n", stderr);
	  fputs("VideoDescriptor:\n", stderr);
	  fprintf(stderr, "Frame Buffer size: %lu\n", Options.fb_size);
	  MPEG2::VideoDescriptorDump(VDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      Kumu::GenRandomUUID(Info.AssetUUID);

      if ( Options.use_smpte_labels )
	{
	  Info.LabelSetType = LS_MXF_SMPTE;
	  fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");
	}

      // configure encryption
      if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	  else
	    RNG.FillRandom(Info.CryptographicKeyID, UUIDlen);

	  Context = new AESEncContext;
	  result = Context->InitKey(Options.key_value);

	  if ( ASDCP_SUCCESS(result) )
	    result = Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));

	  if ( ASDCP_SUCCESS(result) && Options.write_hmac )
	    {
	      Info.UsesHMAC = true;
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	}

      if ( ASDCP_SUCCESS(result) )
	result = Writer.OpenWrite(Options.out_file, Info, VDesc);
    }

  if ( ASDCP_SUCCESS(result) )
    // loop through the frames
    {
      result = Parser.Reset();
      ui32_t duration = 0;

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  if ( ! Options.do_repeat || duration == 1 )
	    {
	      result = Parser.ReadFrame(FrameBuffer);

	      if ( ASDCP_SUCCESS(result) )
		{
		  if ( Options.verbose_flag )
		    FrameBuffer.Dump(stderr, Options.fb_dump_size);
		  
		  if ( Options.encrypt_header_flag )
		    FrameBuffer.PlaintextOffset(0);
		}
	    }

	  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
	    {
	      result = Writer.WriteFrame(FrameBuffer, Context, HMAC);

	      // The Writer class will forward the last block of ciphertext
	      // to the encryption context for use as the IV for the next
	      // frame. If you want to use non-sequitur IV values, un-comment
	      // the following  line of code.
	      // if ( ASDCP_SUCCESS(result) && Options.key_flag )
	      //   Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));
	    }
	}

      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    result = Writer.Finalize();

  return result;
}

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

  Result_t result = Reader.OpenRead(Options.filenames[0]);

  if ( ASDCP_SUCCESS(result) )
    {
      MPEG2::VideoDescriptor VDesc;
      Reader.FillVideoDescriptor(VDesc);
      frame_count = VDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %lu\n", Options.fb_size);
	  MPEG2::VideoDescriptorDump(VDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      char filename[256];
      snprintf(filename, 256, "%s.ves", Options.file_root);
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

	  ui32_t write_count = 0;
	  result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);
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

  Result_t result = Reader.OpenRead(Options.filenames[0]);

  if ( ASDCP_SUCCESS(result) )
    {
      MPEG2::VideoDescriptor VDesc;
      Reader.FillVideoDescriptor(VDesc);
      frame_count = VDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %lu\n", Options.fb_size);
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

	  fprintf(stderr, "Requested frame %lu, got %lu\n", i, FrameBuffer.FrameNumber());
	}
    }

  return result;
}

//------------------------------------------------------------------------------------------
// JPEG 2000 essence

// Write one or more plaintext JPEG 2000 codestreams to a plaintext ASDCP file
// Write one or more plaintext JPEG 2000 codestreams to a ciphertext ASDCP file
//
Result_t
write_JP2K_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  JP2K::MXFWriter         Writer;
  JP2K::FrameBuffer       FrameBuffer(Options.fb_size);
  JP2K::PictureDescriptor PDesc;
  JP2K::SequenceParser    Parser;
  byte_t                  IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG        RNG;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames[0]);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillPictureDescriptor(PDesc);
      PDesc.EditRate = Options.PictureRate();

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "JPEG 2000 pictures\n");
	  fputs("PictureDescriptor:\n", stderr);
          fprintf(stderr, "Frame Buffer size: %lu\n", Options.fb_size);
	  JP2K::PictureDescriptorDump(PDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      Kumu::GenRandomUUID(Info.AssetUUID);

      if ( Options.use_smpte_labels )
	{
	  Info.LabelSetType = LS_MXF_SMPTE;
	  fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");
	}

      // configure encryption
      if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	  else
	    RNG.FillRandom(Info.CryptographicKeyID, UUIDlen);

	  Context = new AESEncContext;
	  result = Context->InitKey(Options.key_value);

	  if ( ASDCP_SUCCESS(result) )
	    result = Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));

	  if ( ASDCP_SUCCESS(result) && Options.write_hmac )
	    {
	      Info.UsesHMAC = true;
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	}

      if ( ASDCP_SUCCESS(result) )
	result = Writer.OpenWrite(Options.out_file, Info, PDesc);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t duration = 0;
      result = Parser.Reset();

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  if ( ! Options.do_repeat || duration == 1 )
	    {
	      result = Parser.ReadFrame(FrameBuffer);

	      if ( ASDCP_SUCCESS(result) )
		{
		  if ( Options.verbose_flag )
		    FrameBuffer.Dump(stderr, Options.fb_dump_size);
		  
		  if ( Options.encrypt_header_flag )
		    FrameBuffer.PlaintextOffset(0);
		}
	    }

	  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
	    {
	      result = Writer.WriteFrame(FrameBuffer, Context, HMAC);

	      // The Writer class will forward the last block of ciphertext
	      // to the encryption context for use as the IV for the next
	      // frame. If you want to use non-sequitur IV values, un-comment
	      // the following  line of code.
	      // if ( ASDCP_SUCCESS(result) && Options.key_flag )
	      //   Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));
	    }
	}

      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    result = Writer.Finalize();

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

  Result_t result = Reader.OpenRead(Options.filenames[0]);

  if ( ASDCP_SUCCESS(result) )
    {
      JP2K::PictureDescriptor PDesc;
      Reader.FillPictureDescriptor(PDesc);

      frame_count = PDesc.ContainerDuration;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Frame Buffer size: %lu\n", Options.fb_size);
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

  for ( ui32_t i = Options.start_frame; ASDCP_SUCCESS(result) && i < last_frame; i++ )
    {
      result = Reader.ReadFrame(i, FrameBuffer, Context, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  Kumu::FileWriter OutFile;
	  char filename[256];
	  ui32_t write_count;
	  snprintf(filename, 256, "%s%06lu.j2c", Options.file_root, i);
	  result = OutFile.OpenWrite(filename);

	  if ( ASDCP_SUCCESS(result) )
	    result = OutFile.Write(FrameBuffer.Data(), FrameBuffer.Size(), &write_count);

	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);
	}
    }

  return result;
}

//------------------------------------------------------------------------------------------
// PCM essence


// Write one or more plaintext PCM audio streams to a plaintext ASDCP file
// Write one or more plaintext PCM audio streams to a ciphertext ASDCP file
//
Result_t
write_PCM_file(CommandOptions& Options)
{
  AESEncContext*    Context = 0;
  HMACContext*      HMAC = 0;
  PCMParserList     Parser;
  PCM::MXFWriter    Writer;
  PCM::FrameBuffer  FrameBuffer;
  PCM::AudioDescriptor ADesc;
  Rational          PictureRate = Options.PictureRate();
  byte_t            IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG  RNG;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.file_count, Options.filenames, PictureRate);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillAudioDescriptor(ADesc);

      ADesc.SampleRate = PictureRate;
      FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "48Khz PCM Audio, %s fps (%lu spf)\n",
		  Options.szPictureRate(),
		  PCM::CalcSamplesPerFrame(ADesc));
	  fputs("AudioDescriptor:\n", stderr);
	  PCM::AudioDescriptorDump(ADesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      Kumu::GenRandomUUID(Info.AssetUUID);

      if ( Options.use_smpte_labels )
	{
	  Info.LabelSetType = LS_MXF_SMPTE;
	  fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");
	}

      // configure encryption
      if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	  else
	    RNG.FillRandom(Info.CryptographicKeyID, UUIDlen);

	  Context = new AESEncContext;
	  result = Context->InitKey(Options.key_value);

	  if ( ASDCP_SUCCESS(result) )
	    result = Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));

	  if ( ASDCP_SUCCESS(result) && Options.write_hmac )
	    {
	      Info.UsesHMAC = true;
	      HMAC = new HMACContext;
	      result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
	    }
	}

      if ( ASDCP_SUCCESS(result) )
	result = Writer.OpenWrite(Options.out_file, Info, ADesc);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      result = Parser.Reset();
      ui32_t duration = 0;

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  result = Parser.ReadFrame(FrameBuffer);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      if ( FrameBuffer.Size() != FrameBuffer.Capacity() )
		{
		  fprintf(stderr, "WARNING: Last frame read was short, PCM input is possibly not frame aligned.\n");
		  fprintf(stderr, "Expecting %lu bytes, got %lu.\n", FrameBuffer.Capacity(), FrameBuffer.Size());
		  result = RESULT_ENDOFFILE;
		  continue;
		}

	      if ( Options.verbose_flag )
		FrameBuffer.Dump(stderr, Options.fb_dump_size);

	      if ( ! Options.no_write_flag )
		{
		  result = Writer.WriteFrame(FrameBuffer, Context, HMAC);

		  // The Writer class will forward the last block of ciphertext
		  // to the encryption context for use as the IV for the next
		  // frame. If you want to use non-sequitur IV values, un-comment
		  // the following  line of code.
		  // if ( ASDCP_SUCCESS(result) && Options.key_flag )
		  //   Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));
		}
	    }
	}

      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    result = Writer.Finalize();

  return result;
}

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

  Result_t result = Reader.OpenRead(Options.filenames[0]);

  if ( ASDCP_SUCCESS(result) )
    {
      Reader.FillAudioDescriptor(ADesc);

      if ( ADesc.SampleRate != EditRate_23_98
	   && ADesc.SampleRate != EditRate_24
	   && ADesc.SampleRate != EditRate_48 )
	ADesc.SampleRate = Options.PictureRate();

      FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));

      if ( Options.verbose_flag )
	PCM::AudioDescriptorDump(ADesc);
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
      OutWave.OpenWrite(ADesc, Options.file_root, Options.split_wav);
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

	  result = OutWave.WriteFrame(FrameBuffer);
	}
    }

  return result;
}


//------------------------------------------------------------------------------------------
//

//
// These classes wrap the irregular names in the asdcplib API
// so that I can use a template to simplify the implementation
// of show_file_info()

class MyVideoDescriptor : public MPEG2::VideoDescriptor
{
 public:
  void FillDescriptor(MPEG2::MXFReader& Reader) {
    Reader.FillVideoDescriptor(*this);
  }

  void Dump(FILE* stream) {
    MPEG2::VideoDescriptorDump(*this, stream);
  }
};

class MyPictureDescriptor : public JP2K::PictureDescriptor
{
 public:
  void FillDescriptor(JP2K::MXFReader& Reader) {
    Reader.FillPictureDescriptor(*this);
  }

  void Dump(FILE* stream) {
    JP2K::PictureDescriptorDump(*this, stream);
  }
};

class MyAudioDescriptor : public PCM::AudioDescriptor
{
 public:
  void FillDescriptor(PCM::MXFReader& Reader) {
    Reader.FillAudioDescriptor(*this);
  }

  void Dump(FILE* stream) {
    PCM::AudioDescriptorDump(*this, stream);
  }
};


// MSVC didn't like the function template, so now it's a static class method
template<class ReaderT, class DescriptorT>
class FileInfoWrapper
{
public:
  static void file_info(CommandOptions& Options, FILE* stream = 0)
  {
    if ( stream == 0 )
      stream = stdout;

    if ( Options.verbose_flag || Options.showheader_flag )
      {
	ReaderT     Reader;
	Result_t result = Reader.OpenRead(Options.filenames[0]);

	if ( ASDCP_SUCCESS(result) )
	  {
	    if ( Options.showheader_flag )
	      Reader.DumpHeaderMetadata(stream);

	    WriterInfo WI;
	    Reader.FillWriterInfo(WI);
	    WriterInfoDump(WI, stream);

	    DescriptorT Desc;
	    Desc.FillDescriptor(Reader);
	    Desc.Dump(stream);

	    if ( Options.showindex_flag )
	      Reader.DumpIndex(stream);
	  }
	else if ( result == RESULT_FORMAT && Options.showheader_flag )
	  {
	    Reader.DumpHeaderMetadata(stream);
	  }
      }
  }
};

// Read header metadata from an ASDCP file
//
Result_t
show_file_info(CommandOptions& Options)
{
  EssenceType_t EssenceType;
  Result_t result = ASDCP::EssenceType(Options.filenames[0], EssenceType);

  if ( ASDCP_FAILURE(result) )
    return result;

  if ( EssenceType == ESS_MPEG2_VES )
    {
      fputs("File essence type is MPEG2 video.\n", stdout);
      FileInfoWrapper<ASDCP::MPEG2::MXFReader, MyVideoDescriptor>::file_info(Options);
    }
  else if ( EssenceType == ESS_PCM_24b_48k )
    {
      fputs("File essence type is PCM audio.\n", stdout);
      FileInfoWrapper<ASDCP::PCM::MXFReader, MyAudioDescriptor>::file_info(Options);
    }
  else if ( EssenceType == ESS_JPEG_2000 )
    {
      fputs("File essence type is JPEG 2000 pictures.\n", stdout);
      FileInfoWrapper<ASDCP::JP2K::MXFReader, MyPictureDescriptor>::file_info(Options);
    }
  else
    {
      fprintf(stderr, "File is not AS-DCP: %s\n", Options.filenames[0]);
      Kumu::FileReader   Reader;
      MXF::OPAtomHeader TestHeader;

      result = Reader.OpenRead(Options.filenames[0]);

      if ( ASDCP_SUCCESS(result) )
	result = TestHeader.InitFromFile(Reader); // test UL and OP

      if ( ASDCP_SUCCESS(result) )
	{
	  TestHeader.Partition::Dump();

	  if ( MXF::Identification* ID = TestHeader.GetIdentification() )
	    ID->Dump();
	  else
	    fputs("File contains no Identification object.\n", stdout);

	  if ( MXF::SourcePackage* SP = TestHeader.GetSourcePackage() )
	    SP->Dump();
	  else
	    fputs("File contains no SourcePackage object.\n", stdout);
	}
      else
	{
	  fputs("File is not MXF.\n", stdout);
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

  if ( Options.info_flag )
    {
      result = show_file_info(Options);
    }
  else if ( Options.gop_start_flag )
    {
      result = gop_start_test(Options);
    }
  else if ( Options.genkey_flag )
    {
      Kumu::FortunaRNG RNG;
      byte_t bin_buf[KeyLen];
      char   str_buf[40];

      RNG.FillRandom(bin_buf, KeyLen);
      printf("%s\n", Kumu::bin2hex(bin_buf, KeyLen, str_buf, 40));
    }
  else if ( Options.genid_flag )
    {
      UUID TmpID;
      Kumu::GenRandomValue(TmpID);
      char   str_buf[40];
      printf("%s\n", TmpID.EncodeHex(str_buf, 40));
    }
  else if ( Options.extract_flag )
    {
      EssenceType_t EssenceType;
      result = ASDCP::EssenceType(Options.filenames[0], EssenceType);

      if ( ASDCP_SUCCESS(result) )
	{
	  switch ( EssenceType )
	    {
	    case ESS_MPEG2_VES:
	      result = read_MPEG2_file(Options);
	      break;

	    case ESS_JPEG_2000:
	      result = read_JP2K_file(Options);
	      break;

	    case ESS_PCM_24b_48k:
	      result = read_PCM_file(Options);
	      break;

	    default:
	      fprintf(stderr, "%s: Unknown file type, not ASDCP essence.\n", Options.filenames[0]);
	      return 5;
	    }
	}
    }
  else if ( Options.create_flag )
    {
      if ( Options.do_repeat && ! Options.duration_flag )
	{
	  fputs("Option -R requires -d <duration>\n", stderr);
	  return RESULT_FAIL;
	}

      EssenceType_t EssenceType;
      result = ASDCP::RawEssenceType(Options.filenames[0], EssenceType);

      if ( ASDCP_SUCCESS(result) )
	{
	  switch ( EssenceType )
	    {
	    case ESS_MPEG2_VES:
	      result = write_MPEG2_file(Options);
	      break;

	    case ESS_JPEG_2000:
	      result = write_JP2K_file(Options);
	      break;

	    case ESS_PCM_24b_48k:
	      result = write_PCM_file(Options);
	      break;

	    default:
	      fprintf(stderr, "%s: Unknown file type, not ASDCP-compatible essence.\n",
		      Options.filenames[0]);
	      return 5;
	    }
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
// end asdcp-test.cpp
//
