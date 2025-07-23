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

   Source File Name = utilLZWDictionary.cpp

   Descriptive Name = Implementation of LZW dictionary.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilDictionary.hpp"
#include "utilLZWDictionary.hpp"

namespace engine
{
   void getDictionaryDetail( void *dictionary,
                             utilDictionaryDetail &detail )
   {
      utilDictHead *head = (utilDictHead*)dictionary ;
      utilLZWDictionary dict ;

      SDB_ASSERT( UTIL_DICT_LZW == head->_type, "Dictionary type invalid" ) ;
      SDB_ASSERT( UTIL_LZW_DICT_VERSION == head->_version,
                  "Ditionary version is invalid" ) ;

      dict.attach( dictionary ) ;

      detail._maxCode = dict.getMaxValidCode();
      detail._codeSize = dict.getCodeSize();
      detail._varLenCompEnable = 1 ;
   }
}

