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

   Source File Name = lsmLobChunkKey.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

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
