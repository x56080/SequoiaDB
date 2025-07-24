/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = vergen.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vergen.h"
#include "ossVer.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std ;

#define VER_MAJOR       "major"
#define VER_MINOR       "minor"

static void genDoc()
{
   #define VER_DOC_PATH    "../../doc/config/version.json"
   ofstream fout( VER_DOC_PATH ) ;

   fout << "{" << endl ;
   fout << "    " << "\"" << VER_MAJOR << "\": " << SDB_ENGINE_VERISON_CURRENT << "," << endl ;
   fout << "    " << "\"" << VER_MINOR << "\": " << SDB_ENGINE_SUBVERSION_CURRENT << endl ;
   fout << "}" << endl ;
}

static void genPython()
{
   #define VER_PY_PATH     "../../driver/python/version.py"
   ofstream fout( VER_PY_PATH ) ;

   fout << "# auto-generated, do not edit!!!" << endl;
   fout << "version = '" 
        << SDB_ENGINE_VERISON_CURRENT << "." << SDB_ENGINE_SUBVERSION_CURRENT
        << "'" << endl;
}

VerGen::VerGen()
{
}

VerGen::~VerGen()
{
}

void VerGen::run(bool doc)
{
   if (doc)
   {
      genDoc();
   }
   else
   {
      genPython();
   }
}

