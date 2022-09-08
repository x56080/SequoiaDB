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

   Source File Name = lsmIndexMetaKeyBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/11/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmIndexMetaKeyBuilder.h"

namespace engine
{
namespace vessel
{
   rocksdb::Slice lsmIndexMetaKeyBuilder::build(UINT32 csLid)
   {
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data);
      return rocksdb::Slice(_data, sizeof(UINT32));
   }

   rocksdb::Slice lsmIndexMetaKeyBuilder::build(UINT32 csLid,
                                                UINT32 clLid)
   {
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data);
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data + sizeof(UINT32));
      return rocksdb::Slice(_data, sizeof(UINT32) + sizeof(UINT32));
   }

   rocksdb::Slice lsmIndexMetaKeyBuilder::build(const globalIndexID &indexId)
   {
      keyStringCoder coder;
      coder.encodeGlobalIndexId(indexId, FALSE, _data);
      return rocksdb::Slice(_data, keyStringCoder::INDEX_ID_ENCODEING_SIZE);
   }

   rocksdb::Slice lsmIndexMetaKeyBuilder::buildUpperKey(UINT32 csLid)
   {
      SDB_ASSERT(OSS_UINT32_MAX != csLid , "out of bound");
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid + 1, FALSE, _data);
      return rocksdb::Slice(_data, sizeof(UINT32));
   }

   rocksdb::Slice lsmIndexMetaKeyBuilder::buildUpperKey(UINT32 csLid,
                                                        UINT32 clLid)
   {
      SDB_ASSERT(OSS_UINT32_MAX != clLid, "out of bound");
      keyStringCoder coder;
      coder.encodeUnsignedNative<UINT32>(csLid, FALSE, _data);
      coder.encodeUnsignedNative<UINT32>(clLid + 1, FALSE, _data + sizeof(UINT32));
      return rocksdb::Slice(_data, sizeof(UINT32) + sizeof(UINT32));
   }

   rocksdb::Slice lsmIndexMetaKeyBuilder::buildUpperKey(const globalIndexID &indexId)
   {
      SDB_ASSERT(OSS_UINT32_MAX != indexId.getLogicalIndexID(), "out of bound");
      keyStringCoder coder;
      coder.encodeGlobalIndexId(indexId, TRUE, _data);
      return rocksdb::Slice(_data, keyStringCoder::INDEX_ID_ENCODEING_SIZE);
   }

} // namespace vessel
} // namespace engine

