/*
Copyright (c) 2003-2016, John Hurst, Wolfgang Ruppel
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
/*! \file    as-02-info.cpp
    \version $Id$
    \brief   AS-02 file metadata utility

  This program provides metadata information about an AS-02 file.

  For more information about asdcplib, please refer to the header file AS_DCP.h
*/

#include <KM_fileio.h>
#include <KM_log.h>
#include <AS_DCP.h>
#include <AS_02.h>
#include <JP2K.h>
#include <MXF.h>
#include <Metadata.h>
#include <cfloat>

using namespace Kumu;
using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "as-02-info";  // program name for messages


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
USAGE:%s [-h|-help] [-V]\n\
\n\
       %s [options] <input-file>+\n\
\n\
Options:\n\
  -c          - Show essence coding UL\n\
  -d          - Show essence descriptor info\n\
  -h | -help  - Show help\n\
  -H          - Show MXF header metadata\n\
  -i          - Show identity info\n\
  -n          - Show index\n\
  -r          - Show bit-rate (Mb/s)\n\
  -t <int>    - Set high-bitrate threshold (Mb/s)\n\
  -V          - Show version information\n\
\n\
  NOTES: o There is no option grouping, all options must be distinct arguments.\n\
         o All option arguments must be separated from the option by whitespace.\n\n",
	  PROGRAM_NAME, PROGRAM_NAME);

}

//
class CommandOptions
{
  CommandOptions();

public:
  bool   error_flag;     // true if the given options are in error or not complete
  bool   version_flag;   // true if the version display option was selected
  bool   help_flag;      // true if the help display option was selected
  bool   verbose_flag;   // true if the verbose option was selected
  PathList_t filenames;  // list of filenames to be processed
  bool   showindex_flag; // true if index is to be displayed
  bool   showheader_flag; // true if MXF file header is to be displayed
  bool   showid_flag;          // if true, show file identity info (the WriterInfo struct)
  bool   showdescriptor_flag;  // if true, show the essence descriptor
  bool   showcoding_flag;      // if true, show the coding UL
  bool   showrate_flag;        // if true and is image file, show bit rate
  bool   max_bitrate_flag;     // true if -t option given
  double max_bitrate;          // if true and is image file, max bit rate for rate test

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), version_flag(false), help_flag(false), verbose_flag(false),
    showindex_flag(false), showheader_flag(false),
    showid_flag(false), showdescriptor_flag(false), showcoding_flag(false),
    showrate_flag(false), max_bitrate_flag(false), max_bitrate(0.0)
  {
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
	      case 'c': showcoding_flag = true; break;
	      case 'd': showdescriptor_flag = true; break;
	      case 'H': showheader_flag = true; break;
	      case 'h': help_flag = true; break;
	      case 'i': showid_flag = true; break;
	      case 'n': showindex_flag = true; break;
	      case 'r': showrate_flag = true; break;

	      case 't':
		TEST_EXTRA_ARG(i, 't');
		max_bitrate = Kumu::xabs(strtol(argv[i], 0, 10));
		max_bitrate_flag = true;
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
      return;

    if ( filenames.empty() )
      {
	fputs("At least one filename argument is required.\n", stderr);
	return;
      }

    error_flag = false;
  }
};

//------------------------------------------------------------------------------------------
//

//
// These classes wrap the irregular names in the asdcplib API
// so that I can use a template to simplify the implementation
// of show_file_info()

static int s_exp_lookup[16] = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,2048, 4096, 8192, 16384, 32768 };

using namespace ASDCP::MXF;

template <class ReaderType, class DescriptorType>
DescriptorType *get_descriptor_by_type(ReaderType& reader, const UL& type_ul)
{
  InterchangeObject *obj = 0;
  reader.OP1aHeader().GetMDObjectByType(type_ul.Value(), &obj);
  return dynamic_cast<DescriptorType*>(obj);
}

class MyPictureDescriptor : public JP2K::PictureDescriptor
{
  RGBAEssenceDescriptor *m_RGBADescriptor;
  CDCIEssenceDescriptor *m_CDCIDescriptor;
  JPEG2000PictureSubDescriptor *m_JP2KSubDescriptor;

 public:
  MyPictureDescriptor() :
    m_RGBADescriptor(0),
    m_CDCIDescriptor(0),
    m_JP2KSubDescriptor(0) {}

  void FillDescriptor(AS_02::JP2K::MXFReader& Reader)
  {
    m_CDCIDescriptor = get_descriptor_by_type<AS_02::JP2K::MXFReader, CDCIEssenceDescriptor>
      (Reader, DefaultCompositeDict().ul(MDD_CDCIEssenceDescriptor));

    m_RGBADescriptor = get_descriptor_by_type<AS_02::JP2K::MXFReader, RGBAEssenceDescriptor>
      (Reader, DefaultCompositeDict().ul(MDD_RGBAEssenceDescriptor));

    if ( m_RGBADescriptor != 0 )
      {
    	SampleRate = m_RGBADescriptor->SampleRate;
    	ContainerDuration = m_RGBADescriptor->ContainerDuration;
      }
    else if ( m_CDCIDescriptor != 0 )
      {
    	SampleRate = m_CDCIDescriptor->SampleRate;
    	ContainerDuration = m_CDCIDescriptor->ContainerDuration;
      }
    else
      {
	DefaultLogSink().Error("Picture descriptor not found.\n");
      }

    m_JP2KSubDescriptor = get_descriptor_by_type<AS_02::JP2K::MXFReader, JPEG2000PictureSubDescriptor>
      (Reader, DefaultCompositeDict().ul(MDD_JPEG2000PictureSubDescriptor));

    if ( m_JP2KSubDescriptor == 0 )
      {
	DefaultLogSink().Error("JPEG2000PictureSubDescriptor not found.\n");
      }

    std::list<InterchangeObject*> ObjectList;
    Reader.OP1aHeader().GetMDObjectsByType(DefaultCompositeDict().ul(MDD_Track), ObjectList);
    
    if ( ObjectList.empty() )
      {
	DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
      }

    EditRate = ((Track*)ObjectList.front())->EditRate;
  }

  void MyDump(FILE* stream) {
    if ( stream == 0 )
      {
	stream = stderr;
      }

    if ( m_CDCIDescriptor != 0 )
      {
	m_CDCIDescriptor->Dump(stream);
      }
    else if ( m_RGBADescriptor != 0 )
      {
	m_RGBADescriptor->Dump(stream);
      }
    else
      {
	return;
      }

    if ( m_JP2KSubDescriptor != 0 )
      {
	m_JP2KSubDescriptor->Dump(stream);

	fprintf(stream, "    ImageComponents: (max=%d)\n", JP2K::MaxComponents);

	//
	ui32_t component_sizing = m_JP2KSubDescriptor->PictureComponentSizing.const_get().Length();
	JP2K::ImageComponent_t image_components[JP2K::MaxComponents];

	if ( component_sizing == 17 ) // ( 2 * sizeof(ui32_t) ) + 3 components * 3 byte each
	  {
	    memcpy(&image_components,
		   m_JP2KSubDescriptor->PictureComponentSizing.const_get().RoData() + 8,
		   component_sizing - 8);
	  }
	else
	  {
	    DefaultLogSink().Warn("Unexpected PictureComponentSizing size: %u, should be 17.\n", component_sizing);
	  }

	fprintf(stream, "  bits  h-sep v-sep\n");

	for ( int i = 0; i < m_JP2KSubDescriptor->Csize && i < JP2K::MaxComponents; i++ )
	  {
	    fprintf(stream, "  %4d  %5d %5d\n",
		    image_components[i].Ssize + 1, // See ISO 15444-1, Table A11, for the origin of '+1'
		    image_components[i].XRsize,
		    image_components[i].YRsize
		    );
	  }

	//
	JP2K::CodingStyleDefault_t coding_style_default;

	memcpy(&coding_style_default,
	       m_JP2KSubDescriptor->CodingStyleDefault.const_get().RoData(),
	       m_JP2KSubDescriptor->CodingStyleDefault.const_get().Length());

	fprintf(stream, "               Scod: %hhu\n", coding_style_default.Scod);
	fprintf(stream, "   ProgressionOrder: %hhu\n", coding_style_default.SGcod.ProgressionOrder);
	fprintf(stream, "     NumberOfLayers: %hd\n",
		KM_i16_BE(Kumu::cp2i<ui16_t>(coding_style_default.SGcod.NumberOfLayers)));
    
	fprintf(stream, " MultiCompTransform: %hhu\n", coding_style_default.SGcod.MultiCompTransform);
	fprintf(stream, "DecompositionLevels: %hhu\n", coding_style_default.SPcod.DecompositionLevels);
	fprintf(stream, "     CodeblockWidth: %hhu\n", coding_style_default.SPcod.CodeblockWidth);
	fprintf(stream, "    CodeblockHeight: %hhu\n", coding_style_default.SPcod.CodeblockHeight);
	fprintf(stream, "     CodeblockStyle: %hhu\n", coding_style_default.SPcod.CodeblockStyle);
	fprintf(stream, "     Transformation: %hhu\n", coding_style_default.SPcod.Transformation);
    
	ui32_t precinct_set_size = 0;

	for ( int i = 0; coding_style_default.SPcod.PrecinctSize[i] != 0 && i < JP2K::MaxPrecincts; ++i )
	  {
	    ++precinct_set_size;
	  }

	fprintf(stream, "          Precincts: %u\n", precinct_set_size);
	fprintf(stream, "precinct dimensions:\n");

	for ( int i = 0; i < precinct_set_size && i < JP2K::MaxPrecincts; i++ )
	  fprintf(stream, "    %d: %d x %d\n", i + 1,
		  s_exp_lookup[coding_style_default.SPcod.PrecinctSize[i]&0x0f],
		  s_exp_lookup[(coding_style_default.SPcod.PrecinctSize[i]>>4)&0x0f]
		  );
      }
  }
};

class MyAudioDescriptor : public PCM::AudioDescriptor
{
  WaveAudioDescriptor *m_WaveAudioDescriptor;
  std::list<MCALabelSubDescriptor*> m_ChannelDescriptorList;

 public:
  MyAudioDescriptor() : m_WaveAudioDescriptor(0) {}
  void FillDescriptor(AS_02::PCM::MXFReader& Reader)
  {
    m_WaveAudioDescriptor = get_descriptor_by_type<AS_02::PCM::MXFReader, WaveAudioDescriptor>
      (Reader, DefaultCompositeDict().ul(MDD_WaveAudioDescriptor));

    if ( m_WaveAudioDescriptor != 0 )
      {
	AudioSamplingRate = m_WaveAudioDescriptor->SampleRate;
	ContainerDuration = m_WaveAudioDescriptor->ContainerDuration;
      }
    else
      {
	DefaultLogSink().Error("Audio descriptor not found.\n");
      }

    std::list<InterchangeObject*> object_list;
    Reader.OP1aHeader().GetMDObjectsByType(DefaultCompositeDict().ul(MDD_AudioChannelLabelSubDescriptor), object_list);
    Reader.OP1aHeader().GetMDObjectsByType(DefaultCompositeDict().ul(MDD_SoundfieldGroupLabelSubDescriptor), object_list);
    Reader.OP1aHeader().GetMDObjectsByType(DefaultCompositeDict().ul(MDD_GroupOfSoundfieldGroupsLabelSubDescriptor), object_list);

    std::list<InterchangeObject*>::iterator i = object_list.begin();
    for ( ; i != object_list.end(); ++i )
      {
	MCALabelSubDescriptor *p = dynamic_cast<MCALabelSubDescriptor*>(*i);

	if ( p )
	  {
	    m_ChannelDescriptorList.push_back(p);
	  }
	else
	  {
	    char buf[64];
	    DefaultLogSink().Error("Audio sub-descriptor type error.\n", (**i).InstanceUID.EncodeHex(buf, 64));
	  }
      }

    object_list.clear();
    Reader.OP1aHeader().GetMDObjectsByType(DefaultCompositeDict().ul(MDD_Track), object_list);
    
    if ( object_list.empty() )
      {
	DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
      }

    EditRate = ((Track*)object_list.front())->EditRate;
  }

  void MyDump(FILE* stream) {
    if ( stream == 0 )
      {
	stream = stderr;
      }

    if ( m_WaveAudioDescriptor != 0 )
      {
	m_WaveAudioDescriptor->Dump(stream);
      }

    if ( ! m_ChannelDescriptorList.empty() )
      {
	fprintf(stream, "Audio Channel Subdescriptors:\n");

	std::list<MCALabelSubDescriptor*>::const_iterator i = m_ChannelDescriptorList.begin();
	for ( ; i != m_ChannelDescriptorList.end(); ++i )
	  {
	    (**i).Dump(stream);
	  }
      }
  }
};

class MyTextDescriptor : public TimedText::TimedTextDescriptor
{
 public:
  void FillDescriptor(TimedText::MXFReader& Reader) {
    Reader.FillTimedTextDescriptor(*this);
  }

  void Dump(FILE* stream) {
    TimedText::DescriptorDump(*this, stream);
  }
};

struct RateInfo
{
  UL ul;
  double bitrate;
  std::string label;

  RateInfo(const UL& u, const double& b, const std::string& l) {
    ul = u; bitrate = b; label = l;
  }

};

static const double dci_max_bitrate = 250.0;
static const double p_hfr_max_bitrate = 400.0;
typedef std::map<const UL, const RateInfo> rate_info_map;
static rate_info_map g_rate_info;

//
void
init_rate_info()
{
  UL rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_1);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, 200.0, "ISO/IEC 15444-1 Amendment 3 Level 1")));

  rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_2);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, 200.0, "ISO/IEC 15444-1 Amendment 3 Level 2")));

  rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_3);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, 200.0, "ISO/IEC 15444-1 Amendment 3 Level 3")));

  rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_4);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, 400.0, "ISO/IEC 15444-1 Amendment 3 Level 4")));

  rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_5);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, 800.0, "ISO/IEC 15444-1 Amendment 3 Level 5")));

  rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_6);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, 1600.0, "ISO/IEC 15444-1 Amendment 3 Level 6")));

  rate_ul = DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_BroadcastProfile_7);
  g_rate_info.insert(rate_info_map::value_type(rate_ul, RateInfo(rate_ul, DBL_MAX, "ISO/IEC 15444-1 Amendment 3 Level 7")));
}


//
//
template<class ReaderT, class DescriptorT>
class FileInfoWrapper
{
  ReaderT  m_Reader;
  DescriptorT m_Desc;
  WriterInfo m_WriterInfo;
  double m_MaxBitrate, m_AvgBitrate;
  UL m_PictureEssenceCoding;

  KM_NO_COPY_CONSTRUCT(FileInfoWrapper);

  template <class T>
  Result_t OpenRead(const T& m, const CommandOptions& Options)
  {
  	return m.OpenRead(Options.filenames.front().c_str());
  };
  Result_t OpenRead(const AS_02::PCM::MXFReader& m, const CommandOptions& Options)
  {
  	return m.OpenRead(Options.filenames.front().c_str(), EditRate_24);
  	//Result_t OpenRead(const std::string& filename, const ASDCP::Rational& EditRate);
  };

public:
  FileInfoWrapper() : m_MaxBitrate(0.0), m_AvgBitrate(0.0) {}
  virtual ~FileInfoWrapper() {}

  Result_t
  file_info(CommandOptions& Options, const char* type_string, FILE* stream = 0)
  {
    assert(type_string);
    if ( stream == 0 )
      {
	stream = stdout;
      }

    Result_t result = RESULT_OK;
    result = OpenRead(m_Reader, Options);

    if ( ASDCP_SUCCESS(result) )
      {
	m_Desc.FillDescriptor(m_Reader);
	m_Reader.FillWriterInfo(m_WriterInfo);

	fprintf(stdout, "%s file essence type is %s, (%d edit unit%s).\n",
		( m_WriterInfo.LabelSetType == LS_MXF_SMPTE ? "SMPTE 2067-5" : "Unknown" ),
		type_string,
		(m_Desc.ContainerDuration != 0 ? m_Desc.ContainerDuration : m_Reader.AS02IndexReader().GetDuration()),
		(m_Desc.ContainerDuration == 1 ? "":"s"));

	if ( Options.showheader_flag )
	  {
	    m_Reader.DumpHeaderMetadata(stream);
	  }

	if ( Options.showid_flag )
	  {
	    WriterInfoDump(m_WriterInfo, stream);
	  }

	if ( Options.showdescriptor_flag )
	  {
	    m_Desc.MyDump(stream);
	  }

	if ( Options.showindex_flag )
	  {
	    m_Reader.DumpIndex(stream);
	  }
      }
    else if ( result == RESULT_FORMAT && Options.showheader_flag )
      {
	m_Reader.DumpHeaderMetadata(stream);
      }

    return result;
  }

  //
  void get_PictureEssenceCoding(FILE* stream = 0)
  {
    const Dictionary& Dict = DefaultCompositeDict();
    MXF::RGBAEssenceDescriptor *rgba_descriptor = 0;
    MXF::CDCIEssenceDescriptor *cdci_descriptor = 0;

    Result_t result = m_Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_RGBAEssenceDescriptor),
							      reinterpret_cast<MXF::InterchangeObject**>(&rgba_descriptor));

    if ( KM_SUCCESS(result) && rgba_descriptor)
      m_PictureEssenceCoding = rgba_descriptor->PictureEssenceCoding;
    else{
    	result = m_Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_CDCIEssenceDescriptor),
    								      reinterpret_cast<MXF::InterchangeObject**>(&cdci_descriptor));
        if ( KM_SUCCESS(result) && cdci_descriptor)
          m_PictureEssenceCoding = cdci_descriptor->PictureEssenceCoding;
    }
  }


  //
  void dump_PictureEssenceCoding(FILE* stream = 0)
  {
    char buf[64];

    if ( m_PictureEssenceCoding.HasValue() )
      {
	std::string encoding_ul_type = "**UNKNOWN**";

	rate_info_map::const_iterator rate_i = g_rate_info.find(m_PictureEssenceCoding);
	if ( rate_i == g_rate_info.end() )
	  {
	    fprintf(stderr, "Unknown PictureEssenceCoding UL: %s\n", m_PictureEssenceCoding.EncodeString(buf, 64));
	  }
	else
	  {
	    encoding_ul_type = rate_i->second.label;
	  }

	fprintf(stream, "PictureEssenceCoding: %s (%s)\n",
		m_PictureEssenceCoding.EncodeString(buf, 64),
		encoding_ul_type.c_str());
      }
  }

  //
  Result_t
  test_rates(CommandOptions& Options, FILE* stream = 0)
  {
    double max_bitrate = 0; //Options.max_bitrate_flag ? Options.max_bitrate : dci_max_bitrate;
    ui32_t errors = 0;
    char buf[64];

    rate_info_map::const_iterator rate_i = g_rate_info.find(m_PictureEssenceCoding);
    if ( rate_i == g_rate_info.end() )
      {
	fprintf(stderr, "Unknown PictureEssenceCoding UL: %s\n", m_PictureEssenceCoding.EncodeString(buf, 64));
      }
    else
      {
	max_bitrate = rate_i->second.bitrate;
      }

    max_bitrate = Options.max_bitrate_flag ? Options.max_bitrate : max_bitrate;

    if ( m_MaxBitrate > max_bitrate )
      {
	fprintf(stream, "Bitrate %0.0f Mb/s exceeds maximum %0.0f Mb/s\n", m_MaxBitrate, max_bitrate);
	++errors;
      }

    return errors ? RESULT_FAIL : RESULT_OK;
  }

  //
  void
  calc_Bitrate(FILE* stream = 0)
  {
    //MXF::OP1aHeader& footer = m_Reader.OP1aHeader();
    AS_02::MXF::AS02IndexReader& footer = m_Reader.AS02IndexReader();
    ui64_t total_frame_bytes = 0, last_stream_offset = 0;
    ui32_t largest_frame = 0;
    Result_t result = RESULT_OK;
    ui64_t duration = 0;

    if ( m_Desc.EditRate.Numerator == 0 || m_Desc.EditRate.Denominator == 0 )
      {
	fprintf(stderr, "Broken edit rate, unable to calculate essence bitrate.\n");
	return;
      }

    duration = m_Desc.ContainerDuration;
    if ( duration == 0 )
      {
    	fprintf(stderr, "ContainerDuration not set in file descriptor, attempting to use index duration.\n");
    	duration = m_Reader.AS02IndexReader().GetDuration();
      }

    for ( ui32_t i = 0; KM_SUCCESS(result) && i < duration; ++i )
      {
	MXF::IndexTableSegment::IndexEntry entry;
	result = footer.Lookup(i, entry);

	if ( KM_SUCCESS(result) )
	  {
	    if ( last_stream_offset != 0 )
	      {
		ui64_t this_frame_size = entry.StreamOffset - last_stream_offset - 20; // do not count the bytes that represent the KLV wrapping
		total_frame_bytes += this_frame_size;

		if ( this_frame_size > largest_frame )
		  largest_frame = this_frame_size;
	      }

	    last_stream_offset = entry.StreamOffset;
	  }
      }

    if ( KM_SUCCESS(result) )
      {
	// scale bytes to megabits
	static const double mega_const = 1.0 / ( 1000000 / 8.0 );

	// we did not accumulate the last, so duration -= 1
	double avg_bytes_frame = total_frame_bytes / ( duration - 1 );

	m_MaxBitrate = largest_frame * mega_const * m_Desc.EditRate.Quotient();
	m_AvgBitrate = avg_bytes_frame * mega_const * m_Desc.EditRate.Quotient();
      }
  }

  //
  void
  dump_Bitrate(FILE* stream = 0)
  {
    fprintf(stream, "Max BitRate: %0.2f Mb/s\n", m_MaxBitrate);
    fprintf(stream, "Average BitRate: %0.2f Mb/s\n", m_AvgBitrate);
  }

  //
  void dump_WaveAudioDescriptor(FILE* stream = 0)
  {
    const Dictionary& Dict = DefaultCompositeDict();
    MXF::WaveAudioDescriptor *descriptor = 0;

    Result_t result = m_Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_WaveAudioDescriptor),
							      reinterpret_cast<MXF::InterchangeObject**>(&descriptor));

    if ( KM_SUCCESS(result) )
      {
    	char buf[64];
    	fprintf(stream, "ChannelAssignment: %s\n", descriptor->ChannelAssignment.const_get().EncodeString(buf, 64));
      }
  }

};


// Read header metadata from an ASDCP file
//
Result_t
show_file_info(CommandOptions& Options)
{
  EssenceType_t EssenceType;
  Result_t result = ASDCP::EssenceType(Options.filenames.front().c_str(), EssenceType);

  if ( ASDCP_FAILURE(result) )
    return result;

  if ( EssenceType == ESS_AS02_JPEG_2000 )
    {
	  FileInfoWrapper<AS_02::JP2K::MXFReader, MyPictureDescriptor> wrapper;
	  result = wrapper.file_info(Options, "JPEG 2000 pictures");

	  if ( KM_SUCCESS(result) )
	    {
	      wrapper.get_PictureEssenceCoding();
	      wrapper.calc_Bitrate(stdout);

	      if ( Options.showcoding_flag )
		{
		  wrapper.dump_PictureEssenceCoding(stdout);
		}

	      if ( Options.showrate_flag )
		{
		  wrapper.dump_Bitrate(stdout);
		}

	      result = wrapper.test_rates(Options, stdout);
	    }
    }

  else if ( EssenceType == ESS_AS02_PCM_24b_48k || EssenceType == ESS_AS02_PCM_24b_96k )
    {
      FileInfoWrapper<AS_02::PCM::MXFReader, MyAudioDescriptor> wrapper;
      result = wrapper.file_info(Options, "PCM audio");

      if ( ASDCP_SUCCESS(result) && Options.showcoding_flag )
	wrapper.dump_WaveAudioDescriptor(stdout);
    }
  else
    {
      fprintf(stderr, "Unknown/unsupported essence type: %s\n", Options.filenames.front().c_str());
      Kumu::FileReader   Reader;
      const Dictionary* Dict = &DefaultCompositeDict();
      MXF::OP1aHeader TestHeader(Dict);

      result = Reader.OpenRead(Options.filenames.front().c_str());

      if ( ASDCP_SUCCESS(result) )
	result = TestHeader.InitFromFile(Reader); // test UL and OP

      if ( ASDCP_SUCCESS(result) )
	{
	  TestHeader.Partition::Dump(stdout);

	  if ( MXF::Identification* ID = TestHeader.GetIdentification() )
	    ID->Dump(stdout);
	  else
	    fputs("File contains no Identification object.\n", stdout);

	  if ( MXF::SourcePackage* SP = TestHeader.GetSourcePackage() )
	    SP->Dump(stdout);
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

  init_rate_info();

  while ( ! Options.filenames.empty() && ASDCP_SUCCESS(result) )
    {
      result = show_file_info(Options);
      Options.filenames.pop_front();
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
// end as-02-info.cpp
//
