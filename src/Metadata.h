/*
Copyright (c) 2005-2006, John Hurst
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
/*! \file    Metadata.h
    \version $Id$
    \brief   MXF metadata objects
*/

#ifndef _METADATA_H_
#define _METADATA_H_

#include "MXF.h"

namespace ASDCP
{
  namespace MXF
    {
      // Types
      class RGBLayout : public IArchive
        {
        public:
          struct element {
            ui8_t Code;
            ui8_t Depth;
          } PictureElement[8];
          RGBLayout() { memset(PictureElement, 0, sizeof(PictureElement)); }

	  //
          Result_t ReadFrom(ASDCP::MemIOReader& Reader) { return RESULT_OK; }
	  Result_t WriteTo(ASDCP::MemIOWriter& Writer) { return RESULT_OK; }
          inline const char* ToString(char* str_buf) const {
            snprintf(str_buf, IdentBufferLen, "RGBLayout: <PictureElement[8]>\n");
            return str_buf;
          }
        };

      class Raw : public IArchive
	{
	  ASDCP_NO_COPY_CONSTRUCT(Raw);

	public:
	  byte_t* data;
	  Raw() {}
	  ~Raw() {}

	  //
          Result_t ReadFrom(ASDCP::MemIOReader& Reader) { return RESULT_OK; }
	  Result_t WriteTo(ASDCP::MemIOWriter& Writer) { return RESULT_OK; }
          inline const char* ToString(char* str_buf) const {
            snprintf(str_buf, IdentBufferLen, "RAW\n");
            return str_buf;
          }
	};

      //

      //
      class Identification : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Identification);

	public:
          UUID ThisGenerationUID;
          UTF16String CompanyName;
          UTF16String ProductName;
          ui16_t ProductVersion;
          UTF16String VersionString;
          UUID ProductUID;
          Timestamp ModificationDate;
          ui16_t ToolkitVersion;
          UTF16String Platform;

	  Identification() : ProductVersion(0), ToolkitVersion(0) {}
	  virtual ~Identification() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ContentStorage : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(ContentStorage);

	public:
          Batch<UUID> Packages;
          Batch<UUID> EssenceContainerData;

	  ContentStorage() {}
	  virtual ~ContentStorage() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPackage : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericPackage);

	public:
          UMID PackageUID;
          UTF16String Name;
          Timestamp PackageCreationDate;
          Timestamp PackageModifiedDate;
          Batch<UID> Tracks;

	  GenericPackage() {}
	  virtual ~GenericPackage() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class MaterialPackage : public GenericPackage
	{
	  ASDCP_NO_COPY_CONSTRUCT(MaterialPackage);

	public:

	  MaterialPackage() {}
	  virtual ~MaterialPackage() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourcePackage : public GenericPackage
	{
	  ASDCP_NO_COPY_CONSTRUCT(SourcePackage);

	public:

	  SourcePackage() {}
	  virtual ~SourcePackage() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericTrack : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericTrack);

	public:
          ui32_t TrackID;
          ui32_t TrackNumber;
          UTF16String TrackName;
          UUID Sequence;

	  GenericTrack() : TrackID(0), TrackNumber(0) {}
	  virtual ~GenericTrack() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class Track : public GenericTrack
	{
	  ASDCP_NO_COPY_CONSTRUCT(Track);

	public:
          Rational EditRate;
          ui64_t Origin;

	  Track() : Origin(0) {}
	  virtual ~Track() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StructuralComponent : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(StructuralComponent);

	public:
          UL DataDefinition;
          ui64_t Duration;

	  StructuralComponent() : Duration(0) {}
	  virtual ~StructuralComponent() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class Sequence : public StructuralComponent
	{
	  ASDCP_NO_COPY_CONSTRUCT(Sequence);

	public:
          Batch<UID> StructuralComponents;

	  Sequence() {}
	  virtual ~Sequence() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourceClip : public StructuralComponent
	{
	  ASDCP_NO_COPY_CONSTRUCT(SourceClip);

	public:
          ui64_t StartPosition;
          UMID SourcePackageID;
          ui32_t SourceTrackID;

	  SourceClip() : StartPosition(0), SourceTrackID(0) {}
	  virtual ~SourceClip() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimecodeComponent : public StructuralComponent
	{
	  ASDCP_NO_COPY_CONSTRUCT(TimecodeComponent);

	public:
          ui16_t RoundedTimecodeBase;
          ui64_t StartTimecode;
          ui8_t DropFrame;

	  TimecodeComponent() : RoundedTimecodeBase(0), StartTimecode(0), DropFrame(0) {}
	  virtual ~TimecodeComponent() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericDescriptor);

	public:
          Batch<UUID> Locators;
          Batch<UUID> SubDescriptors;

	  GenericDescriptor() {}
	  virtual ~GenericDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class FileDescriptor : public GenericDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(FileDescriptor);

	public:
          ui32_t LinkedTrackID;
          Rational SampleRate;
          ui64_t ContainerDuration;
          UL EssenceContainer;
          UL Codec;

	  FileDescriptor() : LinkedTrackID(0), ContainerDuration(0) {}
	  virtual ~FileDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericSoundEssenceDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericSoundEssenceDescriptor);

	public:
          Rational AudioSamplingRate;
          ui8_t Locked;
          ui8_t AudioRefLevel;
          ui8_t ElectroSpatialFormulation;
          ui32_t ChannelCount;
          ui32_t QuantizationBits;
          ui8_t DialNorm;
          UL SoundEssenceCompression;

	  GenericSoundEssenceDescriptor() : Locked(0), AudioRefLevel(0), ElectroSpatialFormulation(0), ChannelCount(0), QuantizationBits(0), DialNorm(0) {}
	  virtual ~GenericSoundEssenceDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class WaveAudioDescriptor : public GenericSoundEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(WaveAudioDescriptor);

	public:
          ui16_t BlockAlign;
          ui8_t SequenceOffset;
          ui32_t AvgBps;

	  WaveAudioDescriptor() : BlockAlign(0), SequenceOffset(0), AvgBps(0) {}
	  virtual ~WaveAudioDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPictureEssenceDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericPictureEssenceDescriptor);

	public:
          ui8_t FrameLayout;
          ui32_t StoredWidth;
          ui32_t StoredHeight;
          ui32_t DisplayWidth;
          ui32_t DisplayHeight;
          Rational AspectRatio;
          UL Gamma;
          UL PictureEssenceCoding;

	  GenericPictureEssenceDescriptor() : FrameLayout(0), StoredWidth(0), StoredHeight(0), DisplayWidth(0), DisplayHeight(0) {}
	  virtual ~GenericPictureEssenceDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class RGBAEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(RGBAEssenceDescriptor);

	public:
          ui32_t ComponentMaxRef;
          ui32_t ComponentMinRef;
          RGBLayout PixelLayout;

	  RGBAEssenceDescriptor() : ComponentMaxRef(0), ComponentMinRef(0) {}
	  virtual ~RGBAEssenceDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class JPEG2000PictureSubDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(JPEG2000PictureSubDescriptor);

	public:
          ui16_t Rsize;
          ui32_t Xsize;
          ui32_t Ysize;
          ui32_t XOsize;
          ui32_t YOsize;
          ui32_t XTsize;
          ui32_t YTsize;
          ui32_t XTOsize;
          ui32_t YTOsize;
          ui16_t Csize;
          Raw PictureComponentSizing;
          Raw CodingStyleDefault;
          Raw QuantizationDefault;

	  JPEG2000PictureSubDescriptor() : Rsize(0), Xsize(0), Ysize(0), XOsize(0), YOsize(0), XTsize(0), YTsize(0), XTOsize(0), YTOsize(0), Csize(0) {}
	  virtual ~JPEG2000PictureSubDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CDCIEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(CDCIEssenceDescriptor);

	public:
          ui32_t ComponentDepth;
          ui32_t HorizontalSubsampling;
          ui32_t VerticalSubsampling;
          ui8_t ColorSiting;
          ui8_t ReversedByteOrder;
          ui32_t BlackRefLevel;
          ui32_t WhiteReflevel;
          ui32_t ColorRange;

	  CDCIEssenceDescriptor() : ComponentDepth(0), HorizontalSubsampling(0), VerticalSubsampling(0), ColorSiting(0), ReversedByteOrder(0), BlackRefLevel(0), WhiteReflevel(0), ColorRange(0) {}
	  virtual ~CDCIEssenceDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class MPEG2VideoDescriptor : public CDCIEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(MPEG2VideoDescriptor);

	public:
          ui8_t CodedContentType;
          ui8_t LowDelay;
          ui32_t BitRate;
          ui8_t ProfileAndLevel;

	  MPEG2VideoDescriptor() : CodedContentType(0), LowDelay(0), BitRate(0), ProfileAndLevel(0) {}
	  virtual ~MPEG2VideoDescriptor() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicFramework : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(CryptographicFramework);

	public:
          UUID ContextSR;

	  CryptographicFramework() {}
	  virtual ~CryptographicFramework() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicContext : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(CryptographicContext);

	public:
          UUID ContextID;
          UL SourceEssenceContainer;
          UL CipherAlgorithm;
          UL MICAlgorithm;
          UUID CryptographicKeyID;

	  CryptographicContext() {}
	  virtual ~CryptographicContext() {}
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

    } // namespace MXF
} // namespace ASDCP


#endif // _METADATA_H_

//
// end Metadata.h
//
