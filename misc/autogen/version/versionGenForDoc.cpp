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

   Source File Name = versionGenForDoc.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "versionGenForDoc.hpp"
#include "ossVer.h"

#define DOC_FIELD_MAJOR    "major"
#define DOC_FIELD_MINOR    "minor"

#if defined (GENERAL_VER_DOC_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( versionGenForDoc, GENERAL_VER_DOC_FILE ) ;
#endif

versionGenForDoc::versionGenForDoc() : _isFinish( false )
{
}

versionGenForDoc::~versionGenForDoc()
{
}

bool versionGenForDoc::hasNext()
{
   return !_isFinish ;
}

int versionGenForDoc::outputFile( int id, fileOutStream &fout,
                                  string &outputPath )
{
   _isFinish = true ;

   outputPath = VERSION_DOC_PATH ;

   fout << "{"
        << endl
        << "    \"" << DOC_FIELD_MAJOR << "\": "
        << SDB_ENGINE_VERISON_CURRENT << ","
        << endl
        << "    \"" << DOC_FIELD_MINOR << "\": "
        << SDB_ENGINE_SUBVERSION_CURRENT
        << endl
        << "}"
        << endl ;

   return 0 ;
}