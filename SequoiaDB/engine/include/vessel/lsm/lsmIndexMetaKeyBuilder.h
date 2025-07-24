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

   Source File Name = lsmIndexMetaKeyBuilder.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/11/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_INDEX_META_KEY_BUILDER_H_
#define VESSEL_LSM_INDEX_META_KEY_BUILDER_H_

#include "oss.hpp"
#include "vessel/globalIndexID.h"
#include "vessel/keyStringCoder.h"
#include "rocksdb/slice.h"

namespace engine
{
namespace vessel
{
   class lsmIndexMetaKeyBuilder : public SDBObject
   {
      public:
         lsmIndexMetaKeyBuilder() = default;
         ~lsmIndexMetaKeyBuilder() = default;
         lsmIndexMetaKeyBuilder(const lsmIndexMetaKeyBuilder &key) = delete;
         lsmIndexMetaKeyBuilder &operator=(const lsmIndexMetaKeyBuilder &key) = delete;

      public:
         rocksdb::Slice build(UINT32 csLid);
         rocksdb::Slice build(UINT32 csLid, UINT32 clLid);
         rocksdb::Slice build(const globalIndexID &indexId);

      public:
         rocksdb::Slice buildUpperKey(UINT32 csLid);
         rocksdb::Slice buildUpperKey(UINT32 csLid, UINT32 clLid);
         rocksdb::Slice buildUpperKey(const globalIndexID &indexId);

      private:
         /// big endian store
         /// 0-3: CS Logical ID
         /// 4-7: CL Logical ID
         /// 8-11: Index Logical ID
         CHAR _data[keyStringCoder::INDEX_ID_ENCODEING_SIZE] = {};
   }; // class lsmIndexMetaKeyBuilder

} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_INDEX_META_KEY_BUILDER_H_
