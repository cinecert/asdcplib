//
//
// TODO: constructor initializers for all member variables
//

#ifndef _METADATA_H_
#define _METADATA_H_

#include "MXF.h"

namespace ASDCP
{
  namespace MXF
    {
      //
      class Identification : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Identification);

	public:
	  UUID         ThisGenerationUID;
	  UTF16String  CompanyName;
	  UTF16String  ProductName;
	  ui16_t       ProductVersion;
	  UTF16String  VersionString;
	  UUID         ProductUID;
	  Timestamp    ModificationDate;
	  ui16_t       ToolkitVersion;
	  UTF16String  Platform;

	  Identification() {}
	  virtual ~Identification() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};


      //
      class ContentStorage : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(ContentStorage);

	public:
	  UUID        GenerationUID;
	  Batch<UUID> Packages;
	  Batch<UUID> EssenceContainerData;

	  ContentStorage() {}
	  virtual ~ContentStorage() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};

      // 
      class GenericPackage : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericPackage);

	public:
	  UMID          PackageUID;
	  UUID          GenerationUID;
          UTF16String   Name;
          Timestamp     PackageCreationDate;
          Timestamp     PackageModifiedDate;
          Batch<UID>    Tracks;

          GenericPackage() {}
	  virtual ~GenericPackage() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l) = 0;
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0) = 0;
        };


      // 
      class MaterialPackage : public GenericPackage
	{
	  ASDCP_NO_COPY_CONSTRUCT(MaterialPackage);

	public:
          MaterialPackage() {}
	  virtual ~MaterialPackage() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };


      // 
      class SourcePackage : public GenericPackage
	{
	  ASDCP_NO_COPY_CONSTRUCT(SourcePackage);

	public:
          SourcePackage() {}
	  virtual ~SourcePackage() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };

      //
      class Track : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Track);

	public:
	  UUID        GenerationUID;
	  ui32_t      TrackID;
	  ui32_t      TrackNumber;
	  UTF16String TrackName;
	  UUID        Sequence;
	  Rational    EditRate;
	  ui64_t      Origin;

	  Track() {}
	  virtual ~Track() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};

      // 
      class Sequence : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(Sequence);

	public:
	  UUID          GenerationUID;
          UL            DataDefinition;
          ui64_t        Duration;
          Batch<UID>    StructuralComponents;

          Sequence() {}
	  virtual ~Sequence() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };

      // 
      class SourceClip : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(SourceClip);

	public:
	  UUID          GenerationUID;
          UL            DataDefinition;
          ui64_t        StartPosition;
          ui64_t        Duration;
          UMID          SourcePackageID;
          ui32_t        SourceTrackID;

          SourceClip() {}
	  virtual ~SourceClip() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };

       //
       class TimecodeComponent : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(TimecodeComponent);

	public:
	  UUID          GenerationUID;
          UL            DataDefinition;
          ui64_t        Duration;
          ui16_t        RoundedTimecodeBase;
          ui64_t        StartTimecode;
          ui8_t         DropFrame;

          TimecodeComponent() {}
	  virtual ~TimecodeComponent() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };
    
      //
      class MPEG2VideoDescriptor : public ASDCP::MPEG2::VideoDescriptor, public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(MPEG2VideoDescriptor);

        public:
	  ui32_t LinkedTrackID;
	  UL     EssenceContainer;

	  MPEG2VideoDescriptor() : LinkedTrackID(0) {}
	  ~MPEG2VideoDescriptor() {}
          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
          virtual void     Dump(FILE* = 0);
	};

      //
      class FileDescriptor : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(FileDescriptor);

        public:
	  UUID      GenerationUID;
	  Batch<UUID> Locators;
	  Batch<UUID> SubDescriptors;
	  ui32_t    LinkedTrackID;
	  Rational  SampleRate;
	  ui64_t    ContainerDuration;
	  UL        EssenceContainer;
	  UL        Codec;

	  FileDescriptor() : LinkedTrackID(0), ContainerDuration(0) {}
	  ~FileDescriptor() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
	};

      //
      class WaveAudioDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(WaveAudioDescriptor);

        public:
	  Rational AudioSamplingRate;
	  ui8_t    Locked;
	  i8_t     AudioRefLevel;
	  ui8_t    ElectroSpatialFormulation;
	  ui32_t   ChannelCount;
	  ui32_t   QuantizationBits;
	  i8_t     DialNorm;
	  UL       SoundEssenceCompression;
	  ui16_t   BlockAlign;
	  ui8_t    SequenceOffset;
	  ui32_t   AvgBps;
	  //	  Stream   PeakEnvelope;

	  WaveAudioDescriptor() :
	    Locked(false), AudioRefLevel(0), ElectroSpatialFormulation(0),
	    ChannelCount(0), QuantizationBits(0), DialNorm(0), BlockAlign(0), 
	    SequenceOffset(0), AvgBps(0) {}

	  ~WaveAudioDescriptor() {}
          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
          virtual void     Dump(FILE* = 0);
	};

      //
      class GenericPictureEssenceDescriptor : public FileDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(GenericPictureEssenceDescriptor);

        public:
	  ui8_t    FrameLayout;
	  ui32_t   StoredWidth;
	  ui32_t   StoredHeight;
	  ui32_t   DisplayWidth;
	  ui32_t   DisplayHeight;
	  Rational AspectRatio;
	  ui32_t   ComponentMaxRef;
	  ui32_t   ComponentMinRef;
	  UL       Gamma;
	  UL       PictureEssenceCoding;

	  GenericPictureEssenceDescriptor() :
	    FrameLayout(0), StoredWidth(0), StoredHeight(0), DisplayWidth(0), 
	    DisplayHeight(0), ComponentMaxRef(0), ComponentMinRef(0) {}

	  ~GenericPictureEssenceDescriptor() {}
	  //          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  //          virtual void     Dump(FILE* = 0);
	};

      //
      class RGBAEssenceDescriptor : public GenericPictureEssenceDescriptor
	{
	  ASDCP_NO_COPY_CONSTRUCT(RGBAEssenceDescriptor);

        public:
	  class RGBLayout
	    {
	    public:
	      struct element {
		ui8_t Code;
		ui8_t Depth;
	      } PictureElement[8];
	      RGBLayout() { memset(PictureElement, 0, sizeof(PictureElement)); }
	    };

	  RGBLayout PixelLayout;

	  RGBAEssenceDescriptor() {}
	  ~RGBAEssenceDescriptor() {}
	  //          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  //          virtual void     Dump(FILE* = 0);
	};

      class Raw
	{
	  ASDCP_NO_COPY_CONSTRUCT(Raw);

	public:
	  byte_t* data;
	  Raw() {}
	  ~Raw() {}

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

	  JPEG2000PictureSubDescriptor() :
	    Rsize(0), Xsize(0), Ysize(0),
	    XOsize(0), YOsize(0), XTsize(0),
	    YTsize(0), XTOsize(0), YTOsize(0),
	    Csize(0) {}

	  ~JPEG2000PictureSubDescriptor() {}
          //          virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
          //          virtual void     Dump(FILE* = 0);
        };


      //
      class CryptographicFramework : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(CryptographicFramework);

	public:
	  UUID ContextSR;

	  CryptographicFramework() {}
	  ~CryptographicFramework() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  // virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };

      //
      class CryptographicContext : public InterchangeObject
	{
	  ASDCP_NO_COPY_CONSTRUCT(CryptographicContext);

	public:
	  UUID ContextID;
	  UL   SourceEssenceContainer;
	  UL   CipherAlgorithm;
	  UL   MICAlgorithm;
	  UUID CryptographicKeyID;

	  CryptographicContext() {}
	  ~CryptographicContext() {}
	  virtual Result_t InitFromBuffer(const byte_t* p, ui32_t l);
	  // virtual Result_t WriteToBuffer(ASDCP::FrameBuffer&);
	  virtual void     Dump(FILE* = 0);
        };

    } // namespace MXF
} // namespace ASDCP


#endif // _METADATA_H_

//
// end Metadata.h
//
