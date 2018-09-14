/*
Copyright (c) 2018, Bjoern Stresing, Patrick Bichiou, Wolfgang Ruppel,
John Hurst

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
/*
This module implements ACES header parser. 
*/

#ifndef ACES_h__
#define ACES_h__

#include "AS_02_ACES.h"


namespace AS_02
{

namespace ACES
{

const byte_t Magic[] = {0x76, 0x2f, 0x31, 0x01};
const byte_t Version_short[] = {0x02, 0x00, 0x00, 0x00}; // Version Nr. 2 for attribute names and attribute type names shorter than 32 Bytes.
const byte_t Version_long[] = {0x02, 0x00, 0x04, 0x00}; // Version Nr. 1026 for attribute names and attribute type names exceeding 31 Bytes.

class Attribute
{

public:
  Attribute(const byte_t *const buf = NULL) : mAttrType(Invalid), mType(Unknown_t), mAttrName(), mpData(buf), mpValue(NULL), mDataSize(0), mValueSize(0)
  {
    Move(buf);
  }
  ~Attribute() {}
  // Move the internal data pointer to another position.
  void Move(const byte_t *buf);
  // The whole size of the Attribute (attribute name + attribute type name + attribute size + attribute value).
  ui32_t GetTotalSize() const { return mDataSize; };
  // The name of the Attribute.
  std::string GetName() const { return mAttrName; };
  // What kind of Attribute.
  eAttributes GetAttribute() const { return mAttrType; };
  // What Datatype the Attribute contains.
  eTypes GetType() const { return mType; };
  // You should check if the Attribute object is valid before using it.
  bool IsValid() const { return mAttrType != Invalid; };
  // Use this function to copy the raw data to a generic container for later use.
  Result_t CopyToGenericContainer(other &value) const;
  // Getter functions.
  Result_t GetValueAsBasicType(ui8_t  &value) const;
  Result_t GetValueAsBasicType(i16_t  &value) const;
  Result_t GetValueAsBasicType(ui16_t &value) const; // use for real16_t
  Result_t GetValueAsBasicType(i32_t  &value) const;
  Result_t GetValueAsBasicType(ui32_t &value) const;
  Result_t GetValueAsBasicType(ui64_t &value) const;
  Result_t GetValueAsBasicType(real32_t &value) const;
  Result_t GetValueAsBasicType(real64_t &value) const;
  Result_t GetValueAsBox2i(box2i &value) const;
  Result_t GetValueAsChlist(chlist &value) const;
  Result_t GetValueAsChromaticities(chromaticities &value) const;
  Result_t GetValueAsKeycode(keycode &value) const;
  Result_t GetValueAsRational(ASDCP::Rational &value) const;
  Result_t GetValueAsString(std::string &value) const;
  Result_t GetValueAsStringVector(stringVector &value) const;
  Result_t GetValueAsV2f(v2f &value) const;
  Result_t GetValueAsV3f(v3f &value) const;
  Result_t GetValueAsTimecode(timecode &value) const;

private:
  KM_NO_COPY_CONSTRUCT(Attribute);
  void MatchAttribute(const std::string &Type);
  void MatchType(const std::string &Type);

  eAttributes mAttrType;
  eTypes mType;
  std::string mAttrName;
  const byte_t *mpData;
  const byte_t *mpValue;
  ui32_t  mDataSize;
  ui32_t  mValueSize;
};

Result_t GetNextAttribute(const byte_t **buf, Attribute &attr);
Result_t CheckMagicNumber(const byte_t **buf);
Result_t CheckVersionField(const byte_t **buf);

class ACESDataAccessor
{
public:
  static void AsBasicType(const byte_t *buf, ui8_t &value);
  static void AsBasicType(const byte_t *buf, i16_t  &value);
  static void AsBasicType(const byte_t *buf, ui16_t &value); // use for real16_t
  static void AsBasicType(const byte_t *buf, i32_t  &value);
  static void AsBasicType(const byte_t *buf, ui32_t &value);
  static void AsBasicType(const byte_t *buf, ui64_t &value);
  static void AsBasicType(const byte_t *buf, real32_t &value);
  static void AsBasicType(const byte_t *buf, real64_t &value);
  static void AsBox2i(const byte_t *buf, box2i &value);
  static void AsChlist(const byte_t *buf, ui32_t size, chlist &value);
  static void AsChromaticities(const byte_t *buf, chromaticities &value);
  static void AsKeycode(const byte_t *buf, keycode &value);
  static void AsRational(const byte_t *buf, ASDCP::Rational &value);
  static void AsString(const byte_t *buf, ui32_t size, std::string &value);
  static void AsStringVector(const byte_t *buf, ui32_t size, stringVector &value);
  static void AsV2f(const byte_t *buf, v2f &value);
  static void AsV3f(const byte_t *buf, v3f &value);
  static void AsTimecode(const byte_t *buf, timecode &value);
};

} // namespace ACES

} // namespace AS_02

#endif // ACES_h__
