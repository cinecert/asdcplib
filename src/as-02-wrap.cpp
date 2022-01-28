/*
Copyright (c) 2011-2020, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
John Hurst, Wolfgang Ruppel

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
/*! \file    as-02-wrap.cpp
    \version $Id$       
    \brief   AS-02 file manipulation utility

  This program wraps IMF essence (picture or sound) in to an AS-02 MXF file.

  For more information about AS-02, please refer to the header file AS_02.h
  For more information about asdcplib, please refer to the header file AS_DCP.h
*/
#include <KM_fileio.h>
#include <KM_xml.h>
#include <KM_prng.h>
#include <AS_02.h>
#include "AS_02_ACES.h"
#include <PCMParserList.h>
#include <Metadata.h>

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
      { 0x7d, 0x83, 0x6e, 0x16, 0x37, 0xc7, 0x4c, 0x22,
	0xb2, 0xe0, 0x46, 0xa7, 0x17, 0xe8, 0x4f, 0x42 };
      
      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      CompanyName = "WidgetCo";
      ProductName = "as-02-wrap";
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

#define TEST_EXTRA_ARG_STRING(i,s)						\
  if ( ++i >= argc || argv[(i)][0] == '-' ) {				\
    fprintf(stderr, "Argument not found for option -%s.\n", (s));	\
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
Copyright (c) 2011-2018, Robert Scheler, Heiko Sparenberg Fraunhofer IIS, John Hurst\n\n\
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
       %s [options] <input-file>+ <output-file>\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

  fprintf(stream, "\
Options:\n\
  -h | -help        - Show help\n\
  -V                - Show version information\n\
  -a <uuid>         - Specify the Asset ID of the file\n\
  -A <w>/<h>        - Set aspect ratio for image (default 4/3)\n\
  -b <buffer-size>  - Specify size in bytes of picture frame buffer\n\
                      Defaults to 4,194,304 (4MB)\n\
  -c <num>          - Select the IMF color system to be signaled:\n\
                      Application 2 (2067-20): 1, 2, or 3\n\
                      Application 2e (2067-21): 4, 5, or 7\n\
                      All color system values assume YCbCr; also use -R for RGB\n\
  -C <ul>           - Set ChannelAssignment UL value\n\
  -d <duration>     - Number of frames to process, default all\n\
  -D <depth>        - Component depth for YCbCr or RGB images (default: 10)\n\
  -e                - Encrypt JP2K headers (default)\n\
  -E                - Do not encrypt JP2K headers\n\
  -F (0|1)          - Set field dominance for interlaced image (default: 0)\n\
  -g <rfc-5646-code>\n\
                    - Create MCA labels having the given RFC 5646 language code\n\
                      (requires option \"-m\") -- Also used with -G to set the\n\
                      value of the TextMIMEMediaType property\n\
  -G <filename>     - Filename of XML resource to be carried per RP 2057 Generic\n\
                      Stream. May be issued multiple times.\n\
  -i                - Indicates input essence is interlaced fields (forces -Y)\n\
  -j <key-id-str>   - Write key ID instead of creating a random value\n\
  -J                - Write J2CLayout\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -l <first>,<second>\n\
                    - Integer values that set the VideoLineMap\n\
  -m <expr>         - Write MCA labels using <expr>.  Example:\n\
                        51(L,R,C,LFE,Ls,Rs,),HI,VIN\n\
  -M                - Do not create HMAC values when writing\n\
  -n <UL>           - Set the TransferCharacteristic UL\n\
  -o <min>,<max>    - Mastering Display luminance, cd*m*m, e.g., \".05,100\"\n\
  -O <rx>,<ry>,<gx>,<gy>,<bx>,<by>,<wx>,<wy>\n\
                    - Mastering Display Color Primaries and white point\n\
                      e.g., \".64,.33,.3,.6,.15,.06,.3457,.3585\"\n\
  -p <ul>           - Set broadcast profile\n\
  -P <string>       - Set NamespaceURI property when creating timed text MXF\n\
  -q <UL>           - Set the CodingEquations UL\n\
  -r <n>/<d>        - Edit Rate of the output file.  24/1 is the default\n\
  -R                - Indicates RGB image essence (default except with -c)\n\
  -s <seconds>      - Duration of a frame-wrapped partition (default 60)\n\
  -t <min>          - Set RGB component minimum code value (default: 0)\n\
  -T <max>          - Set RGB component maximum code value (default: 1023)\n\
  -u                - Print UL catalog to stdout\n\
  -U <URI>          - ISXD (RDD47) document URI (use 'auto' to read the\n\
                      namespace name from the first edit unit)\n\
  -v                - Verbose, prints informative messages to stderr\n\
  -W                - Read input file only, do not write source file\n\
  -x <int>          - Horizontal subsampling degree (default: 2)\n\
  -X <int>          - Vertical subsampling degree (default: 2)\n\
  -y <white-ref>[,<black-ref>[,<color-range>]]\n\
                    - Same as -Y but White Ref, Black Ref and Color Range are\n\
                      set from the given argument\n\
  -Y                - Indicates YCbCr image essence (default with -c), uses\n\
                      default values for White Ref, Black Ref and Color Range,\n\
                       940,64,897, indicating 10 bit standard Video Range\n\
  -z                - Fail if j2c inputs have unequal parameters (default)\n\
  -Z                - Ignore unequal parameters in j2c inputs\n\
\n\
  --mca-audio-content-kind <string>\n\
                    - UL value for MCA descriptor MCAAudioContentKind property\n\
  --mca-audio-element-kind <string>\n\
                    - UL value for MCA descriptor MCAAudioElementKind property\n\
\n\
\n\
Options specific to ACES ST2067-50:\n\
  -suba <string>    - Create ACES Picture SubDescriptor, set <string> as ACESAuthoringInformation,\n\
                      uses values from -o and -O, if present\n\
  -subt <directoryPath>  - \n\
                      Create one Target Frame SubDescriptor per PNG or TIFF file in <directoryPath>,\n\
                      and  wrap each PNG or TIFF file  as ancillary resource\n\
                      Requires additional options -tfi, -tft, -tfc, -tfr\n\
  -tfi <int>[,<int>*]    - \n\
                      List of TargetFrameIndex values in Target Frame SubDescriptor corresponding to the \n\
                      list of Target Frame frame files in <directoryPath> as given by option -subt\n\
  -tft <string>     - Target Frame Transfer Characteristics Symbol, e.g. TransferCharacteristic_ITU709\n\
  -tfc <string>     - Target Frame Color Primaries Symbol, e.g. ColorPrimaries_ITU709\n\
  -tfr <min>,<max>  - Target Frame Component Min/Max Ref in Target Frame SubDescriptor\n\
  -tfv <string>     - Target Frame Viewing Environment Symbol, e.g. HDTVReferenceViewingEnvironment\n\
\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\n");
}

const float chromaticity_scale = 50000.0;
//
ui32_t
set_primary_from_token(const std::string& token, ui16_t& primary)
{
  float raw_value = strtod(token.c_str(),0);

  if ( raw_value == 0.0 || raw_value > 1.0 )
    {
      fprintf(stderr, "Invalid coordinate value \"%s\".\n", token.c_str());
      return false;
    }

  primary = floor(0.5 + ( raw_value * chromaticity_scale ));
  return true;
}

const float luminance_scale = 10000.0;
//
ui32_t
set_luminance_from_token(const std::string& token, ui32_t& luminance)
{
  float raw_value = strtod(token.c_str(),0);

  if ( raw_value == 0.0 || raw_value > 400000.0 )
    {
      fprintf(stderr, "Invalid luminance value \"%s\".\n", token.c_str());
      return false;
    }

  luminance = floor(0.5 + ( raw_value * luminance_scale ));
  return true;
}

#define SET_LUMINANCE(p,t)			\
  if ( ! set_luminance_from_token(t, p) ) {	\
    return false;				\
  }

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
  bool   write_j2clayout; // true if a J2CLayout field should be written
  bool use_cdci_descriptor; // 
  Rational edit_rate;    // edit rate of JP2K sequence
  ui32_t fb_size;        // size of picture frame buffer
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  bool   key_id_flag;    // true if a key ID was given
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  byte_t asset_id_value[UUIDlen];// value of asset ID (when asset_id_flag is true)
  bool show_ul_values_flag;    // if true, dump the UL table before going tp work.
  Kumu::PathList_t filenames;  // list of filenames to be processed

  UL channel_assignment, picture_coding, transfer_characteristic, color_primaries, coding_equations;
  ASDCP::MXF::AS02_MCAConfigParser mca_config;
  std::string language;

  ui32_t rgba_MaxRef;
  ui32_t rgba_MinRef;

  ui32_t horizontal_subsampling;
  ui32_t vertical_subsampling;
  ui32_t component_depth;
  ui8_t frame_layout;
  ASDCP::Rational aspect_ratio;
  bool aspect_ratio_flag;
  ui8_t field_dominance;
  ui32_t mxf_header_size;
  ui32_t cdci_BlackRefLevel; 
  ui32_t cdci_WhiteRefLevel;
  ui32_t cdci_ColorRange;

  ui32_t md_min_luminance, md_max_luminance;
  ASDCP::MXF::ThreeColorPrimaries md_primaries;
  ASDCP::MXF::ColorPrimary md_white_point;

  //new attributes for AS-02 support 
  AS_02::IndexStrategy_t index_strategy; //Shim parameter index_strategy_frame/clip
  ui32_t partition_space; //Shim parameter partition_spacing

  // ISXD
  std::string isxd_document_namespace;
  std::list<std::string> global_isxd_metadata;

  //
  MXF::LineMapPair line_map;
  bool line_map_flag;
  std::string out_file, profile_name; //
  std::string mca_audio_element_kind, mca_audio_content_kind;

  //ST 2067-50 options
  bool aces_authoring_information_flag, aces_picture_subdescriptor_flag, target_frame_subdescriptor_flag, target_frame_index_flag;
  bool target_frame_transfer_characteristics_flag, target_frame_color_primaries_flag, target_frame_min_max_ref_flag;
  bool target_frame_viewing_environment_flag;
  std::string aces_authoring_information;
  std::string target_frame_directory;
  std::list <ui64_t> target_frame_index_list;
  UL target_frame_transfer_characteristics, target_frame_color_primaries, target_frame_viewing_environment;
  ui32_t target_frame_min_ref, target_frame_max_ref;  
  //
  bool set_video_line_map(const std::string& arg)
  {
    const char* sep_str = strrchr(arg.c_str(), ',');

    if ( sep_str == 0 )
      {
	fprintf(stderr, "Expecting <first>,<second>\n");
	return false;
      }

    line_map.First = Kumu::xabs(strtol(arg.c_str(), 0, 10));
    line_map.Second = Kumu::xabs(strtol(sep_str+1, 0, 10));
    return true;
  }

  //
  bool set_video_ref(const std::string& arg)
  {
    std::list<std::string> ref_tokens = Kumu::km_token_split(arg, ",");

    switch ( ref_tokens.size() )
      {
      case 3:
	cdci_ColorRange = Kumu::xabs(strtol(ref_tokens.back().c_str(), 0, 10));
	ref_tokens.pop_back();
      case 2:
	cdci_BlackRefLevel = Kumu::xabs(strtol(ref_tokens.back().c_str(), 0, 10));
	ref_tokens.pop_back();
      case 1:
	cdci_WhiteRefLevel = Kumu::xabs(strtol(ref_tokens.back().c_str(), 0, 10));
	break;

      default:
	fprintf(stderr, "Expecting <white-ref>[,<black-ref>[,<color-range>]]\n");
	return false;
      }

    if ( cdci_WhiteRefLevel > 65535 || cdci_BlackRefLevel > 65535 || cdci_ColorRange > 65535 )
      {
	fprintf(stderr, "Unexpected CDCI video referece levels.\n");
	return false;
      }

    return true;
  }

  //
  bool set_display_primaries(const std::string& arg)
  {
    std::list<std::string> coordinate_tokens = Kumu::km_token_split(arg, ",");
    if ( coordinate_tokens.size() != 8 )
      {
	fprintf(stderr, "Expecting four coordinate pairs.\n");
	return false;
      }

    std::list<std::string>::const_iterator i = coordinate_tokens.begin();
    if ( ! set_primary_from_token(*(i++), md_primaries.First.X) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.First.Y) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Second.X) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Second.Y) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Third.X) ) return false;
    if ( ! set_primary_from_token(*(i++), md_primaries.Third.Y) ) return false;
    if ( ! set_primary_from_token(*(i++), md_white_point.X) ) return false;
    if ( ! set_primary_from_token(*i, md_white_point.Y) ) return false;

    return true;
  }

  //
  bool set_display_luminance(const std::string& arg)
  {
    std::list<std::string> luminance_tokens = Kumu::km_token_split(arg, ",");
    if ( luminance_tokens.size() != 2 )
      {
	fprintf(stderr, "Expecting a luminance pair.\n");
	return false;
      }

    if ( ! set_luminance_from_token(luminance_tokens.front(), md_min_luminance) ) return false;
    if ( ! set_luminance_from_token(luminance_tokens.back(), md_max_luminance) ) return false;

    return true;
  }

  //
  bool set_color_system_from_arg(const char* arg)
  {
    assert(arg);

    switch ( *arg )
      {
	// Application 2 (ST 2067-20)
      case '1':
	coding_equations = g_dict->ul(MDD_CodingEquations_601);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_ITU709);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU470_PAL);
	use_cdci_descriptor = true;
	break;

      case '2':
	coding_equations = g_dict->ul(MDD_CodingEquations_601);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_ITU709);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_SMPTE170M);
	use_cdci_descriptor = true;
	break;

      case '3':
	coding_equations = g_dict->ul(MDD_CodingEquations_709);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_ITU709);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU709);
	use_cdci_descriptor = true;
	break;

	// Application 2e (ST 2067-21)
      case '4':
	coding_equations = g_dict->ul(MDD_CodingEquations_709);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_IEC6196624_xvYCC);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU709);
	use_cdci_descriptor = true;
	break;

      case '5':
	coding_equations = g_dict->ul(MDD_CodingEquations_709);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_ITU2020);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU2020);
	use_cdci_descriptor = true;
	break;

      case '7':
	coding_equations = g_dict->ul(MDD_CodingEquations_Rec2020);
	transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_SMPTEST2084);
	color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU2020);
	use_cdci_descriptor = true;
	break;

      default:
	fprintf(stderr, "Unrecognized color system number, expecting one of 1-5 or 7.\n");
	return false;
      }
    
    return true;
  }

  bool set_target_frame_min_max_code_value(const std::string& arg)
  {
    std::list<std::string> range_tokens = Kumu::km_token_split(arg, ",");
    if ( range_tokens.size() != 2 )
      {
	fprintf(stderr, "Expecting a code value pair.\n");
	return false;
      }

    target_frame_min_ref = strtol(range_tokens.front().c_str(), 0 , 10);
    target_frame_max_ref = strtol(range_tokens.back().c_str(), 0 , 10);

    return true;
  }

  bool set_target_frame_index_list(const std::string& arg, std::list<ui64_t>& r_target_frame_index_list)
  {
    std::list<std::string> index_tokens = Kumu::km_token_split(arg, ",");
    if ( index_tokens.size() == 0 )
      {
	fprintf(stderr, "Expecting at least one Target Frame Index.\n");
	return false;
      }


    std::list<std::string>::const_iterator i;
    for (i = index_tokens.begin(); i != index_tokens.end(); i++) {
    	r_target_frame_index_list.push_back(strtoll(i->c_str(), 0, 10));
    }

    return true;
  }


  CommandOptions(int argc, const char** argv) :
    error_flag(true), key_flag(false), key_id_flag(false), asset_id_flag(false),
    encrypt_header_flag(true), write_hmac(true), verbose_flag(false), fb_dump_size(0),
    no_write_flag(false), version_flag(false), help_flag(false),
    duration(0xffffffff), j2c_pedantic(true), write_j2clayout(false), use_cdci_descriptor(false),
    edit_rate(24,1), fb_size(FRAME_BUFFER_SIZE),
    show_ul_values_flag(false), index_strategy(AS_02::IS_FOLLOW), partition_space(60),
    mca_config(g_dict), rgba_MaxRef(1023), rgba_MinRef(0),
    horizontal_subsampling(2), vertical_subsampling(2), component_depth(10),
    frame_layout(0), aspect_ratio(ASDCP::Rational(4,3)), aspect_ratio_flag(false), field_dominance(0),
    mxf_header_size(16384), cdci_WhiteRefLevel(940), cdci_BlackRefLevel(64), cdci_ColorRange(897),
    md_min_luminance(0), md_max_luminance(0), line_map(0,0), line_map_flag(false),
	aces_authoring_information_flag(false), aces_picture_subdescriptor_flag(false), target_frame_subdescriptor_flag(false),
	target_frame_index_flag(false), target_frame_transfer_characteristics_flag(false), target_frame_color_primaries_flag(false),
	target_frame_min_max_ref_flag(false), target_frame_viewing_environment_flag(false)
  {
    memset(key_value, 0, KeyLen);
    memset(key_id_value, 0, UUIDlen);

    coding_equations = g_dict->ul(MDD_CodingEquations_709);
    color_primaries = g_dict->ul(MDD_ColorPrimaries_ITU709);
    transfer_characteristic = g_dict->ul(MDD_TransferCharacteristic_ITU709);
    std::string mca_config_str;

    for ( int i = 1; i < argc; i++ )
      {

	if ( (strcmp( argv[i], "-help") == 0) )
	  {
	    help_flag = true;
	    continue;
	  }
         
	if ( (strcmp( argv[i], "-suba") == 0) )
	  {
	    aces_picture_subdescriptor_flag = true;
	    if ((++i < argc) && (argv[i][0] != '-')) {
	    	aces_authoring_information = argv[i];
	    	aces_authoring_information_flag = true;
	    } else i--;
	    continue;
	  }

	if ( (strcmp( argv[i], "-subt") == 0) )
	  {
	    target_frame_subdescriptor_flag = true;
	    TEST_EXTRA_ARG_STRING(i, "subt");
	    target_frame_directory = argv[i];
	    continue;
	  }

	if ( (strcmp( argv[i], "-tfi") == 0) )
	  {
		TEST_EXTRA_ARG_STRING(i, "tfi");
	    if (set_target_frame_index_list(argv[i], target_frame_index_list)) {
			target_frame_index_flag = true;
	    }
	    continue;
	  }

	if ( (strcmp( argv[i], "-tft") == 0) )
	  {
		TEST_EXTRA_ARG_STRING(i, "tft");
		//
		const ASDCP::MDDEntry* entry = g_dict->FindSymbol(std::string(argv[i]));
		if (entry) {
			target_frame_transfer_characteristics_flag = true;
			target_frame_transfer_characteristics = entry->ul;
			fprintf(stderr, "target_frame_transfer_characteristic %s\n", entry->name);
		}
	    continue;
	  }

	if ( (strcmp( argv[i], "-tfc") == 0) )
	  {
		TEST_EXTRA_ARG_STRING(i, "tfc");
		//
		const ASDCP::MDDEntry* entry = g_dict->FindSymbol(std::string(argv[i]));
		if (entry) {
			target_frame_color_primaries_flag = true;
			target_frame_color_primaries = entry->ul;
			fprintf(stderr, "target_frame_color_primaries %s\n", entry->name);
		}
	    continue;
	  }

	if ( (strcmp( argv[i], "-tfr") == 0) )
	  {
		TEST_EXTRA_ARG(i, 'o');
		if ( ! set_target_frame_min_max_code_value(argv[i]) )
		  {
		    return;
		  }
		target_frame_min_max_ref_flag = true;
	    continue;
	  }

	if ( (strcmp( argv[i], "-tfv") == 0) )
	  {
		TEST_EXTRA_ARG_STRING(i, "tfv");
		//
		const ASDCP::MDDEntry* entry = g_dict->FindSymbol(std::string(argv[i]));
		if (entry) {
			target_frame_viewing_environment_flag = true;
			target_frame_viewing_environment = entry->ul;
			fprintf(stderr, "target_frame_viewing_environment %s\n", entry->name);
		}
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
		else
		{
			aspect_ratio_flag = true;
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

	      case 'c':
		TEST_EXTRA_ARG(i, 'c');
		if ( ! set_color_system_from_arg(argv[i]) )
		  {
		    return;
		  }
		break;

	      case 'C':
		TEST_EXTRA_ARG(i, 'C');
		if ( ! channel_assignment.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding ChannelAssignment UL value: %s\n", argv[i]);
		    return;
		  }
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
		language = argv[i];
		break;

	      case 'G':
		TEST_EXTRA_ARG(i, 'G');
		global_isxd_metadata.push_back(argv[i]);
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

	      case 'J': write_j2clayout = true; break;

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
		TEST_EXTRA_ARG(i, 'y');
		if ( ! set_video_line_map(argv[i]) )
		  {
		    return;
		  } else {
                    line_map_flag = true;
		  }
		break;

	      case 'M': write_hmac = false; break;

	      case 'm':
		TEST_EXTRA_ARG(i, 'm');
		mca_config_str = argv[i];
		break;

	      case 'n':
		TEST_EXTRA_ARG(i, 'n');
		if ( ! transfer_characteristic.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding TransferCharacteristic UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'O':
		TEST_EXTRA_ARG(i, 'O');
		if ( ! set_display_primaries(argv[i]) )
		  {
		    return;
		  }
		break;

	      case 'o':
		TEST_EXTRA_ARG(i, 'o');
		if ( ! set_display_luminance(argv[i]) )
		  {
		    return;
		  }
		break;

	      case 'P':
		TEST_EXTRA_ARG(i, 'P');
		profile_name = argv[i];
		break;

	      case 'p':
		TEST_EXTRA_ARG(i, 'p');
		if ( ! picture_coding.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding PictureEssenceCoding UL value: %s\n", argv[i]);
		    return;
		  }
		break;

	      case 'q':
		TEST_EXTRA_ARG(i, 'q');
		if ( ! coding_equations.DecodeHex(argv[i]) )
		  {
		    fprintf(stderr, "Error decoding CodingEquations UL value: %s\n", argv[i]);
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
		isxd_document_namespace = argv[i];
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
		// default 10 bit video range YUV, ref levels already set
		break;

	      case 'y':
		// Use values provided as argument, sharp tool, be careful
		use_cdci_descriptor = true;
		TEST_EXTRA_ARG(i, 'y');
		if ( ! set_video_ref(argv[i]) )
		  {
		    return;
		  }
		break;

	      case 'Z': j2c_pedantic = false; break;
	      case 'z': j2c_pedantic = true; break;

	      default:
		fprintf(stderr, "Unrecognized option: %s\n", argv[i]);
		return;
	      }
	  }
	else if ( argv[i][0] == '-' && argv[i][1] == '-' && isalpha(argv[i][2]) )
	  {
	    if ( strcmp(argv[i]+2, "mca-audio-content-kind") == 0 )
	      {
		if ( ++i >= argc || argv[(i)][0] == '-' )
		  {
		    fprintf(stderr, "Argument not found for option -mca-audio-content-kind.\n");
		    return;
		  }
		
		mca_audio_content_kind = argv[i];
	      }
	    else if ( strcmp(argv[i]+2, "mca-audio-element-kind") == 0 )
	      {
		if ( ++i >= argc || argv[(i)][0] == '-' )
		  {
		    fprintf(stderr, "Argument not found for option -mca-audio-element-kind.\n");
		    return;
		  }

		mca_audio_element_kind = argv[i];
	      }
	    else
	      {
		fprintf(stderr, "Unrecognized argument: %s\n", argv[i]);
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

    if ( ! mca_config_str.empty() )
      {
	if ( language.empty() )
	  {
	    if ( ! mca_config.DecodeString(mca_config_str) )
	      {
		return;
	      }
	  }
	else
	  {
	    if ( ! mca_config.DecodeString(mca_config_str, language) )
	      {
		return;
	      }
	  }
      }

    if ( help_flag || version_flag || show_ul_values_flag )
      {
	return;
      }

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

  Result_t PCM_ADesc_to_MD(ASDCP::PCM::AudioDescriptor& ADesc, ASDCP::MXF::WaveAudioDescriptor* ADescObj);
}

// Write one or more plaintext JPEG 2000 codestreams to a plaintext AS-02 file
// Write one or more plaintext JPEG 2000 codestreams to a ciphertext AS-02 file
//
Result_t
write_JP2K_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  AS_02::JP2K::MXFWriter  Writer;
  JP2K::FrameBuffer       FrameBuffer(Options.fb_size);
  JP2K::SequenceParser    Parser;
  ASDCP::MXF::FileDescriptor *essence_descriptor = 0;
  ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;
  ASDCP::MXF::JPEG2000PictureSubDescriptor *jp2k_sub_descriptor = NULL;

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
	  fprintf(stderr, "JPEG 2000 pictures\n");
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
	      tmp_dscr->CodingEquations = Options.coding_equations;
	      tmp_dscr->TransferCharacteristic = Options.transfer_characteristic;
	      tmp_dscr->ColorPrimaries = Options.color_primaries;
	      tmp_dscr->PictureEssenceCoding = Options.picture_coding;
	      tmp_dscr->HorizontalSubsampling = Options.horizontal_subsampling;
	      tmp_dscr->VerticalSubsampling = Options.vertical_subsampling;
	      tmp_dscr->ComponentDepth = Options.component_depth;
	      tmp_dscr->FrameLayout = Options.frame_layout;
	      tmp_dscr->AspectRatio = Options.aspect_ratio;
	      tmp_dscr->FieldDominance = Options.field_dominance;
	      tmp_dscr->WhiteReflevel = Options.cdci_WhiteRefLevel;
	      tmp_dscr->BlackRefLevel = Options.cdci_BlackRefLevel;
	      tmp_dscr->ColorRange = Options.cdci_ColorRange;
	      if (Options.line_map_flag)  tmp_dscr->VideoLineMap = Options.line_map;

	      if ( Options.md_min_luminance || Options.md_max_luminance )
		{
		  tmp_dscr->MasteringDisplayMinimumLuminance = Options.md_min_luminance;
		  tmp_dscr->MasteringDisplayMaximumLuminance = Options.md_max_luminance;
		}

	      if ( Options.md_primaries.HasValue() )
		{
		  tmp_dscr->MasteringDisplayPrimaries = Options.md_primaries;
		  tmp_dscr->MasteringDisplayWhitePointChromaticity = Options.md_white_point;
		}

	      essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);

	      if (Options.write_j2clayout)
		{
		  jp2k_sub_descriptor = static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back());
		  if (Options.component_depth == 16)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_YUV_16);
		    }
		  else if (Options.component_depth == 12)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_YUV_12);
		    }
		  else if (Options.component_depth == 10)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_YUV_10);
		    }
		  else if (Options.component_depth == 8)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_YUV_8);
		    }
		  else
		    {
		      fprintf(stderr, "Warning: could not determine J2CLayout to write.\n");
		    }
		}
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
	      tmp_dscr->CodingEquations = Options.coding_equations;
	      tmp_dscr->TransferCharacteristic = Options.transfer_characteristic;
	      tmp_dscr->ColorPrimaries = Options.color_primaries;
	      tmp_dscr->ScanningDirection = 0;
	      tmp_dscr->PictureEssenceCoding = Options.picture_coding;
	      tmp_dscr->ComponentMaxRef = Options.rgba_MaxRef;
	      tmp_dscr->ComponentMinRef = Options.rgba_MinRef;
	      if (Options.line_map_flag)  tmp_dscr->VideoLineMap = Options.line_map;

	      if ( Options.md_min_luminance || Options.md_max_luminance )
		{
		  tmp_dscr->MasteringDisplayMinimumLuminance = Options.md_min_luminance;
		  tmp_dscr->MasteringDisplayMaximumLuminance = Options.md_max_luminance;
		}

	      if ( Options.md_primaries.HasValue() )
		{
		  tmp_dscr->MasteringDisplayPrimaries = Options.md_primaries;
		  tmp_dscr->MasteringDisplayWhitePointChromaticity = Options.md_white_point;
		}

	      essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);

	      if (Options.write_j2clayout)
		{
		  jp2k_sub_descriptor = static_cast<ASDCP::MXF::JPEG2000PictureSubDescriptor*>(essence_sub_descriptors.back());
		  if (Options.component_depth == 16)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_RGB_16);
		    }
		  else if (Options.component_depth == 12)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_RGB_12);
		    }
		  else if (Options.component_depth == 10)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_RGB_10);
		    }
		  else if (Options.component_depth == 8)
		    {
		      jp2k_sub_descriptor->J2CLayout = ASDCP::MXF::RGBALayout(ASDCP::MXF::RGBAValue_RGB_8);
		    }
		  else
		    {
		      fprintf(stderr, "Warning: could not determine J2CLayout to write.\n");
		    }
		}
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

#ifdef HAVE_OPENSSL
      // configure encryption
      if( Options.key_flag )
	{
      byte_t                  IV_buf[CBC_BLOCK_SIZE];
      Kumu::FortunaRNG        RNG;
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
#endif // HAVE_OPENSSL

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
    result = Writer.Finalize();

  return result;
}


//------------------------------------------------------------------------------------------
// ACES essence


// Write one or more plaintext ACES codestreams to a plaintext AS-02 file
// Write one or more plaintext ACES codestreams to a ciphertext AS-02 file
//
Result_t
write_ACES_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  AS_02::ACES::MXFWriter  Writer;
  AS_02::ACES::FrameBuffer       FrameBuffer(Options.fb_size);
  AS_02::ACES::SequenceParser    Parser;
  ASDCP::MXF::FileDescriptor *essence_descriptor = 0;
  ASDCP::MXF::InterchangeObject_list_t essence_sub_descriptors;
  AS_02::ACES::PictureDescriptor PDesc;
  AS_02::ACES::ResourceList_t resource_list_t;

  // set up essence parser
  //Result_t result = Parser.OpenRead(Options.filenames.front().c_str(), Options.j2c_aces_pedantic);
  // set up essence parser
  std::list<std::string> target_frame_file_list;
  if (Options.target_frame_subdescriptor_flag)
  {
    Kumu::DirScannerEx dir_reader;
    Kumu::DirectoryEntryType_t ft;
    std::string next_item;
    Result_t result = dir_reader.Open(Options.target_frame_directory);
    if ( KM_SUCCESS(result) )
    {
      while ( KM_SUCCESS(dir_reader.GetNext(next_item, ft)) )
      {
          if ( next_item[0] == '.' ) continue; // no hidden files
          std::string tmp_path = Kumu::PathJoin(Options.target_frame_directory, next_item);
          target_frame_file_list.push_back(tmp_path);
      }
    }
  }
  Result_t result = Parser.OpenRead(Options.filenames.front().c_str(), Options.j2c_pedantic, target_frame_file_list);

  // set up MXF writer
  if (ASDCP_SUCCESS(result))
  {
    Parser.FillPictureDescriptor(PDesc);
    Parser.FillResourceList(resource_list_t);
    PDesc.EditRate = Options.edit_rate;

    if (Options.verbose_flag)
    {
      fprintf(stderr, "ACES pictures\n");
      fputs("PictureDescriptor:\n", stderr);
      fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
      AS_02::ACES::PictureDescriptorDump(PDesc);
    }

    ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
    Kumu::GenRandomValue(tmp_dscr->InstanceUID);
    ASDCP::MXF::ACESPictureSubDescriptor* aces_picture_subdescriptor = new ASDCP::MXF::ACESPictureSubDescriptor(g_dict);
    Kumu::GenRandomValue(aces_picture_subdescriptor->InstanceUID);
    result = AS_02::ACES::ACES_PDesc_to_MD(PDesc, *g_dict, *tmp_dscr);
    ASDCP::MXF::ContainerConstraintsSubDescriptor* gc_subdescriptor = new ASDCP::MXF::ContainerConstraintsSubDescriptor(g_dict);
    Kumu::GenRandomValue(gc_subdescriptor->InstanceUID);
    essence_sub_descriptors.push_back(gc_subdescriptor);

    if (ASDCP_SUCCESS(result))
    {
      if (Options.aspect_ratio_flag) tmp_dscr->AspectRatio = Options.aspect_ratio;

      if (Options.aces_picture_subdescriptor_flag)
      {
        if (Options.aces_authoring_information_flag) aces_picture_subdescriptor->ACESAuthoringInformation = Options.aces_authoring_information;
        if (Options.md_primaries.HasValue())
        {
          aces_picture_subdescriptor->ACESMasteringDisplayPrimaries = Options.md_primaries;
          aces_picture_subdescriptor->ACESMasteringDisplayWhitePointChromaticity = Options.md_white_point;
        }
        if (Options.md_min_luminance && Options.md_max_luminance)
        {
          aces_picture_subdescriptor->ACESMasteringDisplayMinimumLuminance = Options.md_min_luminance;
          aces_picture_subdescriptor->ACESMasteringDisplayMaximumLuminance = Options.md_max_luminance;
        }
        essence_sub_descriptors.push_back(aces_picture_subdescriptor);
      }

      if (Options.target_frame_subdescriptor_flag)
      {
        AS_02::ACES::ResourceList_t::iterator it;
        ui32_t EssenceStreamID = 10;  //start with 10, same value in AncillaryResourceWriter
        for (it = resource_list_t.begin(); it != resource_list_t.end(); it++ )
        {
          ASDCP::MXF::TargetFrameSubDescriptor* target_frame_subdescriptor = new ASDCP::MXF::TargetFrameSubDescriptor(g_dict);
          Kumu::GenRandomValue(target_frame_subdescriptor->InstanceUID);
          target_frame_subdescriptor->TargetFrameAncillaryResourceID.Set(it->ResourceID);
          target_frame_subdescriptor->MediaType.assign(AS_02::ACES::MIME2str(it->Type));
          target_frame_subdescriptor->TargetFrameEssenceStreamID = EssenceStreamID++;
          if (Options.target_frame_index_flag)
          {
            if (Options.target_frame_index_list.size() > 0)
            {
              target_frame_subdescriptor->TargetFrameIndex = Options.target_frame_index_list.front();
              Options.target_frame_index_list.pop_front();
            } else
            {
              fprintf(stderr, "Insufficient number of Target Frame Index values provided\n");
              fprintf(stderr, "Number of Target Frames (%lu) should match number of Target Frame Index values\n", resource_list_t.size());
            }
          }
          if (Options.target_frame_transfer_characteristics_flag) target_frame_subdescriptor->TargetFrameTransferCharacteristic = Options.target_frame_transfer_characteristics;
          if (Options.target_frame_color_primaries_flag) target_frame_subdescriptor->TargetFrameColorPrimaries = Options.target_frame_color_primaries;
          if (Options.target_frame_min_max_ref_flag)
          {
            target_frame_subdescriptor->TargetFrameComponentMinRef = Options.target_frame_min_ref;
            target_frame_subdescriptor->TargetFrameComponentMaxRef = Options.target_frame_max_ref;
          }
          if (Options.aces_picture_subdescriptor_flag) target_frame_subdescriptor->ACESPictureSubDescriptorInstanceID = aces_picture_subdescriptor->InstanceUID;
          essence_sub_descriptors.push_back(target_frame_subdescriptor);
        }
      }

      essence_descriptor = static_cast<ASDCP::MXF::FileDescriptor*>(tmp_dscr);
      if (Options.line_map_flag)  tmp_dscr->VideoLineMap = Options.line_map;
    }
  }

  if (ASDCP_SUCCESS(result) && !Options.no_write_flag)
  {
    WriterInfo Info = s_MyInfo;  // fill in your favorite identifiers here
    Info.LabelSetType = LS_MXF_SMPTE;

    if (Options.asset_id_flag)
      memcpy(Info.AssetUUID, Options.asset_id_value, UUIDlen);
    else
      Kumu::GenRandomUUID(Info.AssetUUID);

#ifdef HAVE_OPENSSL
    byte_t                  IV_buf[CBC_BLOCK_SIZE];
    Kumu::FortunaRNG        RNG;

    // configure encryption
    if (Options.key_flag)
    {
      Kumu::GenRandomUUID(Info.ContextID);
      Info.EncryptedEssence = true;

      if (Options.key_id_flag)
      {
        memcpy(Info.CryptographicKeyID, Options.key_id_value, UUIDlen);
      }
      else
      {
        create_random_uuid(Info.CryptographicKeyID);
      }

      Context = new AESEncContext;
      result = Context->InitKey(Options.key_value);

      if (ASDCP_SUCCESS(result))
        result = Context->SetIVec(RNG.FillRandom(IV_buf, CBC_BLOCK_SIZE));

      if (ASDCP_SUCCESS(result) && Options.write_hmac)
      {
        Info.UsesHMAC = true;
        HMAC = new HMACContext;
        result = HMAC->InitKey(Options.key_value, Info.LabelSetType);
      }
    }
#endif

    if (ASDCP_SUCCESS(result))
    {
      result = Writer.OpenWrite(Options.out_file, Info, essence_descriptor, essence_sub_descriptors,
        Options.edit_rate, AS_02::ACES::ResourceList_t(), Options.mxf_header_size, Options.index_strategy, Options.partition_space);
    }
  }

  if (ASDCP_SUCCESS(result))
  {
    ui32_t duration = 0;
    result = Parser.Reset();

    while (ASDCP_SUCCESS(result) && duration++ < Options.duration)
    {
      result = Parser.ReadFrame(FrameBuffer);

      if (ASDCP_SUCCESS(result))
      {
        if (Options.verbose_flag)
          FrameBuffer.Dump(stderr, Options.fb_dump_size);

        if (Options.key_flag && Options.encrypt_header_flag)
          FrameBuffer.PlaintextOffset(0);
      }

      if (ASDCP_SUCCESS(result) && !Options.no_write_flag)
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

    if (result == RESULT_ENDOFFILE)
      result = RESULT_OK;
  }
    AS_02::ACES::ResourceList_t::const_iterator ri;
    for ( ri = resource_list_t.begin() ; ri != resource_list_t.end() && ASDCP_SUCCESS(result); ri++ )
    {
      result = Parser.ReadAncillaryResource((*ri).filePath, FrameBuffer);

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



  if (ASDCP_SUCCESS(result) && !Options.no_write_flag)
    result = Writer.Finalize();

  return result;
}




//------------------------------------------------------------------------------------------
// PCM essence
// Write one or more plaintext PCM audio streams to a plaintext AS-02 file
// Write one or more plaintext PCM audio streams to a ciphertext AS-02 file
//
Result_t
write_PCM_file(CommandOptions& Options)
{
  AESEncContext*    Context = 0;
  HMACContext*      HMAC = 0;
  PCMParserList     Parser;
  AS_02::PCM::MXFWriter    Writer;
  PCM::FrameBuffer  FrameBuffer;
  ASDCP::MXF::WaveAudioDescriptor *essence_descriptor = 0;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames, Options.edit_rate);

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      ASDCP::PCM::AudioDescriptor ADesc;
      Parser.FillAudioDescriptor(ADesc);

      ADesc.EditRate = Options.edit_rate;
      FrameBuffer.Capacity(PCM::CalcFrameBufferSize(ADesc));

      if ( Options.verbose_flag )
	{
	  char buf[64];
	  fprintf(stderr, "%.1fkHz PCM Audio, %s fps (%u spf)\n",
		  ADesc.AudioSamplingRate.Quotient() / 1000.0,
		  RationalToString(Options.edit_rate, buf, 64),
		  PCM::CalcSamplesPerFrame(ADesc));
	  fputs("AudioDescriptor:\n", stderr);
	  PCM::AudioDescriptorDump(ADesc);
	}

      essence_descriptor = new ASDCP::MXF::WaveAudioDescriptor(g_dict);

      result = ASDCP::PCM_ADesc_to_MD(ADesc, essence_descriptor);

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

	  // This marks all soundfield groups using the same MCA property values
	  MXF::InterchangeObject_list_t::iterator i;
	  for ( i = Options.mca_config.begin(); i != Options.mca_config.end(); ++i )
	    {
	      MXF::SoundfieldGroupLabelSubDescriptor * desc = dynamic_cast<MXF::SoundfieldGroupLabelSubDescriptor*>(*i);
	      if ( desc != 0 )
		{
		  if ( ! Options.mca_audio_content_kind.empty() )
		    {
		      desc->MCAAudioContentKind = Options.mca_audio_content_kind;
		    }
		  if ( ! Options.mca_audio_element_kind.empty() )
		    {
		      desc->MCAAudioElementKind = Options.mca_audio_element_kind;
		    }
		}
	    }

	  essence_descriptor->ChannelAssignment = g_dict->ul(MDD_IMFAudioChannelCfg_MCA);
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

#ifdef HAVE_OPENSSL
      // configure encryption
      if( Options.key_flag )
	{
      byte_t            IV_buf[CBC_BLOCK_SIZE];
      Kumu::FortunaRNG  RNG;
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
#endif // HAVE_OPENSSL

      if ( ASDCP_SUCCESS(result) )
	{
	  result = Writer.OpenWrite(Options.out_file.c_str(), Info, essence_descriptor,
				    Options.mca_config, Options.edit_rate);
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


// Write one or more plaintext timed text streams to a plaintext AS-02 file
// Write one or more plaintext timed text streams to a ciphertext AS-02 file
//
Result_t
write_timed_text_file(CommandOptions& Options)
{
  AESEncContext*    Context = 0;
  HMACContext*      HMAC = 0;
  AS_02::TimedText::ST2052_TextParser  Parser;
  AS_02::TimedText::MXFWriter    Writer;
  TimedText::FrameBuffer  FrameBuffer;
  TimedText::TimedTextDescriptor TDesc;
  byte_t            IV_buf[CBC_BLOCK_SIZE];

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      Parser.FillTimedTextDescriptor(TDesc);
      TDesc.EditRate = Options.edit_rate;
      TDesc.ContainerDuration = Options.duration;
      FrameBuffer.Capacity(Options.fb_size);

      if ( ! Options.profile_name.empty() )
	{
	  TDesc.NamespaceName = Options.profile_name;
	}

      if ( Options.verbose_flag )
	{
	  fputs("IMF Timed-Text Descriptor:\n", stderr);
	  TimedText::DescriptorDump(TDesc);
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

#ifdef HAVE_OPENSSL
      // configure encryption
      if( Options.key_flag )
	{
      Kumu::FortunaRNG  RNG;
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
#endif // HAVE_OPENSSL

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
bool
get_current_dms_text_descriptor(AS_02::ISXD::MXFWriter& writer, ASDCP::MXF::GenericStreamTextBasedSet *&text_object)
{
  std::list<MXF::InterchangeObject*> object_list;
  writer.OP1aHeader().GetMDObjectsByType(DefaultSMPTEDict().ul(MDD_GenericStreamTextBasedSet), object_list);

  if ( object_list.empty() )
    {
      return false;
    }

  text_object = dynamic_cast<MXF::GenericStreamTextBasedSet*>(object_list.back());
  assert(text_object != 0);
  return true;
}
		      

// Write one or more plaintext Aux Data bytestreams to a plaintext AS-02 file
// Write one or more plaintext Aux Data bytestreams to a ciphertext AS-02 file
//
Result_t
write_isxd_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  AS_02::ISXD::MXFWriter Writer;
  DCData::FrameBuffer     FrameBuffer(Options.fb_size);
  DCData::SequenceParser  Parser;

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "ISXD Data\n");
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

#ifdef HAVE_OPENSSL
      // configure encryption
    if( Options.key_flag )
	{
      byte_t                  IV_buf[CBC_BLOCK_SIZE];
      Kumu::FortunaRNG        RNG;
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
#endif // HAVE_OPENSSL

    if ( ASDCP_SUCCESS(result) )
      {
	if ( Options.isxd_document_namespace == "auto" )
	  {
	    // get ns of first item
	    std::string ns_prefix, type_name, namespace_name;
	    result = Parser.ReadFrame(FrameBuffer);

	    if ( ASDCP_SUCCESS(result) )
	      {
		Kumu::AttributeList doc_attr_list;
		result = GetXMLDocType(FrameBuffer.RoData(), FrameBuffer.Size(), ns_prefix, type_name,
				       namespace_name, doc_attr_list) ? RESULT_OK : RESULT_FAIL;
	      }

	    if ( ASDCP_SUCCESS(result) && ! namespace_name.empty() )
	      {
		Options.isxd_document_namespace = namespace_name;
	      }
	    else
	      {
		fprintf(stderr, "Unable to parse an XML namespace name from the input document.\n");
		return RESULT_FAIL;
	      }
	  }

	result = Writer.OpenWrite(Options.out_file, Info, Options.isxd_document_namespace, Options.edit_rate);
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
	{
	  result = RESULT_OK;
	}
    }
  
  if ( KM_SUCCESS(result) && ! Options.no_write_flag )
    {
      ASDCP::FrameBuffer global_metadata;
      std::list<std::string>::iterator i;
      
      for ( i = Options.global_isxd_metadata.begin(); i != Options.global_isxd_metadata.end(); ++i )
	{
	  ui32_t file_size = Kumu::FileSize(*i);
	  result = global_metadata.Capacity(file_size);

	  if ( KM_SUCCESS(result) )
	    {
	      ui32_t read_count = 0;
	      Kumu::FileReader Reader;
	      std::string namespace_name;

	      result = Reader.OpenRead(*i);

	      if ( KM_SUCCESS(result) )
		{
		  result = Reader.Read(global_metadata.Data(), file_size, &read_count);
		}

	      if ( KM_SUCCESS(result) )
		{
		  if ( file_size != read_count) 
		    return RESULT_READFAIL;

		  global_metadata.Size(read_count);
		  std::string ns_prefix, type_name;
		  Kumu::AttributeList doc_attr_list;
		  result = GetXMLDocType(global_metadata.RoData(), global_metadata.Size(), ns_prefix, type_name,
					 namespace_name, doc_attr_list) ? RESULT_OK : RESULT_FAIL;
		}

	      if ( KM_SUCCESS(result) )
		{
		  result = Writer.AddDmsGenericPartUtf8Text(global_metadata, Context, HMAC);
		}

	      if ( KM_SUCCESS(result) )
		{
		  ASDCP::MXF::GenericStreamTextBasedSet *text_object = 0;
		  get_current_dms_text_descriptor(Writer, text_object);
		  assert(text_object);
		  text_object->TextMIMEMediaType = "text/xml";
		  text_object->TextDataDescription = namespace_name;

		  // this is not really useful when inserting multiple objects because
		  // it cannot be set per object without some other CLI syntax for
		  // associating language codes with 2057 blobs, e.g., <filename>:<lang>
		  text_object->RFC5646TextLanguageCode = Options.language;
		}
	    }
	}

      if ( KM_SUCCESS(result) )
	{
	  result = Writer.Finalize();
	}
    }

  return result;
}

//
int
main(int argc, const char** argv)
{
  Result_t result = RESULT_OK;
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
	 // PB
	case ::ESS_AS02_ACES:
	  result = write_ACES_file(Options);
	  break;
	case ESS_PCM_24b_48k:
	case ESS_PCM_24b_96k:
	  result = write_PCM_file(Options);
	  break;

	case ESS_TIMED_TEXT:
	  result = write_timed_text_file(Options);
	  break;

	case ESS_DCDATA_UNKNOWN:
	  if ( ! Options.isxd_document_namespace.empty() )
	    {
	      result = write_isxd_file(Options);      
	    }
	  else
	    {
	      fprintf(stderr, "%s: Unknown synchronous data file type, not AS-02-compatible essence.\n",
		      Options.filenames.front().c_str());
	      return 5;
	    }
	  break;

	default:
	  fprintf(stderr, "%s: Unknown file type, not AS-02-compatible essence.\n",
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
// end as-02-wrap.cpp
//
