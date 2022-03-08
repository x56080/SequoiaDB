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

   Source File Name = lobChunkKey.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_CHUNK_KEY_H_
#define VESSEL_LOB_CHUNK_KEY_H_

#include "vessel/lobDef.h"
#include "ossUtil.hpp"
#include "../../bson/oid.h"

namespace engine
{
namespace vessel
{
   class lobChunkKey : public SDBObject
   {
      public:
         lobChunkKey(){}
         ~lobChunkKey(){}
         lobChunkKey(const lobChunkKey &o):
         _oid(o._oid),
         _chunkId(o._chunkId){}
         lobChunkKey &operator=(const lobChunkKey &o)
         {
            _oid = o._oid;
            _chunkId = o._chunkId;
            return *this;
         }

         BOOLEAN operator==(const lobChunkKey &o)const
         {
            return _chunkId == o._chunkId &&
                   _oid == o._oid;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_LOB_CHUNK_ID != _chunkId &&
                   _oid.isSet();
         }
         OSS_INLINE void set(const bson::OID &oid,
                             LOB_CHUNK_ID chunkId)
         {
            _oid = oid;
            _chunkId = chunkId;
            return;
         }

         OSS_INLINE void reset()
         {
            _oid.clear();
            _chunkId = INVALID_LOB_CHUNK_ID;
         }

         OSS_INLINE const bson::OID &getOid()const {return _oid;}
         OSS_INLINE LOB_CHUNK_ID getChunkId()const {return _chunkId;}

         OSS_INLINE UINT32 hash()const
         {
            return ossHash((const BYTE * )(_oid.getData()), sizeof(_oid),
                           (const BYTE *)(&_chunkId), sizeof(_chunkId));
         }

      private:
         bson::OID _oid;
         LOB_CHUNK_ID _chunkId = INVALID_LOB_CHUNK_ID;
   };//class lobChunkKey
} // namespace vessel

} // namespace engine




#endif//VESSEL_LOB_CHUNK_KEY_H_
