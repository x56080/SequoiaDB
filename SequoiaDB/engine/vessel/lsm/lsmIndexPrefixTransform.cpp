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

   Source File Name = lsmIndexPrefixTransform.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/23/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/keyString.h"
#include "vessel/sliceTransfer.h"
#include "rocksdb/slice_transform.h"

namespace engine
{
namespace vessel
{
   class lsmIndexPrefixTransform : public rocksdb::SliceTransform
   {
      public:
         virtual rocksdb::Slice Transform(const rocksdb::Slice &key) const override
         {
            keyString ks(key.size(), key.data());
            SDB_ASSERT(ks.isValid(), "can not be invalid");
            return toRocksdbSlice(ks.getFilterSlice());
         }
         virtual bool InDomain(const rocksdb::Slice &key) const override
         {
            keyString ks(toSlice(key));
            return ks.isValid() && ks.hasKeyBody();
         }

      public:
         virtual const CHAR *Name() const override {return "sdb.lsmIndexPrefixTransform";}
   }; // class lsmIndexPrefixTransform

   const rocksdb::SliceTransform *getLsmIndexPrefixTransform()
   {
      return new(std::nothrow) lsmIndexPrefixTransform();
   }

} // namespace vessel
} // namespace engine