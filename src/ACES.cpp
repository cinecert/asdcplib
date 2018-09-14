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

#include "ACES.h"
#include "KM_log.h"


namespace
{

const std::string AttrAcesImageContainerFlag("acesImageContainerFlag");
const std::string AttrChannels("channels");
const std::string AttrChromaticities("chromaticities");
const std::string AttrCompression("compression");
const std::string AttrDataWindow("dataWindow");
const std::string AttrDisplayWindow("displayWindow");
const std::string AttrLineOrder("lineOrder");
const std::string AttrPixelAspectRatio("pixelAspectRatio");
const std::string AttrScreenWindowCenter("screenWindowCenter");
const std::string AttrScreenWindowWidth("screenWindowWidth");

const std::string TypeUnsignedChar("unsigned char");
const std::string TypeUnsignedChar_("unsignedChar");
const std::string TypeShort("short");
const std::string TypeUnsignedShort("unsigned short");
const std::string TypeUnsignedShort_("unsignedShort");
const std::string TypeInt("int");
const std::string TypeUnsignedInt("unsigned int");
const std::string TypeUnsignedInt_("unsignedInt");
const std::string TypeUnsignedLong("unsigned long");
const std::string TypeUnsignedLong_("unsignedLong");
const std::string TypeHalf("half");
const std::string TypeFloat("float");
const std::string TypeDouble("double");
const std::string TypeBox2i("box2i");
const std::string TypeChlist("chlist");
const std::string TypeChromaticities("chromaticities");
const std::string TypeCompression("compression");
const std::string TypeLineOrder("lineOrder");
const std::string TypeKeycode("keycode");
const std::string TypeRational("rational");
const std::string TypeString("string");
const std::string TypeStringVector("stringVector");
const std::string TypeTimecode("timecode");
const std::string TypeV2f("v2f");
const std::string TypeV3f("v3f");

} // namespace

void AS_02::ACES::Attribute::Move(const byte_t *buf)
{
  mAttrType = Invalid;
  mType = Unknown_t;
  mAttrName.clear();
  mpValue = NULL;
  mDataSize = 0;
  mValueSize = 0;
  if(buf)
  {
    mpData = buf;
    while(*buf != 0x00 && buf - mpData < 256)
    {
      buf++;
    }
    if(buf - mpData < 1)
    {
       Kumu::DefaultLogSink().Error("Size of attribute name == 0 Bytes\n");
       return;
    }
    else if(buf - mpData > 255)
    {
       Kumu::DefaultLogSink().Error("Size of attribute name > 255 Bytes\n");
       return;
    }
    mAttrName.assign((const char*)mpData, buf - mpData); // We don't want the Null termination.
    buf++; // Move to "attribute type name".
    const byte_t *ptmp = buf;
    while(*buf != 0x00 && buf - ptmp < 256)
    {
      buf++;
    }
    if(buf - ptmp < 1)
    {
       Kumu::DefaultLogSink().Error("Size of attribute type == 0 Bytes\n");
       return;
    }
    else if(buf - ptmp > 255)
    {
       Kumu::DefaultLogSink().Error("Size of attribute type > 255 Bytes\n");
       return;
    }
    std::string attribute_type_name;
    attribute_type_name.assign((const char*)ptmp, buf - ptmp); // We don't want the Null termination.
    buf++; // Move to "attribute size".
    i32_t size = KM_i32_LE(*(i32_t*)(buf));
    if(size < 0)
    {
       Kumu::DefaultLogSink().Error("Attribute size is negative\n");
       return;
    }
    mValueSize = size;
    mpValue = buf + 4;
    mDataSize = mpValue - mpData + mValueSize;
    MatchAttribute(mAttrName);
    MatchType(attribute_type_name);
  }
}

void AS_02::ACES::Attribute::MatchAttribute(const std::string &Type)
{

  if(Type == AttrAcesImageContainerFlag) mAttrType = AcesImageContainerFlag;
  else if(Type == AttrChannels) mAttrType = Channels;
  else if(Type == AttrChromaticities) mAttrType = Chromaticities;
  else if(Type == AttrCompression) mAttrType = Compression;
  else if(Type == AttrDataWindow) mAttrType = DataWindow;
  else if(Type == AttrDisplayWindow) mAttrType = DisplayWindow;
  else if(Type == AttrLineOrder) mAttrType = LineOrder;
  else if(Type == AttrPixelAspectRatio) mAttrType = PixelAspectRatio;
  else if(Type == AttrScreenWindowCenter) mAttrType = ScreenWindowCenter;
  else if(Type == AttrScreenWindowWidth) mAttrType = SreenWindowWidth;
  else mAttrType = Other;
}

void AS_02::ACES::Attribute::MatchType(const std::string &Type)
{

  if(Type == TypeUnsignedChar || Type == TypeUnsignedChar_) mType = UnsignedChar_t;
  else if(Type == TypeShort) mType = Short_t;
  else if(Type == TypeUnsignedShort || Type == TypeUnsignedShort_) mType = UnsignedShort_t;
  else if(Type == TypeInt) mType = Int_t;
  else if(Type == TypeUnsignedInt || Type == TypeUnsignedInt_) mType = UnsignedInt_t;
  else if(Type == TypeUnsignedLong || Type == TypeUnsignedLong_) mType = UnsignedLong_t;
  else if(Type == TypeHalf) mType = Half_t;
  else if(Type == TypeFloat) mType = Float_t;
  else if(Type == TypeDouble) mType = Double_t;
  else if(Type == TypeBox2i) mType = Box2i_t;
  else if(Type == TypeChlist) mType = Chlist_t;
  else if(Type == TypeChromaticities) mType = Chromaticities_t;
  else if(Type == TypeCompression) mType = Compression_t;
  else if(Type == TypeLineOrder) mType = LineOrder_t;
  else if(Type == TypeKeycode) mType = Keycode_t;
  else if(Type == TypeRational) mType = Rational_t;
  else if(Type == TypeString) mType = String_t;
  else if(Type == TypeStringVector) mType = StringVector_t;
  else if(Type == TypeTimecode) mType = Timecode_t;
  else if(Type == TypeV2f) mType = V2f_t;
  else if(Type == TypeV3f) mType = V3f_t;
  else mType = Unknown_t;
}

AS_02::Result_t AS_02::ACES::Attribute::CopyToGenericContainer(other &value) const
{

  generic gen;
  if(mValueSize > sizeof(gen.data)) return RESULT_FAIL;
  memcpy(gen.data, mpValue, mValueSize);
  gen.type = mType;
  gen.size = mValueSize;
  gen.attributeName = mAttrName;
  value.push_back(gen);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(ui8_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(i16_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(ui16_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(i32_t &value) const {

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(ui32_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(ui64_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(real32_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBasicType(real64_t &value) const
{

  if(sizeof(value) != mValueSize) return RESULT_FAIL;
  ACESDataAccessor::AsBasicType(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsBox2i(box2i &value) const
{

  ACESDataAccessor::AsBox2i(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsChlist(chlist &value) const
{

  ACESDataAccessor::AsChlist(mpValue, mValueSize, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsChromaticities(chromaticities &value) const
{

  ACESDataAccessor::AsChromaticities(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsKeycode(keycode &value) const
{

  ACESDataAccessor::AsKeycode(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsRational(ASDCP::Rational &value) const
{

  ACESDataAccessor::AsRational(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsString(std::string &value) const
{

  ACESDataAccessor::AsString(mpValue, mValueSize, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsStringVector(stringVector &value) const
{

  ACESDataAccessor::AsStringVector(mpValue, mValueSize, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsV2f(v2f &value) const
{

  ACESDataAccessor::AsV2f(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsV3f(v3f &value) const
{

  ACESDataAccessor::AsV3f(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::Attribute::GetValueAsTimecode(timecode &value) const
{

  ACESDataAccessor::AsTimecode(mpValue, value);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::GetNextAttribute(const byte_t **buf, Attribute &attr)
{

  assert((buf != NULL) && (*buf != NULL));
  while(**buf != 0x00) { (*buf)++; }
  (*buf)++;
  while(**buf != 0x00) { (*buf)++; }
  (*buf)++;
  i32_t size = KM_i32_LE(*(i32_t*)(*buf));
  if(size < 0)
  {
    Kumu::DefaultLogSink().Error("Attribute size is negative\n");
    return RESULT_FAIL;
  }
  *buf += 4 + size;
  if(**buf == 0x00)
  {
    return RESULT_ENDOFFILE; // Indicates end of header.
  }
  attr.Move(*buf);
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::CheckMagicNumber(const byte_t **buf)
{

  assert((buf != NULL) && (*buf != NULL));
  if(memcmp(Magic, *buf, 4) != 0) return RESULT_FAIL;
  *buf += 4;
  return RESULT_OK;
}

AS_02::Result_t AS_02::ACES::CheckVersionField(const byte_t **buf)
{

  assert((buf != NULL) && (*buf != NULL));
  if(memcmp(Version_short, *buf, 4) != 0 && memcmp(Version_long, *buf, 4) != 0) return RESULT_FAIL;
  *buf += 4;
  return RESULT_OK;
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, ui8_t &value)
{

  value = *(ui8_t*)(buf);
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, i16_t &value)
{

  value = KM_i16_LE(*(i16_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, ui16_t &value)
{

  value = KM_i16_LE(*(ui16_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, i32_t &value)
{

  value = KM_i32_LE(*(i32_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, ui32_t &value)
{

  value = KM_i32_LE(*(ui32_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, ui64_t &value)
{

  value = KM_i64_LE(*(ui64_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, real32_t &value)
{

  value = KM_i32_LE(*(real32_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBasicType(const byte_t *buf, real64_t &value)
{

  value = KM_i64_LE(*(real64_t*)(buf));
}

void AS_02::ACES::ACESDataAccessor::AsBox2i(const byte_t *buf, box2i &value)
{

  value.xMin = KM_i32_LE(*(i32_t*)(buf));
  value.yMin = KM_i32_LE(*(i32_t*)(buf + 4));
  value.xMax = KM_i32_LE(*(i32_t*)(buf + 8));
  value.yMax = KM_i32_LE(*(i32_t*)(buf + 12));
}

void AS_02::ACES::ACESDataAccessor::AsChlist(const byte_t *buf, ui32_t size, chlist &value)
{

  const byte_t *end = buf + size - 1;
  while(buf < end)
  {
    const byte_t *ptmp = buf;
    while(*buf != 0x00 && buf - ptmp < 256) { buf++; }
    if(buf - ptmp < 1)
    {
       Kumu::DefaultLogSink().Error("Size of name == 0 Bytes\n");
       return;
    }
    else if(buf - ptmp > 255)
    {
       Kumu::DefaultLogSink().Error("Size of name > 255 Bytes\n");
       return;
    }
    channel ch;
    ch.name.assign((const char*)ptmp, buf - ptmp); // We don't want the Null termination.
    buf++;
    ch.pixelType = KM_i32_LE(*(i32_t*)(buf));
    buf += 4;
    ch.pLinear = KM_i32_LE(*(ui32_t*)(buf));
    buf += 4;
    ch.xSampling = KM_i32_LE(*(i32_t*)(buf));
    buf += 4;
    ch.ySampling = KM_i32_LE(*(i32_t*)(buf));
    buf += 4;
    value.push_back(ch);
  }
}

void AS_02::ACES::ACESDataAccessor::AsChromaticities(const byte_t *buf, chromaticities &value)
{

  value.red.x = KM_i32_LE(*(real32_t*)(buf));
  value.red.y = KM_i32_LE(*(real32_t*)(buf + 4));
  value.green.x = KM_i32_LE(*(real32_t*)(buf + 8));
  value.green.y = KM_i32_LE(*(real32_t*)(buf + 12));
  value.blue.x = KM_i32_LE(*(real32_t*)(buf + 16));
  value.blue.y = KM_i32_LE(*(real32_t*)(buf + 20));
  value.white.x = KM_i32_LE(*(real32_t*)(buf + 24));
  value.white.y = KM_i32_LE(*(real32_t*)(buf + 28));
}

void AS_02::ACES::ACESDataAccessor::AsKeycode(const byte_t *buf, keycode &value)
{

  value.filmMfcCode = KM_i32_LE(*(i32_t*)(buf));
  value.filmType = KM_i32_LE(*(i32_t*)(buf + 4));
  value.prefix = KM_i32_LE(*(i32_t*)(buf + 8));
  value.count = KM_i32_LE(*(i32_t*)(buf + 12));
  value.perfOffset = KM_i32_LE(*(i32_t*)(buf + 16));
  value.perfsPerFrame = KM_i32_LE(*(i32_t*)(buf + 20));
  value.perfsPerCount = KM_i32_LE(*(i32_t*)(buf + 24));
}

void AS_02::ACES::ACESDataAccessor::AsRational(const byte_t *buf, ASDCP::Rational &value)
{

  value.Numerator = KM_i32_LE(*(i32_t*)(buf));
  value.Denominator = KM_i32_LE(*(ui32_t*)(buf + 4));
}

void AS_02::ACES::ACESDataAccessor::AsString(const byte_t *buf, ui32_t size, std::string &value)
{

  value.assign((const char*)buf, size);
}

void AS_02::ACES::ACESDataAccessor::AsStringVector(const byte_t *buf, ui32_t size, stringVector &value)
{

  const byte_t *end = buf + size - 1;
  while(buf < end)
  {
    i32_t str_length = KM_i32_LE(*(i32_t*)(buf));
    std::string str;
    str.assign((const char*)buf, str_length);
    value.push_back(str);
    if(buf + str_length >= end) break;
    else buf += str_length;
  }
}

void AS_02::ACES::ACESDataAccessor::AsV2f(const byte_t *buf, v2f &value)
{

  value.x = KM_i32_LE(*(real32_t*)(buf));
  value.y = KM_i32_LE(*(real32_t*)(buf + 4));
}

void AS_02::ACES::ACESDataAccessor::AsV3f(const byte_t *buf, v3f &value)
{

  value.x = KM_i32_LE(*(real32_t*)(buf));
  value.y = KM_i32_LE(*(real32_t*)(buf + 4));
  value.z = KM_i32_LE(*(real32_t*)(buf + 8));
}

void AS_02::ACES::ACESDataAccessor::AsTimecode(const byte_t *buf, timecode &value)
{

  value.timeAndFlags = KM_i32_LE(*(ui32_t*)(buf));
  value.userData = KM_i32_LE(*(ui32_t*)(buf + 4));
}
