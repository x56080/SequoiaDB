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

   Source File Name = utilLocale.cpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2023/03/21  YJF  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilLocale.hpp"

namespace engine
{
   _utilNumberFacet::_utilNumberFacet( CHAR decimalPoint, CHAR thousandsSep,
                                       const CHAR *grouping )
      : _decimalPoint( decimalPoint ),
        _thousandsSep( thousandsSep ),
        _grouping( grouping )
   {}

   CHAR _utilNumberFacet::decimalPoint() const
   {
      return _decimalPoint ;
   }

   CHAR _utilNumberFacet::thousandsSep() const
   {
      return _thousandsSep ;
   }

   const CHAR* _utilNumberFacet::grouping() const
   {
      return _grouping ;
   }

   _utilNumberFacet::~_utilNumberFacet()
   {}

   const _utilNumberFacet& utilGetDefaultNumberFacet()
   {
      static _utilNumberFacet defaultNumberFacet( '.', ',', "\x03" ) ;
      return defaultNumberFacet ;
   }
}

