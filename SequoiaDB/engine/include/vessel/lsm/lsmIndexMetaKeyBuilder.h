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
