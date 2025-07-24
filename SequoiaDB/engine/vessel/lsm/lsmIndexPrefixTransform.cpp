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
            return toRocksdbSlice(ks.getKeyElementsSlice());
         }
         virtual bool InDomain(const rocksdb::Slice &key) const override
         {
            keyString ks(key.size(), key.data());
            return ks.isValid() && ks.hasKeyElements();
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