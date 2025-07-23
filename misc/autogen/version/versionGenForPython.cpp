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

   Source File Name = versionGenForPython.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "versionGenForPython.hpp"
#include "ossVer.h"

#if defined (GENERAL_VER_PYTHON_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( versionGenForPython, GENERAL_VER_PYTHON_FILE ) ;
#endif

versionGenForPython::versionGenForPython() : _isFinish( false )
{
}

versionGenForPython::~versionGenForPython()
{
}

bool versionGenForPython::hasNext()
{
   return !_isFinish ;
}

int versionGenForPython::outputFile( int id, fileOutStream &fout,
                                  string &outputPath )
{
   int rc = 0 ;

   if ( id == 0 )
   {
      rc = _genPythonFile( fout, outputPath ) ;
   }

   _isFinish = true ;

   return rc ;
}


int versionGenForPython::_genPythonFile( fileOutStream &fout, string &outputPath )
{
   outputPath = VERSION_PYTHON_PATH ;

   fout << "# auto-generated, do not edit!!!"
        << endl
        << "version = '"
        << SDB_ENGINE_VERISON_CURRENT
        << "." << SDB_ENGINE_SUBVERSION_CURRENT
#ifdef SDB_ENGINE_FIXVERSION_CURRENT
        << "." << SDB_ENGINE_FIXVERSION_CURRENT
#endif
        << "'"
        << endl ;
   return 0 ;
}

