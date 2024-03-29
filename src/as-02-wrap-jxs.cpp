/*
Copyright (c) 2011-2020, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
John Hurst, Wolfgang Ruppel, Thomas Richter

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
/*! \file    as-02-wrap-jxs.cpp
    \version $Id$       
    \brief   AS-02 file wrapping utility

  This program wraps JPEG XS picture essence in an AS-02 MXF file.

  For more information about AS-02, please refer to the header file AS_02.h
  For more information about asdcplib, please refer to the header file AS_DCP.h
*/

#include <KM_prng.h>
#include <Metadata.h>
#include "JXS.h"
#include "AS_02_JXS.h"

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
        { 0x40, 0xf5, 0x4c, 0x1d, 0x46, 0xf0, 0x41, 0xd3,
          0x8b, 0x68, 0x84, 0xb6, 0x29, 0xf8, 0xad, 0x74 };


      
      memcpy(ProductUUID, default_ProductUUID_Data, UUIDlen);
      CompanyName = "WidgetCo";
      ProductName = "as-02-wrap-jxs";
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
  -d <duration>     - Number of frames to process, default all\n\
  -D <depth>        - Component depth for YCbCr images (default: 10)\n\
  -e                - Encrypt JP2K headers (default)\n\
  -E                - Do not encrypt JP2K headers\n\
  -F (0|1)          - Set field dominance for interlaced image (default: 0)\n\
  -i                - Indicates input essence is interlaced fields (forces -Y)\n\
  -j <key-id-str>   - Write key ID instead of creating a random value\n\
  -k <key-string>   - Use key for ciphertext operations\n\
  -l <first>,<second>\n\
                    - Integer values that set the VideoLineMap\n\
  -M                - Do not create HMAC values when writing\n\
  -n <UL>           - Set the TransferCharacteristic UL\n\
  -o <min>,<max>    - Mastering Display luminance, cd*m*m, e.g., \".05,100\"\n\
  -O <rx>,<ry>,<gx>,<gy>,<bx>,<by>,<wx>,<wy>\n\
                    - Mastering Display Color Primaries and white point\n\
                      e.g., \".64,.33,.3,.6,.15,.06,.3457,.3585\"\n\
  -p <ul>           - Set broadcast profile\n\
  -q <UL>           - Set the CodingEquations UL\n\
  -r <n>/<d>        - Edit Rate of the output file.  24/1 is the default\n\
  -R                - Indicates RGB image essence (default except with -c)\n\
  -s <seconds>      - Duration of a frame-wrapped partition (default 60)\n\
  -t <min>          - Set RGB component minimum code value (default: 0)\n\
  -T <max>          - Set RGB component maximum code value (default: 1023)\n\
  -u                - Print UL catalog to stdout\n\
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
  bool use_cdci_descriptor; // 
  Rational edit_rate;    // edit rate of JP2K sequence
  ui32_t fb_size;        // size of picture frame buffer
  byte_t key_value[KeyLen];  // value of given encryption key (when key_flag is true)
  bool   key_id_flag;    // true if a key ID was given
  byte_t key_id_value[UUIDlen];// value of given key ID (when key_id_flag is true)
  byte_t asset_id_value[UUIDlen];// value of asset ID (when asset_id_flag is true)
  bool show_ul_values_flag;    // if true, dump the UL table before going tp work.
  Kumu::PathList_t filenames;  // list of filenames to be processed

  UL picture_coding, transfer_characteristic, color_primaries, coding_equations;

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

  //
  MXF::LineMapPair line_map;
  bool line_map_flag;
  std::string out_file, profile_name; //
  std::string mca_audio_element_kind, mca_audio_content_kind;

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


  CommandOptions(int argc, const char** argv) :
    error_flag(true), key_flag(false), key_id_flag(false), asset_id_flag(false),
    encrypt_header_flag(true), write_hmac(true), verbose_flag(false), fb_dump_size(0),
    no_write_flag(false), version_flag(false), help_flag(false),
    duration(0xffffffff), j2c_pedantic(true), use_cdci_descriptor(false),
    edit_rate(24,1), fb_size(FRAME_BUFFER_SIZE),
    show_ul_values_flag(false), index_strategy(AS_02::IS_FOLLOW), partition_space(60),
    rgba_MaxRef(1023), rgba_MinRef(0),
    horizontal_subsampling(2), vertical_subsampling(2), component_depth(10),
    frame_layout(0), aspect_ratio(ASDCP::Rational(4,3)), aspect_ratio_flag(false), field_dominance(0),
    mxf_header_size(16384), cdci_WhiteRefLevel(940), cdci_BlackRefLevel(64), cdci_ColorRange(897),
    md_min_luminance(0), md_max_luminance(0), line_map(0,0), line_map_flag(false)
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
// JPEG XS essence

// Write one or more plaintext JPEG XS codestreams to a plaintext AS-02 file
// Write one or more plaintext JPEG XS codestreams to a ciphertext AS-02 file
//
Result_t
write_JXS_file(CommandOptions& Options)
{
  AESEncContext*          Context = 0;
  HMACContext*            HMAC = 0;
  AS_02::JXS::MXFWriter   Writer;
  ASDCP::JXS::FrameBuffer        FrameBuffer(Options.fb_size);
  ASDCP::JXS::SequenceParser     Parser;
  ASDCP::MXF::GenericPictureEssenceDescriptor *picture_descriptor = 0;
  ASDCP::MXF::JPEGXSPictureSubDescriptor jxs_sub_descriptor(g_dict);

  // set up essence parser
  Result_t result = Parser.OpenRead(Options.filenames.front().c_str());

  // set up MXF writer
  if ( ASDCP_SUCCESS(result) )
    {
      //      PDesc.EditRate = Options.edit_rate;

      if ( Options.verbose_flag )
	{
	  fprintf(stderr, "JPEG XS pictures\n");
	  fputs("PictureDescriptor:\n", stderr);
          fprintf(stderr, "Frame Buffer size: %u\n", Options.fb_size);
	}

      if ( Options.use_cdci_descriptor )
	{
	  ASDCP::MXF::CDCIEssenceDescriptor* tmp_dscr = new ASDCP::MXF::CDCIEssenceDescriptor(g_dict);
          Parser.FillPictureDescriptor(*tmp_dscr, jxs_sub_descriptor);

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

	      picture_descriptor = static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr);
	    }
	}
      else
	{ // use RGB
	  ASDCP::MXF::RGBAEssenceDescriptor* tmp_dscr = new ASDCP::MXF::RGBAEssenceDescriptor(g_dict);
          Parser.FillPictureDescriptor(*tmp_dscr, jxs_sub_descriptor);

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

	      picture_descriptor = static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(tmp_dscr);
	    }
	}
    }

  if ( ASDCP_SUCCESS(result) && Options.verbose_flag )
    {
      picture_descriptor->Dump();
      jxs_sub_descriptor.Dump();
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
	  result = Writer.OpenWrite(Options.out_file, Info, *picture_descriptor, jxs_sub_descriptor,
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

  if ( ASDCP_SUCCESS(result) && EssenceType != ESS_JPEG_XS )
    {
      fprintf(stderr, "%s: Unknown file type, not AS-02-compatible essence.\n",
              Options.filenames.front().c_str());
      return 5;
    }

  if ( ASDCP_SUCCESS(result) )
    {
      result = write_JXS_file(Options);
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
