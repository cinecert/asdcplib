/*
Copyright (c) 2004-2013, John Hurst
Copyright (c) 2020, Thomas Richter Fraunhofer IIS,
Copyright (c) 2020, Christian Minuth Fraunhofer IIS,
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
/*! \file    JXS_Codestream_Parser.cpp
	\version $Id$
	\brief   AS-DCP library, JPEG XS codestream essence reader implementation
*/

#include <KM_fileio.h>
#include <JXS.h>
#include <assert.h>
#include <KM_log.h>
using Kumu::DefaultLogSink;

//------------------------------------------------------------------------------------------

class ASDCP::JXS::CodestreamParser::h__CodestreamParser
{
	ASDCP_NO_COPY_CONSTRUCT(h__CodestreamParser);

public:
	ASDCP::MXF::GenericPictureEssenceDescriptor m_PDesc;
	ASDCP::MXF::JPEGXSPictureSubDescriptor m_JxsSubdesc;
	Kumu::FileReader   m_File;

	h__CodestreamParser() 
          : m_PDesc(&DefaultSMPTEDict()), m_JxsSubdesc(&DefaultSMPTEDict()) {}

	~h__CodestreamParser() {}

	Result_t OpenReadFrame(const std::string& filename, FrameBuffer& FB)
	{
		m_File.Close();
		Result_t result = m_File.OpenRead(filename);

		if (ASDCP_SUCCESS(result))
		{
			Kumu::fsize_t file_size = m_File.Size();

			if (FB.Capacity() < file_size)
			{
				DefaultLogSink().Error("FrameBuf.Capacity: %u frame length: %u\n", FB.Capacity(), (ui32_t)file_size);
				return RESULT_SMALLBUF;
			}
		}

		ui32_t read_count;

		if (ASDCP_SUCCESS(result))
			result = m_File.Read(FB.Data(), FB.Capacity(), &read_count);

		if (ASDCP_SUCCESS(result))
			FB.Size(read_count);

		if (ASDCP_SUCCESS(result))
		{
			byte_t start_of_data = 0; // out param
			result = ParseMetadataIntoDesc(FB, m_PDesc, m_JxsSubdesc, &start_of_data);

			if (ASDCP_SUCCESS(result))
				FB.PlaintextOffset(start_of_data);
		}

		return result;
	}
};

ASDCP::Result_t
ASDCP::JXS::ParseMetadataIntoDesc(const FrameBuffer& FB, ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
		      ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_subdescriptor, byte_t* start_of_data)
{
	Result_t result = RESULT_OK;
	Marker NextMarker;
	ui32_t i;
	const byte_t* p = FB.RoData();
	const byte_t* end_p = p + FB.Size();

	/* initialize optional items */
	bool  pih = false;
	bool  havesoc = false;

        ImageComponent_t image_components[MaxComponents];

	while (p < end_p && ASDCP_SUCCESS(result))
	{
		result = GetNextMarker(&p, NextMarker);

		if (ASDCP_FAILURE(result))
		{
			result = RESULT_RAW_ESS;
			break;
		}

		switch (NextMarker.m_Type)
		{
		case MRK_SOC:
		{
			// This is the SOC symbol.
			if (havesoc)
				DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: found a duplicate SOC marker");
			havesoc = true;
		}
		break;
		case MRK_EOC:
		{
			DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: found an EOC before any actual picture data");
		}
		break;
		
		case MRK_PIH:
		{
			// This is the real picture header.

			if (!havesoc)
				DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: the SOC marker is missing at the start of the frame");
			if (pih) {
				DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: found a duplicate picture header");
			}
			else {
				ui32_t size;
				ui8_t flags;
				//
				
				Accessor::PIH PIH_(NextMarker);
				pih = true;
				size = PIH_.LpihSize();

				if (size != 28 - 2)
					DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: the jpeg xs picture header has an unsupported size");

				// size of the bitstream: ignore, just store away.
				size = PIH_.LcodSize();
				// Profile and level
				jxs_subdescriptor.JPEGXSPpih = PIH_.Ppih();
				jxs_subdescriptor.JPEGXSPlev = PIH_.Plev();
				// Width and Height
				picture_descriptor.AspectRatio = Rational(PIH_.Wf(), PIH_.Hf());
				picture_descriptor.StoredWidth = PIH_.Wf();
				picture_descriptor.StoredHeight = PIH_.Hf();
				jxs_subdescriptor.JPEGXSWf = PIH_.Wf();
				jxs_subdescriptor.JPEGXSHf = PIH_.Hf();
				jxs_subdescriptor.JPEGXSCw = PIH_.Cw();
				jxs_subdescriptor.JPEGXSHsl = PIH_.Hsl();
				if (PIH_.Hsl() < 1 || PIH_.Hsl() > 0xffff) // This includes the EOF check
					DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: unsupported slice height specified, must be > 0 and < 65536");
				// Number of compoennts.
				jxs_subdescriptor.JPEGXSNc = PIH_.Nc();
				if (jxs_subdescriptor.JPEGXSNc != 3)
				{
					DefaultLogSink().Error("Unexpected number of components: %u\n", jxs_subdescriptor.JPEGXSNc);
					return RESULT_RAW_FORMAT;
				}
				// A lot of settings that must be fixed right now.
				if (PIH_.Ng() != 4)
					DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: the number of coefficients per coding group must be 4");
				if (PIH_.Ss() != 8)
					DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: the number of coding groups per significance group must be 8");
				//
				if (PIH_.Nlx() == 0)
					DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: number of horizontal decomposition levels is out of range, must be >0");
				if (PIH_.Nly() > MaxVerticalLevels)
					DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: number of vertical decomposition levels is out of range, must be <2");
			}
		}
		break;
		case MRK_CDT:
		{
			if(pih) {
				Accessor::CDT CDT_(NextMarker);
				int i, count = NextMarker.m_DataSize >> 1;

				for (i = 0; i < count && i < jxs_subdescriptor.JPEGXSNc; i++) {
					image_components[i].Bc = CDT_.Bc(i);
					image_components[i].Sx = CDT_.Sx(i);
					image_components[i].Sy = CDT_.Sy(i);
				}
		}
		else {
				DefaultLogSink().Error("JXS::ParseMetadataIntoDesc: found a component table marker upfront the picture header");
		}
		}
		break;
		case MRK_SLH: /* slice header: the entropy coded data starts here */
			if (start_of_data != 0)
				*start_of_data = p - FB.RoData();

			p = end_p;
			break;
		}
	}

	const ui32_t cdt_buffer_len = 8 * 2; // at most 8 components.
	byte_t tmp_buffer[cdt_buffer_len];
	int comps = (jxs_subdescriptor.JPEGXSNc > 8)?8:jxs_subdescriptor.JPEGXSNc;
	jxs_subdescriptor.JPEGXSComponentTable.Length(4 + (comps << 1));

	// thor: unclear whether the marker size is part of this data.
	tmp_buffer[0] = 0xff;
	tmp_buffer[1] = 0x13; // the marker 
	tmp_buffer[2] = 0x00;
	tmp_buffer[3] = comps * 2 + 2; // The size.
	for(int j = 0;j < comps;j++) {
	    tmp_buffer[4 + (j << 1)] = image_components[j].Bc;
	    tmp_buffer[5 + (j << 1)] = (image_components[j].Sx << 4) | (image_components[j].Sy);
	}

	memcpy(jxs_subdescriptor.JPEGXSComponentTable.Data(), tmp_buffer, 4 + (comps << 1));
	return result;
}

//------------------------------------------------------------------------------------------

ASDCP::JXS::CodestreamParser::CodestreamParser()
{
}

ASDCP::JXS::CodestreamParser::~CodestreamParser()
{
}

// Opens the stream for reading, parses enough data to provide a complete
// set of stream metadata for the MXFWriter below.
ASDCP::Result_t
ASDCP::JXS::CodestreamParser::OpenReadFrame(const std::string& filename, FrameBuffer& FB) const
{
	const_cast<ASDCP::JXS::CodestreamParser*>(this)->m_Parser = new h__CodestreamParser;
	return m_Parser->OpenReadFrame(filename, FB);
}

//
ASDCP::Result_t
ASDCP::JXS::CodestreamParser::FillPictureDescriptor(
    ASDCP::MXF::GenericPictureEssenceDescriptor& picture_descriptor,
    ASDCP::MXF::JPEGXSPictureSubDescriptor& jxs_subdescriptor) const
{
	if (m_Parser.empty())
		return RESULT_INIT;

	picture_descriptor = m_Parser->m_PDesc;
	jxs_subdescriptor = m_Parser->m_JxsSubdesc;
	return RESULT_OK;
}

//
// end JXS_Codestream_Parser.cpp
//
