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
/*! \file    ST2095_PinkNoise.cpp
    \version $Id$
    \brief   Pink Noise filter and LCG generator
*/

#include "ST2095_PinkNoise.h"

//
// This file is full of magic numbers.  Details behind the
// selection of these values can be found in SMPTE ST 2095-1:2015.
//

static ui32_t const C_rand_step = 52737;
static float const C_max_ampl_32 = pow(2.0, 31) - 1.0;
static float const C_max_peak = -9.5;     // Clipping Threshold in dB FS (+/-1.0 = 0 dB)
static float const C_max_amp = pow(10.0, C_max_peak / 20.0);


//
ASDCP::LinearCongruentialGenerator::LinearCongruentialGenerator(const ui32_t sample_rate) : m_Seed(0)
{
  ui32_t samples_per_period = 524288;

  if ( sample_rate > 48000 )
    {
      samples_per_period = 1048576;
    }

  m_RandMax = samples_per_period - 1;
  m_ScaleFactor = 2.0 / float(m_RandMax);
}

//
float
ASDCP::LinearCongruentialGenerator::GetNextSample()
{
  m_Seed = (1664525 * m_Seed + C_rand_step) & m_RandMax;
  float out = float(m_Seed) * m_ScaleFactor - 1.0;
  return out;
}

//
ASDCP::PinkFilter::PinkFilter(const i32_t sample_rate, float high_pass_fc, float low_pass_fc)
{
  //  Disaster check: filters in order, low_pass_fc <= Nyquist
  assert(high_pass_fc < low_pass_fc);
  assert(low_pass_fc < sample_rate / 2.0);

  // Calculate omegaT for matched Z transform highpass filters
  const float w0t = 2.0 * M_PI * high_pass_fc / sample_rate;

  // Calculate k for bilinear transform lowpass filters
  const float k = tan(( 2.0 * M_PI * low_pass_fc / sample_rate ) / 2.0);

  // precalculate k^2 (makes for a little bit cleaner code)
  const float k2 = k * k;

  // Calculate biquad coefficients for bandpass filter components
  hp1_a1 = -2.0 * exp(-0.3826835 * w0t) * cos(0.9238795 * w0t);
  hp1_a2 = exp(2.0 * -0.3826835 * w0t);
  hp1_b0 = (1.0 - hp1_a1 + hp1_a2) / 4.0;
  hp1_b1 = -2.0 * hp1_b0;
  hp1_b2 = hp1_b0;

  hp2_a1 = -2.0 * exp(-0.9238795 * w0t) * cos(0.3826835 * w0t);
  hp2_a2 = exp(2.0 * -0.9238795 * w0t);
  hp2_b0 = (1.0 - hp2_a1 + hp2_a2) / 4.0;
  hp2_b1 = -2.0 * hp2_b0;
  hp2_b2 = hp2_b0;

  lp1_a1 = (2.0 * (k2 - 1.0)) / (k2 + (k / 1.306563) + 1.0);
  lp1_a2 = (k2 - (k / 1.306563) + 1.0) / (k2 + (k / 1.306563) + 1.0);
  lp1_b0 = k2 / (k2 + (k / 1.306563) + 1.0);
  lp1_b1 = 2.0 * lp1_b0;
  lp1_b2 = lp1_b0;

  lp2_a1 = (2.0 * (k2 - 1.0)) / (k2 + (k / 0.541196) + 1.0);
  lp2_a2 = (k2 - (k / 0.541196) + 1.0) / (k2 + (k / 0.541196) + 1.0);
  lp2_b0 = k2 / (k2 + (k / 0.541196) + 1.0);
  lp2_b1 = 2.0 * lp2_b0;
  lp2_b2 = lp2_b0;

  // Declare delay line variables for bandpass filter and initialize to zero
  hp1w1 = hp1w2 = hp2w1 = hp2w2 = 0.0;
  lp1w1 = lp1w2 = lp2w1 = lp2w2 = 0.0;

  // Declare delay lines for pink filter network and initialize to zero
  lp1 = lp2 = lp3 = lp4 = lp5 = lp6 = 0.0;
}


//
float
ASDCP::PinkFilter::GetNextSample(const float white)
{
  // Run pink filter; a parallel network of 1st order LP filters
  // Scaled for conventional RNG (need to rescale by sqrt(1/3) for MLS)
  lp1 = 0.9994551 * lp1 + 0.00198166688621989 * white;
  lp2 = 0.9969859 * lp2 + 0.00263702334184061 * white;
  lp3 = 0.9844470 * lp3 + 0.00643213710202331 * white;
  lp4 = 0.9161757 * lp4 + 0.01438952538362820 * white;
  lp5 = 0.6563399 * lp5 + 0.02698408541064610 * white;
  float pink = lp1 + lp2 + lp3 + lp4 + lp5 + lp6 + white * 0.0342675832159306;
  lp6 = white * 0.0088766118009356;

  // Run bandpass filter; a series network of 4 biquad filters
  // Biquad filters implemented in Direct Form II
  float w = pink - hp1_a1 * hp1w1 - hp1_a2 * hp1w2;
  pink = hp1_b0 * w + hp1_b1 * hp1w1 + hp1_b2 * hp1w2;
  hp1w2 = hp1w1;
  hp1w1 = w;

  w = pink - hp2_a1 * hp2w1 - hp2_a2 * hp2w2;
  pink = hp2_b0 * w + hp2_b1 * hp2w1 + hp2_b2 * hp2w2;
  hp2w2 = hp2w1;
  hp2w1 = w;

  w = pink - lp1_a1 * lp1w1 - lp1_a2 * lp1w2;
  pink = lp1_b0 * w + lp1_b1 * lp1w1 + lp1_b2 * lp1w2;
  lp1w2 = lp1w1;
  lp1w1 = w;

  w = pink - lp2_a1 * lp2w1 - lp2_a2 * lp2w2;
  pink = lp2_b0 * w + lp2_b1 * lp2w1 + lp2_b2 * lp2w2;
  lp2w2 = lp2w1;
  lp2w1 = w;

  // Limit peaks to +/-C_max_amp
  if ( pink > C_max_amp )
    {
      pink = C_max_amp;
    }
  else if ( pink < -C_max_amp )
    {
      pink = -C_max_amp;
    }

  return pink;
}

//
void
ASDCP::ScalePackSample(float sample, byte_t* p, ui32_t word_size)
{
  byte_t tmp_buf[4];
  Kumu::i2p<i32_t>(KM_i32_LE(sample * C_max_ampl_32), tmp_buf);

  switch ( word_size )
    {
    case 4: *p++ = tmp_buf[0];
    case 3: *p++ = tmp_buf[1];
    case 2: *p++ = tmp_buf[2];
    case 1: *p++ = tmp_buf[3];
    }
}
//
// end ST2095_PinkNoise.cpp
//
