/*
Copyright (c) 2018, Dolby Laboratories

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
/*! \file    AS_02_IAB.cpp
    \brief   AS-02 library, IAB essence reader and writer implementation (ST 2067-201)
>>>>>>> a641e228ad968542caa471160551ef9cd2c32140
*/

#include "AS_02_internal.h"
#include "AS_02_IAB.h"
#include "Metadata.h"
#include <iostream>


using namespace ASDCP;
using namespace AS_02;
using namespace AS_02::IAB;


//------------------------------------------------------------------------------------------

AS_02_NAMESPACE_BEGIN
    namespace IAB
{
    static std::string IAB_PACKAGE_LABEL = "File Package: SMPTE-GC clip wrapped IAB essence";
    static std::string IAB_DEF_LABEL = "IAB Track";
    static byte_t IAB_ESSENCE_CODING[SMPTE_UL_LENGTH] =
    { 
        0x06, 0x0e, 0x2b, 0x34, 
        0x04, 0x01, 0x01, 0x05,
        0x0e, 0x09, 0x06, 0x04, 
        0x00, 0x00, 0x00, 0x00 
    };

} // namespace ATMOS
AS_02_NAMESPACE_END

std::ostream& AS_02::IAB::operator<< (std::ostream& strm, const IABDescriptor& desc)
{
    strm << "        FrameRate: " << (unsigned) desc.FrameRate.Numerator << "/" << (unsigned) desc.FrameRate.Denominator << std::endl;
    strm << "       SampleRate: " << (unsigned) desc.SampleRate.Numerator << "/" << (unsigned) desc.SampleRate.Denominator << std::endl;
    strm << "         BitDepth: " << desc.BitDepth << std::endl;
    strm << "ContainerDuration: " << (unsigned) desc.ContainerDuration << std::endl;
    return strm;
}

void AS_02::IAB::DescriptorDump(const IABDescriptor& desc, FILE *stream)
{
    if ( stream == 0 )
        stream = stderr;

    fprintf(stream, "        FrameRate: %u/%u\n", desc.FrameRate.Numerator, desc.FrameRate.Denominator);
    fprintf(stream, "       SampleRate: %u/%u\n", desc.SampleRate.Numerator, desc.SampleRate.Denominator);
    fprintf(stream, "         BitDepth: %u\n",    desc.BitDepth);
    fprintf(stream, "ContainerDuration: %u\n",    desc.ContainerDuration);
}

class AS_02::IAB::MXFReader::h__Reader : public AS_02::h__AS02Reader
{
    ui64_t m_ClipEnd;
    IABEssenceDescriptor* m_EssenceDescriptor;
    IABSoundfieldLabelSubDescriptor* m_EssenceSubDescriptor;

    ASDCP_NO_COPY_CONSTRUCT(h__Reader);

public:
    h__Reader(const Dictionary& d) :
        AS_02::h__AS02Reader(d),
        m_ClipEnd(0),
        m_EssenceDescriptor(NULL),
        m_EssenceSubDescriptor(NULL)
    {}
    virtual ~h__Reader() {}

    IABDescriptor m_Desc;

    Result_t    OpenRead(const std::string& filename);
    Result_t    ReadFrame(ui32_t, ASDCP::FrameBuffer&, AESDecContext*, HMACContext*);
    Result_t    ReadMetadata(const std::string &description, std::string &mimeType, ASDCP::FrameBuffer&);
    Result_t    FillDescriptor(IABDescriptor&) const;
};

//
Result_t AS_02::IAB::MXFReader::h__Reader::OpenRead(const std::string& filename)
{
    Result_t result = OpenMXFRead(filename.c_str());
    ui32_t SimplePayloadSID = 0;

    if(KM_SUCCESS(result))
    {
        InterchangeObject* iObj = 0;
        result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(IABEssenceDescriptor), &iObj);

        if (KM_SUCCESS(result))
        {
            m_EssenceDescriptor = dynamic_cast<IABEssenceDescriptor*>(iObj);
        }
    }

    if (m_EssenceDescriptor == 0)
    {
        DefaultLogSink().Error("IABEssenceDescriptor object not found in IMF/IAB MXF file.\n");
        return RESULT_FORMAT;
    }

    m_Desc.FrameRate = m_EssenceDescriptor->SampleRate;
    m_Desc.SampleRate = m_EssenceDescriptor->AudioSamplingRate;
    m_Desc.BitDepth = m_EssenceDescriptor->QuantizationBits;
    m_Desc.ContainerDuration = (ui32_t)m_EssenceDescriptor->ContainerDuration;
    
    if (!m_EssenceDescriptor->ReferenceImageEditRate.empty())
        m_Desc.ImageFrameRate = m_EssenceDescriptor->ReferenceImageEditRate.get();
    if (!m_EssenceDescriptor->ReferenceAudioAlignmentLevel.empty())
        m_Desc.AudioAlignmentLevel = m_EssenceDescriptor->ReferenceAudioAlignmentLevel.get();

    // check for sample/frame rate sanity
    if ( ASDCP_SUCCESS(result)
        && m_Desc.FrameRate != EditRate_23_98
        && m_Desc.FrameRate != EditRate_24
        && m_Desc.FrameRate != EditRate_25
        && m_Desc.FrameRate != EditRate_30
        && m_Desc.FrameRate != EditRate_48
        && m_Desc.FrameRate != EditRate_50
        && m_Desc.FrameRate != EditRate_60
        && m_Desc.FrameRate != EditRate_96
        && m_Desc.FrameRate != EditRate_100
        && m_Desc.FrameRate != EditRate_120)
    {
        DefaultLogSink().Error("IAB file EditRate is not a supported value: %d/%d\n",
            m_Desc.FrameRate.Numerator, m_Desc.FrameRate.Denominator);

        return RESULT_FORMAT;
    }

    if (KM_SUCCESS(result))
    {
        InterchangeObject* iObj = NULL;
        result = m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(IABSoundfieldLabelSubDescriptor), &iObj);
        m_EssenceSubDescriptor = static_cast<ASDCP::MXF::IABSoundfieldLabelSubDescriptor*>(iObj);

        if (iObj == 0)
        {
            DefaultLogSink().Error("IABSoundfieldLabelSubDescriptor object not found.\n");
            return RESULT_FORMAT;
        }

        if (m_EssenceSubDescriptor->RFC5646SpokenLanguage.empty()) // this is required by IMF Plug-in Level 0 spec
        {
            DefaultLogSink().Warn("Missing RFC5646SpokenLanguage value");
        }
        else
        {
            m_Desc.RFC5646SpokenLanguage = m_EssenceSubDescriptor->RFC5646SpokenLanguage.get();
        }

        if (!m_EssenceSubDescriptor->MCAAudioContentKind.empty())
        {
            m_Desc.AudioContentKind.set(m_EssenceSubDescriptor->MCAAudioContentKind.get());
        }
        if (!m_EssenceSubDescriptor->MCAAudioElementKind.empty())
        {
            m_Desc.AudioElementKind.set(m_EssenceSubDescriptor->MCAAudioElementKind.get());
        }
        if (!m_EssenceSubDescriptor->MCATitle.empty())
        {
            m_Desc.Title.set(m_EssenceSubDescriptor->MCATitle.get());
        }
        if (!m_EssenceSubDescriptor->MCATitleVersion.empty())
        {
            m_Desc.TitleVersion.set(m_EssenceSubDescriptor->MCATitleVersion.get());
        }
    }

    bool foundBody = false;
    RIP::const_pair_iterator pi ;
    for ( pi = m_RIP.PairArray.begin(); pi != m_RIP.PairArray.end() && ASDCP_SUCCESS(result); ++pi )
    {
        if (pi->BodySID == 1)
        {
            foundBody = true;
            continue;
        }
        if (!foundBody)
        {
            // assuming that the Body Index Table is in its own partition immediately 
            // following the Body partition with our essence container;
            // This is not as specified in SP 2067-5 where it defines the index strategy to be 'lead'
            // but that is not implemented in this library yet
            continue;
        }

        result = m_File.Seek((*pi).ByteOffset);

        if (ASDCP_SUCCESS(result))
        {
            Partition TmpPart(m_Dict);
            result = TmpPart.InitFromFile(m_File);

            m_ClipEnd = TmpPart.ThisPartition;
            break;
        }
    }
    if (m_ClipEnd == 0)
    {
        DefaultLogSink().Error("Could not find end of clip-wrapped essence\n");
        return RESULT_FORMAT;
    }
    
    return result;
}

//
//
Result_t AS_02::IAB::MXFReader::h__Reader::ReadFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC)
{
    if ( ! m_File.IsOpen() )
    {
        return RESULT_INIT;
    }

    if ( FrameNum >= m_EssenceDescriptor->ContainerDuration )
    {
        return RESULT_RANGE;
    }

    Result_t result = RESULT_OK;

    ASDCP::MXF::IndexTableSegment::IndexEntry Entry;
    ASDCP::MXF::IndexTableSegment::IndexEntry NextEntry;
    ui32_t read_size = 0;
    
    if (KM_FAILURE(m_IndexAccess.Lookup(FrameNum, Entry)))
    {
	    DefaultLogSink().Error("Frame value out of range: %u\n", FrameNum);
	    return RESULT_RANGE;
    }
    ui64_t offset = Entry.StreamOffset;
    if ((FrameNum + 1) < m_EssenceDescriptor->ContainerDuration && KM_SUCCESS(m_IndexAccess.Lookup(FrameNum+1, NextEntry)))
    {
        read_size =  (ui32_t)(NextEntry.StreamOffset - Entry.StreamOffset);
    }
    else
    {
        read_size = (ui32_t)(m_ClipEnd - offset);
    }
    
    if (FrameBuf.Capacity() < read_size)
    {
        return RESULT_SMALLBUF;
    }

    if ( m_File.Tell() != offset )
    {
        result = m_File.Seek(offset);
    }

    if (KM_SUCCESS(result))
    {
        result = m_File.Read(FrameBuf.Data(), read_size);

        if (KM_SUCCESS(result))
        {
            FrameBuf.Size(read_size);

            if ( read_size < FrameBuf.Capacity())
            {
                memset(FrameBuf.Data() + FrameBuf.Size(), 0, FrameBuf.Capacity() - FrameBuf.Size());
            }
        }
    }

    return result;
}

//
//
Result_t AS_02::IAB::MXFReader::h__Reader::ReadMetadata(const std::string &description, std::string &mimeType, ASDCP::FrameBuffer& FrameBuffer)
{
    if ( ! m_File.IsOpen() )
    {
        return RESULT_INIT;
    }

    Result_t result = RESULT_OK;

    std::list<InterchangeObject*> objects;
    if (KM_SUCCESS(m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(GenericStreamTextBasedSet), objects)))
    {
        for (std::list<InterchangeObject*>::iterator it = objects.begin(); it != objects.end(); it++)
        {
            GenericStreamTextBasedSet* set = static_cast<GenericStreamTextBasedSet*>(*it);
            if (set->TextDataDescription == description)
            {
                mimeType = set->TextMIMEMediaType;
                unsigned int GSBodySID = set->GenericStreamSID;

                // need to find GSPartition with the given SID
                RIP::const_pair_iterator pi ;
                for ( pi = m_RIP.PairArray.begin(); pi != m_RIP.PairArray.end() && ASDCP_SUCCESS(result); pi++ )
                {
                    if (pi->BodySID != GSBodySID)
                    {
                        continue;
                    }
                    result = m_File.Seek((*pi).ByteOffset);
                    if (!ASDCP_SUCCESS(result))
                        return result;

                    Partition TmpPart(m_Dict);
                    result = TmpPart.InitFromFile(m_File);
                    if (!ASDCP_SUCCESS(result))
                        return result;

                    KLReader Reader;
                    result = Reader.ReadKLFromFile(m_File);
                    if (!ASDCP_SUCCESS(result))
                        return result;
                    // extend buffer capacity to hold the data
                    result = FrameBuffer.Capacity((ui32_t)Reader.Length());
                    if (!ASDCP_SUCCESS(result))
                        return result;
                    // read the data into the supplied buffer
                    ui32_t read_count;
                    result = m_File.Read(FrameBuffer.Data(), (ui32_t)Reader.Length(), &read_count);
                    if (!ASDCP_SUCCESS(result))
                        return result;
                    if (read_count != Reader.Length())
                        return RESULT_READFAIL;

                    FrameBuffer.Size(read_count);
                    break; // found the partition and processed it
                }
            }
        }
    }

    return result;
}

Result_t AS_02::IAB::MXFReader::h__Reader::FillDescriptor(IABDescriptor& Desc) const
{
    Desc = m_Desc;
    return RESULT_OK;
}

//------------------------------------------------------------------------------------------
//

AS_02::IAB::MXFReader::MXFReader()
{
    m_Reader = new h__Reader(DefaultSMPTEDict());
}


AS_02::IAB::MXFReader::~MXFReader()
{
    if ( m_Reader && m_Reader->m_File.IsOpen())
        m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader& AS_02::IAB::MXFReader::OP1aHeader()
{
    if ( m_Reader.empty())
    {
        assert(g_OP1aHeader);
        return *g_OP1aHeader;
    }

    return m_Reader->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
AS_02::MXF::AS02IndexReader& AS_02::IAB::MXFReader::AS02IndexReader()
{
    if ( m_Reader.empty())
    {
        assert(g_AS02IndexReader);
        return *g_AS02IndexReader;
    }

    return m_Reader->m_IndexAccess;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP& AS_02::IAB::MXFReader::RIP()
{
    if ( m_Reader.empty())
    {
        assert(g_RIP);
        return *g_RIP;
    }

    return m_Reader->m_RIP;
}

// Open the file for reading. The file must exist. Returns error if the
// operation cannot be completed.
Result_t AS_02::IAB::MXFReader::OpenRead(const std::string& filename) const
{
    return m_Reader->OpenRead(filename);
}

//
Result_t AS_02::IAB::MXFReader::Close() const
{
    if ( m_Reader && m_Reader->m_File.IsOpen())
    {
        m_Reader->Close();
        return RESULT_OK;
    }

    return RESULT_INIT;
}

//
Result_t AS_02::IAB::MXFReader::ReadFrame(ui32_t FrameNum, ASDCP::FrameBuffer& FrameBuf, ASDCP::AESDecContext* Ctx, ASDCP::HMACContext* HMAC) const
{
    if ( m_Reader && m_Reader->m_File.IsOpen())
    {
        return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);
    }

    return RESULT_INIT;
}

//
Result_t AS_02::IAB::MXFReader::ReadMetadata(const std::string &description, std::string &mimeType, ASDCP::FrameBuffer& FrameBuf) 
{
    if ( m_Reader && m_Reader->m_File.IsOpen())
    {
        return m_Reader->ReadMetadata(description, mimeType, FrameBuf);
    }

    return RESULT_INIT;
}

// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
Result_t AS_02::IAB::MXFReader::FillWriterInfo(ASDCP::WriterInfo& Info) const
{
    if ( m_Reader && m_Reader->m_File.IsOpen())
    {
        Info = m_Reader->m_Info;
        return RESULT_OK;
    }

    return RESULT_INIT;
}

Result_t AS_02::IAB::MXFReader::FillDescriptor(IABDescriptor& ADesc) const
{
    if ( m_Reader && m_Reader->m_File.IsOpen())
    {
        return m_Reader->FillDescriptor(ADesc);
    }

    return RESULT_INIT;
}

void AS_02::IAB::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
    if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
        m_Reader->m_HeaderPart.Dump(stream);
    }
}


//
void AS_02::IAB::MXFReader::DumpIndex(FILE* stream) const
{
    if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
        m_Reader->m_IndexAccess.Dump(stream);
    }
}

//------------------------------------------------------------------------------------------

//
class AS_02::IAB::MXFWriter::h__Writer : public AS_02::h__AS02WriterClip<AS_02::MXF::AS02IndexWriterVBR>
{
    ui64_t      m_TotalBytesWritten;
    ui32_t      m_StartPayloadOffset; // start of V section of the clip-wrapped KLV
    ui32_t      m_GenericStreamID;  // keep track of text-based metadata partitions
    ui32_t      m_NextTrackID;     // for adding Descriptive metadata tracks
    byte_t      m_EssenceUL[SMPTE_UL_LENGTH];

    ASDCP_NO_COPY_CONSTRUCT(h__Writer);
    h__Writer();

public:
    h__Writer(const Dictionary& d) : 
        h__AS02WriterClip(d),
        m_TotalBytesWritten(0),
        m_StartPayloadOffset(0),
        m_GenericStreamID(2), // starts with 2 because Essence Items are stored in Body Partition with ID = 1
        m_NextTrackID(2)    // starts with 2 because there one other track in Material Package: Audio Essence track
    {
        memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
    }

    virtual ~h__Writer(){}

    Result_t OpenWrite(const std::string&, ui32_t HeaderSize, const IABDescriptor& ADesc);
    Result_t SetSourceStream(const IABDescriptor&, const byte_t*, const std::string&, const std::string&);

    Result_t WriteFrame(const ASDCP::FrameBuffer&, ASDCP::AESEncContext*, ASDCP::HMACContext*);
    Result_t WriteMetadata(const std::string &trackLabel, const std::string &mimeType, const std::string &dataDescription, const ASDCP::FrameBuffer& buffer);
    Result_t Finalize();
};


// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t AS_02::IAB::MXFWriter::h__Writer::OpenWrite(const std::string &filename, ui32_t HeaderSize, const IABDescriptor& ADesc)
{
    if (!m_State.Test_BEGIN())
    {
        return RESULT_STATE;
    }

    Result_t result = m_File.OpenWrite(filename);

    if (KM_SUCCESS(result))
    {
        m_HeaderSize = HeaderSize;

        IABEssenceDescriptor *desc = new IABEssenceDescriptor(m_Dict);

        desc->SampleRate = ADesc.FrameRate;
        desc->AudioSamplingRate = ADesc.SampleRate;
        desc->ChannelCount = 0;
        desc->QuantizationBits = ADesc.BitDepth;
        desc->SoundEssenceCoding = UL(IAB_ESSENCE_CODING);
        if (!ADesc.ImageFrameRate.empty())
            desc->ReferenceImageEditRate = ADesc.ImageFrameRate.get(); 
        if (!ADesc.AudioAlignmentLevel.empty())
            desc->ReferenceAudioAlignmentLevel = ADesc.AudioAlignmentLevel.get();
        
        m_EssenceDescriptor = desc;

        IABSoundfieldLabelSubDescriptor *subDesc = new IABSoundfieldLabelSubDescriptor(m_Dict);

        GenRandomValue(subDesc->MCALinkID);
        subDesc->MCATagName = "IAB";
        subDesc->MCALabelDictionaryID = m_Dict->ul(MDD_IABSoundfield);
        subDesc->MCATagSymbol = "IAB";
        if (!ADesc.AudioContentKind.empty())
        {
            subDesc->MCAAudioContentKind.set(ADesc.AudioContentKind.get());
        }
        if (!ADesc.AudioElementKind.empty())
        {
            subDesc->MCAAudioElementKind.set(ADesc.AudioElementKind.get());
        }
        if (!ADesc.Title.empty())
        {
            subDesc->MCATitle.set(ADesc.Title.get());
        }
        if (!ADesc.TitleVersion.empty())
        {
            subDesc->MCATitleVersion.set(ADesc.TitleVersion.get());
        }
        if (!ADesc.RFC5646SpokenLanguage.empty())
        {
            subDesc->RFC5646SpokenLanguage.set(ADesc.RFC5646SpokenLanguage);
        }
        
        m_EssenceSubDescriptorList.push_back(subDesc);
        GenRandomValue(subDesc->InstanceUID);
        m_EssenceDescriptor->SubDescriptors.push_back(subDesc->InstanceUID);


        result = m_State.Goto_INIT();
    }
    return result;
}

Result_t AS_02::IAB::MXFWriter::h__Writer::SetSourceStream(IABDescriptor const& ADesc,
    const byte_t * essenceCoding,
    const std::string& packageLabel,
    const std::string& trackName)
{
    if (!m_State.Test_INIT())
    {
        return RESULT_STATE;
    }

    if (   ADesc.FrameRate != EditRate_23_98
        && ADesc.FrameRate != EditRate_24
        && ADesc.FrameRate != EditRate_25
        && ADesc.FrameRate != EditRate_30
        && ADesc.FrameRate != EditRate_48
        && ADesc.FrameRate != EditRate_50
        && ADesc.FrameRate != EditRate_60
        && ADesc.FrameRate != EditRate_96
        && ADesc.FrameRate != EditRate_100
        && ADesc.FrameRate != EditRate_120)
    {
        DefaultLogSink().Error("IABEssecenDescriptor.EditRate is not a supported value: %d/%d\n",
            ADesc.FrameRate.Numerator, ADesc.FrameRate.Denominator);
        return RESULT_RAW_FORMAT;
    }

    assert(m_Dict);

    memcpy(m_EssenceUL, m_Dict->ul(MDD_IMF_IABEssenceClipWrappedElement), SMPTE_UL_LENGTH); 
    m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
    Result_t result = m_State.Goto_READY();

    if ( ASDCP_SUCCESS(result))
    {
        ui32_t TCFrameRate = (ui32_t)ceil(ADesc.FrameRate.Quotient());

        // timecode-in-imf best practices requires that we do not use timecodes in IAB track files
        SetUseTimecodes(0);

        result = WriteAS02Header(
            packageLabel, 
            UL(m_Dict->ul(MDD_IMF_IABEssenceClipWrappedContainer)), //  Wrapping
            trackName, 
            UL(m_EssenceUL), // Essence UL
            UL(m_Dict->ul(MDD_SoundDataDef)), // DataDefinition
            ADesc.FrameRate, 
            TCFrameRate);
    }
    if (KM_SUCCESS(result))
    {
        // even though the header was already written out, it will be 
        // written out again at the end when footer is written
        Batch<UL> specs;
        specs.push_back(UL(m_Dict->ul(MDD_IMF_IABTrackFileLevel0)));
        m_HeaderPart.m_Preface->ConformsToSpecifications.set(specs);
    }

    m_IndexWriter.SetEditRate(ADesc.FrameRate);

    return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
Result_t AS_02::IAB::MXFWriter::h__Writer::WriteFrame(const ASDCP::FrameBuffer& frame_buf,
    AESEncContext* Ctx, HMACContext* HMAC)
{
    if (frame_buf.Size() == 0)
    {
        DefaultLogSink().Error("The frame buffer size is zero.\n");
        return RESULT_PARAM;
    }

    Result_t result = RESULT_OK;

    if (m_State.Test_READY())
    {
        result = m_State.Goto_RUNNING(); // first time through
    }

    

    if (KM_SUCCESS(result) && ! HasOpenClip())
    {
        ui64_t position = m_File.Tell();
        result = StartClip(m_EssenceUL, Ctx, HMAC);
        m_StartPayloadOffset = ui32_t(m_File.Tell() - position);
    }

    if (KM_SUCCESS(result))
    {
        result = WriteClipBlock(frame_buf);
    }

    if ( ASDCP_SUCCESS(result) )
    {
        IndexTableSegment::IndexEntry Entry;
        Entry.StreamOffset = m_TotalBytesWritten + m_StartPayloadOffset;

        m_IndexWriter.PushIndexEntry(Entry);

        m_FramesWritten ++;
        m_TotalBytesWritten += frame_buf.Size();
    }

    return result;
}

Result_t AS_02::IAB::MXFWriter::h__Writer::WriteMetadata(const std::string &trackLabel, const std::string &mimeType, const std::string &dataDescription, const ASDCP::FrameBuffer& metadata_buf)
{
    // In this section add Descriptive Metadata elements to the Header
    //

    m_HeaderPart.m_Preface->DMSchemes.push_back(UL(m_Dict->ul(MDD_MXFTextBasedFramework))); // See section 7.1 Table 3 ST RP 2057

    // DM Static Track and Static Track are the same
    StaticTrack* NewTrack = new StaticTrack(m_Dict);
    m_HeaderPart.AddChildObject(NewTrack);
    m_FilePackage->Tracks.push_back(NewTrack->InstanceUID);
    NewTrack->TrackName = trackLabel;
    NewTrack->TrackID = m_NextTrackID++;
    
    Sequence* Seq = new Sequence(m_Dict);
    m_HeaderPart.AddChildObject(Seq);
    NewTrack->Sequence = Seq->InstanceUID;
    Seq->DataDefinition = UL(m_Dict->ul(MDD_DescriptiveMetaDataDef));
    Seq->Duration.set_has_value();
    m_DurationUpdateList.push_back(&Seq->Duration.get());

    DMSegment* Segment = new DMSegment(m_Dict);
    m_HeaderPart.AddChildObject(Segment);
    Seq->StructuralComponents.push_back(Segment->InstanceUID);
    Segment->EventComment = "SMPTE RP 2057 Generic Stream Text-Based Set";
    Segment->DataDefinition = UL(m_Dict->ul(MDD_DescriptiveMetaDataDef));
    if (!Segment->Duration.empty())
    {
        m_DurationUpdateList.push_back(&Segment->Duration.get());
    }

    TextBasedDMFramework* framework = new TextBasedDMFramework(m_Dict);
    m_HeaderPart.AddChildObject(framework);
    Segment->DMFramework = framework->InstanceUID;

    GenericStreamTextBasedSet *set = new GenericStreamTextBasedSet(m_Dict);
    m_HeaderPart.AddChildObject(set);
    framework->ObjectRef = set->InstanceUID;
    
    set->TextDataDescription = dataDescription;
    set->PayloadSchemeID = UL(m_Dict->ul(MDD_MXFTextBasedFramework));
    set->TextMIMEMediaType = mimeType;
    set->RFC5646TextLanguageCode = "en";
    set->GenericStreamSID = m_GenericStreamID;

    // before we set up a new partition
    // make sure we write out the Body partition index
    Result_t result = FlushIndexPartition();
    if (!KM_SUCCESS(result))
        return result;

    //
    // This section sets up the Generic Streaming Partition where we are storing the text-based metadata
    //
    Kumu::fpos_t here = m_File.Tell();

    // create generic stream partition header
    static UL GenericStream_DataElement(m_Dict->ul(MDD_GenericStream_DataElement));
    ASDCP::MXF::Partition GSPart(m_Dict);

    GSPart.MajorVersion = m_HeaderPart.MajorVersion;
    GSPart.MinorVersion = m_HeaderPart.MinorVersion;
    GSPart.ThisPartition = here;
    GSPart.PreviousPartition = m_RIP.PairArray.back().ByteOffset;
    GSPart.OperationalPattern = m_HeaderPart.OperationalPattern;
    GSPart.BodySID = m_GenericStreamID++; 

    m_RIP.PairArray.push_back(RIP::PartitionPair(GSPart.BodySID, here));
    GSPart.EssenceContainers = m_HeaderPart.EssenceContainers;

    static UL gs_part_ul(m_Dict->ul(MDD_GenericStreamPartition));
    result = GSPart.WriteToFile(m_File, gs_part_ul);

    if ( KM_SUCCESS(result) )
    {
        result = Write_EKLV_Packet(m_File, *m_Dict, m_HeaderPart, m_Info, m_CtFrameBuf, m_FramesWritten,
            m_StreamOffset, metadata_buf, GenericStream_DataElement.Value(), MXF_BER_LENGTH, 0, 0);
    }
    return result;
}


// Closes the MXF file, writing the index and other closing information.
//
Result_t AS_02::IAB::MXFWriter::h__Writer::Finalize()
{
    if (!m_State.Test_RUNNING())
    {
        return RESULT_STATE;
    }

    m_State.Goto_FINAL();

    Result_t result = FinalizeClip(m_TotalBytesWritten);

    if (KM_SUCCESS(result))
    {
        m_EssenceDescriptor->ContainerDuration = m_FramesWritten;
        result = WriteAS02Footer();
    }

    return result;
}

//------------------------------------------------------------------------------------------



AS_02::IAB::MXFWriter::MXFWriter()
{
}

AS_02::IAB::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader& AS_02::IAB::MXFWriter::OP1aHeader()
{
    if ( m_Writer.empty())
    {
        assert(g_OP1aHeader);
        return *g_OP1aHeader;
    }

    return m_Writer->m_HeaderPart;
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::RIP& AS_02::IAB::MXFWriter::RIP()
{
    if ( m_Writer.empty())
    {
        assert(g_RIP);
        return *g_RIP;
    }

    return m_Writer->m_RIP;
}

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
Result_t AS_02::IAB::MXFWriter::OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
				       const IABDescriptor& ADesc, ui32_t HeaderSize)
{
    if ( Info.LabelSetType != LS_MXF_SMPTE )
    {
        DefaultLogSink().Error("IAB support requires LS_MXF_SMPTE\n");
        return RESULT_FORMAT;
    }

    m_Writer = new h__Writer(DefaultSMPTEDict());
    m_Writer->m_Info = Info;

    Result_t result = m_Writer->OpenWrite(filename, HeaderSize, ADesc);

    if ( ASDCP_SUCCESS(result))
    {
        result = m_Writer->SetSourceStream(ADesc, IAB_ESSENCE_CODING, IAB_PACKAGE_LABEL, IAB_DEF_LABEL);
    }

    if (ASDCP_FAILURE(result))
    {
        m_Writer.release();
    }

    return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
Result_t AS_02::IAB::MXFWriter::WriteFrame(const ASDCP::FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
    if ( m_Writer.empty())
        return RESULT_INIT;

    return m_Writer->WriteFrame(FrameBuf, Ctx, HMAC);
}

Result_t AS_02::IAB::MXFWriter::WriteMetadata(const std::string &trackLabel, const std::string &mimeType, const std::string &dataDescription, const ASDCP::FrameBuffer& buffer)
{
    if ( m_Writer.empty())
        return RESULT_INIT;

    return m_Writer->WriteMetadata(trackLabel, mimeType, dataDescription, buffer);
}
// Closes the MXF file, writing the index and other closing information.
Result_t AS_02::IAB::MXFWriter::Finalize()
{
    if ( m_Writer.empty())
        return RESULT_INIT;

    return m_Writer->Finalize();
}
