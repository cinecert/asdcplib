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
/*! \file    KM_xml.h
    \version $Id$
    \brief   XML writer
*/


#ifndef _KM_XML_H_
#define _KM_XML_H_

#include <KM_util.h>
#include <list>
#include <string>

namespace Kumu
{
  class XMLElement;

  struct NVPair
  {
    std::string name;
    std::string value;
  };

  typedef std::list<NVPair> AttributeList;
  typedef AttributeList::iterator Attr_i;
  typedef std::list<XMLElement*> ElementList;
  typedef ElementList::iterator Elem_i;

  //
  class XMLElement
    {
      AttributeList m_AttrList;
      ElementList   m_ChildList;

      KM_NO_COPY_CONSTRUCT(XMLElement);
      XMLElement();

    public:
      std::string   m_Name;
      std::string   m_Body;

      XMLElement(const char* name);
      ~XMLElement();

      void        SetAttr(const char* name, const char* value);
      XMLElement* AddChild(const char* name);
      XMLElement* AddChildWithContent(const char* name, const char* value);
      XMLElement* AddChildWithContent(const char* name, const std::string& value);
      XMLElement* AddChildWithPrefixedContent(const char* name, const char* prefix, const char* value);
      void        AddComment(const char* value);
      void        Render(std::string&);
      void        RenderElement(std::string& outbuf, ui32_t depth);
    };
} // namespace Kumu

#endif // _KM_XML_H_

//
// end KM_xml.h
//
