/*
Copyright (c) 2011-2016, John Hurst

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
/*! \file    phdr-wrap.cpp
    \version $Id$       
    \brief   prototype wrapping for HDR images in AS-02

  This program wraps IMF essence (P-HDR picture) in to an AS-02 MXF file.
*/

#include <KM_fileio.h>
#include <KM_prng.h>
#include <AS_02_PHDR.h>

using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;
const ASDCP::Dictionary *g_dict = 0;


const char*
RationalToString(const ASDCP::Rational& r, char* buf, const ui32_t& len)
{
  snprintf(buf, len, "%d/%d", r.Numerator, r.Denominator);
  return buf;
}



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
      { 0xef, 0xe4, 0x59, 0xab, 0xbe, 0x0f, 0x4c, 0x7d,
	0xb3, 0xa2, 0xb8, 0x96, 0x79, 0xe2, 0x3e, 0x8e };
      
      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      CompanyName = "WidgetCo";
      ProductName = "phdr-wrap";
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
Copyright (c) 2011-2015, John Hurst\n\n\
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
       %s [-a <uuid>] [-A <w>/<h>] [-b <buffer-size>] [-C <UL>] [-d <duration>]\n\
            [-D <depth>] [-e|-E] [-i] [-j <key-id-string>] [-k <key-string>]\n\
            [-M] [-m <expr>] [-p <ul>] [-r <n>/<d>] [-R] [-s <seconds>]\n\
            [-t <min>] [-T <max>] [-u] [-v] [-W] [-x <int>] [-X <int>] [-Y]\n\
            [-z|-Z] <input-file>+ <output-file>\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -h | -help        - Show help\n\
  -V                - Show version information\n\
  -a <uuid>         - Specify the Asset ID of the file\n\
  -A <w>/<h>        - Set aspect ratio for image (default 4/3)\n\
  -b <buffer-size>  - Specify size in bytes of picture frame buffer\n\
                      Defaults to 4,194,304 (4MB)\n\
  -C <ul>           - Set ChannelAssignment UL value\n\
  -d <duration>     - Number of frames to process, default all\n\
  -D <depth>        - Component depth for YCbCr images (default: 10)\n\
  -e                - Encrypt JP2K headers (default)\n\
  -E                - Do not encrypt JP2K headers\n\
  -F (0|1)          - Set field dominance for interlaced image (default: 0)\n\
  -g <filename>     - Write global metadata from the named PIDM file.\n\
  -i                - Indicates input essence is interlaced fields (forces -Y)\n\
  -j <key-id-str>   - Write key ID instead of creating a random value\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -M                - Do not create HMAC values when writing\n\
  -m <filename>     - Filename of master metadata instance (the contents of\n\
                        which will be placed in the MXF wrapper)\n\
  -p <ul>           - Set broadcast profile\n\
  -r <n>/<d>        - Edit Rate of the output file.  24/1 is the default\n\
  -R                - Indicates RGB image essence (default)\n\
  -s <seconds>      - Duration of a frame-wrapped partition (default 60)\n\
  -t <min>          - Set RGB component minimum code value (default: 0)\n\
  -T <max>          - Set RGB component maximum code value (default: 1023)\n\
  -u                - Print UL catalog to stderr\n\
  -U <UL>           - Set DataEssenceCoding UL value in an Aux Data file\n\
  -v                - Verbose, prints informative messages to stderr\n\
  -W                - Read input file only, do not write source file\n\
  -x <int>          - Horizontal subsampling degree (default: 2)\n\
  -X <int>          - Vertical subsampling degree (default: 2)\n\
  -Y                - Indicates YCbCr image essence (default: RGB)\n\
  -z                - Fail if j2c inputs have unequal parameters (default)\n\
  -Z                - Ignore unequal parameters in j2c inputs\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\n");
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
  bool   encrypt_header_flag; // true if j2c headers are to be encrypted
  bool   write_hmac;     // true if HMAC values are to be generated and written
  bool   verbose_flag;   // true if the verbose option was selected
  ui32_t fb_dump_size;   // number of bytes of frame buffer to dump
  bool   no_write_flag;  // true if no output files are to be written
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  ui32_t duration;       // number of frames to be processed
  bool   j2c_pedantic;   // passed to JP2K::SequenceParser::OpenRead
  bool use_cdci_descriptor; // 
  Rational edit_rate;    // edit rate of JP2K sequence
  ui32_t fb_size;        // size of picture frame buffer
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  bool   key_id_flag;    // true if a key ID was given
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  byte_t asset_id_value[UUIDlen];// value of asset ID (when asset_id_flag is true)
  std::string out_file; //
  bool show_ul_values_flag;    /// if true, dump the UL table before going tp work.
  Kumu::PathList_t filenames;  // list of filenames to be processed

  UL picture_coding;
  ui32_t rgba_MaxRef;
  ui32_t rgba_MinRef;

  ui32_t horizontal_subsampling;
  ui32_t vertical_subsampling;
  ui32_t component_depth;
  ui8_t frame_layout;
  ASDCP::Rational aspect_ratio;
  ui8_t field_dominance;
  ui32_t mxf_header_size;

  //new attributes for AS-02 support 
  AS_02::IndexStrategy_t index_strategy; //Shim parameter index_strategy_frame/clip
  ui32_t partition_space; //Shim parameter partition_spacing

  std::string PHDR_master_metadata;
  UL aux_data_coding;
  std::string global_metadata_filename;

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), key_flag(false), key_id_flag(false), asset_id_flag(false),
    encrypt_header_flag(true), write_hmac(true), verbose_flag(false), fb_dump_size(0),
    no_write_flag(false), version_flag(false), help_flag(false),
    duration(0xffffffff), j2c_pedantic(true), use_cdci_descriptor(false), edit_rate(24,1), fb_size(FRAME_BUFFER_SIZE),
    show_ul_values_flag(false), index_strategy(AS_02::IS_FOLLOW), partition_space(60),
    rgba_MaxRef(1023), rgba_MinRef(0),
    horizontal_subsampling(2), vertical_subsampling(2), component_depth(10),
    frame_layout(0), aspect_ratio(ASDCP::Rational(4,3)), field_dominance(0),
    mxf_header_size(16384)
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
	      case 'A':
		TEST_EXTRA_ARG(i, 'A');
		if ( ! DecodeRational(argv[i], aspect_ratio) )
		  {
		    fprintf(stderr, "Error decoding aspect ratio value: %s\n", argv[i]);
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

	      case 'D':
		TEST_EXTRA_ARG(i, 'D');
		component_depth = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'd':
		TEST_EXTRA_ARG(i, 'd');
		duration = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'E': encrypt_header_flag = false; break;
	      case 'e': encrypt_header_flag = true; break;

	      case 'F':
		TEST_EXTRA_ARG(i, 'F');
		field_dominance = Kumu::xabs(strtol(argv[i], 0, 10));
		if ( field_dominance > 1 )
		  {
		    fprintf(stderr, "Field dominance value must be \"0\" or \"1\"\n");
		    return;
		  }
		break;

	      case 'g':
		TEST_EXTRA_ARG(i, 'g');
		global_metadata_filename = argv[i];
		break;

	      case 'h': help_flag = true; break;

	      case 'i':
		frame_layout = 1;
		use_cdci_descriptor = true;
		break;

	      case 'j':
		key_id_flag = true;
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

	      case 'M': write_hmac = false; break;

	      case 'm':
		TEST_EXTRA_ARG(i, 'm');
		if ( KM_FAILURE(Kumu::ReadFileIntoString(argv[i], PHDR_master_metadata) ) )
		  {
		    fprintf(stderr, "Unable to read metadata file %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		if ( ! picture_coding.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding PictureEssenceCoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'r':
		TEST_EXTRA_ARG(i, 'r');
		if ( ! DecodeRational(argv[i], edit_rate) )
		  {
		    fprintf(stderr, "Error decoding edit rate value: %s\n", argv[i]);
		    return;
		  }

		break;

	      case 'R':
		use_cdci_descriptor = false;
		break;

	      case 's':
		TEST_EXTRA_ARG(i, 's');
		partition_space = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 't':
		TEST_EXTRA_ARG(i, 't');
		rgba_MinRef = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'T':
		TEST_EXTRA_ARG(i, 'T');
		rgba_MaxRef = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'u': show_ul_values_flag = true; break;

	      case 'U':
		TEST_EXTRA_ARG(i, 'U');
		if ( ! aux_data_coding.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'V': version_flag = true; break;
	      case 'v': verbose_flag = true; break;
	      case 'W': no_write_flag = true; break;

	      case 'x':
		TEST_EXTRA_ARG(i, 'x');
		horizontal_subsampling = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'X':
		TEST_EXTRA_ARG(i, 'X');
		vertical_subsampling = Kumu::xabs(strtol(argv[i], 0, 10));
		break;

	      case 'Y':
		use_cdci_descriptor = true;
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

    if ( ! picture_coding.HasValue() )
      {
	picture_coding = UL(g_dict->ul(MDD_JP2KEssenceCompression_BroadcastProfile_1));
      }

    error_flag = false;
  }
};


//------------------------------------------------------------------------------------------
// JPEG 2000 essence

namespace ASDCP {
  Result_t JP2K_PDesc_to_MD(const ASDCP::JP2K::PictureDescriptor& PDesc,
			    const ASDCP::Dictionary& dict,
			    ASDCP::MXF::GenericPictureEssenceDescriptor& GenericPictureEssenceDescriptor,
			    ASDCP::MXF::JPEG2000PictureSubDescriptor& EssenceSubDescriptor);
}

// Write one or more plaintext JPEG 2000 codestreams to a plaintext AS-02 file
// Write one or more plaintext JPEG 2000 codestreams to a ciphertext AS-02 file
//
Result_t
write_JP2K_file(CommandOptions& Options)
{
  AS_02::PHDR::MXFWriter  Writer;
  AS_02::PHDR::FrameBuffer FrameBuffer(Options.fb_size);
  AS_02::PHDR::SequenceParser Parser;

  AESEncContext* Context = 0;
  HMACContext* HMAC = 0;
  byte_t IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG RNG;

  ASDCP::MXF::FileDescriptor *essence_descriptor = 0;
  ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front().c_str(), Options.j2c_pedantic);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      ASDCP::JP2K::PictureDescriptor PDesc;
      Parser.FillPictureDescriptor(PDesc);
      PDesc.EditRate = Options.edit_rate;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "JPEG 2000 P-HDR pictures\n");
	  fputs("PictureDescriptor:\n", stderr);
          fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	  JP2K::PictureDescriptorDump(PDesc);
	}

      if ( Options.use_cdci_descriptor )
	{
	  ASDCP::MXF::CDCIEssenceDescriptor* tmp_dscr = new ASDCP::MXF::CDCIEssenceDescriptor(g_dict);
	  essence_sub_descriptors.push_back(new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict));
	  
	  result = ASDCP::JP2K_PDesc_to_MD(PDesc, *g_dict,
					   *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr),
					   *static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back()));

	  if ( ASDCP_SUCCESS(result) )
	    {
	      tmp_dscr->PictureEssenceCoding = Options.picture_coding;
	      tmp_dscr->HorizontalSubsampling = Options.horizontal_subsampling;
	      tmp_dscr->VerticalSubsampling = Options.vertical_subsampling;
	      tmp_dscr->ComponentDepth = Options.component_depth;
	      tmp_dscr->FrameLayout = Options.frame_layout;
	      tmp_dscr->AspectRatio = Options.aspect_ratio;
	      tmp_dscr->FieldDominance = Options.field_dominance;
	      essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);
	    }
	}
      else
	{ // use RGB
	  ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
	  essence_sub_descriptors.push_back(new ASDCP::MXF::JPEG2000PictureSubDescriptor(g_dict));
	  
	  result = ASDCP::JP2K_PDesc_to_MD(PDesc, *g_dict,
					   *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr),
					   *static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back()));

	  if ( ASDCP_SUCCESS(result) )
	    {
	      tmp_dscr->PictureEssenceCoding = UL(g_dict->ul(MDD_JP2KEssenceCompression_BroadcastProfile_1));
	      tmp_dscr->ComponentMaxRef = Options.rgba_MaxRef;
	      tmp_dscr->ComponentMinRef = Options.rgba_MinRef;
	      essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);
	    }
	}
    }

  if ( ASDCP_SUCCESS(result) && ! Options.no_write_flag )
    {
      WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
      Info.LabelSetType = LS_MXF_SMPTE;

      if ( Options.asset_id_flag )
	memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
      else
	Kumu::GenRandomUUID(Info.AssetUUID);

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
	{
	  result = Writer.OpenWrite(Options.out_file, Info, essence_descriptor, essence_sub_descriptors,
				    Options.edit_rate, Options.mxf_header_size, Options.index_strategy, Options.partition_space);
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
    result = Writer.Finalize(Options.PHDR_master_metadata);

  return result;
}



// Write one or more plaintext Aux Data bytestreams to a plaintext AS-02 file
// Write one or more plaintext Aux Data bytestreams to a ciphertext AS-02 file
//
Result_t
write_aux_data_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  AS_02::PIDM::MXFWriter Writer;
  DCData::FrameBuffer     FrameBuffer(Options.fb_size);
  DCData::SequenceParser  Parser;
  byte_t                  IV_buf[CBC_BLOCK_SIZE];
  Kumu::FortunaRNG        RNG;

  if ( ! Options.global_metadata_filename.empty() )
    {
      if ( ! Kumu::PathIsFile(Options.global_metadata_filename) )
	{
	  fprintf(stderr, "No such file or filename: \"%s\".\n", Options.global_metadata_filename.c_str());
	  return RESULT_PARAM;
	}
    }

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
  {

    if ( Options.verbose_flag )
	{
	  fprintf(stderr, "Aux Data\n");
	  fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
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
      {
	result = Writer.OpenWrite(Options.out_file, Info, Options.aux_data_coding, Options.edit_rate);
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
    {
      if ( Options.global_metadata_filename.empty() )
	{
	  result = Writer.Finalize();
	}
      else
	{
	  ASDCP::FrameBuffer global_metadata;
	  ui32_t file_size = Kumu::FileSize(Options.global_metadata_filename);
	  result = global_metadata.Capacity(file_size);

	  if ( ASDCP_SUCCESS(result) )
	    {
	      ui32_t read_count = 0;
	      Kumu::FileReader Reader;

	      result = Reader.OpenRead(Options.global_metadata_filename);

	      if ( ASDCP_SUCCESS(result) )
		result = Reader.Read(global_metadata.Data(), file_size, &read_count);
    
	      if ( ASDCP_SUCCESS(result) )
		{
		  if ( file_size != read_count) 
		    return RESULT_READFAIL;

		  global_metadata.Size(read_count);
		}
	    }
  
	  result = Writer.Finalize(global_metadata);
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
  g_dict = &ASDCP::DefaultSMPTEDict();
  assert(g_dict);

  CommandOptions Options(argc, argv);

  if ( Options.version_flag )
    banner();

  if ( Options.help_flag )
    usage();

  if ( Options.show_ul_values_flag )
    {
      g_dict->Dump(stdout);
    }

  if ( Options.version_flag || Options.help_flag || Options.show_ul_values_flag )
    return 0;

  if ( Options.error_flag )
    {
      fprintf(stderr, "There was a problem. Type %s -h for help.\n", PROGRAM_NAME);
      return 3;
    }

  EssenceType_t EssenceType;
  result = ASDCP::RawEssenceType(Options.filenames.front().c_str(), EssenceType);

  if ( ASDCP_SUCCESS(result) )
    {
      switch ( EssenceType )
	{
	case ESS_JPEG_2000:
	  result = write_JP2K_file(Options);
	  break;

	case ESS_DCDATA_UNKNOWN:
	  if ( ! Options.aux_data_coding.HasValue() )
	    {
	      fprintf(stderr, "Option \"-U <UL>\" is required for Aux Data essence.\n");
	      return 3;
	    }
	  else
	    {
	      result = write_aux_data_file(Options);
	    }
	  break;

	default:
	  fprintf(stderr, "%s: Unknown file type, not P-HDR-compatible essence.\n",
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
// end phdr-wrap.cpp
//
