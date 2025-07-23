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

   Source File Name = rcGenForCPP.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rcGenForCPP.hpp"

#if defined (GENERAL_RC_CPP_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForCPP, GENERAL_RC_CPP_FILE ) ;
#endif

rcGenForCPP::rcGenForCPP() : _isFinish( false )
{
}

rcGenForCPP::~rcGenForCPP()
{
}

bool rcGenForCPP::hasNext()
{
   return !_isFinish ;
}

int rcGenForCPP::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_CPP_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left
        << headerDesc << "\n"
        << "#include \"ossErr.h\"\n\n"
        << "const CHAR* getErrDesp ( INT32 errCode )\n"
           "{\n"
           "   INT32 code = -errCode ;\n"
           "   const static CHAR* errDesp[] =\n"
           "   {\n"
           "      \"Succeed\",\n" ;

   for (int i = 0; i < listSize; ++i )
   {
      fout << "      \"" << _rcInfoList[i].getDesc( _lang ) << "\""
           << ( ( i + 1 < listSize ) ? ",\n" : "\n" ) ;

   }

   fout << "   } ;\n"
           "\n"
           "   if ( code < 0 || (UINT32)code >= ( sizeof ( errDesp ) / sizeof ( CHAR* ) ) )\n"
           "   {\n"
           "      return \"unknown error\" ;\n"
           "   }\n"
           "\n"
           "   return errDesp[code] ;\n"
           "}\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}