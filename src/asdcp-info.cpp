/*
Copyright (c) 2003-2014, John Hurst
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
/*! \file    asdcp-info.cpp
    \version $Id$
    \brief   AS-DCP file metadata utility

  This program provides metadata information about an AS-DCP file.

  For more information about asdcplib, please refer to the header file AS_DCP.h
*/

#include <KM_fileio.h>
#include <AS_DCP.h>
#include <AS_02.h>
#include <MXF.h>
#include <Metadata.h>

using namespace Kumu;
using namespace ASDCP;

const ui32_t FRAME_BUFFER_SIZE = 4 * Kumu::Megabyte;

const byte_t P_HFR_UL_2K[16] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d,
  0x0e, 0x16, 0x02, 0x02, 0x03, 0x01, 0x01, 0x03
};

const byte_t P_HFR_UL_4K[16] = {
  0x06, 0x0e, 0x2b, 0x34, 0x04, 0x01, 0x01, 0x0d,
  0x0e, 0x16, 0x02, 0x02, 0x03, 0x01, 0x01, 0x04
};

//------------------------------------------------------------------------------------------
//
// command line option parser class

static const char* PROGRAM_NAME = "asdcp-info";  // program name for messages


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
  -3          - Force stereoscopic interpretation of a JP2K file\n\
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
  bool   stereo_image_flag; // if true, expect stereoscopic JP2K input (left eye first)
  bool   showid_flag;          // if true, show file identity info (the WriterInfo struct)
  bool   showdescriptor_flag;  // if true, show the essence descriptor
  bool   showcoding_flag;      // if true, show the coding UL
  bool   showrate_flag;        // if true and is image file, show bit rate
  bool   max_bitrate_flag;     // true if -t option given
  double max_bitrate;          // if true and is image file, max bit rate for rate test

  //
  CommandOptions(int argc, const char** argv) :
    error_flag(true), version_flag(false), help_flag(false), verbose_flag(false),
    showindex_flag(), showheader_flag(), stereo_image_flag(false),
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
	      case '3': stereo_image_flag = true; break;
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

class MyStereoPictureDescriptor : public JP2K::PictureDescriptor
{
 public:
  void FillDescriptor(JP2K::MXFSReader& Reader) {
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

class MyDCDataDescriptor : public DCData::DCDataDescriptor
{
 public:
  void FillDescriptor(DCData::MXFReader& Reader) {
    Reader.FillDCDataDescriptor(*this);
  }

  void Dump(FILE* stream) {
      DCData::DCDataDescriptorDump(*this, stream);
  }
};

class MyAtmosDescriptor : public ATMOS::AtmosDescriptor
{
 public:
  void FillDescriptor(ATMOS::MXFReader& Reader) {
    Reader.FillAtmosDescriptor(*this);
  }

  void Dump(FILE* stream) {
      ATMOS::AtmosDescriptorDump(*this, stream);
  }
};

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

public:
  FileInfoWrapper() : m_MaxBitrate(0.0), m_AvgBitrate(0.0) {}
  virtual ~FileInfoWrapper() {}

  Result_t
  file_info(CommandOptions& Options, const char* type_string, FILE* stream = 0)
  {
    assert(type_string);
    if ( stream == 0 )
      stream = stdout;

    Result_t result = RESULT_OK;
    result = m_Reader.OpenRead(Options.filenames.front().c_str());

    if ( ASDCP_SUCCESS(result) )
      {
	m_Desc.FillDescriptor(m_Reader);
	m_Reader.FillWriterInfo(m_WriterInfo);

	fprintf(stdout, "%s file essence type is %s, (%d edit unit%s).\n",
		( m_WriterInfo.LabelSetType == LS_MXF_SMPTE ? "SMPTE 429" : LS_MXF_INTEROP ? "Interop" : "Unknown" ),
		type_string, m_Desc.ContainerDuration, (m_Desc.ContainerDuration==1?"":"s"));

	if ( Options.showheader_flag )
	  m_Reader.DumpHeaderMetadata(stream);

	if ( Options.showid_flag )
	  WriterInfoDump(m_WriterInfo, stream);

	if ( Options.showdescriptor_flag )
	  m_Desc.Dump(stream);

	if ( Options.showindex_flag )
	  m_Reader.DumpIndex(stream);
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
    MXF::RGBAEssenceDescriptor *descriptor = 0;

    Result_t result = m_Reader.OP1aHeader().GetMDObjectByType(DefaultCompositeDict().ul(MDD_RGBAEssenceDescriptor),
							      reinterpret_cast<MXF::InterchangeObject**>(&descriptor));

    if ( KM_SUCCESS(result) )
      m_PictureEssenceCoding = descriptor->PictureEssenceCoding;
  }


  //
  void dump_PictureEssenceCoding(FILE* stream = 0)
  {
    char buf[64];

    if ( m_PictureEssenceCoding.HasValue() )
      {
	const char *encoding_ul_type = "**UNKNOWN**";

	if ( m_PictureEssenceCoding == UL(P_HFR_UL_2K) )
	  encoding_ul_type = "P-HFR-2K";
	else if ( m_PictureEssenceCoding == UL(P_HFR_UL_4K) )
	  encoding_ul_type = "**P-HFR-4K**";
	else if ( m_PictureEssenceCoding == DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_2K) )
	  encoding_ul_type = "ST-429-4-2K";
	else if ( m_PictureEssenceCoding == DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_4K) )
	  encoding_ul_type = "ST-429-4-4K";

	fprintf(stream, "PictureEssenceCoding: %s (%s)\n", m_PictureEssenceCoding.EncodeString(buf, 64), encoding_ul_type);
      }
  }

  //
  Result_t
  test_rates(CommandOptions& Options, FILE* stream = 0)
  {
    static const double dci_max_bitrate = 250.0;
    static const double p_hfr_max_bitrate = 400.0;

    double max_bitrate = Options.max_bitrate_flag ? Options.max_bitrate : dci_max_bitrate;
    ui32_t errors = 0;

    if ( m_PictureEssenceCoding == UL(P_HFR_UL_2K) )
      {
	if ( m_Desc.StoredWidth > 2048 ) // 4k
	  {
	    fprintf(stream, "4k images marked as 2k HFR.\n");
	    ++errors;
	  }

	if ( m_Desc.SampleRate < ASDCP::EditRate_96 )
	  {
	    fprintf(stream, "HFR UL used for fps < 96.\n");
	    ++errors;
	  }

	if ( ! Options.max_bitrate_flag )
	  max_bitrate = p_hfr_max_bitrate;
      }
    else if ( m_PictureEssenceCoding == UL(P_HFR_UL_4K) )
      {
	fprintf(stream, "4k HFR support undefined.\n");
	++errors;

	if ( m_Desc.StoredWidth <= 2048 ) // 2k
	  {
	    fprintf(stream, "2k images marked as 4k HFR.\n");
	    ++errors;
	  }
      }
    else if ( m_PictureEssenceCoding != DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_2K)
	      && m_PictureEssenceCoding != DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_4K) )
      {
	fprintf(stream, "Unknown PictureEssenceCoding UL value.\n");
	++errors;
      }
    else
      {
	if ( m_PictureEssenceCoding == DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_2K) )
	  {
	    if ( m_Desc.StoredWidth > 2048 ) // 4k
	      {
		fprintf(stream, "4k images marked as 2k ST 429-4.\n");
		++errors;
	      }
	  }
	else if ( m_PictureEssenceCoding == DefaultCompositeDict().ul(MDD_JP2KEssenceCompression_4K) )
	  {
	    if ( m_Desc.StoredWidth <= 2048 ) // 2k
	      {
		fprintf(stream, "2k images marked as 4k ST 429-4.\n");
		++errors;
	      }
	  }
      }

    if ( m_MaxBitrate > max_bitrate )
      {
	fprintf(stream, "Bitrate %0.0f exceeds maximum %0.0f (see option -r).\n", m_MaxBitrate, max_bitrate);
	++errors;
      }

    return errors ? RESULT_FAIL : RESULT_OK;
  }

  //
  void
  calc_Bitrate(FILE* stream = 0)
  {
    MXF::OPAtomIndexFooter& footer = m_Reader.OPAtomIndexFooter();
    ui64_t total_frame_bytes = 0, last_stream_offset = 0;
    ui32_t largest_frame = 0;
    Result_t result = RESULT_OK;

    for ( ui32_t i = 0; KM_SUCCESS(result) && i < m_Desc.ContainerDuration; ++i )
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

	// we did not accumulate the first or last frame, so duration -= 2
	double avg_bytes_frame = total_frame_bytes / ( m_Desc.ContainerDuration - 2 );

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

  if ( EssenceType == ESS_MPEG2_VES )
    {
      FileInfoWrapper<ASDCP::MPEG2::MXFReader, MyVideoDescriptor> wrapper;
      result = wrapper.file_info(Options, "MPEG2 video");

      if ( ASDCP_SUCCESS(result) && Options.showrate_flag )
	wrapper.dump_Bitrate(stdout);
    }
  else if ( EssenceType == ESS_PCM_24b_48k || EssenceType == ESS_PCM_24b_96k )
    {
      FileInfoWrapper<ASDCP::PCM::MXFReader, MyAudioDescriptor> wrapper;
      result = wrapper.file_info(Options, "PCM audio");

      if ( ASDCP_SUCCESS(result) && Options.showcoding_flag )
	wrapper.dump_WaveAudioDescriptor();
    }
  else if ( EssenceType == ESS_JPEG_2000 )
    {
      if ( Options.stereo_image_flag )
	{
	  FileInfoWrapper<ASDCP::JP2K::MXFSReader, MyStereoPictureDescriptor> wrapper;
	  result = wrapper.file_info(Options, "JPEG 2000 stereoscopic pictures");

	  if ( KM_SUCCESS(result) )
	    {
	      wrapper.get_PictureEssenceCoding();
	      wrapper.calc_Bitrate();

	      if ( Options.showcoding_flag )
		wrapper.dump_PictureEssenceCoding(stdout);

	      if ( Options.showrate_flag )
		wrapper.dump_Bitrate(stdout);

	      result = wrapper.test_rates(Options, stdout);
	    }
	}
      else
	{
	  FileInfoWrapper<ASDCP::JP2K::MXFReader, MyPictureDescriptor>wrapper;
	  result = wrapper.file_info(Options, "JPEG 2000 pictures");

	  if ( KM_SUCCESS(result) )
	    {
	      wrapper.get_PictureEssenceCoding();
	      wrapper.calc_Bitrate();

	      if ( Options.showcoding_flag )
		wrapper.dump_PictureEssenceCoding(stdout);

	      if ( Options.showrate_flag )
		wrapper.dump_Bitrate(stdout);

	      result = wrapper.test_rates(Options, stdout);
	    }
	}
    }
  else if ( EssenceType == ESS_JPEG_2000_S )
    {
      FileInfoWrapper<ASDCP::JP2K::MXFSReader, MyStereoPictureDescriptor>wrapper;
      result = wrapper.file_info(Options, "JPEG 2000 stereoscopic pictures");

      if ( KM_SUCCESS(result) )
	{
	  wrapper.get_PictureEssenceCoding();
	  wrapper.calc_Bitrate();

	  if ( Options.showcoding_flag )
	    wrapper.dump_PictureEssenceCoding(stdout);

	  if ( Options.showrate_flag )
	    wrapper.dump_Bitrate(stdout);

	  result = wrapper.test_rates(Options, stdout);
	}
    }
  else if ( EssenceType == ESS_TIMED_TEXT )
    {
      FileInfoWrapper<ASDCP::TimedText::MXFReader, MyTextDescriptor>wrapper;
      result = wrapper.file_info(Options, "Timed Text");
    }
  else if ( EssenceType == ESS_DCDATA_UNKNOWN )
    {
      FileInfoWrapper<ASDCP::DCData::MXFReader, MyDCDataDescriptor> wrapper;
      result = wrapper.file_info(Options, "D-Cinema Generic Data");
    }
  else if ( EssenceType == ESS_DCDATA_DOLBY_ATMOS )
    {
      FileInfoWrapper<ASDCP::ATMOS::MXFReader, MyAtmosDescriptor> wrapper;
      result = wrapper.file_info(Options, "Dolby ATMOS");
    }
  else if ( EssenceType == ESS_AS02_PCM_24b_48k
	    || EssenceType == ESS_AS02_PCM_24b_96k
	    || EssenceType == ESS_AS02_JPEG_2000
	    || EssenceType == ESS_AS02_TIMED_TEXT )
    {
      fprintf(stderr, "File is AS-02. Inspection in not supported by this command.\n");
    }
  else
    {
      fprintf(stderr, "File is not AS-DCP: %s\n", Options.filenames.front().c_str());
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

  while ( ! Options.filenames.empty() && ASDCP_SUCCESS(result) )
    {
      result = show_file_info(Options);
      Options.filenames.pop_front();
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
// end asdcp-info.cpp
//
