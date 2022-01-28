/*
Copyright (c) 2018, Bjoern Stresing, Patrick Bichiou, Wolfgang Ruppel,
John Hurst

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
/*
This module implements MXF AS-02 IMF App #1 ACES. It's a set of file access objects that
offer simplified access to files conforming to the draft IMF App #1 ACES. The file
format, labeled IMF Essence Component (AKA "AS-02" for historical
reasons), is described in the following document:

o SMPTE 2067-5:2013 IMF Essence Component

The following use cases are supported by the module:

o Write essence to a plaintext or ciphertext AS-02 file:
ACES codestreams

o Read essence from a plaintext or ciphertext AS-02 file:
ACES codestreams

o Read header metadata from an AS-02 file
*/

#ifndef AS_02_ACES_h__
#define AS_02_ACES_h__

#include "AS_DCP.h"
#include "AS_02.h"
#include "Metadata.h"
#include <vector>


typedef ui16_t      real16_t;
typedef float       real32_t;
typedef double      real64_t;

namespace AS_02
{

using Kumu::Result_t;
using Kumu::RESULT_FALSE;
using Kumu::RESULT_OK;
using Kumu::RESULT_FAIL;
using Kumu::RESULT_PTR;
using Kumu::RESULT_NULL_STR;
using Kumu::RESULT_ALLOC;
using Kumu::RESULT_PARAM;
using Kumu::RESULT_NOTIMPL;
using Kumu::RESULT_SMALLBUF;
using Kumu::RESULT_INIT;
using Kumu::RESULT_NOT_FOUND;
using Kumu::RESULT_NO_PERM;
using Kumu::RESULT_FILEOPEN;
using Kumu::RESULT_BADSEEK;
using Kumu::RESULT_READFAIL;
using Kumu::RESULT_WRITEFAIL;
using Kumu::RESULT_STATE;
using Kumu::RESULT_ENDOFFILE;
using Kumu::RESULT_CONFIG;

ASDCP::Rational ConvertToRational(double in);

namespace ACES
{

static const byte_t ACESPixelLayoutMonoscopicWOAlpha[ASDCP::MXF::RGBAValueLength] = {0x42, 0xfd, 0x47, 0xfd, 0x52, 0xfd, 0x00};
static const byte_t ACESPixelLayoutMonoscopicWAlpha[ASDCP::MXF::RGBAValueLength] = {0x41, 0xfd, 0x42, 0xfd, 0x47, 0xfd, 0x52, 0xfd, 0x00};

enum eAttributes
{

  Invalid = 0,
  AcesImageContainerFlag,
  Channels,
  Chromaticities,
  Compression,
  DataWindow,
  DisplayWindow,
  LineOrder,
  PixelAspectRatio,
  ScreenWindowCenter,
  SreenWindowWidth,
  Other
};

enum eTypes
{

  Unknown_t = 0,
  UnsignedChar_t,
  Short_t,
  UnsignedShort_t,
  Int_t,
  UnsignedInt_t,
  UnsignedLong_t,
  Half_t,
  Float_t,
  Double_t,
  Box2i_t,
  Chlist_t,
  Chromaticities_t,
  Compression_t,
  LineOrder_t,
  Keycode_t,
  Rational_t,
  String_t,
  StringVector_t,
  Timecode_t,
  V2f_t,
  V3f_t
};

struct channel
{
  std::string  name; // Shall be one of R, G, B, Left.R, Left.G, Left.B
  i32_t  pixelType;
  ui32_t  pLinear;
  i32_t  xSampling;
  i32_t  ySampling;
  bool operator==(const channel &Other) const;
  bool operator!=(const channel &Other) const { return !(*this == Other); }
};

struct box2i
{
  i32_t xMin;
  i32_t yMin;
  i32_t xMax;
  i32_t yMax;
  bool operator==(const box2i &Other) const;
  bool operator!=(const box2i &Other) const { return !(*this == Other); }
};

struct keycode
{
  i32_t filmMfcCode; //  0 .. 99
  i32_t filmType; //  0 .. 99
  i32_t prefix; //  0 .. 999999
  i32_t count; //  0 .. 9999
  i32_t perfOffset; //  1 .. 119
  i32_t perfsPerFrame; //  1 .. 15
  i32_t perfsPerCount; //  20 .. 120
  bool operator==(const keycode &Other) const;
  bool operator!=(const keycode &Other) const { return !(*this == Other); }
};

struct v2f
{
  real32_t x;
  real32_t y;
  bool operator==(const v2f &Other) const;
  bool operator!=(const v2f &Other) const { return !(*this == Other); }
};

struct v3f
{
  real32_t x;
  real32_t y;
  real32_t z;
  bool operator==(const v3f &Other) const;
  bool operator!=(const v3f &Other) const { return !(*this == Other); }
};

struct chromaticities
{
  v2f red;
  v2f green;
  v2f blue;
  v2f white;
  bool operator==(const chromaticities &Other) const;
  bool operator!=(const chromaticities &Other) const { return !(*this == Other); }
};

struct timecode
{
  ui32_t timeAndFlags;
  ui32_t userData;
  bool operator==(const timecode &Other) const;
  bool operator!=(const timecode &Other) const { return !(*this == Other); }
};

// Extract optional metadata with ACESDataAccessor functions.
struct generic
{
  generic() : type(Unknown_t), size(0) {}
  std::string attributeName;
  eTypes type;
  ui16_t size;
  byte_t data[1024];
};

typedef std::vector<channel> chlist;
typedef std::vector<std::string> stringVector;
typedef std::vector<generic> other;

enum MIMEType_t { MT_PNG, MT_TIFF, MT_UNDEF };

const char*
MIME2str(AS_02::ACES::MIMEType_t m);

struct AncillaryResourceDescriptor
{
  byte_t      ResourceID[16];
  MIMEType_t  Type;
  std::string  filePath;

  AncillaryResourceDescriptor() : Type(MT_UNDEF) {}
};

typedef std::list<AncillaryResourceDescriptor> ResourceList_t;


struct PictureDescriptor
{
  ASDCP::Rational  EditRate;
  ui32_t      ContainerDuration;
  ASDCP::Rational SampleRate;

  i32_t      AcesImageContainerFlag;
  chromaticities  Chromaticities;
  ui8_t      Compression; // shall be 0
  ui8_t      LineOrder; // 0 increasing Y line order, 1 decreasing Y line order. Should be 0
  box2i      DataWindow;
  box2i      DisplayWindow;
  real32_t    PixelAspectRatio;
  v2f                                  ScreenWindowCenter;
  real32_t    ScreenWindowWidth;
  chlist      Channels; // vector
  other      Other; // vector
  //ResourceList_t   ResourceList;

  // Doesn't compare Other.
  bool operator==(const PictureDescriptor &Other) const;
  // Doesn't compare Other.
  bool operator!=(const PictureDescriptor &Other) const { return !(*this == Other); }
};

// Print debugging information to std::ostream
std::ostream& operator << (std::ostream &strm, const PictureDescriptor &PDesc);
// Print debugging information to stream (stderr default)
void PictureDescriptorDump(const PictureDescriptor &PDesc, FILE *stream = NULL);
// Convert a PictureDescriptor to MXF RGBA Picture Essence Descriptor.
// ACESVersion defaults to 0.0.0 Unknown
// OdtId defaults to 0 == None
// You may want to change ACESVersion and/or OdtId afterwards
Result_t ACES_PDesc_to_MD(const PictureDescriptor &PDesc,
                          const ASDCP::Dictionary &dict,
                          ASDCP::MXF::RGBAEssenceDescriptor &EssenceDescriptor);

const byte_t PNGMagic[8] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a };
const byte_t TIFFMagicLE[4] = { 0x49, 0x49, 0x2a, 0x00 };
const byte_t TIFFMagicBE[4] = { 0x4d, 0x4d, 0x00, 0x2a };

//
int const NS_ID_LENGTH = 16;
//
static byte_t s_ns_id_target_frame_prefix[NS_ID_LENGTH] =
{
  // RFC 4122 type 5
  // 2067-50 8.2.5.4 / RFC4122 Appendix C
  //urn:uuid:bba41561-c505-4c9c-ab5a-71c68c2d70ea
  0xbb, 0xa4, 0x15, 0x61, 0xc5, 0x05, 0x4c, 0x9c,
  0xab, 0x5a, 0x71, 0xc6, 0x8c, 0x2d, 0x70, 0xea
};
static byte_t  s_asset_id_prefix[NS_ID_LENGTH] =
{
  // RFC 4122 type 5
  // 2067-2:2016 7.3.1
    0xaf, 0x86, 0xb7, 0xec, 0x4c, 0xdf, 0x4f, 0x9f,
    0x82, 0x0f, 0x6f, 0xd8, 0xd3, 0x00, 0x30, 0x23
};
// Generate UUID asset ID values from target frame file contents
AS_02::Result_t CreateTargetFrameAssetId(Kumu::UUID& rID, const std::string& target_frame_file);
static Kumu::UUID create_4122_type5_id(const byte_t* subject_name, Kumu::fsize_t size, const byte_t* ns_id);

class FrameBuffer : public ASDCP::FrameBuffer
{
public:
  FrameBuffer() {}
  FrameBuffer(ui32_t size) { Capacity(size); }
  virtual ~FrameBuffer() {}

  // Print debugging information to stream (stderr default)
  void Dump(FILE *stream = NULL, ui32_t dump_bytes = 0) const;
};

// Parses the data in the frame buffer to fill in the picture descriptor. Copies
// the offset of the image data into start_of_data. Returns error if the parser fails.
Result_t ParseMetadataIntoDesc(const FrameBuffer &FB, PictureDescriptor &PDesc, byte_t *start_of_data = NULL);


// An object which opens and reads a ACES codestream file. The file is expected
// to contain exactly one complete frame of picture essence as an unwrapped codestream.
class CodestreamParser
{
  class h__CodestreamParser;
  ASDCP::mem_ptr<h__CodestreamParser> m_Parser;
  ASDCP_NO_COPY_CONSTRUCT(CodestreamParser);

public:
  CodestreamParser();
  virtual ~CodestreamParser();

  // Opens a file for reading, parses enough data to provide a complete
  // set of stream metadata for the MXFWriter below.
  // The frame buffer's PlaintextOffset parameter will be set to the first
  // byte of the data segment. Set this value to zero if you want
  // encrypted headers.
  Result_t OpenReadFrame(const std::string &filename, FrameBuffer &FB) const;

  // Fill a PictureDescriptor struct with the values from the file's codestream.
  // Returns RESULT_INIT if the file is not open.
  Result_t FillPictureDescriptor(PictureDescriptor &PDesc) const;
};


class SequenceParser
{
  class h__SequenceParser;
  ASDCP::mem_ptr<h__SequenceParser> m_Parser;
  ASDCP_NO_COPY_CONSTRUCT(SequenceParser);

public:
  SequenceParser();
  virtual ~SequenceParser();

  // Opens a directory for reading.  The directory is expected to contain one or
  // more files, each containing the codestream for exactly one picture. The
  // files must be named such that the frames are in temporal order when sorted
  // alphabetically by filename. The parser will automatically parse enough data
  // from the first file to provide a complete set of stream metadata for the
  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
  // parser will check the metadata for each codestream and fail if a
  // mismatch is detected.
  Result_t OpenRead(const std::string &directory, bool pedantic = false, const std::list<std::string> &target_frame_file_list = std::list<std::string>()) const;

  // Opens a file sequence for reading.  The sequence is expected to contain one or
  // more filenames, each naming a file containing the codestream for exactly one
  // picture. The parser will automatically parse enough data
  // from the first file to provide a complete set of stream metadata for the
  // MXFWriter below.  If the "pedantic" parameter is given and is true, the
  // parser will check the metadata for each codestream and fail if a
  // mismatch is detected.
  Result_t OpenRead(const std::list<std::string> &file_list, bool pedantic = false, const std::list<std::string> &target_frame_file_list = std::list<std::string>()) const;

  // Fill a PictureDescriptor struct with the values from the first file's codestream.
  // Returns RESULT_INIT if the directory is not open.
  Result_t FillPictureDescriptor(PictureDescriptor &PDesc) const;

  // Fill a ResourceList_t struct with the value from the sequence parser.
  // Returns RESULT_INIT if empty.
  Result_t FillResourceList(ResourceList_t &rResourceList_t) const;

  // Rewind the directory to the beginning.
  Result_t Reset() const;

  // Reads the next sequential frame in the directory and places it in the
  // frame buffer. Fails if the buffer is too small or the directory
  // contains no more files.
  // The frame buffer's PlaintextOffset parameter will be set to the first
  // byte of the data segment. Set this value to zero if you want
  // encrypted headers.
  Result_t ReadFrame(FrameBuffer &FB) const;

  Result_t ReadAncillaryResource(const std::string &filename, FrameBuffer &FB) const;
};


class MXFWriter
{
  class h__Writer;
  ASDCP::mem_ptr<h__Writer> m_Writer;
  ASDCP_NO_COPY_CONSTRUCT(MXFWriter);

public:
  MXFWriter();
  virtual ~MXFWriter();

  // Warning: direct manipulation of MXF structures can interfere
  // with the normal operation of the wrapper.  Caveat emptor!
  virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
  virtual ASDCP::MXF::RIP& RIP();

  // Open the file for writing. The file must not exist. Returns error if
  // the operation cannot be completed or if nonsensical data is discovered
  // in the essence descriptor.
  Result_t OpenWrite(const std::string &filename, const ASDCP::WriterInfo &Info,
                                 ASDCP::MXF::FileDescriptor *essence_descriptor,
                                 ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
                                 const ASDCP::Rational &edit_rate,
                                 const ResourceList_t &ancillary_resources = ResourceList_t(),
                                 const ui32_t &header_size = 16384,
                                 const AS_02::IndexStrategy_t &strategy = AS_02::IS_FOLLOW,
                                 const ui32_t &partition_space = 10);

  // Writes a frame of essence to the MXF file. If the optional AESEncContext
  // argument is present, the essence is encrypted prior to writing.
  // Fails if the file is not open, is finalized, or an operating system
  // error occurs.
  Result_t WriteFrame(const FrameBuffer &FrameBuf, ASDCP::AESEncContext *Ctx = NULL, ASDCP::HMACContext *HMAC = NULL);

  // Writes an Ancillary Resource to the MXF file. If the optional AESEncContext
  // argument is present, the essence is encrypted prior to writing.
  // Fails if the file is not open, is finalized, or an operating system
  // error occurs. RESULT_STATE will be returned if the method is called before
  // WriteFrame()
  Result_t WriteAncillaryResource(const AS_02::ACES::FrameBuffer &rBuf, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

  // Closes the MXF file, writing the index and revised header.
  Result_t Finalize();
};


class MXFReader
{
  class h__Reader;
  ASDCP::mem_ptr<h__Reader> m_Reader;
  ASDCP_NO_COPY_CONSTRUCT(MXFReader);

public:
  MXFReader(const Kumu::IFileReaderFactory& fileReaderFactory);
  virtual ~MXFReader();

  // Warning: direct manipulation of MXF structures can interfere
  // with the normal operation of the wrapper.  Caveat emptor!
  virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
  virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
  virtual ASDCP::MXF::RIP& RIP();

  // Open the file for reading. The file must exist. Returns error if the
  // operation cannot be completed.
  Result_t OpenRead(const std::string &filename) const;

  // Fill a ResourceList_t struct with the ancillary resources that are present in the file.
  // Returns RESULT_INIT if the file is not open.
  Result_t FillAncillaryResourceList(AS_02::ACES::ResourceList_t &ancillary_resources) const;

  // Returns RESULT_INIT if the file is not open.
  Result_t Close() const;

  // Fill a WriterInfo struct with the values from the file's header.
  // Returns RESULT_INIT if the file is not open.
  Result_t FillWriterInfo(ASDCP::WriterInfo &Info) const;

  // Reads a frame of essence from the MXF file. If the optional AESEncContext
  // argument is present, the essence is decrypted after reading. If the MXF
  // file is encrypted and the AESDecContext argument is NULL, the frame buffer
  // will contain the ciphertext frame data. If the HMACContext argument is
  // not NULL, the HMAC will be calculated (if the file supports it).
  // Returns RESULT_INIT if the file is not open, failure if the frame number is
  // out of range, or if optional decrypt or HAMC operations fail.
  Result_t ReadFrame(ui32_t FrameNum, AS_02::ACES::FrameBuffer &FrameBuf, ASDCP::AESDecContext *Ctx = 0, ASDCP::HMACContext *HMAC = 0) const;

  // Reads the ancillary resource having the given UUID from the MXF file. If the
  // optional AESEncContext argument is present, the resource is decrypted after
  // reading. If the MXF file is encrypted and the AESDecContext argument is NULL,
  // the frame buffer will contain the ciphertext frame data. If the HMACContext
  // argument is not NULL, the HMAC will be calculated (if the file supports it).
  // Returns RESULT_INIT if the file is not open, failure if the frame number is
  // out of range, or if optional decrypt or HAMC operations fail.
  Result_t ReadAncillaryResource(const Kumu::UUID&, AS_02::ACES::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

  // Print debugging information to stream
         void     DumpHeaderMetadata(FILE* = 0) const;
         void     DumpIndex(FILE* = 0) const;

};
} // namespace ACES

} // namespace AS_02

#endif // AS_02_ACES_h__
