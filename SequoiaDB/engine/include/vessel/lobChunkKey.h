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

#include "vessel/vesselIdDef.h"
#include "ossUtil.hpp"
#include "../../bson/oid.h"
#include "vessel/objectIdentifier.h"

namespace engine
{
namespace vessel
{
#pragma pack(4)
   class lobChunkKey : public SDBObject
   {
      public:
         lobChunkKey(){}
         lobChunkKey(const lobChunkKey &o):
         _oid(o._oid),
         _chunkId(o._chunkId){}
         explicit lobChunkKey(const bson::OID &oid,
                              UINT32 chunkId):
         _oid(oid),
         _chunkId(chunkId){}
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

         BOOLEAN operator<(const lobChunkKey &o)const
         {
            return compare(o) < 0;
         }

         INT32 compare(const lobChunkKey &o)const
         {
            INT32 oidCmp = _oid.compare(o._oid);
            if (0 != oidCmp)
            {
               return oidCmp;
            }
            else if (_chunkId > o._chunkId)
            {
               return 1;
            }
            else if (_chunkId < o._chunkId)
            {
               return -1;
            }
            else
            {
               return 0;
            }
         }


      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return _oid.isSet();
         }
         OSS_INLINE void set(const bson::OID &oid,
                             UINT32 chunkId)
         {
            _oid = oid;
            _chunkId = chunkId;
            return;
         }

         OSS_INLINE void reset()
         {
            _oid.clear();
            _chunkId = 0;
         }

         OSS_INLINE const bson::OID &getOid()const {return _oid;}
         OSS_INLINE UINT32 getChunkId()const {return _chunkId;}

         OSS_INLINE UINT32 hash()const
         {
            return ossHash((const BYTE * )(_oid.getData()), sizeof(_oid),
                           (const BYTE *)(&_chunkId), sizeof(_chunkId));
         }

         ossPoolString toString()const
         {
            ossPoolString str;
            str.reserve(32);
            str.append(_oid.toString().c_str());
            str.append(":");
            CHAR buf[12] = {};
            ossItoa(_chunkId, buf, 12);
            str.append(buf);
            return std::move(str);
         }

      private:
         bson::OID _oid;
         UINT32 _chunkId = 0;
   };//class lobChunkKey

   class globalLobChunkKey : public SDBObject
   {
      public:
         globalLobChunkKey(){}
         globalLobChunkKey(const globalLobChunkKey &o):
         _sid(o._sid),
         _mbid(o._mbid),
         _key(o._key){}
         explicit globalLobChunkKey(SPACE_ID sid,
                                    CL_MB_ID mbid,
                                    const lobChunkKey &key):
         _sid(sid),
         _mbid(mbid),
         _key(key){}

         globalLobChunkKey &operator=(const globalLobChunkKey &o)
         {
            _sid = o._sid;
            _mbid = o._mbid;
            _key = o._key;
            return *this;
         }

         BOOLEAN operator==(const globalLobChunkKey &o)const
         {
            return _sid == o._sid &&
                   _mbid == o._mbid &&
                   _key == o._key;
         }

      public:
         OSS_INLINE void set(SPACE_ID sid, CL_MB_ID mbid, const lobChunkKey &key)
         {
            _sid = sid;
            _mbid = mbid;
            _key = key;
         }
         OSS_INLINE void reset()
         {
            _sid = INVALID_SPACE_ID;
            _mbid = INVALID_CL_MB_ID;
            _key.reset();
         }
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_SPACE_ID != _sid &&
                   INVALID_CL_MB_ID != _mbid &&
                   _key.isValid();
         }

         OSS_INLINE SPACE_ID getSpaceId()const {return _sid;}
         OSS_INLINE CL_MB_ID getMbId()const {return _mbid;}
         OSS_INLINE const lobChunkKey &getKey()const {return _key;}

         OSS_INLINE UINT32 hash()const
         {
            return _key.hash();
         }

         ossPoolString toString()const
         {
            static const UINT32 _BUF_SIZE = 16;
            CHAR buf[_BUF_SIZE] = {};
            ossPoolString str;
            str.reserve(96);
            str.append("{sid:");
            ossItoa(_sid, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", mbid:");
            ossItoa(_mbid, buf, _BUF_SIZE);
            str.append(buf);
            str.append(", chunkid:");
            ossItoa(_key.getChunkId(), buf, _BUF_SIZE);
            str.append(buf);
            str.append(", oid:");
            str.append(_key.getOid().str().c_str());
            str.append("}");
            return std::move(str);
         }

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         CL_MB_ID _mbid = INVALID_CL_MB_ID;
         lobChunkKey _key;

   };//class globalLobChunkKey

#pragma pack()
} // namespace vessel

} // namespace engine




#endif//VESSEL_LOB_CHUNK_KEY_H_
