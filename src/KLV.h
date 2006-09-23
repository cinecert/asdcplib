/*
Copyright (c) 2005-2006, John Hurst
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
/*! \file    KLV.h
  \version $Id$
  \brief   KLV objects
*/

#ifndef _KLV_H_
#define _KLV_H_

#include <KM_fileio.h>
#include <KM_memio.h>
#include "AS_DCP.h"
#include "MDD.h"


namespace ASDCP
{
  const ui32_t MXF_BER_LENGTH = 4;
  const ui32_t MXF_TAG_LENGTH = 2;
  const ui32_t SMPTE_UL_LENGTH = 16;
  const ui32_t SMPTE_UMID_LENGTH = 32;
  const byte_t SMPTE_UL_START[4] = { 0x06, 0x0e, 0x2b, 0x34 };
  const ui32_t MAX_KLV_PACKET_LENGTH = 1024*1024*64;

  const ui32_t IdentBufferLen = 128;

const ui32_t IntBufferLen = 64;

inline const char* i64sz(i64_t i, char* buf)
{ 
  assert(buf);
#ifdef WIN32
  snprintf(buf, IntBufferLen, "%I64d", i);
#else
  snprintf(buf, IntBufferLen, "%lld", i);
#endif
  return buf;
}

inline const char* ui64sz(ui64_t i, char* buf)
{ 
  assert(buf);
#ifdef WIN32
  snprintf(buf, IntBufferLen, "%I64u", i);
#else
  snprintf(buf, IntBufferLen, "%llu", i);
#endif
  return buf;
}

  struct TagValue
  {
    byte_t a;
    byte_t b;

    inline bool operator<(const TagValue& rhs) const {
      if ( a < rhs.a ) return true;
      if ( a == rhs.a && b < rhs.b ) return true;
      return false;
    }

    inline bool operator==(const TagValue& rhs) const {
      if ( a != rhs.a ) return false;
      if ( b != rhs.b ) return false;
      return true;
    }
  };

  using Kumu::UUID;

  // Universal Label
  class UL : public Kumu::Identifier<SMPTE_UL_LENGTH>
    {
    public:
      UL() {}
      UL(const UL& rhs) : Kumu::Identifier<SMPTE_UL_LENGTH>(rhs) {}
      UL(const byte_t* value) : Kumu::Identifier<SMPTE_UL_LENGTH>(value) {}
      virtual ~UL() {}

      const char* EncodeString(char* str_buf, ui32_t buf_len) const;
    };

  // UMID
  class UMID : public Kumu::Identifier<SMPTE_UMID_LENGTH>
    {
    public:
      UMID() {}
      UMID(const UMID& rhs) : Kumu::Identifier<SMPTE_UMID_LENGTH>(rhs) {}
      UMID(const byte_t* value) : Kumu::Identifier<SMPTE_UMID_LENGTH>(value) {}
      virtual ~UMID() {}

      void MakeUMID(int Type);
      void MakeUMID(int Type, const UUID& ID);
      const char* EncodeString(char* str_buf, ui32_t buf_len) const;
    };

  const byte_t nil_UMID[SMPTE_UMID_LENGTH] = {0};
  const UMID NilUMID(nil_UMID);

  //
  struct MDDEntry
  {
    byte_t        ul[SMPTE_UL_LENGTH];
    TagValue      tag;
    bool          optional;
    const char*   name;
  };

  //
  class Dict
    {
    public:
      static const MDDEntry* FindUL(const byte_t*);
      static const MDDEntry* FindName(const char*);
      static const MDDEntry& Type(MDD_t type_id);
      static bool            Replace(const MDDEntry& Entry);
      static void            Restore(const byte_t* ul);
      static void            RestoreAll();

      inline static const byte_t* ul(MDD_t type_id) {
	return Type(type_id).ul;
      }

    private:
      Dict* m_Dict;
      ASDCP_NO_COPY_CONSTRUCT(Dict);

    protected:
      Dict();

    public:
      ~Dict();
    };

  //
  class IPrimerLookup
    {
    public:
      virtual ~IPrimerLookup() {}
      virtual void     ClearTagList() = 0;
      virtual Result_t InsertTag(const MDDEntry& Entry, ASDCP::TagValue& Tag) = 0;
      virtual Result_t TagForKey(const ASDCP::UL& Key, ASDCP::TagValue& Tag) = 0;
    };

  //
  class KLVPacket
    {
      ASDCP_NO_COPY_CONSTRUCT(KLVPacket);

    protected:
      const byte_t* m_KeyStart;
      ui32_t        m_KLLength;
      const byte_t* m_ValueStart;
      ui32_t        m_ValueLength;

    public:
      KLVPacket() : m_KeyStart(0), m_KLLength(0), m_ValueStart(0), m_ValueLength(0) {}
      virtual ~KLVPacket() {}

      ui32_t  PacketLength() {
	return m_KLLength + m_ValueLength;
      }

      virtual bool     HasUL(const byte_t*);
      virtual Result_t InitFromBuffer(const byte_t*, ui32_t);
      virtual Result_t InitFromBuffer(const byte_t*, ui32_t, const byte_t* label);
      virtual Result_t WriteKLToBuffer(ASDCP::FrameBuffer&, const byte_t* label, ui32_t length);
      virtual void     Dump(FILE*, bool);
    };

  //
  class KLVFilePacket : public KLVPacket
    {
      ASDCP_NO_COPY_CONSTRUCT(KLVFilePacket);

    protected:
      ASDCP::FrameBuffer m_Buffer;

    public:
      KLVFilePacket() {}
      virtual ~KLVFilePacket() {}

      virtual Result_t InitFromFile(const Kumu::FileReader&);
      virtual Result_t InitFromFile(const Kumu::FileReader&, const byte_t* label);
      virtual Result_t WriteKLToFile(Kumu::FileWriter& Writer, const byte_t* label, ui32_t length);
    };

} // namespace ASDCP

#endif // _KLV_H_


//
// end KLV.h
//
