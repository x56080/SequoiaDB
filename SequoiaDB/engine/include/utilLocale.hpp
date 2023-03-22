/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

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

