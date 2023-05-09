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

   Source File Name = utilDEKFetcher.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/30/2023  ZHY Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_DEK_FETCHER_HPP_
#define UTIL_DEK_FETCHER_HPP_
#include "oss.hpp"
#include "../bson/bsonobj.h"
#include "ossGMCrypto.hpp"

namespace engine
{
   class _utilDEKFetcher : public SDBObject
   {
      public:
         virtual ~_utilDEKFetcher() {} ;
         virtual INT32 fetch( ossSM4Key dek ) = 0 ;
   } ;

   typedef _utilDEKFetcher utilDEKFetcher ;
} // namespace engine

#endif