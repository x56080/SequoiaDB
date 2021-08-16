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

   Source File Name = lsmOwnedRecord.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lsm/lsmOwnedRecord.h"
#include "pdTrace.hpp"
#include "vessel/indexDefPage.h"

namespace engine
{
namespace vessel
{
   lsmOwnedRecord::lsmOwnedRecord()
   {}

   lsmOwnedRecord::~lsmOwnedRecord()
   {}

   void lsmOwnedRecord::fini()
   {
      _key.reset();
      _value.reset();
      _mb.release();
   }

   INT32 lsmOwnedRecord::init(const rocksdb::Slice &k,
                              const rocksdb::Slice &v)
   {
      INT32 rc = SDB_OK;
      fini();
      globalIndexID indexId;
      orderingWrapper ow;
      ixmKey key;
      dmsRecordID rid;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      DPS_TRANS_ID transID;
      UINT32 keyDataSize = 0;

      if (k.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!v.empty() && sizeof(lsmIndexValue) != v.size())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = lsmUnpackIndexFullKey(k.data(), k.size(),
                                 indexId, ow, key,
                                 rid, lsn, transID);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to unpack full key:%d", rc);
         goto error;
      }

      keyDataSize = key.dataSize();
      if (MAX_INDEX_DEF_OBJ_SIZE <= keyDataSize)
      {
         PD_LOG(PDERROR, "invalid key data size[%d]", keyDataSize);
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _mb.reserve(keyDataSize);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to reserve mb size:%d", rc);
         goto error;
      }

      ossMemcpy(_mb.getBuffer(), key.data(), keyDataSize);
      key.assign(ixmKey(_mb.getBuffer()));
      _key.shallowCopy(key, rid, lsn, transID);

      if (!v.empty())
      {
         ossMemcpy(&_value, v.data(), v.size());
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 lsmOwnedRecord::copy(const lsmOwnedRecord &o)
   {
      INT32 rc = SDB_OK;
      fini();

      if (!o.isValid())
      {
         goto done;
      }

      rc = _mb.copy(o._mb.getCapacity(), o._mb.getBuffer());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy mb data:%d", rc);
         goto error;
      }

      _key.shallowCopy(ixmKey(_mb.getBuffer()), o._key.getRid(),
                       o._key.getDataLsn(), o._key.getTransID());
      _value = o._value;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine