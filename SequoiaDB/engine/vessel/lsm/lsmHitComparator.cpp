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

   Source File Name = lsmHitComparator.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
