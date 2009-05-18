/*
Copyright (c) 2005-2009, John Hurst
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

#ifndef _Metadata_H_
#define _Metadata_H_

#include "MXF.h"

namespace ASDCP
{
  namespace MXF
    {
      void Metadata_InitTypes(const Dictionary*& Dict);

      //

      //
      class Identification : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Identification);
	  Identification();

	public:
	  const Dictionary*& m_Dict;
          UUID ThisGenerationUID;
          UTF16String CompanyName;
          UTF16String ProductName;
          VersionType ProductVersion;
          UTF16String VersionString;
          UUID ProductUID;
          Timestamp ModificationDate;
          VersionType ToolkitVersion;
          UTF16String Platform;

  Identification(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~Identification() {}
          virtual const char* HasName() { return "Identification"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class ContentStorage : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(ContentStorage);
	  ContentStorage();

	public:
	  const Dictionary*& m_Dict;
          Batch<UUID> Packages;
          Batch<UUID> EssenceContainerData;

  ContentStorage(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~ContentStorage() {}
          virtual const char* HasName() { return "ContentStorage"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class EssenceContainerData : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(EssenceContainerData);
	  EssenceContainerData();

	public:
	  const Dictionary*& m_Dict;
          UMID LinkedPackageUID;
          ui32_t IndexSID;
          ui32_t BodySID;

  EssenceContainerData(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), IndexSID(0), BodySID(0) {}
	  virtual ~EssenceContainerData() {}
          virtual const char* HasName() { return "EssenceContainerData"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPackage : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericPackage);
	  GenericPackage();

	public:
	  const Dictionary*& m_Dict;
          UMID PackageUID;
          UTF16String Name;
          Timestamp PackageCreationDate;
          Timestamp PackageModifiedDate;
          Batch<UUID> Tracks;

  GenericPackage(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~GenericPackage() {}
          virtual const char* HasName() { return "GenericPackage"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class MaterialPackage : public GenericPackage
	{
	  ASDCP_NO_COPY_CONSTRUCT(MaterialPackage);
	  MaterialPackage();

	public:
	  const Dictionary*& m_Dict;

  MaterialPackage(const Dictionary*& d) : GenericPackage(d), m_Dict(d) {}
	  virtual ~MaterialPackage() {}
          virtual const char* HasName() { return "MaterialPackage"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourcePackage : public GenericPackage
	{
	  ASDCP_NO_COPY_CONSTRUCT(SourcePackage);
	  SourcePackage();

	public:
	  const Dictionary*& m_Dict;
          UUID Descriptor;

  SourcePackage(const Dictionary*& d) : GenericPackage(d), m_Dict(d) {}
	  virtual ~SourcePackage() {}
          virtual const char* HasName() { return "SourcePackage"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericTrack : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericTrack);
	  GenericTrack();

	public:
	  const Dictionary*& m_Dict;
          ui32_t TrackID;
          ui32_t TrackNumber;
          UTF16String TrackName;
          UUID Sequence;

  GenericTrack(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), TrackID(0), TrackNumber(0) {}
	  virtual ~GenericTrack() {}
          virtual const char* HasName() { return "GenericTrack"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class StaticTrack : public GenericTrack
	{
	  ASDCP_NO_COPY_CONSTRUCT(StaticTrack);
	  StaticTrack();

	public:
	  const Dictionary*& m_Dict;

  StaticTrack(const Dictionary*& d) : GenericTrack(d), m_Dict(d) {}
	  virtual ~StaticTrack() {}
          virtual const char* HasName() { return "StaticTrack"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class Track : public GenericTrack
	{
	  ASDCP_NO_COPY_CONSTRUCT(Track);
	  Track();

	public:
	  const Dictionary*& m_Dict;
          Rational EditRate;
          ui64_t Origin;

  Track(const Dictionary*& d) : GenericTrack(d), m_Dict(d), Origin(0) {}
	  virtual ~Track() {}
          virtual const char* HasName() { return "Track"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StructuralComponent : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(StructuralComponent);
	  StructuralComponent();

	public:
	  const Dictionary*& m_Dict;
          UL DataDefinition;
          ui64_t Duration;

  StructuralComponent(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), Duration(0) {}
	  virtual ~StructuralComponent() {}
          virtual const char* HasName() { return "StructuralComponent"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class Sequence : public StructuralComponent
	{
	  ASDCP_NO_COPY_CONSTRUCT(Sequence);
	  Sequence();

	public:
	  const Dictionary*& m_Dict;
          Batch<UUID> StructuralComponents;

  Sequence(const Dictionary*& d) : StructuralComponent(d), m_Dict(d) {}
	  virtual ~Sequence() {}
          virtual const char* HasName() { return "Sequence"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class SourceClip : public StructuralComponent
	{
	  ASDCP_NO_COPY_CONSTRUCT(SourceClip);
	  SourceClip();

	public:
	  const Dictionary*& m_Dict;
          ui64_t StartPosition;
          UMID SourcePackageID;
          ui32_t SourceTrackID;

  SourceClip(const Dictionary*& d) : StructuralComponent(d), m_Dict(d), StartPosition(0), SourceTrackID(0) {}
	  virtual ~SourceClip() {}
          virtual const char* HasName() { return "SourceClip"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimecodeComponent : public StructuralComponent
	{
	  ASDCP_NO_COPY_CONSTRUCT(TimecodeComponent);
	  TimecodeComponent();

	public:
	  const Dictionary*& m_Dict;
          ui16_t RoundedTimecodeBase;
          ui64_t StartTimecode;
          ui8_t DropFrame;

  TimecodeComponent(const Dictionary*& d) : StructuralComponent(d), m_Dict(d), RoundedTimecodeBase(0), StartTimecode(0), DropFrame(0) {}
	  virtual ~TimecodeComponent() {}
          virtual const char* HasName() { return "TimecodeComponent"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericDescriptor);
	  GenericDescriptor();

	public:
	  const Dictionary*& m_Dict;
          Batch<UUID> Locators;
          Batch<UUID> SubDescriptors;

  GenericDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~GenericDescriptor() {}
          virtual const char* HasName() { return "GenericDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class FileDescriptor : public GenericDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(FileDescriptor);
	  FileDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui32_t LinkedTrackID;
          Rational SampleRate;
          ui64_t ContainerDuration;
          UL EssenceContainer;
          UL Codec;

  FileDescriptor(const Dictionary*& d) : GenericDescriptor(d), m_Dict(d), LinkedTrackID(0), ContainerDuration(0) {}
	  virtual ~FileDescriptor() {}
          virtual const char* HasName() { return "FileDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericSoundEssenceDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericSoundEssenceDescriptor);
	  GenericSoundEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          Rational AudioSamplingRate;
          ui8_t Locked;
          ui8_t AudioRefLevel;
          ui32_t ChannelCount;
          ui32_t QuantizationBits;
          ui8_t DialNorm;

  GenericSoundEssenceDescriptor(const Dictionary*& d) : FileDescriptor(d), m_Dict(d), Locked(0), AudioRefLevel(0), ChannelCount(0), QuantizationBits(0), DialNorm(0) {}
	  virtual ~GenericSoundEssenceDescriptor() {}
          virtual const char* HasName() { return "GenericSoundEssenceDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class WaveAudioDescriptor : public GenericSoundEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(WaveAudioDescriptor);
	  WaveAudioDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui16_t BlockAlign;
          ui8_t SequenceOffset;
          ui32_t AvgBps;
          UL ChannelAssignment;

  WaveAudioDescriptor(const Dictionary*& d) : GenericSoundEssenceDescriptor(d), m_Dict(d), BlockAlign(0), SequenceOffset(0), AvgBps(0) {}
	  virtual ~WaveAudioDescriptor() {}
          virtual const char* HasName() { return "WaveAudioDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericPictureEssenceDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericPictureEssenceDescriptor);
	  GenericPictureEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui8_t FrameLayout;
          ui32_t StoredWidth;
          ui32_t StoredHeight;
          Rational AspectRatio;
          UL PictureEssenceCoding;

  GenericPictureEssenceDescriptor(const Dictionary*& d) : FileDescriptor(d), m_Dict(d), FrameLayout(0), StoredWidth(0), StoredHeight(0) {}
	  virtual ~GenericPictureEssenceDescriptor() {}
          virtual const char* HasName() { return "GenericPictureEssenceDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class RGBAEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(RGBAEssenceDescriptor);
	  RGBAEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui32_t ComponentMaxRef;
          ui32_t ComponentMinRef;

  RGBAEssenceDescriptor(const Dictionary*& d) : GenericPictureEssenceDescriptor(d), m_Dict(d), ComponentMaxRef(0), ComponentMinRef(0) {}
	  virtual ~RGBAEssenceDescriptor() {}
          virtual const char* HasName() { return "RGBAEssenceDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class JPEG2000PictureSubDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(JPEG2000PictureSubDescriptor);
	  JPEG2000PictureSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
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

  JPEG2000PictureSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), Rsize(0), Xsize(0), Ysize(0), XOsize(0), YOsize(0), XTsize(0), YTsize(0), XTOsize(0), YTOsize(0), Csize(0) {}
	  virtual ~JPEG2000PictureSubDescriptor() {}
          virtual const char* HasName() { return "JPEG2000PictureSubDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CDCIEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(CDCIEssenceDescriptor);
	  CDCIEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui32_t ComponentDepth;
          ui32_t HorizontalSubsampling;
          ui32_t VerticalSubsampling;
          ui8_t ColorSiting;

  CDCIEssenceDescriptor(const Dictionary*& d) : GenericPictureEssenceDescriptor(d), m_Dict(d), ComponentDepth(0), HorizontalSubsampling(0), VerticalSubsampling(0), ColorSiting(0) {}
	  virtual ~CDCIEssenceDescriptor() {}
          virtual const char* HasName() { return "CDCIEssenceDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class MPEG2VideoDescriptor : public CDCIEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(MPEG2VideoDescriptor);
	  MPEG2VideoDescriptor();

	public:
	  const Dictionary*& m_Dict;
          ui8_t CodedContentType;
          ui8_t LowDelay;
          ui32_t BitRate;
          ui8_t ProfileAndLevel;

  MPEG2VideoDescriptor(const Dictionary*& d) : CDCIEssenceDescriptor(d), m_Dict(d), CodedContentType(0), LowDelay(0), BitRate(0), ProfileAndLevel(0) {}
	  virtual ~MPEG2VideoDescriptor() {}
          virtual const char* HasName() { return "MPEG2VideoDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class DMSegment : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(DMSegment);
	  DMSegment();

	public:
	  const Dictionary*& m_Dict;
          UL DataDefinition;
          ui64_t EventStartPosition;
          ui64_t Duration;
          UTF16String EventComment;
          UUID DMFramework;

  DMSegment(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), EventStartPosition(0), Duration(0) {}
	  virtual ~DMSegment() {}
          virtual const char* HasName() { return "DMSegment"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicFramework : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(CryptographicFramework);
	  CryptographicFramework();

	public:
	  const Dictionary*& m_Dict;
          UUID ContextSR;

  CryptographicFramework(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~CryptographicFramework() {}
          virtual const char* HasName() { return "CryptographicFramework"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class CryptographicContext : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(CryptographicContext);
	  CryptographicContext();

	public:
	  const Dictionary*& m_Dict;
          UUID ContextID;
          UL SourceEssenceContainer;
          UL CipherAlgorithm;
          UL MICAlgorithm;
          UUID CryptographicKeyID;

  CryptographicContext(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~CryptographicContext() {}
          virtual const char* HasName() { return "CryptographicContext"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class GenericDataEssenceDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericDataEssenceDescriptor);
	  GenericDataEssenceDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UL DataEssenceCoding;

  GenericDataEssenceDescriptor(const Dictionary*& d) : FileDescriptor(d), m_Dict(d) {}
	  virtual ~GenericDataEssenceDescriptor() {}
          virtual const char* HasName() { return "GenericDataEssenceDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimedTextDescriptor : public GenericDataEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(TimedTextDescriptor);
	  TimedTextDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UUID ResourceID;
          UTF16String UCSEncoding;
          UTF16String NamespaceURI;

  TimedTextDescriptor(const Dictionary*& d) : GenericDataEssenceDescriptor(d), m_Dict(d) {}
	  virtual ~TimedTextDescriptor() {}
          virtual const char* HasName() { return "TimedTextDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class TimedTextResourceSubDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(TimedTextResourceSubDescriptor);
	  TimedTextResourceSubDescriptor();

	public:
	  const Dictionary*& m_Dict;
          UUID AncillaryResourceID;
          UTF16String MIMEMediaType;
          ui32_t EssenceStreamID;

  TimedTextResourceSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d), EssenceStreamID(0) {}
	  virtual ~TimedTextResourceSubDescriptor() {}
          virtual const char* HasName() { return "TimedTextResourceSubDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

      //
      class StereoscopicPictureSubDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(StereoscopicPictureSubDescriptor);
	  StereoscopicPictureSubDescriptor();

	public:
	  const Dictionary*& m_Dict;

  StereoscopicPictureSubDescriptor(const Dictionary*& d) : InterchangeObject(d), m_Dict(d) {}
	  virtual ~StereoscopicPictureSubDescriptor() {}
          virtual const char* HasName() { return "StereoscopicPictureSubDescriptor"; }
          virtual Result_t InitFromTLVSet(TLVReader& TLVSet);
          virtual Result_t WriteToTLVSet(TLVWriter& TLVSet);
	  virtual void     Dump(FILE* = 0);
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	};

    } // namespace MXF
} // namespace ASDCP


#endif // _Metadata_H_

//
// end Metadata.h
//
