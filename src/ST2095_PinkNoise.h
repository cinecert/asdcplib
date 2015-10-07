/*
Copyright (c) 2015, John Hurst
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
/*! \file    ST2095_PinkNoise.h
    \version $Id$
    \brief   Pink Noise filter and LCG generator
*/


#ifndef _ST2095_PINKNOISE_H_
#define _ST2095_PINKNOISE_H_

#include <KM_fileio.h>
#include <cmath>

//
// No attempt will be made here to explain the theory of operation of
// this noise source since all of the gory detail can be found in SMPTE
// ST 2095-1:2015.  Get your copy from SMPTE today!
//


namespace ASDCP
{
  // A source of pseudo-random numbers suitable for use in generating
  // noise signals.  NOT TO BE USED FOR CRYPTOGRAPHIC FUNCTIONS as its
  // output is 100% deterministic.
  class LinearCongruentialGenerator
  {
    ui32_t m_Seed, m_RandMax;
    float m_ScaleFactor;

    KM_NO_COPY_CONSTRUCT(LinearCongruentialGenerator);

  public:

    LinearCongruentialGenerator(const ui32_t sample_rate);
    float GetNextSample();
  };

  //
  float const PinkFilterHighPassConstant = 10.0; // Hz
  float const PinkFilterLowPassConstant = 22400.0; // Hz

  //
  class PinkFilter
  {
    // storage for biquad coefficients for bandpass filter components
    float hp1_a1, hp1_a2;
    float hp1_b0, hp1_b1, hp1_b2;
    float hp2_a1, hp2_a2;
    float hp2_b0, hp2_b1, hp2_b2;
    float lp1_a1, lp1_a2;
    float lp1_b0, lp1_b1, lp1_b2;
    float lp2_a1, lp2_a2;
    float lp2_b0, lp2_b1, lp2_b2;

    // storage for delay line variables for bandpass filter and initialize to zero
    float hp1w1, hp1w2, hp2w1, hp2w2;
    float lp1w1, lp1w2, lp2w1, lp2w2;

    // storage for delay lines for pink filter network and initialize to zero
    float lp1, lp2, lp3, lp4, lp5, lp6;

    KM_NO_COPY_CONSTRUCT(PinkFilter);

  public:

    PinkFilter(const i32_t SampleRate, const float HpFc, const float LpFc);
    
    // Using a white noise sample as input, produce a pink noise sample
    // having properties as defined by SMPTE ST 2095-1:2015.
    float GetNextSample(const float white);
  };

  // Create a little-endian integer audio sample of the sepcified word size
  // (1-4 bytes) from the normalized input value, write it to the buffer at p.
  void ScalePackSample(float sample, byte_t* p, ui32_t word_size);


} // namespace ASDCP



#endif // _ST2095_PINKNOISE_H_


//
// end ST2095_PinkNoise.h
//
