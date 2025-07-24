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

   Source File Name = lsmLobChunkKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
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