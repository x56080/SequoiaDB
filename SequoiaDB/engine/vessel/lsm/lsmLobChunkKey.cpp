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

   Source File Name = lsmLobChunkKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmLobChunkKey.h"
#include "rocksdb/comparator.h"

namespace engine
{
namespace vessel
{
   void lsmLobChunkKey::setAsLowKey(UINT32 csid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csid, "can not be invalid");
      set(csid, 0, bson::OID(), 0);
   }

   void lsmLobChunkKey::setAsLowKey(UINT32 csid, UINT32 clid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clid, "can not be invalid");
      set(csid, clid, bson::OID(), 0);
   }

   void lsmLobChunkKey::setAsUpKey(UINT32 csid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csid, "can not be invalid");
      set(csid + 1, 0, bson::OID(), 0);
   }

   void lsmLobChunkKey::setAsUpKey(UINT32 csid, UINT32 clid)
   {
      SDB_ASSERT(DMS_INVALID_LOGICCSID != csid, "can not be invalid");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != clid, "can not be invalid");
      set(csid, clid + 1, bson::OID(), 0);
   }

   INT32 lsmLobChunkKey::compare(const lsmLobChunkKey &key)const
   {
      INT32 res = 0;
      INT32 oidcmp = 0;

      if (_csId < key._csId)
      {
         res = -1;
         goto done;
      }
      else if (_csId > key._csId)
      {
         res = 1;
         goto done;
      }

      if (_clId < key._clId)
      {
         res = -1;
         goto done;
      }
      else if (_clId > key._clId)
      {
         res = 1;
         goto done;
      }

      oidcmp = _oid.compare(key._oid);
      if (oidcmp > 0)
      {
         res = 1;
         goto done;
      }
      else if (oidcmp < 0)
      {
         res = -1;
         goto done;
      }

      if (_chunkId < key._chunkId)
      {
         res = -1;
         goto done;
      }
      else if (_chunkId > key._chunkId)
      {
         res = 1;
         goto done;
      }

   done:
      return res;
   }

} // namespace vessel

} // namespace engine
