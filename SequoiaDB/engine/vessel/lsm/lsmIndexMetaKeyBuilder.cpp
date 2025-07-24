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

   Source File Name = lsmIndexMetaKeyBuilder.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/11/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
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

