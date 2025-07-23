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

   Source File Name = rcGenForCS.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rcGenForCS.hpp"

#if defined (GENERAL_RC_CS_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForCS, GENERAL_RC_CS_FILE ) ;
#endif

rcGenForCS::rcGenForCS() : _isFinish( false )
{
}

rcGenForCS::~rcGenForCS()
{
}

bool rcGenForCS::hasNext()
{
   return !_isFinish ;
}

int rcGenForCS::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_CS_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc << "\n" ;

   fout << std::left
        << "namespace SequoiaDB\n"
           "{\n"
           "    class Errors\n"
           "    {\n"
           "        public enum errors : int\n"
           "        {\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "            "
           << setw(_maxFieldWidth + 2) << _rcInfoList[i].name << " = "
           << _rcInfoList[i].value
           << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;
   }

   fout << "        } ;\n"
           "\n"
           "        public static readonly string[] descriptions = {\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      fout << "            \"" << _rcInfoList[i].getDesc( _lang ) << "\""
           << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;
   }

   fout << "        } ;\n"
           "    }\n"
           "}" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}