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

   Source File Name = lsmLobChunkKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/
#ifndef VESSEL_LSM_LOB_CHUNK_KEY_H_
#define VESSEL_LSM_LOB_CHUNK_KEY_H_

#include "dms.hpp"
#include "rocksdb/slice.h"
#include "rocksdb/comparator.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   constexpr UINT32 LSM_LOB_CHUNK_KEY_SIZE = 24;
   class lsmLobChunkKey : public SDBObject
   {
      public:
         lsmLobChunkKey() = default;
         ~lsmLobChunkKey() = default;
         lsmLobChunkKey(UINT32 csid,
                        UINT32 clid,
                        const bson::OID &oid,
                        UINT32 chunkId):
         _csId(csid),
         _clId(clid),
         _oid(oid),
         _chunkId(chunkId)
         {}

         lsmLobChunkKey(const lsmLobChunkKey &o):
         _csId(o._csId),
         _clId(o._clId),
         _oid(o._oid),
         _chunkId(o._chunkId)
         {}

         lsmLobChunkKey &operator=(const lsmLobChunkKey &o)
         {
            _csId = o._csId;
            _clId = o._clId;
            _oid = o._oid;
            _chunkId = o._chunkId;
            return *this;
         }
      
      public:
         OSS_INLINE void set(UINT32 csid,
                             UINT32 clid,
                             const bson::OID &oid,
                             UINT32 chunkId)
         {
            _csId = csid;
            _clId = clid;
            _oid = oid;
            _chunkId = chunkId;
         }

         OSS_INLINE void reset()
         {
            _csId = DMS_INVALID_LOGICCSID;
            _clId = DMS_INVALID_LOGICCLID;
            _oid.clear();
            _chunkId = 0;
         }

         OSS_INLINE BOOLEAN isValid()const
         {
            return _csId != DMS_INVALID_LOGICCSID &&
                   _clId != DMS_INVALID_LOGICCLID &&
                   _oid.isSet();
         }

         OSS_INLINE rocksdb::Slice getSlice()const
         {
            return rocksdb::Slice((const CHAR *)this, sizeof(lsmLobChunkKey));
         }

         void setAsLowKey(UINT32 csid);
         void setAsLowKey(UINT32 csid, UINT32 clid);
         void setAsUpKey(UINT32 csid);
         void setAsUpKey(UINT32 csid, UINT32 clid);

         INT32 compare(const lsmLobChunkKey &key)const;

      private:
         UINT32 _csId = DMS_INVALID_LOGICCSID;
         UINT32 _clId = DMS_INVALID_LOGICCLID;
         bson::OID _oid;
         UINT32 _chunkId = 0;
   }; // class lsmLobChunkKey
   static_assert(LSM_LOB_CHUNK_KEY_SIZE == sizeof(lsmLobChunkKey), "invalid size");
#pragma pack()
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_LOB_CHUNK_KEY_H_