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

   Source File Name = utilLocale.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          2023/03/14  YJF  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_LOCALE_HPP_
#define UTIL_LOCALE_HPP_

#include "core.hpp"
#include "ossTypes.h"

namespace engine
{
   class _utilNumberFacet
   {
   public :
      _utilNumberFacet( CHAR decimalPoint, CHAR thousandsSep, const CHAR *grouping ) ;

      CHAR decimalPoint() const ;

      CHAR thousandsSep() const ;

      const CHAR* grouping() const ;

      ~_utilNumberFacet() ;

   private :
      CHAR _decimalPoint ;
      CHAR _thousandsSep ;
      const CHAR *_grouping ;
   } ;

   typedef _utilNumberFacet utilNumberFacet ;

   const _utilNumberFacet& utilGetDefaultNumberFacet() ;
}

#endif // UTIL_LOCALE_HPP_

