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
/*! \file    asdcp-wrap.cpp
    \version $Id$       
    \brief   AS-DCP file manipulation utility

  This program wraps d-cinema essence (picture, sound or text) in t an AS-DCP
  MXF file.

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

#include <KM_fileio.h>
#include <KM_prng.h>
#include <AS_DCP.h>
#include <PCMParserList.h>
#include <Metadata.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "asdcp-wrap";  // program name for messages

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
      ProductName = "asdcp-wrap";
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
       %s [-3] [-a <uuid>] [-b <buffer-size>] [-C <UL>] [-d <duration>]\n\
          [-e|-E] [-f <start-frame>] [-j <key-id-string>] [-k <key-string>]\n\
          [-l <label>] [-L] [-M] [-p <frame-rate>] [-s <num>] [-v] [-W]\n\
          [-z|-Z] <input-file>+ <output-file>\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -3                - With -c, create a stereoscopic image file. Expects two\n\
                      directories of JP2K codestreams (directories must have\n\
                      an equal number of frames; left eye is first).\n\
                    - With -x, force stereoscopic interpretation of a JP2K\n\
                      track file.\n\
  -C <UL>           - Set ChannelAssignment UL value\n\
  -h | -help        - Show help\n\
  -V                - Show version information\n\
  -e                - Encrypt MPEG or JP2K headers (default)\n\
  -E                - Do not encrypt MPEG or JP2K headers\n\
  -j <key-id-str>   - Write key ID instead of creating a random value\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -M                - Do not create HMAC values when writing\n\
  -a <UUID>         - Specify the Asset ID of a file (with -c)\n\
  -b <buffer-size>  - Specify size in bytes of picture frame buffer.\n\
                      Defaults to 4,194,304 (4MB)\n\
  -d <duration>     - Number of frames to process, default all\n\
  -f <start-frame>  - Starting frame number, default 0\n\
  -l <label>        - Use given channel format label when writing MXF sound\n\
                      files. SMPTE 429-2 labels: '5.1', '6.1', '7.1',\n\
                      '7.1DS', 'WTF'\n\
                      Default is no label (valid for Interop only).\n\
  -L                - Write SMPTE UL values instead of MXF Interop\n\
  -p <rate>         - fps of picture when wrapping PCM or JP2K:\n\
                      Use one of [23|24|25|30|48|50|60], 24 is default\n\
  -v                - Verbose, prints informative messages to stderr\n\
  -W                - Read input file only, do not write source file\n\
  -z                - Fail if j2c inputs have unequal parameters (default)\n\
  -Z                - Ignore unequal parameters in j2c inputs\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\
         o An argument of \"23\" to the -p option will be interpreted\n\
           as 24000/1001 fps.\n\
\n");
}

//
PCM::ChannelFormat_t
decode_channel_fmt(const std::string& label_name)
{
  if ( label_name == "5.1" )
    return PCM::CF_CFG_1;

  else if ( label_name == "6.1" )
    return PCM::CF_CFG_2;
  
  else if ( label_name == "7.1" )
    return PCM::CF_CFG_3;

  else if ( label_name == "WTF" )
    return PCM::CF_CFG_4;

  else if ( label_name == "7.1DS" )
    return PCM::CF_CFG_5;

  fprintf(stderr, "Error decoding channel format string: %s\n", label_name.c_str());
  fprintf(stderr, "Expecting '5.1', '6.1', '7.1', '7.1DS' or 'WTF'\n");
  return PCM::CF_NONE;
}

//
//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   key_flag;       // true if an encryption key was given
  bool   asset_id_flag;  // true if an asset ID was given
  bool   encrypt_header_flag; // true if mpeg headers are to be encrypted
  bool   write_hmac;     // true if HMAC values are to be generated and written
  ///  bool   read_hmac;      // true if HMAC values are to be validated
  ///  bool   split_wav;      // true if PCM is to be extracted to stereo WAV files
  ///  bool   mono_wav;       // true if PCM is to be extracted to mono WAV files
  bool   verbose_flag;   // true if the verbose option was selected
  ui32_t fb_dump_size;   // number of bytes of frame buffer to dump
  ///  bool   showindex_flag; // true if index is to be displayed
  ///  bool   showheader_flag; // true if MXF file header is to be displayed
  bool   no_write_flag;  // true if no output files are to be written
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   stereo_image_flag; // if true, expect stereoscopic JP2K input (left eye first)
  ///  ui32_t number_width;   // number of digits in a serialized filename (for JPEG extract)
  ui32_t start_frame;    // frame number to begin processing
  ui32_t duration;       // number of frames to be processed
  bool   use_smpte_labels; // if true, SMPTE UL values will be written instead of MXF Interop values
  bool   j2c_pedantic;   // passed to JP2K::SequenceParser::OpenRead
  ui32_t picture_rate;   // fps of picture when wrapping PCM
  ui32_t fb_size;        // size of picture frame buffer
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  bool   key_id_flag;    // true if a key ID was given
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  byte_t asset_id_value[UUIDlen];// value of asset ID (when asset_id_flag is true)
  PCM::ChannelFormat_t channel_fmt; // audio channel arrangement
  std::string out_file; //
  bool show_ul_values;    /// if true, dump the UL table before going tp work.
  Kumu::PathList_t filenames;  // list of filenames to be processed
  UL channel_assignment;

  //
  Rational PictureRate()
  {
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
    return EditRate_24;
  }

  //
  const char* szPictureRate()
  {
    if ( picture_rate == 23 ) return "23.976";
    if ( picture_rate == 24 ) return "24";
    if ( picture_rate == 25 ) return "25";
    if ( picture_rate == 30 ) return "30";
    if ( picture_rate == 48 ) return "48";
    if ( picture_rate == 50 ) return "50";
    if ( picture_rate == 60 ) return "60";
    if ( picture_rate == 96 ) return "96";
    if ( picture_rate == 100 ) return "100";
    if ( picture_rate == 120 ) return "120";
    return "24";
  }

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), key_flag(false), key_id_flag(false), asset_id_flag(false),
    encrypt_header_flag(true), write_hmac(true),
    verbose_flag(false), fb_dump_size(0),
    no_write_flag(false), version_flag(false), help_flag(false), stereo_image_flag(false),
    start_frame(0),
    duration(0xffffffff), use_smpte_labels(false), j2c_pedantic(true),
    fb_size(FRAME_BUFFER_SIZE),
    channel_fmt(PCM::CF_NONE),
    show_ul_values(false)
  {
    memset(key_value, 0, KeyLen);
    memset(key_id_value, 0, UUIDlen);

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
	      case '3': stereo_image_flag = true; break;

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

	      case 'b':
		TEST_EXTRA_ARG(i, 'b');
		fb_size = abs(atoi(argv[i]));

		if ( verbose_flag )
		  fprintf(stderr, "Frame Buffer size: %u bytes.\n", fb_size);

		break;

	      case 'C':
		TEST_EXTRA_ARG(i, 'U');
		if ( ! channel_assignment.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration = abs(atoi(argv[i]));
		break;

	      case 'E': encrypt_header_flag = false; break;
	      case 'e': encrypt_header_flag = true; break;

	      case 'f':
		TEST_EXTRA_ARG(i, 'f');
		start_frame = abs(atoi(argv[i]));
		break;

	      case 'h': help_flag = true; break;

	      case 'j': key_id_flag = true;
		TEST_EXTRA_ARG(i, 'j');
		{
		  ui32_t length;
		  Kumu::hex2bin(argv[i], key_id_value, UUIDlen, &length);

		  if ( length != UUIDlen )
		    {
		      fprintf(stderr, "Unexpected key ID length: %u, expecting %u characters.\n", length, UUIDlen);
		      return;
		    }
		}
		break;

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

	      case 'l':
		TEST_EXTRA_ARG(i, 'l');
		channel_fmt = decode_channel_fmt(argv[i]);
		break;

	      case 'L': use_smpte_labels = true; break;
	      case 'M': write_hmac = false; break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		picture_rate = abs(atoi(argv[i]));
		break;

	      case 'V': version_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'W': no_write_flag = true; break;
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
    
    if ( filenames.size() < 2 )
      {
	fputs("Option requires at least two filename arguments: <input-file> <output-file>\n", stderr);
	return;
      }

    out_file = filenames.back();
    filenames.pop_back();
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
  Result_t result = Parser.OpenRead(Options.filenames.front().c_str());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillVideoDescriptor(VDesc);

      if ( Options.verbose_flag )
	{
	  fputs("MPEG-2 Pictures\n", stderr);
	  fputs("VideoDescriptor:\n", stderr);
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  MPEG2::VideoDescriptorDump(VDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
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
	result = Writer.OpenWrite(Options.out_file.c_str(), Info, VDesc);
    }

  if ( ASDCP_SUCCESS(result) )
    // loop through the frames
    {
      result = Parser.Reset();
      ui32_t duration = 0;

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  if ( duration == 1 )
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


//------------------------------------------------------------------------------------------
// JPEG 2000 essence

// Write one or more plaintext JPEG 2000 stereoscopic codestream pairs to a plaintext ASDCP file
// Write one or more plaintext JPEG 2000 stereoscopic codestream pairs to a ciphertext ASDCP file
//
Result_t
write_JP2K_S_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  JP2K::MXFSWriter        Writer;
  JP2K::FrameBuffer       FrameBuffer(Options.fb_size);
  JP2K::PictureDescriptor PDesc;
  JP2K::SequenceParser    ParserLeft, ParserRight;
  byte_t                  IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG        RNG;

  if ( Options.filenames.size() != 2 )
    {
      fprintf(stderr, "Two inputs are required for stereoscopic option.\n");
      return RESULT_FAIL;
    }

  // set up essence parser
  Result_t result = ParserLeft.OpenRead(Options.filenames.front().c_str(), Options.j2c_pedantic);

  if ( ASDCP_SUCCESS(result) )
    {
      Options.filenames.pop_front();
      result = ParserRight.OpenRead(Options.filenames.front().c_str(), Options.j2c_pedantic);
    }

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      ParserLeft.FillPictureDescriptor(PDesc);
      PDesc.EditRate = Options.PictureRate();

      if ( Options.verbose_flag )
	{
	  fputs("JPEG 2000 stereoscopic pictures\nPictureDescriptor:\n", stderr);
          fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  JP2K::PictureDescriptorDump(PDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
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
	result = Writer.OpenWrite(Options.out_file.c_str(), Info, PDesc);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t duration = 0;
      result = ParserLeft.Reset();
      if ( ASDCP_SUCCESS(result) ) result = ParserRight.Reset();

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  result = ParserLeft.ReadFrame(FrameBuffer);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      if ( Options.verbose_flag )
		FrameBuffer.Dump(stderr, Options.fb_dump_size);
		  
	      if ( Options.encrypt_header_flag )
		FrameBuffer.PlaintextOffset(0);
	    }

	  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
	    result = Writer.WriteFrame(FrameBuffer, JP2K::SP_LEFT, Context, HMAC);

	  if ( ASDCP_SUCCESS(result) )
	    result = ParserRight.ReadFrame(FrameBuffer);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      if ( Options.verbose_flag )
		FrameBuffer.Dump(stderr, Options.fb_dump_size);
		  
	      if ( Options.encrypt_header_flag )
		FrameBuffer.PlaintextOffset(0);
	    }

	  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
	    result = Writer.WriteFrame(FrameBuffer, JP2K::SP_RIGHT, Context, HMAC);
	}

      if ( result == RESULT_ENDOFFILE )
	result = RESULT_OK;
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    result = Writer.Finalize();

  return result;
}

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
  Result_t result = Parser.OpenRead(Options.filenames.front().c_str(), Options.j2c_pedantic);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillPictureDescriptor(PDesc);
      PDesc.EditRate = Options.PictureRate();

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "JPEG 2000 pictures\n");
	  fputs("PictureDescriptor:\n", stderr);
          fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  JP2K::PictureDescriptorDump(PDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
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
	result = Writer.OpenWrite(Options.out_file.c_str(), Info, PDesc);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t duration = 0;
      result = Parser.Reset();

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  if ( duration == 1 )
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
  Result_t result = Parser.OpenRead(Options.filenames, PictureRate);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillAudioDescriptor(ADesc);

      ADesc.EditRate = PictureRate;
      FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));
      ADesc.ChannelFormat = Options.channel_fmt;

      if ( Options.use_smpte_labels && ADesc.ChannelFormat == PCM::CF_NONE)
	{
	  fprintf(stderr, "ATTENTION! Writing SMPTE audio without ChannelAssignment property (see option -l)\n");
	}

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "%.1fkHz PCM Audio, %s fps (%u spf)\n",
		  ADesc.AudioSamplingRate.Quotient() / 1000.0,
		  Options.szPictureRate(),
		  PCM::CalcSamplesPerFrame(ADesc));
	  fputs("AudioDescriptor:\n", stderr);
	  PCM::AudioDescriptorDump(ADesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
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
	result = Writer.OpenWrite(Options.out_file.c_str(), Info, ADesc);

      if ( ASDCP_SUCCESS(result) && Options.channel_assignment.HasValue() )
	{
	  MXF::WaveAudioDescriptor *descriptor = 0;
	  Writer.OPAtomHeader().GetMDObjectByType(DefaultSMPTEDict().ul(MDD_WaveAudioDescriptor),
						  reinterpret_cast<MXF::InterchangeObject**>(&descriptor));
	  descriptor->ChannelAssignment = Options.channel_assignment;
	}
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
		  fprintf(stderr, "Expecting %u bytes, got %u.\n", FrameBuffer.Capacity(), FrameBuffer.Size());
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


//------------------------------------------------------------------------------------------
// TimedText essence


// Write one or more plaintext timed text streams to a plaintext ASDCP file
// Write one or more plaintext timed text streams to a ciphertext ASDCP file
//
Result_t
write_timed_text_file(CommandOptions& Options)
{
  AESEncContext*    Context = 0;
  HMACContext*      HMAC = 0;
  TimedText::DCSubtitleParser  Parser;
  TimedText::MXFWriter    Writer;
  TimedText::FrameBuffer  FrameBuffer;
  TimedText::TimedTextDescriptor TDesc;
  byte_t            IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG  RNG;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front().c_str());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillTimedTextDescriptor(TDesc);
      FrameBuffer.Capacity(Options.fb_size);

      if ( Options.verbose_flag )
	{
	  fputs("D-Cinema Timed-Text Descriptor:\n", stderr);
	  TimedText::DescriptorDump(TDesc);
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
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
	result = Writer.OpenWrite(Options.out_file.c_str(), Info, TDesc);
    }

  if ( ASDCP_FAILURE(result) )
    return result;

  std::string XMLDoc;
  TimedText::ResourceList_t::const_iterator ri;

  result = Parser.ReadTimedTextResource(XMLDoc);

  if ( ASDCP_SUCCESS(result) )
    result = Writer.WriteTimedTextResource(XMLDoc, Context, HMAC);

  for ( ri = TDesc.ResourceList.begin() ; ri != TDesc.ResourceList.end() && ASDCP_SUCCESS(result); ri++ )
    {
      result = Parser.ReadAncillaryResource((*ri).ResourceID, FrameBuffer);

      if ( ASDCP_SUCCESS(result) )
	{
	  if ( Options.verbose_flag )
	    FrameBuffer.Dump(stderr, Options.fb_dump_size);

	  if ( ! Options.no_write_flag )
	    {
	      result = Writer.WriteAncillaryResource(FrameBuffer, Context, HMAC);
	      
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

  if ( Options.show_ul_values )
    {
      if ( Options.use_smpte_labels )
	DefaultSMPTEDict().Dump(stdout);
      else
	DefaultInteropDict().Dump(stdout);
    }

  EssenceType_t EssenceType;
  result = ASDCP::RawEssenceType(Options.filenames.front().c_str(), EssenceType);

  if ( ASDCP_SUCCESS(result) )
    {
      switch ( EssenceType )
	{
	case ESS_MPEG2_VES:
	  result = write_MPEG2_file(Options);
	  break;

	case ESS_JPEG_2000:
	  if ( Options.stereo_image_flag )
	    {
	      result = write_JP2K_S_file(Options);
	    }
	  else
	    {
	      result = write_JP2K_file(Options);
	    }
	  break;

	case ESS_PCM_24b_48k:
	case ESS_PCM_24b_96k:
	  result = write_PCM_file(Options);
	  break;

	case ESS_TIMED_TEXT:
	  result = write_timed_text_file(Options);
	  break;

	default:
	  fprintf(stderr, "%s: Unknown file type, not ASDCP-compatible essence.\n",
		  Options.filenames.front().c_str());
	  return 5;
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
// end asdcp-wrap.cpp
//
