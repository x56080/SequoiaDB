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

   Source File Name = mthCommon.hpp

   Descriptive Name = Method Common Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Method component.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/12/2013  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MTHCOMMON_HPP__
#define MTHCOMMON_HPP__

#include "core.hpp"
#include <vector>

namespace engine
{

   INT32 mthAppendString ( CHAR **ppStr, INT32 &bufLen,
                           INT32 strLen, const CHAR *newStr,
                           INT32 newStrLen, INT32 *pMergedLen = NULL ) ;

   INT32 mthDoubleBufferSize ( CHAR **ppStr, INT32 &bufLen ) ;


   INT32 mthCheckFieldName( const CHAR *pField, INT32 &dollarNum ) ;

   BOOLEAN mthCheckUnknowDollar( const CHAR *pField,
                                 std::vector<INT64> *dollarList ) ;

   INT32 mthConvertSubElemToNumeric( const CHAR *desc,
                                     INT32 &number ) ;

}

#endif //MTHCOMMON_HPP__
