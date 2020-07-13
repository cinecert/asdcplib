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

/*! \file    AS_02_IAB.h
    \brief   AS-02 library, IAB essence reader and writer implementation (ST 2067-201)
*/


#ifndef _AS_02_ATMOS_H_
#define _AS_02_ATMOS_H_

#include "AS_02_namespace.h"
#include "AS_DCP.h"
#include "KM_error.h"

ASDCP_NAMESPACE_BEGIN

namespace MXF {
    // #include<Metadata.h> to use these
    class OP1aHeader;
    class RIP;
}

ASDCP_NAMESPACE_END


AS_02_NAMESPACE_BEGIN

namespace MXF
{
    class AS02IndexReader;
}



namespace IAB
{ 
    template<typename T>
    class optional
    {
    private:
        bool m_has_value;
        T    m_value;
    public:
        optional() : m_has_value(false) {}
        optional(const T& value) : m_has_value(true), m_value(value) {}

        const T& get() const { return m_value; }
        void set(const T& v) { m_value = v; m_has_value = true; }
        bool empty() const { return !m_has_value; }
    };

    struct IABDescriptor
    {
    public:
        IABDescriptor() : FrameRate(24, 1), SampleRate(48000, 1), BitDepth(24), ContainerDuration(0)
        {
        }
        ASDCP::Rational FrameRate; 
        ASDCP::Rational SampleRate;
        ui32_t          BitDepth;
        ui32_t          ContainerDuration; // set on read; does not need to be set on create/write
        optional<ASDCP::Rational> ImageFrameRate; 
        optional<ui8_t>           AudioAlignmentLevel;
        std::string               RFC5646SpokenLanguage;
        optional<std::string>     AudioContentKind;
        optional<std::string>     AudioElementKind;
        optional<std::string>     Title;
        optional<std::string>     TitleVersion;
    };

     void   DescriptorDump(const IABDescriptor&, FILE* = 0);
     std::ostream& operator << (std::ostream& strm, const IABDescriptor&);

    //
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
        Kumu::Result_t OpenWrite(const std::string& filename, const ASDCP::WriterInfo& Info,
				       const IABDescriptor& Desc, ui32_t HeaderSize = 16384);

        // Writes a frame of essence to the MXF file. If the optional AESEncContext
        // argument is present, the essence is encrypted prior to writing.
        // Fails if the file is not open, is finalized, or an operating system
        // error occurs.
        Kumu::Result_t WriteFrame(const ASDCP_NAMESPACE::FrameBuffer&, ASDCP::AESEncContext* = 0, ASDCP::HMACContext* = 0);

        // Writes ST RP 2057 text-based program-level metadata stored in a Generic Stream Partition
        // trackLabel provides description of track content
        // mimeType identifies the data type of the text: https://www.iana.org/assignments/media-types/media-types.xhtml
        // dataDescription can refer to the XML namespace identifying the text metadata passed in as argument
        Kumu::Result_t WriteMetadata(const std::string &trackLabel, const std::string &mimeType, const std::string &dataDescription, const ASDCP::FrameBuffer&);

        // Closes the MXF file, writing the final index, the PHDR master metadata and the revised header.
        Kumu::Result_t Finalize();
    };

    //
    class MXFReader
    {
        class h__Reader;
        ASDCP::mem_ptr<h__Reader> m_Reader;
        ASDCP_NO_COPY_CONSTRUCT(MXFReader);

    public:
        MXFReader();
        virtual ~MXFReader();

        // Warning: direct manipulation of MXF structures can interfere
        // with the normal operation of the wrapper.  Caveat emptor!
        virtual ASDCP::MXF::OP1aHeader& OP1aHeader();
        virtual AS_02::MXF::AS02IndexReader& AS02IndexReader();
        virtual ASDCP::MXF::RIP& RIP();

        // Open the file for reading. The file must exist. Returns error if the
        // operation cannot be completed. If master metadata is available it will
        // be placed into the string object passed as the second argument.
        Kumu::Result_t OpenRead(const std::string& filename) const;

        // Returns RESULT_INIT if the file is not open.
        Kumu::Result_t Close() const;

        // Fill an IABDescriptor struct with the values from the file's header.
        // Returns RESULT_INIT if the file is not open.
        Kumu::Result_t FillDescriptor(IABDescriptor&) const;

        // Fill a WriterInfo struct with the values from the file's header.
        // Returns RESULT_INIT if the file is not open.
        Kumu::Result_t FillWriterInfo(ASDCP::WriterInfo&) const;

        // Reads a frame of essence from the MXF file. If the optional AESEncContext
        // argument is present, the essence is decrypted after reading. If the MXF
        // file is encrypted and the AESDecContext argument is NULL, the frame buffer
        // will contain the ciphertext frame data. If the HMACContext argument is
        // not NULL, the HMAC will be calculated (if the file supports it).
        // Returns RESULT_INIT if the file is not open, failure if the frame number is
        // out of range, or if optional decrypt or HAMC operations fail.
        Kumu::Result_t ReadFrame(ui32_t frame_number, ASDCP_NAMESPACE::FrameBuffer&, ASDCP::AESDecContext* = 0, ASDCP::HMACContext* = 0) const;

        // Reads ST RP 2057 text-based program-level metadata stored in a Generic Stream Partition
        // mimeType identifies the data type of the text: https://www.iana.org/assignments/media-types/media-types.xhtml
        // dataDescription can refer to the XML namespace
        Kumu::Result_t ReadMetadata(const std::string &dataDescription, std::string &mimeType, ASDCP::FrameBuffer&);


        // Print debugging information to stream
        void     DumpHeaderMetadata(FILE* = 0) const;
        void     DumpIndex(FILE* = 0) const;
    };

} // end namespace ATMOS


AS_02_NAMESPACE_END

#endif // _AS_02_ATMOS_H_
