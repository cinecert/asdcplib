/*
Copyright (c) 2004-2013, John Hurst,
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
/* !\file    JXS.h
\version $Id$
\brief   JPEG XS parser implementation

This is not a complete enumeration of all things JXS.There is just enough here to
support parsing picture metadata from a codestream header.
*/

#include "JXS.h"
#include <KM_log.h>
using Kumu::DefaultLogSink;

//
ASDCP::Result_t
ASDCP::JXS::GetNextMarker(const byte_t** buf, JXS::Marker& Marker)
{
	assert((buf != 0) && (*buf != 0));

	if (*(*buf)++ != 0xff)
		return ASDCP::RESULT_FAIL;

	Marker.m_Type = (Marker_t)(0xff00 | *(*buf)++);
	Marker.m_IsSegment = Marker.m_Type != MRK_SOC && Marker.m_Type != MRK_SLH && Marker.m_Type != MRK_EOC;

	if (Marker.m_IsSegment)
	{
		Marker.m_DataSize = *(*buf)++ << 8;
		Marker.m_DataSize |= *(*buf)++;
		Marker.m_DataSize -= 2;
		Marker.m_Data = *buf;
		*buf += Marker.m_DataSize;
	}

	return ASDCP::RESULT_OK;
}

//
const char*
ASDCP::JXS::GetMarkerString(Marker_t m)
{
	switch (m)
	{
	case MRK_NIL: return "NIL"; break;
	case MRK_SOC: return "SOC: Start of codestream"; break;
	case MRK_EOC: return "SOT: End of codestream"; break;
	case MRK_PIH: return "PIH: Picture header"; break;
	case MRK_CDT: return "CDT: Component table"; break;
	case MRK_WGT: return "WGT: Weights table"; break;
	case MRK_COM: return "COM: Extension marker"; break;
	case MRK_NLT: return "NLT: Nonlinearity marker"; break;
	case MRK_CWD: return "CWD: Component dependent wavelet decomposition marker"; break;
	case MRK_CTS: return "CTS: Colour transformation specification marker"; break;
	case MRK_CRG: return "CRG: Component registration marker"; break;
	case MRK_SLH: return "SLH: Slice header"; break;
	case MRK_CAP: return "CAP: Capabilities marker"; break;
	}

	return "Unknown marker code";
}

//TODO: Dump functions for markers

//
void
ASDCP::JXS::Accessor::CAP::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "CAP: \n");
	fprintf(stream, "  Size: %hu\n", Size());
	Kumu::hexdump(m_MarkerData, m_DataSize, stream);
}

//
void
ASDCP::JXS::Accessor::NLT::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "SLZ: \n");
	fprintf(stream, "  Size: %hu\n", Size());
}

//
void
ASDCP::JXS::Accessor::PIH::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "PIH: \n");
	fprintf(stream, "  LpihSize: %hu\n", LpihSize());
	fprintf(stream, "  LcodSize: %hu\n", LcodSize());
	fprintf(stream, "  Ppih: %hu\n", Ppih());
	fprintf(stream, "  Plev: %hu\n", Plev());
	fprintf(stream, "  Wf: %hu\n", Wf());
	fprintf(stream, "  Hf: %hu\n", Hf());
	fprintf(stream, "  Cw: %hu\n", Cw());
	fprintf(stream, "  Hsl: %hu\n", Hsl());
	fprintf(stream, "  Nc: %hu\n", Nc());
	fprintf(stream, "  Ng: %hu\n", Ng());
	fprintf(stream, "  Ss: %hu\n", Ss());
	fprintf(stream, "  Cpih: %hu\n", Cpih());
	fprintf(stream, "  Nlx: %hu\n", Nlx());
	fprintf(stream, "  Nly: %hu\n", Nly());
	Kumu::hexdump(m_MarkerData, m_DataSize, stream);
}

//
void
ASDCP::JXS::Accessor::CDT::Dump(FILE* stream) const
{
	if (stream == 0)
		stream = stderr;

	fprintf(stream, "CDT: \n");

	for (ui32_t i = 0; i < 3; i++) {
	  fprintf(stream, " Component %u Bc: %hu\n", i,Bc(i));
	  fprintf(stream, " Component %u Sx: %hu\n", i,Sx(i));
	  fprintf(stream, " Component %u Sy: %hu\n", i,Sy(i));
	}
}
//
// end JXS.cpp
//
