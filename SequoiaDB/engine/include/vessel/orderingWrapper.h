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

   Source File Name = orderingWrapper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
