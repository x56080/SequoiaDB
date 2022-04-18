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

   Source File Name = lobChunkSearchEntry.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
