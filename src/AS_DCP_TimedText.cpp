/*
Copyright (c) 2008-2021, John Hurst
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
/*! \file    AS_DCP_TimedText.cpp
    \version $Id$       
    \brief   AS-DCP library, PCM essence reader and writer implementation
*/


#include "AS_DCP_internal.h"
#include "KM_xml.h"
#include <iostream>
#include <iomanip>

using Kumu::GenRandomValue;

static std::string TIMED_TEXT_PACKAGE_LABEL = "File Package: SMPTE 429-5 clip wrapping of D-Cinema Timed Text data";
static std::string TIMED_TEXT_DEF_LABEL = "Timed Text Track";


//------------------------------------------------------------------------------------------

//
static const char*
MIME2str(TimedText::MIMEType_t m)
{
  if ( m == TimedText::MT_PNG )
    return "image/png";

  else if( m == TimedText::MT_OPENTYPE )
    return "application/x-font-opentype";
  
  return "application/octet-stream";
}

//
std::ostream&
ASDCP::TimedText::operator << (std::ostream& strm, const TimedTextDescriptor& TDesc)
{
  UUID TmpID(TDesc.AssetID);
  char buf[64];

  strm << "              EditRate: " << (unsigned) TDesc.EditRate.Numerator << "/" << (unsigned) TDesc.EditRate.Denominator << std::endl;
  strm << "     ContainerDuration: " << (unsigned) TDesc.ContainerDuration << std::endl;
  strm << "               AssetID: " << TmpID.EncodeHex(buf, 64) << std::endl;
  strm << "         NamespaceName: " << TDesc.NamespaceName << std::endl;
  strm << "         ResourceCount: " << (unsigned long) TDesc.ResourceList.size() << std::endl;
  strm << "RFC5646LanguageTagList: " << TDesc.RFC5646LanguageTagList << std::endl;

  TimedText::ResourceList_t::const_iterator ri;
  for ( ri = TDesc.ResourceList.begin() ; ri != TDesc.ResourceList.end(); ri++ )
    {
      TmpID.Set((*ri).ResourceID);
      strm << "    " << TmpID.EncodeHex(buf, 64) << ": " << MIME2str((*ri).Type) << std::endl;
    }

  return strm;
}

//
void
ASDCP::TimedText::DescriptorDump(ASDCP::TimedText::TimedTextDescriptor const& TDesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  UUID TmpID(TDesc.AssetID);
  char buf[64];

  fprintf(stream, "              EditRate: %u/%u\n", TDesc.EditRate.Numerator, TDesc.EditRate.Denominator);
  fprintf(stream, "     ContainerDuration: %u\n",    TDesc.ContainerDuration);
  fprintf(stream, "               AssetID: %s\n",    TmpID.EncodeHex(buf, 64));
  fprintf(stream, "         NamespaceName: %s\n",    TDesc.NamespaceName.c_str());
  fprintf(stream, "         ResourceCount: %zu\n",   TDesc.ResourceList.size());
  fprintf(stream, "RFC5646LanguageTagList: %s\n",    TDesc.RFC5646LanguageTagList.c_str());

  TimedText::ResourceList_t::const_iterator ri;
  for ( ri = TDesc.ResourceList.begin() ; ri != TDesc.ResourceList.end(); ri++ )
    {
      TmpID.Set((*ri).ResourceID);
      fprintf(stream, "    %s: %s\n",
	      TmpID.EncodeHex(buf, 64), 
	      MIME2str((*ri).Type));
    }
}

//
void
ASDCP::TimedText::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  UUID TmpID(m_AssetID);
  char buf[64];
  fprintf(stream, "%s | %s | %u\n", TmpID.EncodeHex(buf, 64), m_MIMEType.c_str(), Size());

  if ( dump_len > 0 )
    Kumu::hexdump(m_Data, dump_len, stream);
}

//------------------------------------------------------------------------------------------

typedef std::map<UUID, UUID> ResourceMap_t;

class ASDCP::TimedText::MXFReader::h__Reader : public ASDCP::h__ASDCPReader
{
  MXF::TimedTextDescriptor* m_EssenceDescriptor;
  ResourceMap_t             m_ResourceMap;

  ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
  TimedTextDescriptor m_TDesc;

  h__Reader(const Dictionary *d, const Kumu::IFileReaderFactory& fileReaderFactory) : ASDCP::h__ASDCPReader(d, fileReaderFactory), m_EssenceDescriptor(0) {
    memset(&m_TDesc.AssetID, 0, UUIDlen);
  }

  virtual ~h__Reader() {}

  Result_t    OpenRead(const std::string&);
  Result_t    MD_to_TimedText_TDesc(TimedText::TimedTextDescriptor& TDesc);
  Result_t    ReadTimedTextResource(FrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC);
  Result_t    ReadAncillaryResource(const byte_t*, FrameBuffer& FrameBuf, AESDecContext* Ctx, HMACContext* HMAC);
};

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::h__Reader::MD_to_TimedText_TDesc(TimedText::TimedTextDescriptor& TDesc)
{
  assert(m_EssenceDescriptor);
  memset(&m_TDesc.AssetID, 0, UUIDlen);
  MXF::TimedTextDescriptor* TDescObj = (MXF::TimedTextDescriptor*)m_EssenceDescriptor;

  TDesc.EditRate = TDescObj->SampleRate;
  if ( ! TDescObj->ContainerDuration.empty() )
    {
      assert(TDescObj->ContainerDuration <= 0xFFFFFFFFL);
      TDesc.ContainerDuration = (ui32_t) TDescObj->ContainerDuration;
    }
  memcpy(TDesc.AssetID, TDescObj->ResourceID.Value(), UUIDlen);
  TDesc.NamespaceName = TDescObj->NamespaceURI;
  TDesc.EncodingName = TDescObj->UCSEncoding;
  TDesc.RFC5646LanguageTagList = TDescObj->RFC5646LanguageTagList;
  TDesc.ResourceList.clear();

  Array<UUID>::const_iterator sdi = TDescObj->SubDescriptors.begin();
  TimedTextResourceSubDescriptor* DescObject = 0;
  Result_t result = RESULT_OK;

  for ( ; sdi != TDescObj->SubDescriptors.end() && KM_SUCCESS(result); sdi++ )
    {
      InterchangeObject* tmp_iobj = 0;
      result = m_HeaderPart.GetMDObjectByID(*sdi, &tmp_iobj);
      DescObject = static_cast<TimedTextResourceSubDescriptor*>(tmp_iobj);

      if ( KM_SUCCESS(result) )
	{
	  TimedTextResourceDescriptor TmpResource;
	  memcpy(TmpResource.ResourceID, DescObject->AncillaryResourceID.Value(), UUIDlen);

	  if ( DescObject->MIMEMediaType.find("application/x-font-opentype") != std::string::npos
	       || DescObject->MIMEMediaType.find("application/x-opentype") != std::string::npos
	       || DescObject->MIMEMediaType.find("font/opentype") != std::string::npos )
	    TmpResource.Type = MT_OPENTYPE;

	  else if ( DescObject->MIMEMediaType.find("image/png") != std::string::npos )
	    TmpResource.Type = MT_PNG;

	  else
	    TmpResource.Type = MT_BIN;

	  TDesc.ResourceList.push_back(TmpResource);
	  m_ResourceMap.insert(ResourceMap_t::value_type(DescObject->AncillaryResourceID, *sdi));
	}
      else
	{
	  DefaultLogSink().Error("Broken sub-descriptor link\n");
	  return RESULT_FORMAT;
	}
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
  Result_t result = OpenMXFRead(filename);
  
  if( ASDCP_SUCCESS(result) )
    {
      if ( m_EssenceDescriptor == 0 )
	{
	  InterchangeObject* tmp_iobj = 0;
	  result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(TimedTextDescriptor), &tmp_iobj);
	  m_EssenceDescriptor = static_cast<MXF::TimedTextDescriptor*>(tmp_iobj);
	}

      if( ASDCP_SUCCESS(result) )
	result = MD_to_TimedText_TDesc(m_TDesc);
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::h__Reader::ReadTimedTextResource(FrameBuffer& FrameBuf,
							      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File->IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  Result_t result = ReadEKLVFrame(0, FrameBuf, m_Dict->ul(MDD_TimedTextEssence), Ctx, HMAC);

 if( ASDCP_SUCCESS(result) )
   {
     FrameBuf.AssetID(m_TDesc.AssetID);
     FrameBuf.MIMEType("text/xml");
   }

 return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::h__Reader::ReadAncillaryResource(const byte_t* uuid, FrameBuffer& frame_buf,
							      AESDecContext* Ctx, HMACContext* HMAC)
{
  KM_TEST_NULL_L(uuid);
  UUID RID(uuid);

  ResourceMap_t::const_iterator ri = m_ResourceMap.find(RID);
  if ( ri == m_ResourceMap.end() )
    {
      char buf[64];
      DefaultLogSink().Error("No such resource: %s\n", RID.EncodeHex(buf, 64));
      return RESULT_RANGE;
    }

  // get the subdescriptor
  InterchangeObject* tmp_iobj = 0;
  Result_t result = m_HeaderPart.GetMDObjectByID((*ri).second, &tmp_iobj);
  TimedTextResourceSubDescriptor* desc_object = dynamic_cast<TimedTextResourceSubDescriptor*>(tmp_iobj);

  if ( KM_SUCCESS(result) )
    {
      assert(desc_object);
      result = ReadGenericStreamPartitionPayload(desc_object->EssenceStreamID, frame_buf, Ctx, HMAC);
    }

  if ( KM_SUCCESS(result) )
    {
      frame_buf.AssetID(uuid);
      frame_buf.MIMEType(desc_object->MIMEMediaType);
    }

  return result;
}


//------------------------------------------------------------------------------------------

ASDCP::TimedText::MXFReader::MXFReader(const Kumu::IFileReaderFactory& fileReaderFactory)
{
  m_Reader = new h__Reader(&DefaultSMPTEDict(), fileReaderFactory);
}


ASDCP::TimedText::MXFReader::~MXFReader()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::TimedText::MXFReader::OP1aHeader()
{
  if ( m_Reader.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::TimedText::MXFReader::OPAtomIndexFooter()
{
  if ( m_Reader.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::TimedText::MXFReader::RIP()
{
  if ( m_Reader.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Reader->m_RIP;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
ASDCP::Result_t
ASDCP::TimedText::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename);
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::TimedText::MXFReader::FillTimedTextDescriptor(TimedText::TimedTextDescriptor& TDesc) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      TDesc = m_Reader->m_TDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::TimedText::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::ReadTimedTextResource(std::string& s, AESDecContext* Ctx, HMACContext* HMAC) const
{
  FrameBuffer FrameBuf(2*Kumu::Megabyte);

  Result_t result = ReadTimedTextResource(FrameBuf, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) )
    s.assign((char*)FrameBuf.Data(), FrameBuf.Size());

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::ReadTimedTextResource(FrameBuffer& FrameBuf,
						   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    return m_Reader->ReadTimedTextResource(FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::ReadAncillaryResource(const byte_t* uuid, FrameBuffer& FrameBuf,
						   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    return m_Reader->ReadAncillaryResource(uuid, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}


//
void
ASDCP::TimedText::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File->IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::TimedText::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File->IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File->IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------


//
class ASDCP::TimedText::MXFWriter::h__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  TimedTextDescriptor m_TDesc;
  byte_t              m_EssenceUL[SMPTE_UL_LENGTH];
  ui32_t              m_EssenceStreamID;

  h__Writer(const Dictionary *d) : ASDCP::h__ASDCPWriter(d), m_EssenceStreamID(10) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~h__Writer() {}

  Result_t OpenWrite(const std::string&, ui32_t HeaderSize);
  Result_t SetSourceStream(const TimedTextDescriptor&);
  Result_t WriteTimedTextResource(const std::string& XMLDoc, AESEncContext* = 0, HMACContext* = 0);
  Result_t WriteAncillaryResource(const FrameBuffer&, AESEncContext* = 0, HMACContext* = 0);
  Result_t Finalize();
  Result_t TimedText_TDesc_to_MD(TimedText::TimedTextDescriptor& TDesc);
};

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::h__Writer::TimedText_TDesc_to_MD(TimedText::TimedTextDescriptor& TDesc)
{
  assert(m_EssenceDescriptor);
  MXF::TimedTextDescriptor* TDescObj = (MXF::TimedTextDescriptor*)m_EssenceDescriptor;

  TDescObj->SampleRate = TDesc.EditRate;
  TDescObj->ContainerDuration = TDesc.ContainerDuration;
  TDescObj->ResourceID.Set(TDesc.AssetID);
  TDescObj->NamespaceURI = TDesc.NamespaceName;
  TDescObj->UCSEncoding = TDesc.EncodingName;
  TDescObj->RFC5646LanguageTagList = TDesc.RFC5646LanguageTagList;

  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::h__Writer::OpenWrite(const std::string& filename, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      m_EssenceDescriptor = new MXF::TimedTextDescriptor(m_Dict);
      result = m_State.Goto_INIT();
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::h__Writer::SetSourceStream(ASDCP::TimedText::TimedTextDescriptor const& TDesc)
{
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  m_TDesc = TDesc;
  ResourceList_t::const_iterator ri;
  Result_t result = TimedText_TDesc_to_MD(m_TDesc);

  /* st0429-5-2017 limitation:  the maximum number of sub-descriptors
       is limited by the maximum length of the sub-descriptor array
       contained in the timed text descriptor in the MXF header

     catch it here for operator messages

     max bytes 65536

     8    - array len + array item size
     16*n - 16 * number of sub-descriptors

     max sub-descriptors is 4095
      (4095*16) + 8 = 65258
      (4096*16) + 8 = 65544
  */
  int resource_available = 4095;

  /* this method will grow the requested header buffer, m_HeaderSize,
      to accommodate the space needed for the timed text subdescriptors

     m_HeaderSize is already set to something, but the default of 16384
      is not enough for large sets of PNG subtitles
  */

  bool sd_array_init = false;
  for ( ri = m_TDesc.ResourceList.begin() ; ri != m_TDesc.ResourceList.end() && ASDCP_SUCCESS(result); ri++ )
    {
      if (! resource_available--)
        {
          DefaultLogSink().Error("Exceeded allowed SMPTE ST 429-5 resources.\n");
          return RESULT_FORMAT;
        }

      TimedTextResourceSubDescriptor* resourceSubdescriptor = new TimedTextResourceSubDescriptor(m_Dict);
      GenRandomValue(resourceSubdescriptor->InstanceUID);
      resourceSubdescriptor->AncillaryResourceID.Set((*ri).ResourceID);
      resourceSubdescriptor->MIMEMediaType = MIME2str((*ri).Type);
      resourceSubdescriptor->EssenceStreamID = m_EssenceStreamID++;
      m_EssenceSubDescriptorList.push_back((FileDescriptor*)resourceSubdescriptor);
      m_EssenceDescriptor->SubDescriptors.push_back(resourceSubdescriptor->InstanceUID);

      /* the first subdescriptor requires 12 bytes to initialize
           the sub-descriptor UUID array

            4 - tag/len
            4 - item count
            4 - each item len
          ----
           12 - total

      */
      if ( ! sd_array_init )
        {
          sd_array_init = true;
          m_HeaderSize += 12;
        }


      /* each subdescriptor requires 88 bytes + MIMEMediaType.archiveLength()
           of extra header space

         TimedTextDescriptor (array of sub-descriptors)
           16 - uuid in the TimedTextDescriptor sub-descriptor array

         TimedTextSubDescriptor
           16 - key
            4 - len (fixed 4 byte long length type)
            4 - tag/len
           16 - instance uuid
            4 - tag/len
           16 - ancillary resource uuid
            4 - tag/len
            n - MIMEMediaType UTF16 length
            4 - tag/len
            4 - id
        --------------
         88+n - total

      */

      /* MIMEMediaType.ArchiveLength:
          - includes sizeof(ui32_t) for an internal length representation
          - returns half of what it should for UTF16 representation of
            pure ASCII bytes
          - ( MXFTypes.h UTF16String::ArchiveLength )

         fix up the return value before using it
      */
      int n = (resourceSubdescriptor->MIMEMediaType.ArchiveLength() - sizeof(ui32_t)) * 2;
      m_HeaderSize += (88 + n);
    }

  m_EssenceStreamID = 10;
  assert(m_Dict);

  if ( ASDCP_SUCCESS(result) )
    {
      InitHeader(MXFVersion_2004);

      // First RIP Entry
      if ( m_Info.LabelSetType == LS_MXF_SMPTE )  // ERK
	{
	  m_RIP.PairArray.push_back(RIP::PartitionPair(0, 0)); // 3-part, no essence in header
	}
      else
	{
	  DefaultLogSink().Error("Unable to write Interop timed-text MXF file.  Use SMPTE DCP options instead.\n");
	  return RESULT_FORMAT;
	}

      // timecode rate and essence rate are the same
      AddSourceClip(m_TDesc.EditRate, m_TDesc.EditRate, derive_timecode_rate_from_edit_rate(m_TDesc.EditRate),
		    TIMED_TEXT_DEF_LABEL, m_EssenceUL, UL(m_Dict->ul(MDD_DataDataDef)), TIMED_TEXT_PACKAGE_LABEL);

      AddEssenceDescriptor(UL(m_Dict->ul(MDD_TimedTextWrappingClip)));
      result = m_HeaderPart.WriteToFile(m_File, m_HeaderSize);
      
      if ( KM_SUCCESS(result) )
	result = CreateBodyPart(m_TDesc.EditRate);
    }

  if ( ASDCP_SUCCESS(result) )
    {
      memcpy(m_EssenceUL, m_Dict->ul(MDD_TimedTextEssence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::h__Writer::WriteTimedTextResource(const std::string& XMLDoc,
							       ASDCP::AESEncContext* Ctx, ASDCP::HMACContext* HMAC)
{
  Result_t result = m_State.Goto_RUNNING();

  if ( ASDCP_SUCCESS(result) )
    {
      // TODO: make sure it's XML

      ui32_t str_size = XMLDoc.size();
      FrameBuffer FrameBuf(str_size);
      
      memcpy(FrameBuf.Data(), XMLDoc.c_str(), str_size);
      FrameBuf.Size(str_size);

      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = m_StreamOffset;
      result = WriteEKLVPacket(FrameBuf, m_EssenceUL, MXF_BER_LENGTH, Ctx, HMAC);

      if ( ASDCP_SUCCESS(result) )
	{
	  m_FooterPart.PushIndexEntry(Entry);
	  m_FramesWritten++;
	}
    }

  return result;
}


//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::h__Writer::WriteAncillaryResource(const ASDCP::TimedText::FrameBuffer& FrameBuf,
							       ASDCP::AESEncContext* Ctx, ASDCP::HMACContext* HMAC)
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  Kumu::fpos_t here = m_File.TellPosition();
  assert(m_Dict);

  // create generic stream partition header
  static UL GenericStream_DataElement(m_Dict->ul(MDD_GenericStream_DataElement));
  MXF::Partition GSPart(m_Dict);

  GSPart.ThisPartition = here;
  GSPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;
  GSPart.BodySID = m_EssenceStreamID;
  GSPart.OperationalPattern = m_HeaderPart.OperationalPattern;

  m_RIP.PairArray.push_back(RIP::PartitionPair(m_EssenceStreamID++, here));
  GSPart.EssenceContainers = m_HeaderPart.EssenceContainers;
  UL TmpUL(m_Dict->ul(MDD_GenericStreamPartition));
  Result_t result = GSPart.WriteToFile(m_File, TmpUL);

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, GenericStream_DataElement.Value(), MXF_BER_LENGTH, Ctx, HMAC);

  m_FramesWritten++;
  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::h__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;
  m_FramesWritten = m_TDesc.ContainerDuration;
  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}


//------------------------------------------------------------------------------------------

ASDCP::TimedText::MXFWriter::MXFWriter()
{
}

ASDCP::TimedText::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::TimedText::MXFWriter::OP1aHeader()
{
  if ( m_Writer.empty() )
    {
      assert(g_OP1aHeader);
      return *g_OP1aHeader;
    }

  return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OPAtomIndexFooter&
ASDCP::TimedText::MXFWriter::OPAtomIndexFooter()
{
  if ( m_Writer.empty() )
    {
      assert(g_OPAtomIndexFooter);
      return *g_OPAtomIndexFooter;
    }

  return m_Writer->m_FooterPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP&
ASDCP::TimedText::MXFWriter::RIP()
{
  if ( m_Writer.empty() )
    {
      assert(g_RIP);
      return *g_RIP;
    }

  return m_Writer->m_RIP;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				       const TimedTextDescriptor& TDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType != LS_MXF_SMPTE )
    {
      DefaultLogSink().Error("Timed Text support requires LS_MXF_SMPTE\n");
      return RESULT_FORMAT;
    }

  m_Writer = new h__Writer(&DefaultSMPTEDict());
  m_Writer->m_Info = Info;
  
  Result_t result = m_Writer->OpenWrite(filename, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(TDesc);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::WriteTimedTextResource(const std::string& XMLDoc, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteTimedTextResource(XMLDoc, Ctx, HMAC);
}

//
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::WriteAncillaryResource(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteAncillaryResource(FrameBuf, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::TimedText::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}



//
// end AS_DCP_timedText.cpp
//
