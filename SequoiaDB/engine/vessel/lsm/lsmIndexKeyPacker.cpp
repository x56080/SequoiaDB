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

   Source File Name = lsmIndexKeyPacker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#include "vessel/lsm/lsmIndexKeyPacker.h"

namespace engine
{
namespace vessel
{
   lsmIndexKeyPacker::~lsmIndexKeyPacker()
   {
      reset();
   }

   INT32 lsmIndexKeyPacker::packFullKey(const ixmKey &key,
                                        const globalIndexID &idxId,
                                        const orderingWrapper &ow,
                                        const recordID &rid,
                                        UINT64 lsn,
                                        const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;

      reset();
      if (!key.isValid() ||
          !idxId.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _keySize = lsmCalFullDataKeyLen(key.dataSize());
      _keyBuf = (CHAR *)_keyAllocator.malloc(_keySize);
      if (nullptr == _keyBuf)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "out of memory");
         goto error;
      }

      rc = lsmPackIndexFullKey(_keyBuf, _keySize,
                               idxId, ow, key,
                               rid, lsn, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "pack index full key failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 lsmIndexKeyPacker::packFullKey(const lsmKeyEntry &key,
                                        const lsmIndexMeta &meta)
   {
      INT32 rc = SDB_OK;

      reset();
      if (!key.isValid() ||
          !meta.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = packFullKey(key.getKey(),
                       meta.getIdxId(),
                       meta.getOrdering(),
                       key.getRid(),
                       key.getDataLsn(),
                       key.getTransID());
      if (SDB_OK != rc)  
      {
         PD_LOG(PDERROR, "pack full key failed, rc:%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      reset();
      goto done;
   }

   rocksdb::Slice lsmIndexKeyPacker::getFullKeySlice()const
   {
      SDB_ASSERT(nullptr != _keyBuf, "can not be null");
      SDB_ASSERT(0 != _keySize, "can not be zero");
      return rocksdb::Slice(_keyBuf, _keySize);
   }

   void lsmIndexKeyPacker::reset()
   {
      _keySize = 0;
      if (nullptr != _keyBuf)
      {
         _keyAllocator.free(_keyBuf);
         _keyBuf = nullptr;
      }
   }

} // namespace vessel
} // namespace engine