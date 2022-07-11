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

   Source File Name = orderingWrapper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_ORDERING_WRAPPER_H_
#define VESSEL_ORDERING_WRAPPER_H_

#include "core.hpp"
#include "oss.hpp"
#include "../bson/ordering.h"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   class orderingWrapper : public SDBObject
   {
      public:
         orderingWrapper() = default;
         ~orderingWrapper() = default;
         explicit orderingWrapper(UINT32 b, UINT32 n):
         _bits(b),
         _nkeys(n)
         {
            SDB_ASSERT(0 < _nkeys, "can not be zero");
            SDB_ASSERT(_nkeys <= 32, "can not be out of bound");
         }
      public:
         const bson::Ordering &toBsonOrdering()const
         {
            SDB_ASSERT(0 < _nkeys, "can not be zero");
            SDB_ASSERT(_nkeys <= 32, "can not be out of bound");
            return *((const bson::Ordering *)this);
         }

         UINT32 getNkeys() const
         {
            return _nkeys;
         }

      private:
         UINT32 _bits = 0;
         UINT32 _nkeys = 0;
   };//class orderingWrapper

   static_assert(sizeof(orderingWrapper) == sizeof(bson::Ordering), "must be same");
}//namespace vessel
}//namespace engine

#endif//VESSEL_ORDERING_WRAPPER_H_
