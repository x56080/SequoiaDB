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

   Source File Name = lsmHitComparator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/keyString.h"
#include "rocksdb/comparator.h"
#include "vessel/sliceTransfer.h"

namespace engine
{
namespace vessel
{
   class lsmHitComparator : public rocksdb::Comparator
   {
      public:
         virtual INT32 Compare(const rocksdb::Slice & a,
                               const rocksdb::Slice & b)const override
         {
         #if defined(_DEBUG)
            SDB_ASSERT(!a.empty() && !b.empty(), "can not be empty");
         #endif
            return keyString::compareCoding(a.size(), a.data(),
                                            b.size(), b.data());
         }

         virtual const char* Name() const override { return "sdb.lsmHitComparator"; }
         void FindShortestSeparator(std::string*,const rocksdb::Slice&)const override{}
         void FindShortSuccessor(std::string*) const override {}
   };//class lsmHitComparator

   const rocksdb::Comparator* getHitComparator()
   {
      static lsmHitComparator comparator;
      return &comparator;
   }
} // namespace vessel

} // namespace engine
