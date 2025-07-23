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

   Source File Name = rcGenForDoc.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who         Description
   ====== =========== =========== =============================================
          23/07/2025  fangjiabin  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rcGenForDoc.hpp"

#if defined (GENERAL_RC_DOC_FILE)
IMPLEMENT_GENERATOR_AUTO_REGISTER( rcGenForDoc, GENERAL_RC_DOC_FILE ) ;
#endif

rcGenForDoc::rcGenForDoc() : _isFinish( false )
{
}

rcGenForDoc::~rcGenForDoc()
{
}

bool rcGenForDoc::hasNext()
{
   return !_isFinish ;
}

int rcGenForDoc::outputFile( int id, fileOutStream &fout, string &outputPath )
{
   int rc = 0 ;
   int i  = 0 ;
   int listSize = (int)_rcInfoList.size() ;
   string headerDesc ;

   outputPath = RC_MD_FILE_PATH ;

   fout << std::left
        << "| Name | Error Code | Description |" << endl
        << "| --- | --- | --- |" << endl ;

   for ( i = 0; i < listSize; ++i )
   {
      const RCInfo& rcInfo = _rcInfoList[i] ;

      if ( rcInfo.reserved )
      {
         continue;
      }

      fout << "| "  << rcInfo.name
           << " | " << rcInfo.value
           << " | " << rcInfo.getDesc( LANG_CN )
           << " |"  << endl ;
   }

   _isFinish = true ;
   return rc ;
}