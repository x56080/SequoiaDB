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

   Source File Name = optGenForC.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "optGenForC.hpp"

#if defined (GENERAL_OPT_C_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( optGenForC, GENERAL_OPT_C_FILE ) ;
#endif

optGenForC::optGenForC() : _isFinish( false )
{
}

optGenForC::~optGenForC()
{
}

bool optGenForC::hasNext()
{
   return !_isFinish ;
}

int optGenForC::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_optionList.size() ;
   string headerDesc ;

   outputPath = OPT_C_FILE_PATH ;

   rc = _buildStatement( 1, headerDesc ) ;
   if ( rc )
   {
      printLog( PD_ERROR ) << "failed to build file header desc" << endl ;
      rc = 1 ;
      goto error ;
   }

   fout << std::left << headerDesc
        << "\n"
           "#ifndef PMDOPTIONS_H_\n"
           "#define PMDOPTIONS_H_\n" ;

   for ( i = 0; i < listSize; ++i )
   {
      const OPTION_ELE& optEle = _optionList[i] ;

      fout << "#define " << std::setw( 64 )
           << optEle.nametag << "\"" << optEle.longtag << "\"\n" ;
   }

   fout << "#endif\n" ;

done:
   _isFinish = true ;
   return rc ;
error:
   goto done ;
}