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
/*! \file    KM_xml.cpp
    \version $Id$
    \brief   XML writer
*/

#include <KM_xml.h>



Kumu::XMLElement::XMLElement(const char* name)
{
  m_Name = name;
}

Kumu::XMLElement::~XMLElement()
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    delete *i;
}

//
void
Kumu::XMLElement::SetAttr(const char* name, const char* value)
{
  NVPair TmpVal;
  TmpVal.name = name;
  TmpVal.value = value;

  m_AttrList.push_back(TmpVal);
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChild(const char* name)
{
  XMLElement* tmpE = new XMLElement(name);
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithContent(const char* name, const std::string& value)
{
  return AddChildWithContent(name, value.c_str());
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithContent(const char* name, const char* value)
{
  assert(name);
  assert(value);
  XMLElement* tmpE = new XMLElement(name);
  tmpE->m_Body = value;
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
Kumu::XMLElement*
Kumu::XMLElement::AddChildWithPrefixedContent(const char* name, const char* prefix, const char* value)
{
  XMLElement* tmpE = new XMLElement(name);
  tmpE->m_Body = prefix;
  tmpE->m_Body += value;
  m_ChildList.push_back(tmpE);
  return tmpE;
}

//
void
Kumu::XMLElement::AddComment(const char* value)
{
  m_Body += "  <!-- ";
  m_Body += value;
  m_Body += " -->\n";
}

//
void
Kumu::XMLElement::Render(std::string& outbuf)
{
  outbuf = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  RenderElement(outbuf, 0);
}

//
inline void
add_spacer(std::string& outbuf, i32_t depth)
{
  while ( depth-- )
    outbuf+= "  ";
}

//
void
Kumu::XMLElement::RenderElement(std::string& outbuf, ui32_t depth)
{
  add_spacer(outbuf, depth);

  outbuf += "<";
  outbuf += m_Name;

  // render attributes
  for ( Attr_i i = m_AttrList.begin(); i != m_AttrList.end(); i++ )
    {
      outbuf += " ";
      outbuf += (*i).name;
      outbuf += "=\"";
      outbuf += (*i).value;
      outbuf += "\"";
    }

  // body contents and children
  if ( ! m_ChildList.empty() )
    {
      outbuf += ">\n";

      // render body
      if ( m_Body.length() > 0 )
	outbuf += m_Body;

      for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
	(*i)->RenderElement(outbuf, depth + 1);

      add_spacer(outbuf, depth);
      outbuf += "</";
      outbuf += m_Name;
      outbuf += ">\n";
    }
  else if ( m_Body.length() > 0 )
    {
      outbuf += ">";
      outbuf += m_Body;
      outbuf += "</";
      outbuf += m_Name;
      outbuf += ">\n";
    }
  else
    {
      outbuf += " />\n";
    }
}


//
// end KM_xml.cpp
//
