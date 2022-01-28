/*
Copyright (c) 2018-2021, Bjoern Stresing, Patrick Bichiou, Wolfgang Ruppel,
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


#include <KM_sha1.h>
#include "AS_02_ACES.h"
#include "AS_02_internal.h"
#include <limits>
#include <iostream>

#ifdef min
#undef min
#undef max
#endif

using namespace Kumu;

const char*
AS_02::ACES::MIME2str(AS_02::ACES::MIMEType_t m)
{
  if(m == AS_02::ACES::MT_PNG) return "image/png";
  else if(m == AS_02::ACES::MT_TIFF) return "image/tiff";
  return "application/octet-stream";
}

using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------
static std::string ACES_PACKAGE_LABEL = "File Package: frame wrapping of ACES codestreams";
static std::string PICT_DEF_LABEL = "Image Track";
//------------------------------------------------------------------------------------------


typedef std::map<Kumu::UUID, Kumu::UUID> ResourceMap_t;

ASDCP::Rational AS_02::ConvertToRational(double in)
{

  //rational approximation to given real number
  //David Eppstein / UC Irvine / 8 Aug 1993
  i32_t m[2][2];
  double x, startx;
  i32_t maxden = std::numeric_limits<i32_t>::max();
  i32_t ai;

  startx = x = in;

  /* initialize matrix */
  m[0][0] = m[1][1] = 1;
  m[0][1] = m[1][0] = 0;

  /* loop finding terms until denom gets too big */
  while(m[1][0] * (ai = (long)x) + m[1][1] <= maxden)
  {
    i32_t t;
    t = m[0][0] * ai + m[0][1];
    m[0][1] = m[0][0];
    m[0][0] = t;
    t = m[1][0] * ai + m[1][1];
    m[1][1] = m[1][0];
    m[1][0] = t;
    if(x == (double)ai) break;     // AF: division by zero
    x = 1 / (x - (double)ai);
    if(x > (double)0x7FFFFFFF) break;  // AF: representation failure
  }

  /* now remaining x is between 0 and 1/ai */
  /* approx as either 0 or 1/m where m is max that will fit in maxden */
  /* first try zero */
  //printf("%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double)m[0][0] / (double)m[1][0]));

  return(ASDCP::Rational(m[0][0], m[1][0]));
  /* now try other possibility */
  //   ai = (maxden - m[1][1]) / m[1][0];
  //   m[0][0] = m[0][0] * ai + m[0][1];
  //   m[1][0] = m[1][0] * ai + m[1][1];
  //   printf("%ld/%ld, error = %e\n", m[0][0], m[1][0], startx - ((double)m[0][0] / (double)m[1][0]));
}

std::ostream& AS_02::ACES::operator<<(std::ostream& strm, const PictureDescriptor& PDesc)
{

  strm << "          EditRate: " << PDesc.EditRate.Numerator << "/" << PDesc.EditRate.Denominator << std::endl;
  strm << "        SampleRate: " << PDesc.SampleRate.Numerator << "/" << PDesc.SampleRate.Denominator << std::endl;
  strm << "    Chromaticities: " << std::endl;
  strm << "               x_red: " << PDesc.Chromaticities.red.x << " y_red: " << PDesc.Chromaticities.red.y << std::endl;
  strm << "             x_green: " << PDesc.Chromaticities.green.x << " y_green: " << PDesc.Chromaticities.green.y << std::endl;
  strm << "              x_blue: " << PDesc.Chromaticities.blue.x << " y_blue: " << PDesc.Chromaticities.blue.y << std::endl;
  strm << "             x_white: " << PDesc.Chromaticities.white.x << " y_white: " << PDesc.Chromaticities.white.y << std::endl;
  strm << "       Compression: " << (unsigned)PDesc.Compression << std::endl;
  strm << "         LineOrder: " << (unsigned)PDesc.LineOrder << std::endl;
  strm << "        DataWindow: " << std::endl;
  strm << "                xMin: " << PDesc.DataWindow.xMin << std::endl;
  strm << "                yMin: " << PDesc.DataWindow.yMin << std::endl;
  strm << "                xMax: " << PDesc.DataWindow.xMax << std::endl;
  strm << "                yMax: " << PDesc.DataWindow.yMax << std::endl;
  strm << "     DisplayWindow: " << std::endl;
  strm << "                xMin: " << PDesc.DisplayWindow.xMin << std::endl;
  strm << "                yMin: " << PDesc.DisplayWindow.yMin << std::endl;
  strm << "                xMax: " << PDesc.DisplayWindow.xMax << std::endl;
  strm << "                yMax: " << PDesc.DisplayWindow.yMax << std::endl;
  strm << "  PixelAspectRatio: " << PDesc.PixelAspectRatio;
  strm << "ScreenWindowCenter: " << "x: " << PDesc.ScreenWindowCenter.x << "y: " << PDesc.ScreenWindowCenter.y << std::endl;
  strm << " ScreenWindowWidth: " << PDesc.ScreenWindowWidth;
  strm << "          Channels: " << std::endl;

  for(ui32_t i = 0; i < PDesc.Channels.size(); i++)
  {
    if(PDesc.Channels[i].name.empty() == false)
    {
      strm << "                Name: " << PDesc.Channels[i].name << std::endl;
      strm << "           pixelType: " << PDesc.Channels[i].pixelType << std::endl;
      strm << "             pLinear: " << PDesc.Channels[i].pLinear << std::endl;
      strm << "           xSampling: " << PDesc.Channels[i].xSampling << std::endl;
      strm << "           ySampling: " << PDesc.Channels[i].ySampling << std::endl;
    }
  }
  strm << "Number of other entries: " << PDesc.Other.size();

  return strm;
}

void AS_02::ACES::PictureDescriptorDump(const PictureDescriptor &PDesc, FILE *stream /*= NULL*/)
{

  if(stream == NULL) stream = stderr;
  fprintf(stream, "          EditRate: %i/%i\n", PDesc.EditRate.Numerator, PDesc.EditRate.Denominator);
  fprintf(stream, "        SampleRate: %i/%i\n", PDesc.SampleRate.Numerator, PDesc.SampleRate.Denominator);
  fprintf(stream, "    Chromaticities: \n");
  fprintf(stream, "               x_red: %f y_red: %f\n", (double)PDesc.Chromaticities.red.x, (double)PDesc.Chromaticities.red.y);
  fprintf(stream, "             x_green: %f y_green: %f\n", (double)PDesc.Chromaticities.green.x, (double)PDesc.Chromaticities.green.y);
  fprintf(stream, "              x_blue: %f y_blue: %f\n", (double)PDesc.Chromaticities.blue.x, (double)PDesc.Chromaticities.blue.y);
  fprintf(stream, "             x_white: %f y_white: %f\n", (double)PDesc.Chromaticities.white.x, (double)PDesc.Chromaticities.white.y);
  fprintf(stream, "       Compression: %u\n", (unsigned)PDesc.Compression);
  fprintf(stream, "         LineOrder: %u\n", (unsigned)PDesc.LineOrder);
  fprintf(stream, "        DataWindow: \n");
  fprintf(stream, "                xMin: %i\n", PDesc.DataWindow.xMin);
  fprintf(stream, "                yMin: %i\n", PDesc.DataWindow.yMin);
  fprintf(stream, "                xMax: %i\n", PDesc.DataWindow.xMax);
  fprintf(stream, "                yMax: %i\n", PDesc.DataWindow.yMax);
  fprintf(stream, "     DisplayWindow: \n");
  fprintf(stream, "                xMin: %i\n", PDesc.DisplayWindow.xMin);
  fprintf(stream, "                yMin: %i\n", PDesc.DisplayWindow.yMin);
  fprintf(stream, "                xMax: %i\n", PDesc.DisplayWindow.xMax);
  fprintf(stream, "                yMax: %i\n", PDesc.DisplayWindow.yMax);
  fprintf(stream, "  PixelAspectRatio: %f \n", (double)PDesc.PixelAspectRatio);
  fprintf(stream, "ScreenWindowCenter: x: %f y: %f\n", (double)PDesc.ScreenWindowCenter.x, (double)PDesc.ScreenWindowCenter.y);
  fprintf(stream, " ScreenWindowWidth: %f\n", (double)PDesc.ScreenWindowWidth);
  fprintf(stream, "          Channels: \n");

  for(ui32_t i = 0; i < PDesc.Channels.size(); i++)
  {
    if(PDesc.Channels[i].name.empty() == false)
    {
      fprintf(stream, "                Name: %s\n", PDesc.Channels[i].name.c_str());
      fprintf(stream, "           pixelType: %i\n", PDesc.Channels[i].pixelType);
      fprintf(stream, "             pLinear: %u\n", PDesc.Channels[i].pLinear);
      fprintf(stream, "           xSampling: %i\n", PDesc.Channels[i].xSampling);
      fprintf(stream, "           ySampling: %i\n", PDesc.Channels[i].ySampling);
    }
  }
  fprintf(stream, "Number of other entries: %lu\n", PDesc.Other.size());
}

AS_02::Result_t AS_02::ACES::ACES_PDesc_to_MD(const PictureDescriptor &PDesc, const ASDCP::Dictionary &dict, ASDCP::MXF::RGBAEssenceDescriptor &EssenceDescriptor)
{

  EssenceDescriptor.ContainerDuration = PDesc.ContainerDuration;
  EssenceDescriptor.SampleRate = PDesc.EditRate;
  EssenceDescriptor.FrameLayout = 0x00;
  EssenceDescriptor.StoredWidth = PDesc.DataWindow.xMax - PDesc.DataWindow.xMin + 1;
  EssenceDescriptor.StoredHeight = PDesc.DataWindow.yMax - PDesc.DataWindow.yMin + 1;
  EssenceDescriptor.DisplayWidth = PDesc.DisplayWindow.xMax - PDesc.DisplayWindow.xMin + 1;
  EssenceDescriptor.DisplayHeight = PDesc.DisplayWindow.yMax - PDesc.DisplayWindow.yMin + 1;
  EssenceDescriptor.DisplayXOffset = PDesc.DisplayWindow.xMin - PDesc.DataWindow.xMin;
  EssenceDescriptor.DisplayYOffset = PDesc.DisplayWindow.yMin - PDesc.DataWindow.yMin;
  ASDCP::Rational aspect_ratio(EssenceDescriptor.DisplayWidth, EssenceDescriptor.DisplayHeight);
  if(aspect_ratio.Denominator != 0)  EssenceDescriptor.AspectRatio = AS_02::ConvertToRational(aspect_ratio.Quotient());
  EssenceDescriptor.AlphaTransparency = 0x00;
  EssenceDescriptor.ColorPrimaries = dict.ul(MDD_ColorPrimaries_ACES);
  EssenceDescriptor.TransferCharacteristic = dict.ul(MDD_TransferCharacteristic_linear);
  if(PDesc.Channels.size() == 3 && PDesc.Channels.at(0).name == "B" && PDesc.Channels.at(1).name == "G" && PDesc.Channels.at(2).name == "R")
  {
    EssenceDescriptor.PictureEssenceCoding = dict.ul(MDD_ACESUncompressedMonoscopicWithoutAlpha); //Picture Essence Coding Label per 2065-5 section 8.2
    EssenceDescriptor.PixelLayout = RGBALayout(ACESPixelLayoutMonoscopicWOAlpha);
  }
  else if(PDesc.Channels.size() == 4 && PDesc.Channels.at(0).name == "A" && PDesc.Channels.at(1).name == "B" && PDesc.Channels.at(2).name == "G" && PDesc.Channels.at(3).name == "R")
  {
    EssenceDescriptor.PictureEssenceCoding = dict.ul(MDD_ACESUncompressedMonoscopicWithAlpha); //Picture Essence Coding Label per 2065-5
    EssenceDescriptor.PixelLayout = RGBALayout(ACESPixelLayoutMonoscopicWAlpha);
  }
  else if(PDesc.Channels.size() == 6 && PDesc.Channels.at(0).name == "B" && PDesc.Channels.at(1).name == "G" && PDesc.Channels.at(2).name == "R" &&
                            PDesc.Channels.at(3).name == "left.B" && PDesc.Channels.at(4).name == "left.G" && PDesc.Channels.at(5).name == "left.R")
  {
    return RESULT_NOTIMPL;
  }
  else if(PDesc.Channels.size() == 8 && PDesc.Channels.at(0).name == "A" && PDesc.Channels.at(1).name == "B" && PDesc.Channels.at(2).name == "G" && PDesc.Channels.at(3).name == "R" &&
                            PDesc.Channels.at(4).name == "left.A" && PDesc.Channels.at(5).name == "left.B" && PDesc.Channels.at(6).name == "left.G" && PDesc.Channels.at(7).name == "left.R")
  {
    return RESULT_NOTIMPL;
  }
  else
  {
    return RESULT_NOTIMPL;
  }
  return RESULT_OK;
}
//static Kumu::UUID CreateTargetFrameAssetId(const std::string& target_frame_file);

AS_02::Result_t
AS_02::ACES::CreateTargetFrameAssetId(Kumu::UUID& rID, const std::string& target_frame_file)
{
  Kumu::FileReader reader;
  Result_t result = Kumu::RESULT_OK;
  result = reader.OpenRead(target_frame_file);
  if (KM_SUCCESS(result))
  {
    byte_t* read_buffer = (byte_t*)malloc(reader.Size());
    if (read_buffer)
    {
      result = reader.Read(read_buffer, reader.Size());
      rID = AS_02::ACES::create_4122_type5_id(read_buffer, reader.Size(), s_ns_id_target_frame_prefix);
      free(read_buffer);
    } else result = Kumu::RESULT_FAIL;
  }
  return result;
}

static Kumu::UUID
AS_02::ACES::create_4122_type5_id(const byte_t* subject_name, Kumu::fsize_t size, const byte_t* ns_id)
{
  SHA1_CTX ctx;
  SHA1_Init(&ctx);
  SHA1_Update(&ctx, ns_id, NS_ID_LENGTH);
  SHA1_Update(&ctx, subject_name, size);

  const ui32_t sha_len = 20;
  byte_t bin_buf[sha_len];
  SHA1_Final(bin_buf, &ctx);

  // Derive the asset ID from the digest. Make it a type-5 UUID
  byte_t buf[Kumu::UUID_Length];
  memcpy(buf, bin_buf, Kumu::UUID_Length);
  buf[6] &= 0x0f; // clear bits 4-7
  buf[6] |= 0x50; // set UUID version 'digest'
  buf[8] &= 0x3f; // clear bits 6&7
  buf[8] |= 0x80; // set bit 7
  return Kumu::UUID(buf);
}

void AS_02::ACES::FrameBuffer::Dump(FILE *stream /*= NULL*/, ui32_t dump_bytes /*= NULL*/) const
{

  if(stream == 0) stream = stderr;
  fprintf(stream, "Frame: %06u, %7u bytes", m_FrameNumber, m_Size);
  fputc('\n', stream);
  if(dump_bytes > 0) Kumu::hexdump(m_Data, dump_bytes, stream);
}

class AS_02::ACES::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  ResourceMap_t m_ResourceMap;
  ASDCP::MXF::RGBAEssenceDescriptor *m_EssenceDescriptor;

public:
  h__Reader(const Dictionary *d, const Kumu::IFileReaderFactory& fileReaderFactory) :
    AS_02::h__AS02Reader(d, fileReaderFactory), m_EssenceDescriptor(NULL) {}

  AS_02::ACES::ResourceList_t m_Anc_Resources;

  virtual ~h__Reader() {}

  Result_t    OpenRead(const std::string&);
  Result_t    FillAncillaryResourceDescriptor(AS_02::ACES::ResourceList_t &ancillary_resources);
  Result_t    ReadFrame(ui32_t, AS_02::ACES::FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    ReadAncillaryResource(const Kumu::UUID&, AS_02::ACES::FrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC);
};



Result_t AS_02::ACES::MXFReader::h__Reader::OpenRead(const std::string& filename)
{

  Result_t result = OpenMXFRead(filename.c_str());

  if(KM_SUCCESS(result))
  {
    InterchangeObject* tmp_iobj = 0;

    result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);

    if(ASDCP_SUCCESS(result))
    {
      if(m_EssenceDescriptor == NULL)
      {
        m_EssenceDescriptor = static_cast<RGBAEssenceDescriptor*>(tmp_iobj);
        FillAncillaryResourceDescriptor(m_Anc_Resources);
      }
    }
    else
    {
      DefaultLogSink().Error("RGBAEssenceDescriptor not found.\n");
    }

    std::list<InterchangeObject*> ObjectList;
    m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

    if(ObjectList.empty())
    {
      DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
      return AS_02::RESULT_AS02_FORMAT;
    }
  }
  return result;
}

Result_t AS_02::ACES::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, AS_02::ACES::FrameBuffer& FrameBuf, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if(!m_File->IsOpen()) return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_ACESFrameWrappedEssence), Ctx, HMAC); //PB:new UL

}

AS_02::Result_t AS_02::ACES::MXFReader::h__Reader::ReadAncillaryResource(const Kumu::UUID &uuid, AS_02::ACES::FrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC)
{


  ResourceMap_t::const_iterator ri = m_ResourceMap.find(uuid);
  if(ri == m_ResourceMap.end())
  {
    char buf[64];
    DefaultLogSink().Error("No such resource: %s\n", uuid.EncodeHex(buf, 64));
    return RESULT_RANGE;
  }
  TargetFrameSubDescriptor* DescObject = 0;
  // get the subdescriptor
  InterchangeObject* tmp_iobj = 0;
  Result_t result = m_HeaderPart.GetMDObjectByID((*ri).second, &tmp_iobj);
  if (KM_SUCCESS(result) && tmp_iobj->IsA(m_Dict->ul(MDD_TargetFrameSubDescriptor)))
  {
    DescObject = static_cast<TargetFrameSubDescriptor*>(tmp_iobj);

    RIP::const_pair_iterator pi;
    RIP::PartitionPair TmpPair;
    ui32_t sequence = 0;

    // Look up the partition start in the RIP using the SID.
    // Count the sequence length in because this is the sequence
    // value needed to  complete the HMAC.
    for(pi = m_RIP.PairArray.begin(); pi != m_RIP.PairArray.end(); ++pi, ++sequence)
    {
      if((*pi).BodySID == DescObject->TargetFrameEssenceStreamID)
      {
        TmpPair = *pi;
        break;
      }
    }

    if(TmpPair.ByteOffset == 0)
    {
      DefaultLogSink().Error("Body SID not found in RIP set: %d\n", DescObject->TargetFrameEssenceStreamID);
      return RESULT_FORMAT;
    }

    // seek to the start of the partition
    if((Kumu::fpos_t)TmpPair.ByteOffset != m_LastPosition)
    {
      m_LastPosition = TmpPair.ByteOffset;
      result = m_File->Seek(TmpPair.ByteOffset);
    }

    // read the partition header
    ASDCP::MXF::Partition GSPart(m_Dict);
    result = GSPart.InitFromFile(*m_File);

    if(ASDCP_SUCCESS(result))
    {
      // check the SID
      if(DescObject->TargetFrameEssenceStreamID != GSPart.BodySID)
      {
        char buf[64];
        DefaultLogSink().Error("Generic stream partition body differs: %s\n", uuid.EncodeHex(buf, 64));
        return RESULT_FORMAT;
      }

        // read the essence packet
      assert(m_Dict);
      if(ASDCP_SUCCESS(result))
        result = ReadEKLVPacket(0, sequence, FrameBuf, m_Dict->ul(MDD_GenericStream_DataElement), Ctx, HMAC);
    }
  }

  return result;
}

AS_02::Result_t AS_02::ACES::MXFReader::h__Reader::FillAncillaryResourceDescriptor(AS_02::ACES::ResourceList_t &ancillary_resources)
{

  assert(m_EssenceDescriptor);
  ASDCP::MXF::RGBAEssenceDescriptor* TDescObj = (ASDCP::MXF::RGBAEssenceDescriptor*)m_EssenceDescriptor;

  Array<Kumu::UUID>::const_iterator sdi = TDescObj->SubDescriptors.begin();
  TargetFrameSubDescriptor* DescObject = NULL;
  Result_t result = RESULT_OK;

  for(; sdi != TDescObj->SubDescriptors.end() && KM_SUCCESS(result); sdi++)
  {
    InterchangeObject* tmp_iobj = NULL;
    result = m_HeaderPart.GetMDObjectByID(*sdi, &tmp_iobj);
    if (!tmp_iobj->IsA(m_Dict->ul(MDD_TargetFrameSubDescriptor)))
        continue;
    DescObject = static_cast<TargetFrameSubDescriptor*>(tmp_iobj);
    if(KM_SUCCESS(result) && DescObject)
    {
      AncillaryResourceDescriptor TmpResource; 
      memcpy(TmpResource.ResourceID, DescObject->TargetFrameAncillaryResourceID.Value(), UUIDlen);
      
      if(DescObject->MediaType.find("image/png") != std::string::npos)
      {
        TmpResource.Type = AS_02::ACES::MT_PNG;
      }
      else if(DescObject->MediaType.find("image/tiff") != std::string::npos)
      {
        TmpResource.Type = AS_02::ACES::MT_TIFF;
      }
      else
      {
        TmpResource.Type = AS_02::ACES::MT_UNDEF;
      }

      ancillary_resources.push_back(TmpResource);
      m_ResourceMap.insert(ResourceMap_t::value_type(DescObject->TargetFrameAncillaryResourceID, *sdi));
    }
    else
    {
      DefaultLogSink().Error("Broken sub-descriptor link\n");
      return RESULT_FORMAT;
    }
  }

  return result;
}

void
AS_02::ACES::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      m_Reader->m_HeaderPart.Dump(stream);
    }
}


//
void
AS_02::ACES::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      m_Reader->m_IndexAccess.Dump(stream);
    }
}



class AS_02::ACES::MXFWriter::h__Writer : public AS_02::h__AS02WriterFrame{

  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  byte_t m_EssenceUL[SMPTE_UL_LENGTH];
  ui32_t m_EssenceStreamID;

  h__Writer(const Dictionary *d) : h__AS02WriterFrame(d), m_EssenceStreamID(10)
  {

    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer() {}

  Result_t OpenWrite(const std::string &filename, ASDCP::MXF::FileDescriptor *essence_descriptor,
                     ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
                     const AS_02::IndexStrategy_t &IndexStrategy,
                     const ui32_t &PartitionSpace_sec, const ui32_t &HeaderSize);
  Result_t SetSourceStream(const std::string &label, const ASDCP::Rational &edit_rate);
  Result_t WriteFrame(const AS_02::ACES::FrameBuffer &FrameBuf, ASDCP::AESEncContext *Ctx, ASDCP::HMACContext *HMAC);
  Result_t WriteAncillaryResource(const AS_02::ACES::FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
};

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
AS_02::Result_t AS_02::ACES::MXFWriter::h__Writer::OpenWrite(const std::string &filename, ASDCP::MXF::FileDescriptor *essence_descriptor,
       ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
    const AS_02::IndexStrategy_t &IndexStrategy, const ui32_t &PartitionSpace_sec, const ui32_t &HeaderSize)
{

  if(!m_State.Test_BEGIN())
  {
    KM_RESULT_STATE_HERE();
    return RESULT_STATE;
  }

  if(m_IndexStrategy != AS_02::IS_FOLLOW)
  {
    DefaultLogSink().Error("Only strategy IS_FOLLOW is supported at this time.\n");
    return Kumu::RESULT_NOTIMPL;
  }

  Result_t result = m_File.OpenWrite(filename.c_str());

  if(KM_SUCCESS(result))
  {
    m_IndexStrategy = IndexStrategy;
    m_PartitionSpace = PartitionSpace_sec; // later converted to edit units by SetSourceStream()
    m_HeaderSize = HeaderSize;

    if(essence_descriptor->GetUL() != UL(m_Dict->ul(MDD_RGBAEssenceDescriptor)))
    {
      DefaultLogSink().Error("Essence descriptor is not a ACES Picture Essence Descriptor.\n");
      essence_descriptor->Dump();
      return AS_02::RESULT_AS02_FORMAT;
    }

    m_EssenceDescriptor = essence_descriptor;

    ASDCP::MXF::InterchangeObject_list_t::iterator i;
    for ( i = essence_sub_descriptor_list.begin(); i != essence_sub_descriptor_list.end(); ++i )
    {
      if ( ( (*i)->GetUL() != UL(m_Dict->ul(MDD_ACESPictureSubDescriptor)) )
               && ( (*i)->GetUL() != UL(m_Dict->ul(MDD_TargetFrameSubDescriptor)) )
               && ( (*i)->GetUL() != UL(m_Dict->ul(MDD_ContainerConstraintsSubDescriptor)) )
			  )
        {
          DefaultLogSink().Error("Essence sub-descriptor is not an ACESPictureSubDescriptor or a TargetFrameSubDescriptor.\n");
          (*i)->Dump();
        }

      m_EssenceSubDescriptorList.push_back(*i);
      if (!(*i)->InstanceUID.HasValue()) GenRandomValue((*i)->InstanceUID);
      m_EssenceDescriptor->SubDescriptors.push_back((*i)->InstanceUID);
      *i = 0; // parent will only free the ones we don't keep
    }
    result = m_State.Goto_INIT();
  }
  return result;
}

// Automatically sets the MXF file's metadata from the first aces codestream stream.
AS_02::Result_t AS_02::ACES::MXFWriter::h__Writer::SetSourceStream(const std::string &label, const ASDCP::Rational &edit_rate)
{

  assert(m_Dict);
  if(!m_State.Test_INIT())
  {
    KM_RESULT_STATE_HERE();
    return RESULT_STATE;
  }

  Result_t result = RESULT_OK;
  ui32_t EssenceStreamID_backup = m_EssenceStreamID;

  if(KM_SUCCESS(result))
  {
    memcpy(m_EssenceUL, m_Dict->ul(MDD_ACESFrameWrappedEssence), SMPTE_UL_LENGTH);
    m_EssenceUL[SMPTE_UL_LENGTH - 1] = 1; // first (and only) essence container
    Result_t result = m_State.Goto_READY();
  }

  if(KM_SUCCESS(result))
  {
    result = WriteAS02Header(label, UL(m_Dict->ul(MDD_MXFGCFrameWrappedACESPictures)), //Essence Container Label per 2065-5 section 8.1 (frame wrapping)
                             PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
                             edit_rate);

    if(KM_SUCCESS(result))
    {
      this->m_IndexWriter.SetPrimerLookup(&this->m_HeaderPart.m_Primer);
    }
  }

  m_EssenceStreamID = EssenceStreamID_backup;
  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
AS_02::Result_t AS_02::ACES::MXFWriter::h__Writer::WriteFrame(const AS_02::ACES::FrameBuffer &FrameBuf, ASDCP::AESEncContext *Ctx, ASDCP::HMACContext *HMAC)
{

  if(FrameBuf.Size() == 0)
  {
    DefaultLogSink().Error("The frame buffer size is zero.\n");
    return RESULT_PARAM;
  }

  Result_t result = RESULT_OK;

  if(m_State.Test_READY())
  {
    result = m_State.Goto_RUNNING(); // first time through
  }

  if(KM_SUCCESS(result))
  {
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, MXF_BER_LENGTH, Ctx, HMAC);
    m_FramesWritten++;
  }

  return result;
}

// Closes the MXF file, writing the index and other closing information.
AS_02::Result_t AS_02::ACES::MXFWriter::h__Writer::Finalize()
{

  if(!m_State.Test_RUNNING())
  {
    KM_RESULT_STATE_HERE();
    return RESULT_STATE;
  }

  Result_t result = m_State.Goto_FINAL();

  if(KM_SUCCESS(result))
  {
    result = WriteAS02Footer();
  }

  return result;
}

AS_02::Result_t AS_02::ACES::MXFWriter::h__Writer::WriteAncillaryResource(const AS_02::ACES::FrameBuffer &FrameBuf, AESEncContext *Ctx , HMACContext *HMAC )
{

  if(!m_State.Test_RUNNING())
  {
    KM_RESULT_STATE_HERE();
    return RESULT_STATE;
  }

  Kumu::fpos_t here = m_File.TellPosition();
  assert(m_Dict);

  // create generic stream partition header
  static UL GenericStream_DataElement(m_Dict->ul(MDD_GenericStream_DataElement));
  ASDCP::MXF::Partition GSPart(m_Dict);

  GSPart.MajorVersion = m_HeaderPart.MajorVersion;
  GSPart.MinorVersion = m_HeaderPart.MinorVersion;
  GSPart.ThisPartition = here;
  GSPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;
  GSPart.BodySID = m_EssenceStreamID;
  GSPart.OperationalPattern = m_HeaderPart.OperationalPattern;

  m_RIP.PairArray.push_back(RIP::PartitionPair(m_EssenceStreamID++, here));
  GSPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  //GSPart.EssenceContainers.push_back(UL(m_Dict->ul(MDD_ACESFrameWrappedEssence))); //MDD_ACESEssence
  UL TmpUL(m_Dict->ul(MDD_GenericStreamPartition));
  Result_t result = GSPart.WriteToFile(m_File, TmpUL);

  if(KM_SUCCESS(result))
  {
    ui64_t this_stream_offset = m_StreamOffset; // m_StreamOffset will be changed by the call to Write_EKLV_Packet

    result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
			       m_StreamOffset, FrameBuf, GenericStream_DataElement.Value(),
			       MXF_BER_LENGTH, Ctx, HMAC);
  }
  return result;
}

AS_02::ACES::MXFWriter::MXFWriter()
{

}

AS_02::ACES::MXFWriter::~MXFWriter()
{

}

ASDCP::MXF::OP1aHeader& AS_02::ACES::MXFWriter::OP1aHeader()
{

  if(m_Writer.empty())
  {
    assert(g_OP1aHeader);
    return *g_OP1aHeader;
  }
  return m_Writer->m_HeaderPart;
}

ASDCP::MXF::RIP& AS_02::ACES::MXFWriter::RIP()
{

  if(m_Writer.empty())
  {
    assert(g_RIP);
    return *g_RIP;
  }
  return m_Writer->m_RIP;
}

AS_02::Result_t AS_02::ACES::MXFWriter::OpenWrite(const std::string &filename, const ASDCP::WriterInfo &Info,
    ASDCP::MXF::FileDescriptor *essence_descriptor,
    ASDCP::MXF::InterchangeObject_list_t& essence_sub_descriptor_list,
    const ASDCP::Rational &edit_rate, const AS_02::ACES::ResourceList_t &ancillary_resources /*= ResourceList_t()*/, const ui32_t &header_size /*= 16384*/, const AS_02::IndexStrategy_t &strategy /*= IS_FOLLOW*/, const ui32_t &partition_space /*= 10*/)
{

  if(essence_descriptor == NULL)
  {
    DefaultLogSink().Error("Essence descriptor object required.\n");
    return RESULT_PARAM;
  }

  m_Writer = new AS_02::ACES::MXFWriter::h__Writer(&DefaultSMPTEDict());
  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, essence_descriptor, essence_sub_descriptor_list, strategy, partition_space, header_size);

  if(KM_SUCCESS(result)) result = m_Writer->SetSourceStream(ACES_PACKAGE_LABEL, edit_rate);
  if(KM_FAILURE(result)) m_Writer.release();
  return result;
}

AS_02::Result_t AS_02::ACES::MXFWriter::WriteFrame(const FrameBuffer &FrameBuf, ASDCP::AESEncContext *Ctx /*= NULL*/, ASDCP::HMACContext *HMAC /*= NULL*/)
{

  if(m_Writer.empty()) return RESULT_INIT;
  return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

AS_02::Result_t AS_02::ACES::MXFWriter::Finalize()
{

  if(m_Writer.empty()) return RESULT_INIT;
  return m_Writer->Finalize();
}


AS_02::Result_t AS_02::ACES::MXFWriter::WriteAncillaryResource(const AS_02::ACES::FrameBuffer &rBuf, ASDCP::AESEncContext *Ctx , ASDCP::HMACContext *HMAC )
{

  if(m_Writer.empty())
    return RESULT_INIT;

  return m_Writer->WriteAncillaryResource(rBuf, Ctx, HMAC);
}


AS_02::ACES::MXFReader::MXFReader(const Kumu::IFileReaderFactory& fileReaderFactory)
{
  m_Reader = new h__Reader(&DefaultCompositeDict(), fileReaderFactory);
}

AS_02::ACES::MXFReader::~MXFReader()
{

}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
ASDCP::MXF::OP1aHeader& AS_02::ACES::MXFReader::OP1aHeader()
{

  if(m_Reader.empty())
  {
    assert(g_OP1aHeader);
    return *g_OP1aHeader;
  }
  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
AS_02::MXF::AS02IndexReader& AS_02::ACES::MXFReader::AS02IndexReader()
{

  if(m_Reader.empty())
  {
    assert(g_AS02IndexReader);
    return *g_AS02IndexReader;
  }
  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
ASDCP::MXF::RIP& AS_02::ACES::MXFReader::RIP()
{

  if(m_Reader.empty())
  {
    assert(g_RIP);
    return *g_RIP;
  }
  return m_Reader->m_RIP;
}

AS_02::Result_t AS_02::ACES::MXFReader::OpenRead(const std::string& filename) const
{

  return m_Reader->OpenRead(filename);
}

AS_02::Result_t AS_02::ACES::MXFReader::Close() const
{

  if(m_Reader && m_Reader->m_File->IsOpen())
  {
    m_Reader->Close();
    return RESULT_OK;
  }
  return RESULT_INIT;
}

AS_02::Result_t AS_02::ACES::MXFReader::FillWriterInfo(ASDCP::WriterInfo& Info) const
{

  if(m_Reader && m_Reader->m_File->IsOpen())
  {
    Info = m_Reader->m_Info;
    return RESULT_OK;
  }
  return RESULT_INIT;
}

AS_02::Result_t AS_02::ACES::MXFReader::ReadFrame(ui32_t FrameNum, AS_02::ACES::FrameBuffer &FrameBuf, ASDCP::AESDecContext *Ctx /*= 0*/, ASDCP::HMACContext *HMAC /*= 0*/) const
{

  if(m_Reader && m_Reader->m_File->IsOpen())
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

AS_02::Result_t AS_02::ACES::MXFReader::ReadAncillaryResource(const Kumu::UUID &uuid, AS_02::ACES::FrameBuffer &FrameBuf, ASDCP::AESDecContext *Ctx , ASDCP::HMACContext *HMAC ) const
{

  if(m_Reader && m_Reader->m_File->IsOpen())
    return m_Reader->ReadAncillaryResource(uuid, FrameBuf, Ctx, HMAC);
  
  return RESULT_INIT;
}

AS_02::Result_t AS_02::ACES::MXFReader::FillAncillaryResourceList(AS_02::ACES::ResourceList_t &ancillary_resources) const
{

  if(m_Reader && m_Reader->m_File->IsOpen())
  {
    ancillary_resources = m_Reader->m_Anc_Resources;
    return RESULT_OK;
  }
  return RESULT_INIT;
}

bool AS_02::ACES::channel::operator==(const channel &Other) const
{

  if(name != Other.name) return false;
  if(pixelType != Other.pixelType) return false;
  if(pLinear != Other.pLinear) return false;
  if(xSampling != Other.xSampling) return false;
  if(ySampling != Other.ySampling) return false;
  return true;
}

bool AS_02::ACES::box2i::operator==(const box2i &Other) const
{

  if(xMin != Other.xMin) return false;
  if(yMin != Other.yMin) return false;
  if(xMax != Other.xMax) return false;
  if(yMax != Other.yMax) return false;
  return true;
}

bool AS_02::ACES::keycode::operator==(const keycode &Other) const
{

  if(filmMfcCode != Other.filmMfcCode) return false;
  if(filmType != Other.filmType) return false;
  if(prefix != Other.prefix) return false;
  if(count != Other.count) return false;
  if(perfOffset != Other.perfOffset) return false;
  if(perfsPerFrame != Other.perfsPerFrame) return false;
  if(perfsPerCount != Other.perfsPerCount) return false;
  return true;
}

bool AS_02::ACES::v2f::operator==(const v2f &Other) const
{

  if(x != Other.x) return false;
  if(y != Other.y) return false;
  return true;
}

bool AS_02::ACES::v3f::operator==(const v3f &Other) const
{

  if(x != Other.x) return false;
  if(y != Other.y) return false;
  if(z != Other.z) return false;
  return true;
}

bool AS_02::ACES::chromaticities::operator==(const chromaticities &Other) const
{

  if(red != Other.red) return false;
  if(green != Other.green) return false;
  if(blue != Other.blue) return false;
  if(white != Other.white) return false;
  return true;
}

bool AS_02::ACES::timecode::operator==(const timecode &Other) const
{

  if(timeAndFlags != Other.timeAndFlags) return false;
  if(userData != Other.userData) return false;
  return true;
}

bool AS_02::ACES::PictureDescriptor::operator==(const PictureDescriptor &Other) const
{

  if(EditRate != Other.EditRate) return false;
  if(SampleRate != Other.SampleRate) return false;
  if(AcesImageContainerFlag != Other.AcesImageContainerFlag) return false;
  if(Chromaticities != Other.Chromaticities) return false;
  if(Compression != Other.Compression) return false;
  if(LineOrder != Other.LineOrder) return false;
  if(DataWindow != Other.DataWindow) return false;
  if(DisplayWindow != Other.DisplayWindow) return false;
  if(PixelAspectRatio != Other.PixelAspectRatio) return false;
  if(ScreenWindowCenter != Other.ScreenWindowCenter) return false;
  if(ScreenWindowWidth != Other.ScreenWindowWidth) return false;
  if(Channels.size() != Other.Channels.size()) return false;
  for(int i = 0; i < Channels.size(); i++)
  {
    if(Channels.at(i) != Other.Channels.at(i)) return false;
  }
  return true;
}
