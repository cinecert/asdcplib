/*
Copyright (c) 2004-2009, John Hurst
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
/*! \file    path-test.cpp
  \version $Id$
  \brief     test harness for path manglers defined in KM_fileio.h
*/


#include <KM_fileio.h>
#include <iostream>

using namespace std;
using namespace Kumu;


//
int
main(int argc, const char** argv)
{

  string Path_1 = "path-test.cpp";
  assert(PathExists(Path_1));
  assert(PathIsFile(Path_1));
  assert(!PathIsDirectory(Path_1));

  string Path_2 = ".";
  assert(PathExists(Path_2));
  assert(!PathIsFile(Path_2));
  assert(PathIsDirectory(Path_2));
  
  string Path_3 = "/foo/bar/baz.buz"; // must have 3 elements
  PathCompList_t PathList_3;
  PathToComponents(Path_3, PathList_3);

  assert(PathList_3.size() == 3);

  string Path_4 = ComponentsToPath(PathList_3);
  string Path_5 = PathMakeAbsolute(Path_4);

  fprintf(stderr, "PathMakeAbsolute in: %s\n", Path_4.c_str());
  fprintf(stderr, "PathMakeAbsolute out: %s\n", Path_5.c_str());

  string Path_6 = ComponentsToAbsolutePath(PathList_3);
  assert(Path_3 == Path_6);
  assert(PathsAreEquivalent(Path_3, Path_6));
  assert(!PathsAreEquivalent(Path_3, Path_4));

  assert(!PathHasComponents(PathList_3.back()));
  assert(PathHasComponents(Path_3));

  assert(!PathIsAbsolute(Path_4));
  assert(PathIsAbsolute(Path_5));
  assert(PathMakeLocal(Path_3, "/foo") == "bar/baz.buz");

  assert(PathsAreEquivalent("/foo/bar/baz", "/foo/bar/./baz"));
  assert(PathsAreEquivalent("/foo/baz", "/foo/bar/../baz"));

  assert(PathBasename(Path_3) == "baz.buz");
  assert(PathDirname(Path_3) == "/foo/bar");
  assert(PathDirname("/foo") == "/");

  assert(PathGetExtension(Path_3) == "buz");
  assert(PathGetExtension("foo") == "");
  assert(PathSetExtension("foo.bar", "") == "foo");
  assert(PathSetExtension(Path_3, "xml") == "baz.xml");

  string Path_7 = "//tmp///////fooo";

  PathCompList_t PathList_7;
  PathToComponents(Path_7, PathList_7);
  for ( PathCompList_t::const_iterator i = PathList_7.begin(); i != PathList_7.end(); i++ )
    fprintf(stderr, "xx: \"%s\"\n", i->c_str());
  assert(PathsAreEquivalent(PathMakeLocal(PathMakeCanonical(Path_7), "/tmp"), "fooo"));

  string Path_8 = "tmp/foo/bar/ack";
  CreateDirectoriesInPath(Path_8);
  assert(PathExists(Path_8));
  DeletePath(Path_8);
  assert(!PathExists(Path_8));

  PathList_t InList, OutList;
  InList.push_back("tmp");
  InList.push_back("Darwin");
  InList.push_back(".");

  cerr << "----------------------------------" << endl;
  FindInPaths(PathMatchAny(), InList, OutList);
  PathList_t::iterator pi;

  if ( false )
    {
      for ( pi = OutList.begin(); pi != OutList.end(); pi++ )
	cerr << *pi << endl;
    }
  else
    {
      cerr << OutList.size() << ( ( OutList.size() == 1 ) ? " file" : " files" ) << endl;
    }

  cerr << "----------------------------------" << endl;
  OutList.clear();
  FindInPaths(PathMatchRegex("^[A-J].*\\.h$"), InList, OutList);

  for ( pi = OutList.begin(); pi != OutList.end(); pi++ )
    cerr << *pi << endl;

  cerr << "----------------------------------" << endl;
  OutList.clear();
  FindInPaths(PathMatchGlob("*.h"), InList, OutList);

  for ( pi = OutList.begin(); pi != OutList.end(); pi++ )
    cerr << *pi << endl;

  cerr << "----------------------------------" << endl;

  fsize_t free_space, total_space;
  FreeSpaceForPath("/", free_space, total_space);
  cerr << "Free space: " << free_space << endl;
  cerr << "Total space: " << total_space << endl;
  cerr << "Used space: " << ( (total_space - free_space ) / float(total_space) ) << endl;

  cerr << "OK" << endl;

  return 0;
}

//
// end path-test.cpp
//
