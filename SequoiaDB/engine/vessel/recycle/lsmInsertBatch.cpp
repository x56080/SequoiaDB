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

   Source File Name = lsmInsertBatch.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmInsertBatch.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/threadContext.h"

namespace engine
{
namespace vessel
{
   lsmInsertBatch::~lsmInsertBatch()
   {
      clear();
   }

   INT32 lsmInsertBatch::put(const lsmIndexMeta &meta,
                             const lsmPureKeyEntry &ke,
                             const lsmIndexValue *value)
   {
      INT32 rc = SDB_OK;
      UINT32 keyObjSize = 0;
      UINT32 fullKeySize = 0;
      UINT32 bufferSize = 0;
      CHAR *buffer = nullptr;
      rocksdb::Slice k, v;
      rocksdb::Status status;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");

      if (OSS_UNLIKELY(!meta.isValid() ||
                       !ke.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      keyObjSize = ke.getKey().dataSize();
      fullKeySize = lsmCalFullDataKeyLen(keyObjSize);
      bufferSize = fullKeySize;

      buffer = tc->allocateBuffer(bufferSize);
      if (nullptr == buffer)
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      rc = lsmPackIndexFullKey(buffer,
                               bufferSize,
                               meta.getIdxId(),
                               meta.getOrdering(),
                               ke.getKey(),
                               ke.getRid(),
                               ke.getDataLsn(),
                               ke.getTransID());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to pack full key:%d", rc);
         goto error;
      }
      k = rocksdb::Slice(buffer, fullKeySize);

      if (nullptr != value)
      {
         v = rocksdb::Slice((const CHAR *)value, sizeof(lsmIndexValue));

      }

      status = _batch.Put(k, v);
      if (!status.ok())
      {
         PD_LOG(PDERROR, "failed to push slices to batch:%s",
                status.getState());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
   done:
      if (nullptr != buffer)
      {
         tc->releaseBuffer(buffer);
      }
      return rc;
   error:
      goto done;
   }

   void lsmInsertBatch::clear()
   {
      _batch.Clear();
      return;
   }
}//namespace vessel
}//namespace engine