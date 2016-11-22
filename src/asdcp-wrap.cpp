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
/*! \file    asdcp-wrap.cpp
    \version $Id$
    \brief   AS-DCP file manipulation utility

  This program wraps d-cinema essence (picture, sound or text) into an AS-DCP
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
#include <AtmosSyncChannel_Mixer.h>
#include <AS_DCP.h>
#include <PCMParserList.h>
#include <Metadata.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

const byte_t P_HFR_UL_2K[16] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d,
  0x0e, 0x16, 0x02, 0x02, 0x03, 0x01, 0x01, 0x03
};

const ASDCP::Dictionary *g_dict = 0;

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
       %s [-3] [-a <uuid>] [-b <buffer-size>] [-C <UL>] [-d <duration>]\n\
          [-e|-E] [-f <start-frame>] [-j <key-id-string>] [-k <key-string>]\n\
          [-l <label>] [-L] [-M] [-m <expr>] [-p <frame-rate>] [-s] [-v]\n\
          [-W] [-z|-Z] <input-file>+ <output-file>\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -3                - Create a stereoscopic image file. Expects two\n\
                      directories of JP2K codestreams (directories must have\n\
                      an equal number of frames; the left eye is first)\n\
  -A <UL>           - Set DataEssenceCoding UL value in an Aux Data file\n\
  -C <UL>           - Set ChannelAssignment UL value in a PCM file\n\
  -h | -help        - Show help\n\
  -V                - Show version information\n\
  -e                - Encrypt MPEG or JP2K headers (default)\n\
  -E                - Do not encrypt MPEG or JP2K headers\n\
  -j <key-id-str>   - Write key ID instead of creating a random value\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -M                - Do not create HMAC values when writing\n\
  -m <expr>         - Write MCA labels using <expr>.  Example:\n\
                        51(L,R,C,LFE,Ls,Rs,),HI,VIN\n\
                        Note: The symbol '-' may be used for an unlabeled\n\
                              channel, but not within a soundfield.\n\
  -a <UUID>         - Specify the Asset ID of the file\n\
  -b <buffer-size>  - Specify size in bytes of picture frame buffer\n\
                      Defaults to 4,194,304 (4MB)\n\
  -d <duration>     - Number of frames to process, default all\n\
  -f <start-frame>  - Starting frame number, default 0\n\
  -l <label>        - Use given channel format label when writing MXF sound\n\
                      files. SMPTE 429-2 labels: '5.1', '6.1', '7.1',\n\
                      '7.1DS', 'WTF'\n\
                      Default is no label (valid for Interop only).\n\
  -L                - Write SMPTE UL values instead of MXF Interop\n\
  -P <UL>           - Set PictureEssenceCoding UL value in a JP2K file\n\
  -p <rate>         - fps of picture when wrapping PCM or JP2K:\n\
                      Use one of [23|24|25|30|48|50|60], 24 is default\n\
  -s                - Insert a Dolby Atmos synchronization channel when\n\
                      wrapping PCM. This implies a -L option(SMPTE ULs) and \n\
                      will overide -C and -l options with Configuration 4 \n\
                      Channel Assigment and no format label respectively. \n\
  -v                - Verbose, prints informative messages to stderr\n\
  -w                - When writing 377-4 MCA labels, use the WTF Channel\n\
                      assignment label instead of the standard MCA label\n\
  -W                - Read input file only, do not write output file\n\
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
  bool   verbose_flag;   // true if the verbose option was selected
  ui32_t fb_dump_size;   // number of bytes of frame buffer to dump
  bool   no_write_flag;  // true if no output files are to be written
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   stereo_image_flag; // if true, expect stereoscopic JP2K input (left eye first)
  bool   write_partial_pcm_flag; // if true, write the last frame of PCM input even when it is incomplete
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
  bool show_ul_values_flag;    /// if true, dump the UL table before going to work.
  Kumu::PathList_t filenames;  // list of filenames to be processed
  UL channel_assignment;
  UL picture_coding;
  UL aux_data_coding;
  bool dolby_atmos_sync_flag;  // if true, insert a Dolby Atmos Synchronization channel.
  ui32_t ffoa;                 // first frame of action for atmos wrapping
  ui32_t max_channel_count;    // max channel count for atmos wrapping
  ui32_t max_object_count;     // max object count for atmos wrapping
  bool use_interop_sound_wtf;  // make true to force WTF assignment label instead of MCA
  ASDCP::MXF::ASDCP_MCAConfigParser mca_config;

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
  const char* szPictureRate()
  {
    if ( picture_rate == 16 ) return "16";
    if ( picture_rate == 18 ) return "18.182";
    if ( picture_rate == 20 ) return "20";
    if ( picture_rate == 22 ) return "21.818";
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
    if ( picture_rate == 192 ) return "192";
    if ( picture_rate == 200 ) return "200";
    if ( picture_rate == 240 ) return "240";
    return "24";
  }

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), key_flag(false), key_id_flag(false), asset_id_flag(false),
    encrypt_header_flag(true), write_hmac(true),
    verbose_flag(false), fb_dump_size(0),
    no_write_flag(false), version_flag(false), help_flag(false), stereo_image_flag(false),
    write_partial_pcm_flag(false), start_frame(0),
    duration(0xffffffff), use_smpte_labels(false), j2c_pedantic(true),
    fb_size(FRAME_BUFFER_SIZE),
    channel_fmt(PCM::CF_NONE),
    ffoa(0), max_channel_count(10), max_object_count(118), // hard-coded sample atmos properties
    dolby_atmos_sync_flag(false),
    show_ul_values_flag(false),
    mca_config(g_dict),
    use_interop_sound_wtf(false)
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

	      case 'A':
		TEST_EXTRA_ARG(i, 'A');
		if ( ! aux_data_coding.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

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
		fb_size = Kumu::xabs(strtol(argv[i], 0, 10));

		if ( verbose_flag )
		  fprintf(stderr, "Frame Buffer size: %u bytes.\n", fb_size);

		break;

	      case 'C':
		TEST_EXTRA_ARG(i, 'C');
		if ( ! channel_assignment.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'E': encrypt_header_flag = false; break;
	      case 'e': encrypt_header_flag = true; break;

	      case 'f':
		TEST_EXTRA_ARG(i, 'f');
		start_frame = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'g': write_partial_pcm_flag = true; break;
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

	      case 'm':
		TEST_EXTRA_ARG(i, 'm');
		if ( ! mca_config.DecodeString(argv[i]) )
		  {
		    return;
		  }
		break;

	      case 'P':
		TEST_EXTRA_ARG(i, 'P');
		if ( ! picture_coding.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		picture_rate = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 's': dolby_atmos_sync_flag = true; break;
	      case 'u': show_ul_values_flag = true; break;
	      case 'V': version_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'w': use_interop_sound_wtf = true; break;
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
  Result_t result = Parser.OpenRead(Options.filenames.front());

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
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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
	      result = Parser.ReadFrame(FrameBuffer);

	      if ( ASDCP_SUCCESS(result) )
		{
		  if ( Options.verbose_flag )
		    FrameBuffer.Dump(stderr, Options.fb_dump_size);

		  if ( Options.encrypt_header_flag )
		    FrameBuffer.PlaintextOffset(0);
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

// return false if an error is discovered
bool
check_phfr_params(CommandOptions& Options, JP2K::PictureDescriptor& PDesc)
{
  Rational rate = Options.PictureRate();
  if ( rate != EditRate_96 && rate != EditRate_100 && rate != EditRate_120
       && rate != EditRate_192 && rate != EditRate_200 && rate != EditRate_240 )
    return true;

  if ( PDesc.StoredWidth > 2048 )
    {
      fprintf(stderr, "P-HFR files currently limited to 2K.\n");
      return false;
    }

  if ( ! Options.use_smpte_labels )
    {
      fprintf(stderr, "P-HFR files must be written using SMPTE labels. Use option '-L'.\n");
      return false;
    }

  // do not set the label if the user has already done so
  if ( ! Options.picture_coding.HasValue() )
    Options.picture_coding = UL(P_HFR_UL_2K);

  return true;
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
  Result_t result = ParserLeft.OpenRead(Options.filenames.front(), Options.j2c_pedantic);

  if ( ASDCP_SUCCESS(result) )
    {
      Options.filenames.pop_front();
      result = ParserRight.OpenRead(Options.filenames.front(), Options.j2c_pedantic);
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

  if ( ! check_phfr_params(Options, PDesc) )
    return RESULT_FAIL;

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
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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

      if ( ASDCP_SUCCESS(result) && Options.picture_coding.HasValue() )
	{
	  MXF::RGBAEssenceDescriptor *descriptor = 0;
	  Writer.OP1aHeader().GetMDObjectByType(g_dict->ul(MDD_RGBAEssenceDescriptor),
						reinterpret_cast<MXF::InterchangeObject**>(&descriptor));
	  descriptor->PictureEssenceCoding = Options.picture_coding;
	}
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
  Result_t result = Parser.OpenRead(Options.filenames.front(), Options.j2c_pedantic);

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

  if ( ! check_phfr_params(Options, PDesc) )
    return RESULT_FAIL;

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
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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

      if ( ASDCP_SUCCESS(result) && Options.picture_coding.HasValue() )
	{
	  MXF::RGBAEssenceDescriptor *descriptor = 0;
	  Writer.OP1aHeader().GetMDObjectByType(g_dict->ul(MDD_RGBAEssenceDescriptor),
						reinterpret_cast<MXF::InterchangeObject**>(&descriptor));
	  descriptor->PictureEssenceCoding = Options.picture_coding;
	}
    }

  if ( ASDCP_SUCCESS(result) )
    {
      ui32_t duration = 0;
      result = Parser.Reset();

      while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	      result = Parser.ReadFrame(FrameBuffer);

	      if ( ASDCP_SUCCESS(result) )
		{
		  if ( Options.verbose_flag )
		    FrameBuffer.Dump(stderr, Options.fb_dump_size);

		  if ( Options.encrypt_header_flag )
		    FrameBuffer.PlaintextOffset(0);
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

      if ( Options.use_smpte_labels && ADesc.ChannelFormat == PCM::CF_NONE && Options.mca_config.empty() )
	{
	  fprintf(stderr, "ATTENTION! Writing SMPTE audio without ChannelAssignment property (see options -C, -l and -m)\n");
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
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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

      if ( ASDCP_SUCCESS(result)
	   && ( Options.channel_assignment.HasValue()
		|| ! Options.mca_config.empty() ) )
	{
	  MXF::WaveAudioDescriptor *essence_descriptor = 0;
	  Writer.OP1aHeader().GetMDObjectByType(g_dict->ul(MDD_WaveAudioDescriptor),
						reinterpret_cast<MXF::InterchangeObject**>(&essence_descriptor));
	  assert(essence_descriptor);

	  if ( Options.mca_config.empty() )
	    {
	      essence_descriptor->ChannelAssignment = Options.channel_assignment;
	    }
	  else
	    {
	      if ( Options.mca_config.ChannelCount() != essence_descriptor->ChannelCount )
		{
		  fprintf(stderr, "MCA label count (%d) differs from essence stream channel count (%d).\n",
			  Options.mca_config.ChannelCount(), essence_descriptor->ChannelCount);
		  return RESULT_FAIL;
		}

	      if ( Options.channel_assignment.HasValue() )
		{
		  essence_descriptor->ChannelAssignment = Options.channel_assignment;
		}
	      else if ( Options.use_interop_sound_wtf )
		{
		  essence_descriptor->ChannelAssignment = g_dict->ul(MDD_DCAudioChannelCfg_4_WTF);
		}
	      else
		{
		  essence_descriptor->ChannelAssignment = g_dict->ul(MDD_DCAudioChannelCfg_MCA);
		}

	      // add descriptors to the essence_descriptor and header
	      ASDCP::MXF::InterchangeObject_list_t::iterator i;
	      for ( i = Options.mca_config.begin(); i != Options.mca_config.end(); ++i )
		{
		  if ( (*i)->GetUL() != UL(g_dict->ul(MDD_AudioChannelLabelSubDescriptor))
		       && (*i)->GetUL() != UL(g_dict->ul(MDD_SoundfieldGroupLabelSubDescriptor))
		       && (*i)->GetUL() != UL(g_dict->ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor)) )
		    {
		      fprintf(stderr, "Essence sub-descriptor is not an MCALabelSubDescriptor.\n");
		      (*i)->Dump();
		    }

		  Writer.OP1aHeader().AddChildObject(*i);
		  essence_descriptor->SubDescriptors.push_back((*i)->InstanceUID);
		  *i = 0; // parent will only free the ones we don't keep
		}
	    }
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

		  if ( Options.write_partial_pcm_flag )
		    {
		      result = RESULT_ENDOFFILE;
		      continue;
		    }
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

// Mix one or more plaintext PCM audio streams with a Dolby Atmos Synchronization channel and write them to a plaintext ASDCP file
// Mix one or more plaintext PCM audio streams with a Dolby Atmos Synchronization channel and write them to a ciphertext ASDCP file
//
Result_t
write_PCM_with_ATMOS_sync_file(CommandOptions& Options)
{
  AESEncContext*        Context = 0;
  HMACContext*          HMAC = 0;
  PCM::MXFWriter        Writer;
  PCM::FrameBuffer      FrameBuffer;
  PCM::AudioDescriptor  ADesc;
  Rational              PictureRate = Options.PictureRate();
  byte_t                IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG      RNG;

  WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
  if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
  else
	Kumu::GenRandomUUID(Info.AssetUUID);
  AtmosSyncChannelMixer Mixer(Info.AssetUUID);

  // set up essence parser
  Result_t result = Mixer.OpenRead(Options.filenames, PictureRate);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
  {
    if ( Mixer.ChannelCount() % 2 != 0 )
      {
        result = Mixer.AppendSilenceChannels(1);
      }

    Mixer.FillAudioDescriptor(ADesc);

    ADesc.EditRate = PictureRate;
    FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));
    ADesc.ChannelFormat = PCM::CF_CFG_4;

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
    Info.LabelSetType = LS_MXF_SMPTE;
    fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");

    // configure encryption
    if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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
    result = Mixer.Reset();
    ui32_t duration = 0;

    while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
	  result = Mixer.ReadFrame(FrameBuffer);

	  if ( ASDCP_SUCCESS(result) )
      {
        if ( FrameBuffer.Size() != FrameBuffer.Capacity() )
		{
		  fprintf(stderr, "WARNING: Last frame read was short, PCM input is possibly not frame aligned.\n");
		  fprintf(stderr, "Expecting %u bytes, got %u.\n", FrameBuffer.Capacity(), FrameBuffer.Size());

		  if ( Options.write_partial_pcm_flag )
		    {
		      result = RESULT_ENDOFFILE;
		      continue;
		    }
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
  Result_t result = Parser.OpenRead(Options.filenames.front());

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

      // 428-7 IN 429-5 always uses SMPTE labels
      Info.LabelSetType = LS_MXF_SMPTE;
      fprintf(stderr, "ATTENTION! Writing SMPTE Universal Labels\n");

      // configure encryption
      if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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
	result = Writer.OpenWrite(Options.out_file, Info, TDesc);
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

// Write one or more plaintext Dolby ATMOS bytestreams to a plaintext ASDCP file
// Write one or more plaintext Dolby ATMOS bytestreams to a ciphertext ASDCP file
//
Result_t
write_dolby_atmos_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  ATMOS::MXFWriter         Writer;
  DCData::FrameBuffer       FrameBuffer(Options.fb_size);
  ATMOS::AtmosDescriptor ADesc;
  DCData::SequenceParser    Parser;
  byte_t                  IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG        RNG;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
  {
    Parser.FillDCDataDescriptor(ADesc);
    ADesc.EditRate = Options.PictureRate();
    // TODO: fill AtmosDescriptor
    ADesc.FirstFrame = Options.ffoa;
    ADesc.MaxChannelCount = Options.max_channel_count;
    ADesc.MaxObjectCount = Options.max_object_count;
    Kumu::GenRandomUUID(ADesc.AtmosID);
    ADesc.AtmosVersion = 1;
    if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Dolby ATMOS Data\n");
	  fputs("AtmosDescriptor:\n", stderr);
      fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
      ATMOS::AtmosDescriptorDump(ADesc);
	}
  }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
  {
    WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
    if ( Options.asset_id_flag )
      memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
    else
      Kumu::GenRandomUUID(Info.AssetUUID);

    Info.LabelSetType = LS_MXF_SMPTE;

      // configure encryption
    if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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
    ui32_t duration = 0;
    result = Parser.Reset();

    while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
      result = Parser.ReadFrame(FrameBuffer);

      if ( ASDCP_SUCCESS(result) )
      {
        if ( Options.verbose_flag )
          FrameBuffer.Dump(stderr, Options.fb_dump_size);

        if ( Options.encrypt_header_flag )
          FrameBuffer.PlaintextOffset(0);
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

// Write one or more plaintext Aux Data (ST 429-14) bytestreams to a plaintext ASDCP file
// Write one or more plaintext Aux Data (ST 429-14) bytestreams to a ciphertext ASDCP file
//
Result_t
write_aux_data_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  DCData::MXFWriter       Writer;
  DCData::FrameBuffer     FrameBuffer(Options.fb_size);
  DCData::DCDataDescriptor DDesc;
  DCData::SequenceParser  Parser;
  byte_t                  IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG        RNG;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
  {
    Parser.FillDCDataDescriptor(DDesc);
    memcpy(DDesc.DataEssenceCoding, Options.aux_data_coding.Value(), Options.aux_data_coding.Size());
    DDesc.EditRate = Options.PictureRate();

    if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Aux Data\n");
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  DCData::DCDataDescriptorDump(DDesc);
	}
  }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
  {
    WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
    if ( Options.asset_id_flag )
      memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
    else
      Kumu::GenRandomUUID(Info.AssetUUID);

    Info.LabelSetType = LS_MXF_SMPTE;

      // configure encryption
    if( Options.key_flag )
	{
	  Kumu::GenRandomUUID(Info.ContextID);
	  Info.EncryptedEssence = true;

	  if ( Options.key_id_flag )
	    {
	      memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
	    }
	  else
	    {
	      create_random_uuid(Info.CryptographicKeyID);
	    }

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
      result = Writer.OpenWrite(Options.out_file, Info, DDesc);
  }

  if ( ASDCP_SUCCESS(result) )
  {
    ui32_t duration = 0;
    result = Parser.Reset();

    while ( ASDCP_SUCCESS(result) && duration++ < Options.duration )
	{
      result = Parser.ReadFrame(FrameBuffer);

      if ( ASDCP_SUCCESS(result) )
      {
        if ( Options.verbose_flag )
          FrameBuffer.Dump(stderr, Options.fb_dump_size);

        if ( Options.encrypt_header_flag )
          FrameBuffer.PlaintextOffset(0);
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

//
int
main(int argc, const char** argv)
{
  Result_t result = RESULT_OK;
  char     str_buf[64];
  g_dict = &ASDCP::DefaultSMPTEDict();

  CommandOptions Options(argc, argv);

  if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.show_ul_values_flag )
    {
      if ( Options.use_smpte_labels )
	DefaultSMPTEDict().Dump(stdout);
      else
	DefaultInteropDict().Dump(stdout);
    }

  if ( Options.version_flag || Options.help_flag || Options.show_ul_values_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  EssenceType_t EssenceType;
  result = ASDCP::RawEssenceType(Options.filenames.front(), EssenceType);

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
	  if ( Options.dolby_atmos_sync_flag )
	    {
	      result = write_PCM_with_ATMOS_sync_file(Options);
	    }
	  else
	    {
	      result = write_PCM_file(Options);
	    }
	  break;
	  
	case ESS_TIMED_TEXT:
	  result = write_timed_text_file(Options);
	  break;

	case ESS_DCDATA_DOLBY_ATMOS:
	  result = write_dolby_atmos_file(Options);
	  break;

	case ESS_DCDATA_UNKNOWN:
	  if ( ! Options.aux_data_coding.HasValue() )
	    {
	      fprintf(stderr, "Option \"-A <UL>\" is required for Aux Data essence.\n");
	      return 3;
	    }
	  else
	    {
	      result = write_aux_data_file(Options);
	    }
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
