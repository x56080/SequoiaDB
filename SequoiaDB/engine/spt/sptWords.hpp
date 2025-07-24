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

   Source File Name = sptWords.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          6/4/2017    TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef SPTWORDS_HPP__
#define SPTWORDS_HPP__

#include "core.hpp"

#include <stdio.h>
#include <vector>
#include <string>

#if defined ( _WINDOWS )
#include <stdint.h>
#endif

using std::vector ;
using std::string ;

namespace engine {

#if defined ( _WINDOWS )
   INT32 sptGBKToUTF8( const std::string& strGBK, std::string &strUTF8 ) ;
   INT32 sptUTF8ToGBK( const std::string &strUTF8, std::string &strGBK ) ;
#endif
   void sdbSplitWords( const string &text, 
                       INT32 wordNum, vector<string> &output ) ;
}
#endif // SPTWORDS_HPP__
