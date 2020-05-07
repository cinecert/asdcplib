/*
Copyright (c) 2011-2020, Robert Scheler, Heiko Sparenberg Fraunhofer IIS,
John Hurst, Pierre-Anthony Lemieux

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

#include "AS_02_internal.h"
#include "AS_02_IAB.h"

#include <iostream>
#include <iomanip>
#include <stdexcept>

namespace Kumu {
  class RuntimeError : public std::runtime_error {
    Kumu::Result_t m_Result;
    RuntimeError();
  public:
    RuntimeError(const Kumu::Result_t& result) : std::runtime_error(result.Message()), m_Result(result) {}
    Kumu::Result_t GetResult() { return this->m_Result; }
    ~RuntimeError() throw() {}
  };
}

//------------------------------------------------------------------------------------------


AS_02::IAB::MXFWriter::MXFWriter() : m_ClipStart(0), m_State(ST_BEGIN) {
}

AS_02::IAB::MXFWriter::~MXFWriter() {}

const ASDCP::MXF::OP1aHeader&
AS_02::IAB::MXFWriter::OP1aHeader() const {
  if (this->m_State == ST_BEGIN) {
    throw Kumu::RuntimeError(Kumu::RESULT_INIT);
  }

  return this->m_Writer->m_HeaderPart;
}

const ASDCP::MXF::RIP&
AS_02::IAB::MXFWriter::RIP() const {
  if (this->m_State == ST_BEGIN) {
    throw Kumu::RuntimeError(Kumu::RESULT_INIT);
  }

  return this->m_Writer->m_RIP;
}

Kumu::Result_t
AS_02::IAB::MXFWriter::OpenWrite(
  const std::string& filename,
  const ASDCP::WriterInfo& Info,
  const ASDCP::MXF::IABSoundfieldLabelSubDescriptor& sub,
  const std::vector<ASDCP::UL>& conformsToSpecs,
  const ASDCP::Rational& edit_rate,
  const ASDCP::Rational& sample_rate) {

  /* are we already running */

  if (this->m_State != ST_BEGIN) {
    return Kumu::RESULT_STATE;
  }

  Result_t result = Kumu::RESULT_OK;

  /* initialize the writer */

  this->m_Writer = new AS_02::IAB::MXFWriter::h__Writer(DefaultSMPTEDict());

  this->m_Writer->m_Info = Info;

  this->m_Writer->m_HeaderSize = 16 * 1024;

  try {

    /* open the file */

    result = this->m_Writer->m_File.OpenWrite(filename.c_str());

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    /* initialize IAB descriptor */

    ASDCP::MXF::IABEssenceDescriptor* desc = new ASDCP::MXF::IABEssenceDescriptor(this->m_Writer->m_Dict);

    GenRandomValue(desc->InstanceUID); /* TODO: remove */
    desc->SampleRate = edit_rate;
    desc->AudioSamplingRate = sample_rate;
    desc->ChannelCount = 0;
    desc->SoundEssenceCoding = this->m_Writer->m_Dict->ul(MDD_ImmersiveAudioCoding);
    desc->QuantizationBits = 24;

    this->m_Writer->m_EssenceDescriptor = desc;

    /* copy and add the IAB subdescriptor */

    ASDCP::MXF::IABSoundfieldLabelSubDescriptor* subdesc = new ASDCP::MXF::IABSoundfieldLabelSubDescriptor(sub);

    GenRandomValue(subdesc->InstanceUID); /* TODO: remove */
    subdesc->MCATagName = "IAB";
    subdesc->MCATagSymbol = "IAB";
    subdesc->MCALabelDictionaryID = this->m_Writer->m_Dict->ul(MDD_IABSoundfield);
    GenRandomValue(subdesc->MCALinkID);

    this->m_Writer->m_EssenceSubDescriptorList.push_back(subdesc);
    this->m_Writer->m_EssenceDescriptor->SubDescriptors.push_back(subdesc->InstanceUID);

    /* initialize the index write */

    this->m_Writer->m_IndexWriter.SetEditRate(edit_rate);

    /* Essence Element UL */

    byte_t element_ul_bytes[ASDCP::SMPTE_UL_LENGTH];

    const ASDCP::MDDEntry& element_ul_entry = this->m_Writer->m_Dict->Type(MDD_IMF_IABEssenceClipWrappedElement);

    std::copy(element_ul_entry.ul, element_ul_entry.ul + ASDCP::SMPTE_UL_LENGTH, element_ul_bytes);

    /* only a single track */

    element_ul_bytes[15] = 1;

    /* write the file header*/
    /* WriteAS02Header() takes ownership of desc and subdesc */

    result = this->m_Writer->WriteAS02Header(
      "Clip wrapping of IA bitstreams as specified in SMPTE ST 2067-201",
      UL(this->m_Writer->m_Dict->ul(MDD_IMF_IABEssenceClipWrappedContainer)),
      "IA Bitstream",
      UL(element_ul_bytes),
      UL(this->m_Writer->m_Dict->ul(MDD_SoundDataDef)),
      edit_rate,
      derive_timecode_rate_from_edit_rate(edit_rate),
      &conformsToSpecs
    );

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    /* start the clip */

    this->m_ClipStart = this->m_Writer->m_File.Tell();

    byte_t clip_buffer[24] = { 0 };

    memcpy(clip_buffer, this->m_Writer->m_Dict->ul(MDD_IMF_IABEssenceClipWrappedElement), 16);

    if (!Kumu::write_BER(clip_buffer + 16, 0, 8)) {
      throw Kumu::RuntimeError(Kumu::RESULT_FAIL);
    }

    this->m_Writer->m_StreamOffset = 24;

    result = this->m_Writer->m_File.Write(clip_buffer, 24);

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    this->m_State = ST_READY;

  } catch (Kumu::RuntimeError e) {

    this->Reset();

    return e.GetResult();

  }

  return result;

}

Result_t
AS_02::IAB::MXFWriter::WriteFrame(const ui8_t* frame, ui32_t sz) {

  /* are we running */

  if (this->m_State == ST_BEGIN) {
    return Kumu::RESULT_INIT;
  }

  Result_t result = Kumu::RESULT_OK;

  /* update the index */

  IndexTableSegment::IndexEntry Entry;

  Entry.StreamOffset = this->m_Writer->m_StreamOffset;

  this->m_Writer->m_IndexWriter.PushIndexEntry(Entry);

  try {

    /* write the frame */

    result = this->m_Writer->m_File.Write(frame, sz);

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    /* increment the frame counter */

    this->m_Writer->m_FramesWritten++;

    /* increment stream offset */

    this->m_Writer->m_StreamOffset += sz;

    /* we are running now */

    this->m_State = ST_RUNNING;

  } catch (Kumu::RuntimeError e) {

    this->Reset();

    return e.GetResult();

  }

  return result;
}

Result_t
AS_02::IAB::MXFWriter::Finalize() {

  /* are we running */

  if (this->m_State == ST_BEGIN) {
    return Kumu::RESULT_INIT;
  }

  Result_t result = RESULT_OK;

  try {

    /* write clip length */

    ui64_t current_position = this->m_Writer->m_File.Tell();

    result = this->m_Writer->m_File.Seek(m_ClipStart + 16);

    byte_t clip_buffer[8] = { 0 };

    ui64_t size = static_cast<ui64_t>(this->m_Writer->m_StreamOffset);

    bool check = Kumu::write_BER(clip_buffer, size, 8);

    if (!check) {
      throw Kumu::RuntimeError(Kumu::RESULT_FAIL);
    }

    result = this->m_Writer->m_File.Write(clip_buffer, 8);

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    result = this->m_Writer->m_File.Seek(current_position);

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    /* write footer */

    result = this->m_Writer->WriteAS02Footer();

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

  } catch (Kumu::RuntimeError e) {

    /* nothing to do since we are about to reset */

    result = e.GetResult();

  }

  /* we are ready to start again */

  this->Reset();

  return result;
}

void
AS_02::IAB::MXFWriter::Reset() {
  this->m_Writer.set(NULL);
  this->m_State = ST_BEGIN;
}


//------------------------------------------------------------------------------------------


AS_02::IAB::MXFReader::MXFReader() : m_State(ST_READER_BEGIN) {}

AS_02::IAB::MXFReader::~MXFReader() {}

ASDCP::MXF::OP1aHeader&
AS_02::IAB::MXFReader::OP1aHeader() const {
  if (this->m_State == ST_READER_BEGIN) {
    throw Kumu::RuntimeError(Kumu::RESULT_INIT);
  }

  return this->m_Reader->m_HeaderPart;
}

const ASDCP::MXF::RIP&
AS_02::IAB::MXFReader::RIP() const {
  if (this->m_State == ST_READER_BEGIN) {
    throw Kumu::RuntimeError(Kumu::RESULT_INIT);
  }

  return this->m_Reader->m_RIP;
}

Result_t
AS_02::IAB::MXFReader::OpenRead(const std::string& filename) {

  /* are we already running */

  if (this->m_State != ST_READER_BEGIN) {
    return Kumu::RESULT_STATE;
  }

  Result_t result = Kumu::RESULT_OK;

  /* initialize the writer */

  this->m_Reader = new h__Reader(DefaultCompositeDict());

  try {

    result = this->m_Reader->OpenMXFRead(filename);

    if (result.Failure()) {
      throw Kumu::RuntimeError(result);
    }

    InterchangeObject* tmp_iobj = 0;

    this->m_Reader->m_HeaderPart.GetMDObjectByType(
      this->m_Reader->m_Dict->Type(MDD_IABEssenceDescriptor).ul,
      &tmp_iobj
    );

    if (!tmp_iobj) {
      throw Kumu::RuntimeError(Kumu::RESULT_FAIL);
    }

    this->m_Reader->m_HeaderPart.GetMDObjectByType(
      this->m_Reader->m_Dict->Type(MDD_IABSoundfieldLabelSubDescriptor).ul,
      &tmp_iobj
    );

    if (!tmp_iobj) {
      throw Kumu::RuntimeError(Kumu::RESULT_FAIL);
    }

    std::list<InterchangeObject*> ObjectList;

    this->m_Reader->m_HeaderPart.GetMDObjectsByType(
      this->m_Reader->m_Dict->Type(MDD_Track).ul,
      ObjectList
    );

    if (ObjectList.empty()) {
      throw Kumu::RuntimeError(Kumu::RESULT_FAIL);
    }

    /* invalidate current frame */

    this->m_CurrentFrameIndex = -1;

    /* we are ready */

    this->m_State = ST_READER_READY;

  } catch (Kumu::RuntimeError e) {

    this->Reset();

    return e.GetResult();
  }

  return RESULT_OK;
}


Result_t
AS_02::IAB::MXFReader::Close() {

  /* are we already running */

  if (this->m_State == ST_READER_BEGIN) {
    return Kumu::RESULT_INIT;
  }

  this->Reset();

  return Kumu::RESULT_OK;
}


Result_t AS_02::IAB::MXFReader::GetFrameCount(ui32_t& frameCount) const {

  /* are we already running */

  if (this->m_State == ST_READER_BEGIN) {
    return Kumu::RESULT_INIT;
  }

  frameCount = m_Reader->m_IndexAccess.GetDuration();

  return Kumu::RESULT_OK;
}

Result_t
AS_02::IAB::MXFReader::ReadFrame(ui32_t frame_number, AS_02::IAB::MXFReader::Frame& frame) {

  /* are we already running */

  if (this->m_State == ST_READER_BEGIN) {
    return Kumu::RESULT_INIT;
  }

  Result_t result = RESULT_OK;

  /* have we already read the frame */

  if (frame_number != this->m_CurrentFrameIndex) {

    try {

      // look up frame index node
      IndexTableSegment::IndexEntry index_entry;

      result = this->m_Reader->m_IndexAccess.Lookup(frame_number, index_entry);

      if (result.Failure()) {
        DefaultLogSink().Error("Frame value out of range: %u\n", frame_number);
        throw Kumu::RuntimeError(result);
      }

      result = this->m_Reader->m_File.Seek(index_entry.StreamOffset);

      if (result.Failure()) {
        DefaultLogSink().Error("Cannot seek to stream offset: %u\n", index_entry.StreamOffset);
        throw Kumu::RuntimeError(result);
      }

      /* read the preamble info */

      const int preambleTLLen = 5;
      const int frameTLLen = 5;

      ui32_t buffer_offset = 0;

      this->m_CurrentFrameBuffer.resize(preambleTLLen);

      result = this->m_Reader->m_File.Read(&this->m_CurrentFrameBuffer[buffer_offset], preambleTLLen);

      if (result.Failure()) {
        DefaultLogSink().Error("Error reading IA Frame preamble\n", index_entry.StreamOffset);
        throw Kumu::RuntimeError(result);
      }

      ui32_t preambleLen = ((ui32_t)this->m_CurrentFrameBuffer[1 + buffer_offset] << 24) +
        ((ui32_t)this->m_CurrentFrameBuffer[2 + buffer_offset] << 16) +
        ((ui32_t)this->m_CurrentFrameBuffer[3 + buffer_offset] << 8) +
        (ui32_t)this->m_CurrentFrameBuffer[4 + buffer_offset];

      buffer_offset += preambleTLLen;

      /* read the preamble*/

      if (preambleLen > 0) {

        this->m_CurrentFrameBuffer.resize(preambleTLLen + preambleLen);

        result = this->m_Reader->m_File.Read(&this->m_CurrentFrameBuffer[buffer_offset], preambleLen);

        if (result.Failure()) {
          DefaultLogSink().Error("Error reading IA Frame preamble\n", index_entry.StreamOffset);
          throw Kumu::RuntimeError(result);
        }

        buffer_offset += preambleLen;

      }

      /* read the IA Frame info */

      this->m_CurrentFrameBuffer.resize(preambleTLLen + preambleLen + frameTLLen);

      result = this->m_Reader->m_File.Read(&this->m_CurrentFrameBuffer[buffer_offset], frameTLLen);

      if (result.Failure()) {
        DefaultLogSink().Error("Error reading IA Frame data\n", index_entry.StreamOffset);
        throw Kumu::RuntimeError(result);
      }

      ui32_t frameLen = ((ui32_t)this->m_CurrentFrameBuffer[buffer_offset + 1] << 24) +
        ((ui32_t)this->m_CurrentFrameBuffer[buffer_offset + 2] << 16) +
        ((ui32_t)this->m_CurrentFrameBuffer[buffer_offset + 3] << 8) +
        (ui32_t)this->m_CurrentFrameBuffer[buffer_offset + 4];

      buffer_offset += frameTLLen;

      /* read the IA Frame */

      if (frameLen > 0) {

        this->m_CurrentFrameBuffer.resize(preambleTLLen + preambleLen + frameTLLen + frameLen);

        result = this->m_Reader->m_File.Read(&this->m_CurrentFrameBuffer[buffer_offset], frameLen);

        if (result.Failure()) {
          DefaultLogSink().Error("Error reading IA Frame data\n", index_entry.StreamOffset);
          throw Kumu::RuntimeError(result);
        }

        buffer_offset += frameLen;

      }

      /* update current frame */

      this->m_CurrentFrameIndex = frame_number;

    } catch (Kumu::RuntimeError e) {

      this->Reset();

      return e.GetResult();

    }

  }

  frame = std::pair<size_t, const ui8_t*>(this->m_CurrentFrameBuffer.size(), &this->m_CurrentFrameBuffer[0]);

  this->m_State = ST_READER_RUNNING;

  return result;
}

Result_t
AS_02::IAB::MXFReader::FillWriterInfo(WriterInfo& Info) const {
  /* are we already running */

  if (this->m_State == ST_READER_BEGIN) {
    return Kumu::RESULT_FAIL;
  }

  Info = m_Reader->m_Info;

  return Kumu::RESULT_OK;
}

void
AS_02::IAB::MXFReader::DumpHeaderMetadata(FILE* stream) const {
  if (this->m_State != ST_READER_BEGIN) {
    this->m_Reader->m_HeaderPart.Dump(stream);
  }
}


void
AS_02::IAB::MXFReader::DumpIndex(FILE* stream) const {
  if (this->m_State != ST_READER_BEGIN) {
    this->m_Reader->m_IndexAccess.Dump(stream);
  }
}

void
AS_02::IAB::MXFReader::Reset() {
  this->m_Reader.set(NULL);
  this->m_State = ST_READER_BEGIN;
}

//
// end AS_02_IAB.cpp
//
