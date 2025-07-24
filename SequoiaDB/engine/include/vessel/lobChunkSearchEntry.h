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

   Source File Name = lobChunkSearchEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOB_CHUNK_SEARCH_ENTRY_H_
#define VESSEL_LOB_CHUNK_SEARCH_ENTRY_H_

#include "vessel/lobChunkKey.h"

namespace engine
{
namespace vessel
{
   class lobChunkSearchEntry : public SDBObject
   {
      public:
         lobChunkSearchEntry(){}
         ~lobChunkSearchEntry(){}
         lobChunkSearchEntry(const lobChunkSearchEntry &o):
         _key(o._key),
         _hash(o._hash),
         _lclid(o._lclid),
         _chainPos(o._chainPos){}
         explicit lobChunkSearchEntry(const lobChunkKey &key,
                                      UINT32 lclid,
                                      UINT16 chainPos=0):
         _key(key),
         _lclid(lclid),
         _chainPos(chainPos)
         {
            if (DMS_INVALID_LOGICCLID != _lclid &&
                _key.isValid())
            {
               _hash = key.hash();
            }
         }

         explicit lobChunkSearchEntry(const bson::OID &oid,
                                      UINT32 chunkId,
                                      UINT32 lclid,
                                      UINT16 chainPos=0):
         _key(oid, chunkId),
         _lclid(lclid),
         _chainPos(chainPos)
         {
            if (DMS_INVALID_LOGICCLID != _lclid &&
                _key.isValid())
            {
               _hash = _key.hash();
            }
         }

         lobChunkSearchEntry &operator=(const lobChunkSearchEntry &o)
         {
            _key = o._key;
            _hash = o._hash;
            _lclid = o._lclid;
            _chainPos = o._chainPos;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return DMS_INVALID_LOGICCLID != _lclid &&
                   _key.isValid();
         }
         OSS_INLINE const lobChunkKey &getKey()const {return _key;}
         OSS_INLINE UINT32 getLogicalClId()const {return _lclid;}
         OSS_INLINE UINT32 hash()const {return _hash;}
         OSS_INLINE UINT16 getChainPos()const {return _chainPos;}
         
         void set(const bson::OID &oid,
                  UINT32 chunkId,
                  UINT32 lclid,
                  UINT16 chainPos=0)
         {
            _key.set(oid, chunkId);
            _lclid = lclid;
            _chainPos = chainPos;
            if (DMS_INVALID_LOGICCLID != _lclid &&
                _key.isValid())
            {
               _hash = _key.hash();
            }
            return;
         }

      private:
         lobChunkKey _key;
         UINT32 _hash = 0;
         UINT32 _lclid = DMS_INVALID_LOGICCLID;
         UINT16 _chainPos = 0;
   };//class lobChunkSearchEntry
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOB_CHUNK_SEARCH_ENTRY_H_
