/*
Copyright (c) 2004-2016, John Hurst,
Copyright (c) 2020, Thomas Richter Fraunhofer IIS
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
/*! \file    AS_DCP_JXS.cpp
    \version $Id$
    \brief   AS-DCP library, JPEG XS essence reader and writer implementation
*/

#include "AS_DCP_internal.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace ASDCP::JXS;
using Kumu::GenRandomValue;

//------------------------------------------------------------------------------------------

static std::string JXS_PACKAGE_LABEL = "File Package: SMPTE 2124 frame wrapping of JPEG XS codestreams";
//static std::string JP2K_S_PACKAGE_LABEL = "File Package: SMPTE 429-10 frame wrapping of stereoscopic JPEG XS codestreams";
static std::string PICT_DEF_LABEL = "Picture Track";

static int s_exp_lookup[16] = { 0, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024,2048, 4096, 8192, 16384, 32768 };

//
std::ostream&
ASDCP::JXS::operator << (std::ostream& strm, const PictureDescriptor& PDesc)
{
  strm << "       AspectRatio: " << PDesc.AspectRatio.Numerator << "/" << PDesc.AspectRatio.Denominator << std::endl;
  strm << "          EditRate: " << PDesc.EditRate.Numerator << "/" << PDesc.EditRate.Denominator << std::endl;
  strm << "        SampleRate: " << PDesc.SampleRate.Numerator << "/" << PDesc.SampleRate.Denominator << std::endl;
  strm << "       StoredWidth: " << (unsigned) PDesc.StoredWidth << std::endl;
  strm << "      StoredHeight: " << (unsigned) PDesc.StoredHeight << std::endl;
  strm << "                Wf: " << (unsigned) PDesc.Wf << std::endl; // width of the frame
  strm << "                Hf: " << (unsigned) PDesc.Hf << std::endl; // height of the frame
  strm << " ContainerDuration: " << (unsigned) PDesc.ContainerDuration << std::endl;

  strm << "-- JPEG XS Metadata --" << std::endl;
  strm << "    ImageComponents:" << std::endl;
  strm << "  bits  h-sep v-sep" << std::endl;

  ui32_t i;
  for ( i = 0; i < PDesc.Nc && i < MaxComponents; ++i )
    {
      strm << "  " << std::setw(4) << PDesc.ImageComponents[i].Bc
	   << "  " << std::setw(5) << PDesc.ImageComponents[i].Sx
	   << " " << std::setw(5) << PDesc.ImageComponents[i].Sy
	   << std::endl;
    }

  strm << "       Slice height: " << (short) PDesc.Hsl << std::endl;
  strm << "            Profile: " << (short) PDesc.Ppih << std::endl;
  strm << "              Level: " << (short) (PDesc.Plev >> 8) << std::endl;
  strm << "           Sublevel: " << (short) (PDesc.Plev & 0xff) << std::endl;
  strm << "       Column Width: " << (short) (PDesc.Cw) << std::endl;
  strm << "   Maximum Bit Rate: " << (PDesc.MaximumBitRate) << std::endl;
  strm << "          Primaries: " << (short) (PDesc.Primaries) << std::endl;
  strm << "     Transfer Curve: " << (short) (PDesc.TransferCurve) << std::endl;
  strm << "             Matrix: " << (short) (PDesc.Matrix) << std::endl;
  strm << "         full range: " << (PDesc.fullRange?("yes"):("no")) << std::endl;
  /*
  ** thor: at this point, do not print the CAP marker
  */

  return strm;
}

//
void
ASDCP::JXS::PictureDescriptorDump(const PictureDescriptor& PDesc, FILE* stream)
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "\
       AspectRatio: %d/%d\n\
          EditRate: %d/%d\n\
        SampleRate: %d/%d\n\
       StoredWidth: %u\n\
      StoredHeight: %u\n\
                Wf: %u\n\
                Hf: %u\n\
           Profile: %u\n\
             Level: %u\n\
          Sublevel: %u\n\
   Maximum BitRate: %u\n\
 ContainerDuration: %u\n\
         Primaries: %u\n\
    Transfer Curve: %u\n\
            Matrix: %u\n\
        full range: %s\n",
	  PDesc.AspectRatio.Numerator, PDesc.AspectRatio.Denominator,
	  PDesc.EditRate.Numerator, PDesc.EditRate.Denominator,
	  PDesc.SampleRate.Numerator, PDesc.SampleRate.Denominator,
	  PDesc.StoredWidth,
	  PDesc.StoredHeight,
	  PDesc.Wf,
	  PDesc.Hf,
	  PDesc.Ppih,
	  PDesc.Plev >> 4,
	  PDesc.Plev & 0x0f,
	  PDesc.MaximumBitRate,
	  PDesc.ContainerDuration,
	  PDesc.Primaries,
	  PDesc.TransferCurve,
	  PDesc.Matrix,
	  PDesc.fullRange?("yes"):("no")
	  );

  fprintf(stream, "-- JPEG XS Metadata --\n");
  fprintf(stream, "    ImageComponents:\n");
  fprintf(stream, "  bits  h-sep v-sep\n");

  ui32_t i;
  for ( i = 0; i < PDesc.Nc && i < MaxComponents; i++ )
    {
      fprintf(stream, "  %4d  %5d %5d\n",
	      PDesc.ImageComponents[i].Bc,
	      PDesc.ImageComponents[i].Sx,
	      PDesc.ImageComponents[i].Sy
	      );
    }  
}

//
ASDCP::Result_t
ASDCP::JXS_PDesc_to_MD(const JXS::PictureDescriptor& PDesc,
		       const ASDCP::Dictionary&dict,
		       ASDCP::MXF::GenericPictureEssenceDescriptor& EssenceDescriptor,
		       ASDCP::MXF::JPEGXSPictureSubDescriptor& EssenceSubDescriptor)
{
  EssenceDescriptor.ContainerDuration = PDesc.ContainerDuration;
  EssenceDescriptor.SampleRate = PDesc.EditRate;
  EssenceDescriptor.FrameLayout = 0;
  EssenceDescriptor.StoredWidth = PDesc.StoredWidth;
  EssenceDescriptor.StoredHeight = PDesc.StoredHeight;
  EssenceDescriptor.AspectRatio = PDesc.AspectRatio;

  EssenceSubDescriptor.JPEGXSPpih = PDesc.Ppih;
  EssenceSubDescriptor.JPEGXSPlev = PDesc.Plev;
  EssenceSubDescriptor.JPEGXSWf = PDesc.Wf;
  EssenceSubDescriptor.JPEGXSHf = PDesc.Hf;
  EssenceSubDescriptor.JPEGXSNc = PDesc.Nc;

  // Copy the value of the columns, but only if there are some
  if (PDesc.Cw) {
    EssenceSubDescriptor.JPEGXSCw = optional_property<ui16_t>(PDesc.Cw);
  } else {
    EssenceSubDescriptor.JPEGXSCw.set_has_value(false);
  }

  // Copy the slice height. Actually, this is optional
  // and does not necessarily require copying all the time,
  // but let's copy it nevertheless.
  EssenceSubDescriptor.JPEGXSHsl = optional_property<ui16_t>(PDesc.Hsl);

  if (PDesc.MaximumBitRate) {
    EssenceSubDescriptor.JPEGXSMaximumBitRate = PDesc.MaximumBitRate;
  } else {
    EssenceSubDescriptor.JPEGXSMaximumBitRate.set_has_value(false);
  }

  const ui32_t cdt_buffer_len = 8 * 2; // at most 8 components.
  byte_t tmp_buffer[cdt_buffer_len];
  int i,comps = (PDesc.Nc > 8)?8:PDesc.Nc;
  EssenceSubDescriptor.JPEGXSComponentTable.Length(4 + (comps << 1));
  // thor: unclear whether the marker size is part of this data.
  tmp_buffer[0] = 0xff;
  tmp_buffer[1] = 0x13; // the marker 
  tmp_buffer[2] = 0x00;
  tmp_buffer[3] = comps * 2 + 2; // The size.
  for(i = 0;i < comps;i++) {
    tmp_buffer[4 + (i << 1)] = PDesc.ImageComponents[i].Bc;
    tmp_buffer[5 + (i << 1)] = (PDesc.ImageComponents[i].Sx << 4) | (PDesc.ImageComponents[i].Sy);
  }

  memcpy(EssenceSubDescriptor.JPEGXSComponentTable.Data(), tmp_buffer, 4 + (comps << 1));

  //
  switch(PDesc.Primaries) {
  case 1:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_ColorPrimaries_ITU709);
    break;
  case 5:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_ColorPrimaries_ITU470_PAL);
    break;
  case 6:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_ColorPrimaries_SMPTE170M);
    break;
  case 9:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_ColorPrimaries_ITU2020);
    break;
  case 10:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_ColorPrimaries_SMPTE_DCDM);
    break;
  case 11:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_TheatricalViewingEnvironment);
    break;
  case 12:
    EssenceDescriptor.ColorPrimaries = dict.ul(ASDCP::MDD_ColorPrimaries_P3D65);
    break;
  default:
    return RESULT_PARAM;
    break;
  }
  
  switch(PDesc.TransferCurve) {
  case 1:
  case 6:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_ITU709);
    break;
  case 5: // Display Gamma 2.8, BT.470-6 This does not seem to be supported
  case 9: // Log(100:1) range This does not seem to be supported
  case 10:// Log(100*Sqrt(10):1 range)
    return Kumu::RESULT_NOTIMPL;
    break;
  case 8:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_linear);
    break;
  case 11:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_IEC6196624_xvYCC);
    break;
  case 13:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_sRGB);
    break;
  case 14:
  case 15:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_ITU2020);
    break;
  case 16:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_SMPTEST2084);
    break;
  case 17:
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_ST428);
    break;
  case 18: // HLG
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_HLG);
    break;
  case 12: // Rec. BT.1361
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_BT1361);
    break;
  case 4: // Rec. BT.470
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_BT470);
    break;
  case 7: // SMPTE 240M
    EssenceDescriptor.TransferCharacteristic = dict.ul(ASDCP::MDD_TransferCharacteristic_ST240M);
    break;
  case 2: // Unspecified. This leaves the data intentionally undefined.
    EssenceDescriptor.TransferCharacteristic.set_has_value(false);
    break;
  default:
    return RESULT_PARAM;
    break;
  }
  //
  switch(PDesc.Matrix) {
  case 0: // Identity matrix. Use the BGR coding equations.
    EssenceDescriptor.CodingEquations = dict.ul(ASDCP::MDD_CodingEquations_BGR);
    break;
  case 4: // Title 47.
  case 10: // ITU 2020 constant luminance? Does not seem to be supported
  case 11: // SMPTE ST-2085
    return Kumu::RESULT_NOTIMPL;
  case 1:
    EssenceDescriptor.CodingEquations = dict.ul(ASDCP::MDD_CodingEquations_709);
    break;
    // Note: Matrix=2 does not set the optional parameter. This is intentional.
  case 5:
  case 6:
    EssenceDescriptor.CodingEquations = dict.ul(ASDCP::MDD_CodingEquations_601);
    break;
  case 9: // ITU 2020 non-constant luminance?
    EssenceDescriptor.CodingEquations = dict.ul(ASDCP::MDD_CodingEquations_Rec2020);
    break;
  case 2: // This is unspecified. The metadata item remains undefined on purpose.
    EssenceDescriptor.CodingEquations.set_has_value(false);
    break;
  case 7: // ST 240M
    EssenceDescriptor.CodingEquations = dict.ul(ASDCP::MDD_CodingEquations_ST240M);
    break;
  case 8: // YCgCo
    EssenceDescriptor.CodingEquations = dict.ul(ASDCP::MDD_CodingEquations_YCGCO);
    break;
  default:
    return RESULT_PARAM;
    break;
  }
  //
#if 0
  if (rgba) {
    byte_t layout[ASDCP::MXF::RGBAValueLength];
    if (m_bFullRange) {
      rgba->ComponentMaxRef = (1UL << m_ucPrecision) - 1;
      rgba->ComponentMinRef = 0;
    } else {
      rgba->ComponentMaxRef = 235 * (1UL << (m_ucPrecision - 8));
      rgba->ComponentMinRef = 16  * (1UL << (m_ucPrecision - 8));
    }
    layout[0] = 'R';
    layout[1] = m_ucPrecision;
    layout[2] = 'G';
    layout[3] = m_ucPrecision;
    layout[4] = 'B';
    layout[5] = m_ucPrecision;
    layout[6] = 0;
#endif
  return RESULT_OK;
}

//
ASDCP::Result_t
ASDCP::MD_to_JXS_PDesc(const ASDCP::MXF::GenericPictureEssenceDescriptor&  EssenceDescriptor,
	const ASDCP::MXF::JPEGXSPictureSubDescriptor& EssenceSubDescriptor,
	const ASDCP::Rational& EditRate, const ASDCP::Rational& SampleRate,
	ASDCP::JXS::PictureDescriptor& PDesc)
{
        const Dictionary *dict = &ASDCP::DefaultSMPTEDict();
	const ASDCP::MXF::RGBAEssenceDescriptor *rgba = dynamic_cast<const ASDCP::MXF::RGBAEssenceDescriptor *>(&EssenceDescriptor);
	const ASDCP::MXF::CDCIEssenceDescriptor *cdci = dynamic_cast<const ASDCP::MXF::CDCIEssenceDescriptor *>(&EssenceDescriptor);
        memset(&PDesc, 0, sizeof(PDesc));

	PDesc.EditRate = EditRate;
	PDesc.SampleRate = SampleRate;
	assert(EssenceDescriptor.ContainerDuration.const_get() <= 0xFFFFFFFFL);
	PDesc.ContainerDuration = static_cast<ui32_t>(EssenceDescriptor.ContainerDuration.const_get());
	PDesc.StoredWidth = EssenceDescriptor.StoredWidth;
	PDesc.StoredHeight = EssenceDescriptor.StoredHeight;
	PDesc.AspectRatio = EssenceDescriptor.AspectRatio;

	PDesc.Ppih = EssenceSubDescriptor.JPEGXSPpih;
	PDesc.Plev = EssenceSubDescriptor.JPEGXSPlev;
	PDesc.Wf = EssenceSubDescriptor.JPEGXSWf;
	PDesc.Hf = EssenceSubDescriptor.JPEGXSHf;
	PDesc.Nc = EssenceSubDescriptor.JPEGXSNc;

	if (EssenceDescriptor.ColorPrimaries.empty()) {
	  PDesc.Primaries = 1; // If not set, let us assume 709 primaries. Yuck!
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_ColorPrimaries_ITU709)) {
	  PDesc.Primaries = 1;
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_ColorPrimaries_ITU470_PAL)) {
	  PDesc.Primaries = 5;
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_ColorPrimaries_SMPTE170M)) {
	  PDesc.Primaries = 6;
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_ColorPrimaries_ITU2020)) {
	  PDesc.Primaries = 9;
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_ColorPrimaries_SMPTE_DCDM)) {
	  PDesc.Primaries = 10;
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_TheatricalViewingEnvironment)) {
	  PDesc.Primaries = 11;
	} else if (EssenceDescriptor.ColorPrimaries == dict->ul(ASDCP::MDD_ColorPrimaries_P3D65)) {
	  PDesc.Primaries = 12;
	} else {
	  PDesc.Primaries = 0;
	}
	
	if (EssenceDescriptor.TransferCharacteristic.empty()) {
	  PDesc.TransferCurve = 2; // Unspecified
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_ITU709)) {
	  PDesc.TransferCurve = 1;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_linear)) {
	  PDesc.TransferCurve = 8;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_IEC6196624_xvYCC)) {
	  PDesc.TransferCurve = 11;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_sRGB)) {
	  PDesc.TransferCurve = 13;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_SMPTEST2084)) {
	  PDesc.TransferCurve = 16;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_ST428)) {
	  PDesc.TransferCurve = 17;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_HLG)) {
	  PDesc.TransferCurve = 18;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_BT1361)) {
	  PDesc.TransferCurve = 12;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_BT470)) {
	  PDesc.TransferCurve = 4;
	} else if (EssenceDescriptor.TransferCharacteristic == dict->ul(ASDCP::MDD_TransferCharacteristic_ST240M)) {
	  PDesc.TransferCurve = 7;
	} else {
	  PDesc.TransferCurve = 0;
	}

	if (EssenceDescriptor.CodingEquations.empty()) {
	  PDesc.Matrix = 2;
	} else if (EssenceDescriptor.CodingEquations == dict->ul(ASDCP::MDD_CodingEquations_BGR)) {
	  PDesc.Matrix = 0;
	} else if (EssenceDescriptor.CodingEquations == dict->ul(ASDCP::MDD_CodingEquations_709)) {
	  PDesc.Matrix = 1;
	} else if (EssenceDescriptor.CodingEquations == dict->ul(ASDCP::MDD_CodingEquations_601)) {
	  PDesc.Matrix = 5;
	} else if (EssenceDescriptor.CodingEquations == dict->ul(ASDCP::MDD_CodingEquations_Rec2020)) {
	  PDesc.Matrix = 9;
	} else if (EssenceDescriptor.CodingEquations == dict->ul(ASDCP::MDD_CodingEquations_ST240M)) {
	  PDesc.Matrix = 7;
	} else if (EssenceDescriptor.CodingEquations == dict->ul(ASDCP::MDD_CodingEquations_YCGCO)) {
	  PDesc.Matrix = 8;
	} else {
	  PDesc.Matrix = 0;
	}

	if (EssenceSubDescriptor.JPEGXSCw.const_get()==0 || EssenceSubDescriptor.JPEGXSCw.const_get() == 0)
		PDesc.Cw = 0;
	else
		PDesc.Cw = static_cast<ui16_t>(EssenceSubDescriptor.JPEGXSCw.const_get());

	PDesc.Hsl = static_cast<ui16_t>(EssenceSubDescriptor.JPEGXSHsl.const_get());

	PDesc.MaximumBitRate = static_cast<ui32_t>(EssenceSubDescriptor.JPEGXSMaximumBitRate.const_get());
	
	// JPEGXSComponentTable
	ui32_t tmp_size = EssenceSubDescriptor.JPEGXSComponentTable.Length();

	if (tmp_size > 4 && (tmp_size & 1) == 0 && (PDesc.Nc << 1) + 4 == tmp_size) {
	  const byte_t *data = EssenceSubDescriptor.JPEGXSComponentTable.RoData() + 4;
	  for(int i = 0;i < PDesc.Nc;i++) {
	    PDesc.ImageComponents[i].Bc = data[0];
	    PDesc.ImageComponents[i].Sy = data[1] >> 4;
	    PDesc.ImageComponents[i].Sx = data[1] & 0x0f;
	    data += 2;
	  }
	} else {
	  return RESULT_FAIL;
	}

	if (rgba) {
	  if (rgba->ComponentMinRef.empty()) {
	    if (rgba->ComponentMaxRef.empty()) {
	      PDesc.fullRange = false;
	    } else if (rgba->ComponentMaxRef == (1UL << PDesc.ImageComponents[0].Bc) - 1) {
	      PDesc.fullRange = true;
	    } else {
	      PDesc.fullRange = false;
	    }
	  } else if (rgba->ComponentMinRef == 0) {
	    PDesc.fullRange = true;
	  } else {
	    PDesc.fullRange = false;
	  }
	} else if (cdci) {
	  if (cdci->BlackRefLevel.empty()) {
	    if (cdci->WhiteReflevel.empty()) {
	      PDesc.fullRange = false;
	    } else if (cdci->WhiteReflevel == (1UL << PDesc.ImageComponents[0].Bc) - 1) {
	      PDesc.fullRange = true;
	    } else {
	      PDesc.fullRange = false;
	    }
	  } else if (cdci->BlackRefLevel == 0) {
	    PDesc.fullRange = true;
	  } else {
	    PDesc.fullRange = false;
	  }
	} else {
	  PDesc.fullRange = false;
	}

	return RESULT_OK;
}


//------------------------------------------------------------------------------------------
//
// hidden, internal implementation of JPEG XS reader


class ih__Reader : public ASDCP::h__ASDCPReader
{
  RGBAEssenceDescriptor*        m_EssenceDescriptor;
  JPEGXSPictureSubDescriptor*   m_EssenceSubDescriptor;
  ASDCP::Rational               m_EditRate;
  ASDCP::Rational               m_SampleRate;
  EssenceType_t                 m_Format;

  ASDCP_NO_COPY_CONSTRUCT(ih__Reader);

public:
  PictureDescriptor m_PDesc;        // codestream parameter list

  ih__Reader(const Dictionary *d) :
    ASDCP::h__ASDCPReader(d), m_EssenceDescriptor(0), m_EssenceSubDescriptor(0), m_Format(ESS_UNKNOWN) {}

  virtual ~ih__Reader() {}

  Result_t    OpenRead(const std::string&, EssenceType_t);
  Result_t    ReadFrame(ui32_t, JXS::FrameBuffer&, AESDecContext*, HMACContext*);
  Result_t    CalcFrameBufferSize(ui64_t &size);
};


//
//
ASDCP::Result_t
ih__Reader::OpenRead(const std::string& filename, EssenceType_t type)
{
  Result_t result = OpenMXFRead(filename);

  if( ASDCP_SUCCESS(result) )
    {
      InterchangeObject* tmp_iobj = 0;
      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(RGBAEssenceDescriptor), &tmp_iobj);
      m_EssenceDescriptor = static_cast<RGBAEssenceDescriptor*>(tmp_iobj);

      if ( m_EssenceDescriptor == 0 )
	{
	  DefaultLogSink().Error("RGBAEssenceDescriptor object not found.\n");
	  return RESULT_FORMAT;
	}

      m_HeaderPart.GetMDObjectByType(OBJ_TYPE_ARGS(JPEGXSPictureSubDescriptor), &tmp_iobj);
      m_EssenceSubDescriptor = static_cast<JPEGXSPictureSubDescriptor*>(tmp_iobj);

      if ( m_EssenceSubDescriptor == 0 )
	{
	  m_EssenceDescriptor = 0;
	  DefaultLogSink().Error("JPEGXSPictureSubDescriptor object not found.\n");
	  return RESULT_FORMAT;
	}

      std::list<InterchangeObject*> ObjectList;
      m_HeaderPart.GetMDObjectsByType(OBJ_TYPE_ARGS(Track), ObjectList);

      if ( ObjectList.empty() )
	{
	  DefaultLogSink().Error("MXF Metadata contains no Track Sets.\n");
	  return RESULT_FORMAT;
	}

      m_EditRate = ((Track*)ObjectList.front())->EditRate;
      m_SampleRate = m_EssenceDescriptor->SampleRate;

      if ( type == ASDCP::ESS_JPEG_XS )
	{
	  if ( m_EditRate != m_SampleRate )
	    {
	      DefaultLogSink().Warn("EditRate and SampleRate do not match (%.03f, %.03f).\n",
				    m_EditRate.Quotient(), m_SampleRate.Quotient());
	      /*
	      ** thor: currently, I do not see support for stereoskopic content,
	      ** thus skip this at the time being.
	      if ( ( m_EditRate == EditRate_24 && m_SampleRate == EditRate_48 )
		   || ( m_EditRate == EditRate_25 && m_SampleRate == EditRate_50 )
		   || ( m_EditRate == EditRate_30 && m_SampleRate == EditRate_60 )
		   || ( m_EditRate == EditRate_48 && m_SampleRate == EditRate_96 )
		   || ( m_EditRate == EditRate_50 && m_SampleRate == EditRate_100 )
		   || ( m_EditRate == EditRate_60 && m_SampleRate == EditRate_120 )
		   || ( m_EditRate == EditRate_96 && m_SampleRate == EditRate_192 )
		   || ( m_EditRate == EditRate_100 && m_SampleRate == EditRate_200 )
		   || ( m_EditRate == EditRate_120 && m_SampleRate == EditRate_240 ) )
		{
		  DefaultLogSink().Debug("File may contain JPEG Interop stereoscopic images.\n");
		  return RESULT_SFORMAT;
		}
	      **
	      */

	      return RESULT_FORMAT;
	    }
	}
      /*
      ** thor: support for stereoskopic JPEG XS disabled as it may
      ** not yet be supported by a standard.
      else if ( type == ASDCP::ESS_JPEG_XS_S )
	{
	  if ( m_EditRate == EditRate_24 )
	    {
	      if ( m_SampleRate != EditRate_48 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 24/48 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_25 )
	    {
	      if ( m_SampleRate != EditRate_50 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 25/50 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_30 )
	    {
	      if ( m_SampleRate != EditRate_60 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 30/60 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_48 )
	    {
	      if ( m_SampleRate != EditRate_96 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 48/96 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_50 )
	    {
	      if ( m_SampleRate != EditRate_100 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 50/100 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_60 )
	    {
	      if ( m_SampleRate != EditRate_120 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 60/120 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_96 )
	    {
	      if ( m_SampleRate != EditRate_192 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 96/192 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_100 )
	    {
	      if ( m_SampleRate != EditRate_200 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 100/200 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else if ( m_EditRate == EditRate_120 )
	    {
	      if ( m_SampleRate != EditRate_240 )
		{
		  DefaultLogSink().Error("EditRate and SampleRate not correct for 120/240 stereoscopic essence.\n");
		  return RESULT_FORMAT;
		}
	    }
	  else
	    {
	      DefaultLogSink().Error("EditRate not correct for stereoscopic essence: %d/%d.\n",
				     m_EditRate.Numerator, m_EditRate.Denominator);
	      return RESULT_FORMAT;
	    }
	}
      */
      else
	{
	  DefaultLogSink().Error("'type' argument unexpected: %x\n", type);
	  return RESULT_STATE;
	}

      result = MD_to_JXS_PDesc(*m_EssenceDescriptor, *m_EssenceSubDescriptor, m_EditRate, m_SampleRate, m_PDesc);
    }

  return result;
}

//
//
ASDCP::Result_t
ih__Reader::ReadFrame(ui32_t FrameNum, JXS::FrameBuffer& FrameBuf,
		      AESDecContext* Ctx, HMACContext* HMAC)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ReadEKLVFrame(FrameNum, FrameBuf, m_Dict->ul(MDD_JPEGXSEssence), Ctx, HMAC);
}

//
Result_t ih__Reader::CalcFrameBufferSize(ui64_t &size)
{
  if ( ! m_File.IsOpen() )
    return RESULT_INIT;

  assert(m_Dict);
  return ASDCP::h__ASDCPReader::CalcFrameBufferSize(size);
}

//
class ASDCP::JXS::MXFReader::h__Reader : public ih__Reader
{
  ASDCP_NO_COPY_CONSTRUCT(h__Reader);
  h__Reader();

public:
  h__Reader(const Dictionary *d) : ih__Reader(d) {}
};



//------------------------------------------------------------------------------------------


//
void
ASDCP::JXS::FrameBuffer::Dump(FILE* stream, ui32_t dump_len) const
{
  if ( stream == 0 )
    stream = stderr;

  fprintf(stream, "Frame: %06u, %7u bytes", m_FrameNumber, m_Size);
  
  fputc('\n', stream);

  if ( dump_len > 0 )
    Kumu::hexdump(m_Data, dump_len, stream);
}


//------------------------------------------------------------------------------------------

ASDCP::JXS::MXFReader::MXFReader()
{
  m_Reader = new h__Reader(&DefaultCompositeDict());
}


ASDCP::JXS::MXFReader::~MXFReader()
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    m_Reader->Close();
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JXS::MXFReader::OP1aHeader()
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
ASDCP::JXS::MXFReader::OPAtomIndexFooter()
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
ASDCP::JXS::MXFReader::RIP()
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
ASDCP::JXS::MXFReader::OpenRead(const std::string& filename) const
{
  return m_Reader->OpenRead(filename, ASDCP::ESS_JPEG_XS);
}

//
ASDCP::Result_t
ASDCP::JXS::MXFReader::ReadFrame(ui32_t FrameNum, FrameBuffer& FrameBuf,
				   AESDecContext* Ctx, HMACContext* HMAC) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->ReadFrame(FrameNum, FrameBuf, Ctx, HMAC);

  return RESULT_INIT;
}

//
ASDCP::Result_t
ASDCP::JXS::MXFReader::CalcFrameBufferSize(ui64_t &size) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    return m_Reader->CalcFrameBufferSize(size);

  return RESULT_INIT;
}

ASDCP::Result_t
ASDCP::JXS::MXFReader::LocateFrame(ui32_t FrameNum, Kumu::fpos_t& streamOffset, i8_t& temporalOffset, i8_t& keyFrameOffset) const
{
    return m_Reader->LocateFrame(FrameNum, streamOffset, temporalOffset, keyFrameOffset);
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JXS::MXFReader::FillPictureDescriptor(PictureDescriptor& PDesc) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      PDesc = m_Reader->m_PDesc;
      return RESULT_OK;
    }

  return RESULT_INIT;
}


// Fill the struct with the values from the file's header.
// Returns RESULT_INIT if the file is not open.
ASDCP::Result_t
ASDCP::JXS::MXFReader::FillWriterInfo(WriterInfo& Info) const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      Info = m_Reader->m_Info;
      return RESULT_OK;
    }

  return RESULT_INIT;
}

//
void
ASDCP::JXS::MXFReader::DumpHeaderMetadata(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_HeaderPart.Dump(stream);
}


//
void
ASDCP::JXS::MXFReader::DumpIndex(FILE* stream) const
{
  if ( m_Reader->m_File.IsOpen() )
    m_Reader->m_IndexAccess.Dump(stream);
}

//
ASDCP::Result_t
ASDCP::JXS::MXFReader::Close() const
{
  if ( m_Reader && m_Reader->m_File.IsOpen() )
    {
      m_Reader->Close();
      return RESULT_OK;
    }

  return RESULT_INIT;
}


//------------------------------------------------------------------------------------------

//
class ih__Writer : public ASDCP::h__ASDCPWriter
{
  ASDCP_NO_COPY_CONSTRUCT(ih__Writer);
  ih__Writer();

  JPEGXSPictureSubDescriptor* m_EssenceSubDescriptor;

public:
  PictureDescriptor m_PDesc;
  byte_t            m_EssenceUL[SMPTE_UL_LENGTH];

  ih__Writer(const Dictionary *d) : ASDCP::h__ASDCPWriter(d), m_EssenceSubDescriptor(0) {
    memset(m_EssenceUL, 0, SMPTE_UL_LENGTH);
  }

  virtual ~ih__Writer(){}

  Result_t OpenWrite(const std::string&, EssenceType_t type, ui32_t HeaderSize);
  Result_t SetSourceStream(const PictureDescriptor&, const std::string& label,
			   ASDCP::Rational LocalEditRate = ASDCP::Rational(0,0));
  Result_t WriteFrame(const JXS::FrameBuffer&, bool add_index, AESEncContext*, HMACContext*);
  Result_t Finalize();
};

// Open the file for writing. The file must not exist. Returns error if
// the operation cannot be completed.
ASDCP::Result_t
ih__Writer::OpenWrite(const std::string& filename, EssenceType_t type, ui32_t HeaderSize)
{
  if ( ! m_State.Test_BEGIN() )
    return RESULT_STATE;

  Result_t result = m_File.OpenWrite(filename);

  if ( ASDCP_SUCCESS(result) )
    {
      m_HeaderSize = HeaderSize;
      RGBAEssenceDescriptor* tmp_rgba = new RGBAEssenceDescriptor(m_Dict);
      tmp_rgba->ComponentMaxRef = 4095;
      tmp_rgba->ComponentMinRef = 0;

      m_EssenceDescriptor = tmp_rgba;
      m_EssenceSubDescriptor = new JPEGXSPictureSubDescriptor(m_Dict);
      m_EssenceSubDescriptorList.push_back((InterchangeObject*)m_EssenceSubDescriptor);

      GenRandomValue(m_EssenceSubDescriptor->InstanceUID);
      m_EssenceDescriptor->SubDescriptors.push_back(m_EssenceSubDescriptor->InstanceUID);

      /*
      ** thor: stereoskopic images are currently disabled.
      if ( type == ASDCP::ESS_JPEG_XS_S && m_Info.LabelSetType == LS_MXF_SMPTE )
	{
	  InterchangeObject* StereoSubDesc = new StereoscopicPictureSubDescriptor(m_Dict);
	  m_EssenceSubDescriptorList.push_back(StereoSubDesc);
	  GenRandomValue(StereoSubDesc->InstanceUID);
	  m_EssenceDescriptor->SubDescriptors.push_back(StereoSubDesc->InstanceUID);
	}
      **
      */

      result = m_State.Goto_INIT();
    }

  return result;
}

// Automatically sets the MXF file's metadata from the first jpeg codestream stream.
ASDCP::Result_t
ih__Writer::SetSourceStream(const PictureDescriptor& PDesc, const std::string& label, ASDCP::Rational LocalEditRate)
{
  assert(m_Dict);
  if ( ! m_State.Test_INIT() )
    return RESULT_STATE;

  if ( LocalEditRate == ASDCP::Rational(0,0) )
    LocalEditRate = PDesc.EditRate;

  m_PDesc = PDesc;
  assert(m_Dict);
  assert(m_EssenceDescriptor);
  assert(m_EssenceSubDescriptor);
  Result_t result = JXS_PDesc_to_MD(m_PDesc, *m_Dict,
				    *static_cast<ASDCP::MXF::GenericPictureEssenceDescriptor*>(m_EssenceDescriptor),
				    *m_EssenceSubDescriptor);

  if ( ASDCP_SUCCESS(result) )
    {
      ASDCP::MXF::GenericPictureEssenceDescriptor *gpe = static_cast<ASDCP::MXF::RGBAEssenceDescriptor*>(m_EssenceDescriptor);
      switch(PDesc.Ppih) {
      case 0: // Profile_Unrestricted
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSUnrestrictedCodestream));
	break;
      case 0x1500: // Profile_Light422
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSLight422_10Profile));
	break;
      case 0x1a00: // Profile_Light444
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSLight444_12Profile));
	break;
      case 0x2500: // Profile_LightSubline
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSLightSubline422_10Profile));
	break;
      case 0x3540: // Profile_Main422
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSMain422_10Profile));
	break;
      case 0x3a40: // Profile_Main444
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSMain444_12Profile));
	break;
      case 0x3e40: // Profile_Main4444
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSMain4444_12Profile));
	break;
      case 0x4a40: // Profile_High444
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSHigh444_12Profile));
	break;
      case 0x4e40: // Profile_High4444
	gpe->PictureEssenceCoding.Set(m_Dict->ul(MDD_JPEGXSHigh4444_12Profile));
	break;
      default:
	return RESULT_PARAM;
      }
      memcpy(m_EssenceUL, m_Dict->ul(MDD_JPEGXSEssence), SMPTE_UL_LENGTH);
      m_EssenceUL[SMPTE_UL_LENGTH-1] = 1; // first (and only) essence container
      result = m_State.Goto_READY();
    }

  if ( ASDCP_SUCCESS(result) )
    {
      result = WriteASDCPHeader(label, UL(m_Dict->ul(MDD_MXFGCFUFrameWrappedPictureElement)),
				PICT_DEF_LABEL, UL(m_EssenceUL), UL(m_Dict->ul(MDD_PictureDataDef)),
				LocalEditRate, derive_timecode_rate_from_edit_rate(m_PDesc.EditRate));
    }

  return result;
}

// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
//
ASDCP::Result_t
ih__Writer::WriteFrame(const JXS::FrameBuffer& FrameBuf, bool add_index,
		       AESEncContext* Ctx, HMACContext* HMAC)
{
  Result_t result = RESULT_OK;

  if ( m_State.Test_READY() )
    result = m_State.Goto_RUNNING(); // first time through
 
  ui64_t StreamOffset = m_StreamOffset;

  if ( ASDCP_SUCCESS(result) )
    result = WriteEKLVPacket(FrameBuf, m_EssenceUL, MXF_BER_LENGTH, Ctx, HMAC);

  if ( ASDCP_SUCCESS(result) && add_index )
    {  
      IndexTableSegment::IndexEntry Entry;
      Entry.StreamOffset = StreamOffset;
      m_FooterPart.PushIndexEntry(Entry);
    }

  m_FramesWritten++;
  return result;
}


// Closes the MXF file, writing the index and other closing information.
//
ASDCP::Result_t
ih__Writer::Finalize()
{
  if ( ! m_State.Test_RUNNING() )
    return RESULT_STATE;

  m_State.Goto_FINAL();

  return WriteASDCPFooter();
}


//
class ASDCP::JXS::MXFWriter::h__Writer : public ih__Writer
{
  ASDCP_NO_COPY_CONSTRUCT(h__Writer);
  h__Writer();

public:
  h__Writer(const Dictionary *d) : ih__Writer(d) {}
};


//------------------------------------------------------------------------------------------



ASDCP::JXS::MXFWriter::MXFWriter()
{
}

ASDCP::JXS::MXFWriter::~MXFWriter()
{
}

// Warning: direct manipulation of MXF structures can interfere
// with the normal operation of the wrapper.  Caveat emptor!
//
ASDCP::MXF::OP1aHeader&
ASDCP::JXS::MXFWriter::OP1aHeader()
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
ASDCP::JXS::MXFWriter::OPAtomIndexFooter()
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
ASDCP::JXS::MXFWriter::RIP()
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
ASDCP::JXS::MXFWriter::OpenWrite(const std::string& filename, const WriterInfo& Info,
				 const PictureDescriptor& PDesc, ui32_t HeaderSize)
{
  if ( Info.LabelSetType == LS_MXF_SMPTE )
    m_Writer = new h__Writer(&DefaultSMPTEDict());
  else
    m_Writer = new h__Writer(&DefaultInteropDict());

  m_Writer->m_Info = Info;

  Result_t result = m_Writer->OpenWrite(filename, ASDCP::ESS_JPEG_XS, HeaderSize);

  if ( ASDCP_SUCCESS(result) )
    result = m_Writer->SetSourceStream(PDesc, JXS_PACKAGE_LABEL);

  if ( ASDCP_FAILURE(result) )
    m_Writer.release();

  return result;
}


// Writes a frame of essence to the MXF file. If the optional AESEncContext
// argument is present, the essence is encrypted prior to writing.
// Fails if the file is not open, is finalized, or an operating system
// error occurs.
ASDCP::Result_t
ASDCP::JXS::MXFWriter::WriteFrame(const FrameBuffer& FrameBuf, AESEncContext* Ctx, HMACContext* HMAC)
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->WriteFrame(FrameBuf, true, Ctx, HMAC);
}

// Closes the MXF file, writing the index and other closing information.
ASDCP::Result_t
ASDCP::JXS::MXFWriter::Finalize()
{
  if ( m_Writer.empty() )
    return RESULT_INIT;

  return m_Writer->Finalize();
}

//
// end AS_DCP_JXS.cpp
//
