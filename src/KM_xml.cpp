/*
Copyright (c) 2005-2008, John Hurst
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
#include <KM_log.h>
#include <stack>
#include <map>

#ifdef HAVE_EXPAT
#include <expat.h>
#endif

using namespace Kumu;


class ns_map : public std::map<std::string, XMLNamespace*>
{
public:
  ns_map() {}
  ~ns_map()
  {
    ns_map::iterator ni = begin();

    while (ni != end() )
      {
	//	fprintf(stderr, "deleting namespace %s:%s\n", ni->second->Prefix().c_str(), ni->second->Name().c_str());
	delete ni->second;
	ni++;
      }
  }
};


Kumu::XMLElement::XMLElement(const char* name) : m_Namespace(0), m_NamespaceOwner(0)
{
  m_Name = name;
}

Kumu::XMLElement::~XMLElement()
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    delete *i;

  if ( m_NamespaceOwner != 0 )
    delete (ns_map*)m_NamespaceOwner;
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
Kumu::XMLElement::AddChild(Kumu::XMLElement* element)
{
  m_ChildList.push_back(element); // takes posession!
  return element;
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
void
Kumu::XMLElement::AppendBody(const std::string& value)
{
  m_Body += value;
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
Kumu::XMLElement::Render(std::string& outbuf) const
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
Kumu::XMLElement::RenderElement(std::string& outbuf, ui32_t depth) const
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

  outbuf += ">";

  // body contents and children
  if ( ! m_ChildList.empty() )
    {
      outbuf += "\n";

      // render body
      if ( m_Body.length() > 0 )
	outbuf += m_Body;

      for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
	(*i)->RenderElement(outbuf, depth + 1);

      add_spacer(outbuf, depth);
    }
  else if ( m_Body.length() > 0 )
    {
      outbuf += m_Body;
    }

  outbuf += "</";
  outbuf += m_Name;
  outbuf += ">\n";
}

//
bool
Kumu::XMLElement::HasName(const char* name) const
{
  if ( name == 0 || *name == 0 )
    return false;

  return (m_Name == name);
}


void
Kumu::XMLElement::SetName(const char* name)
{
  if ( name != 0)
    m_Name = name;
}

//
const char*
Kumu::XMLElement::GetAttrWithName(const char* name) const
{
  for ( Attr_i i = m_AttrList.begin(); i != m_AttrList.end(); i++ )
    {
      if ( (*i).name == name )
	return (*i).value.c_str();
    }

  return 0;
}

//
Kumu::XMLElement*
Kumu::XMLElement::GetChildWithName(const char* name) const
{
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    {
      if ( (*i)->HasName(name) )
	return *i;
    }

  return 0;
}

//
const Kumu::ElementList&
Kumu::XMLElement::GetChildrenWithName(const char* name, ElementList& outList) const
{
  assert(name);
  for ( Elem_i i = m_ChildList.begin(); i != m_ChildList.end(); i++ )
    {
      if ( (*i)->HasName(name) )
	outList.push_back(*i);

      if ( ! (*i)->m_ChildList.empty() )
	(*i)->GetChildrenWithName(name, outList);
    }

  return outList;
}

//----------------------------------------------------------------------------------------------------

#ifdef HAVE_EXPAT


class ExpatParseContext
{
  KM_NO_COPY_CONSTRUCT(ExpatParseContext);
  ExpatParseContext();
public:
  ns_map*                  Namespaces;
  std::stack<XMLElement*>  Scope;
  XMLElement*              Root;

  ExpatParseContext(XMLElement* root) : Root(root) {
    Namespaces = new ns_map;
    assert(Root);
  }

  ~ExpatParseContext() {}
};

// expat wrapper functions
// 
static void
xph_start(void* p, const XML_Char* name, const XML_Char** attrs)
{
  assert(p);  assert(name);  assert(attrs);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  XMLElement* Element;

  const char* ns_root = name;
  const char* local_name = strchr(name, '|');
  if ( local_name != 0 )
    name = local_name + 1;

  if ( Ctx->Scope.empty() )
    {
      Ctx->Scope.push(Ctx->Root);
    }
  else
    {
      Element = Ctx->Scope.top();
      Ctx->Scope.push(Element->AddChild(name));
    }

  Element = Ctx->Scope.top();
  Element->SetName(name);

  // map the namespace
  std::string key;
  if ( ns_root != name )
    key.assign(ns_root, name - ns_root - 1);
  
  ns_map::iterator ni = Ctx->Namespaces->find(key);
  if ( ni != Ctx->Namespaces->end() )
    Element->SetNamespace(ni->second);

  // set attributes
  for ( int i = 0; attrs[i] != 0; i += 2 )
    {
      if ( ( local_name = strchr(attrs[i], '|') ) == 0 )
	local_name = attrs[i];
      else
	local_name++;

      Element->SetAttr(local_name, attrs[i+1]);
    }
}

//
static void
xph_end(void* p, const XML_Char* name)
{
  assert(p);  assert(name);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  Ctx->Scope.pop();
}

//
static void
xph_char(void* p, const XML_Char* data, int len)
{
  assert(p);  assert(data);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;

  if ( len > 0 )
    {
      std::string tmp_str;
      tmp_str.assign(data, len);
      Ctx->Scope.top()->AppendBody(tmp_str);
    }
}

//
void
xph_namespace_start(void* p, const XML_Char* ns_prefix, const XML_Char* ns_name)
{
  assert(p);  assert(ns_name);
  ExpatParseContext* Ctx = (ExpatParseContext*)p;
  
  if ( ns_prefix == 0 )
    ns_prefix = "";

  ns_map::iterator ni = Ctx->Namespaces->find(ns_name);

  if  ( ni != Ctx->Namespaces->end() )
    {
      if ( ni->second->Name() != std::string(ns_name) )
	{
	  DefaultLogSink().Error("Duplicate prefix: %s\n", ns_prefix);
	  return;
	}
    }
  else
    {
      XMLNamespace* Namespace = new XMLNamespace(ns_prefix, ns_name);
      Ctx->Namespaces->insert(ns_map::value_type(ns_name, Namespace));
    }
}

//
bool
Kumu::XMLElement::ParseString(const std::string& document)
{
  XML_Parser Parser = XML_ParserCreateNS("UTF-8", '|');

  if ( Parser == 0 )
    {
      DefaultLogSink().Error("Error allocating memory for XML parser.\n");
      return false;
    }

  ExpatParseContext Ctx(this);
  XML_SetUserData(Parser, (void*)&Ctx);
  XML_SetElementHandler(Parser, xph_start, xph_end);
  XML_SetCharacterDataHandler(Parser, xph_char);
  XML_SetStartNamespaceDeclHandler(Parser, xph_namespace_start);

  if ( ! XML_Parse(Parser, document.c_str(), document.size(), 1) )
    {
      XML_ParserFree(Parser);
      DefaultLogSink().Error("XML Parse error on line %d: %s\n",
			     XML_GetCurrentLineNumber(Parser),
			     XML_ErrorString(XML_GetErrorCode(Parser)));
      return false;
    }

  XML_ParserFree(Parser);

  if ( ! Ctx.Namespaces->empty() )
    m_NamespaceOwner = (void*)Ctx.Namespaces;

  return true;
}

//------------------------------------------------------------------------------------------

struct xph_test_wrapper
{
  XML_Parser Parser;
  bool  Status;

  xph_test_wrapper(XML_Parser p) : Parser(p), Status(false) {}
};

// expat wrapper functions, map callbacks to IASAXHandler
// 
static void
xph_test_start(void* p, const XML_Char* name, const XML_Char** attrs)
{
  assert(p);
  xph_test_wrapper* Wrapper = (xph_test_wrapper*)p;

  Wrapper->Status = true;
  XML_StopParser(Wrapper->Parser, false);
}


//
bool
Kumu::StringIsXML(const char* document, ui32_t len)
{
  if ( document == 0 )
    return false;

  if ( len == 0 )
    len = strlen(document);

  XML_Parser Parser = XML_ParserCreate("UTF-8");

  if ( Parser == 0 )
    {
      DefaultLogSink().Error("Error allocating memory for XML parser.\n");
      return false;
    }

  xph_test_wrapper Wrapper(Parser);
  XML_SetUserData(Parser, (void*)&Wrapper);
  XML_SetStartElementHandler(Parser, xph_test_start);

  XML_Parse(Parser, document, len, 1);
  XML_ParserFree(Parser);
  return Wrapper.Status;
}

#else // no XML parser support

//
bool
Kumu::XMLElement::ParseString(const std::string& document)
{
  DefaultLogSink().Error("asdcplib compiled without XML parser support.\n");
  return false;
}

//
bool
Kumu::StringIsXML(const char* document, ui32_t len)
{
  DefaultLogSink().Error("Kumu compiled without XML parser support.\n");
  return false;
}

#endif


//
// end KM_xml.cpp
//
