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

