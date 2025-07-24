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

   Source File Name = lobExtentMetaBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOB_EXTENT_META_BLOCK_H_
#define VESSEL_LOB_EXTENT_META_BLOCK_H_

#include "vessel/lobChunkKey.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"
#include "pdTrace.hpp"
#include "vessel/lextentDescriptor.h"
#include "../../bson/util/builder.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 LOB_EXTENT_META_BLOCK_VERSION = 1;

#pragma pack(4)
   struct lobExtentMetaBlock
   {
      OSS_INLINE BOOLEAN isValid()const
      {
         return LOB_EXTENT_META_BLOCK_VERSION == (UINT32)version &&
                INVALID_PAGE_ID != pid &&
                INVALID_PAGE_SNAPSHOT_VERSION != psv &&
                0 < size &&
                DMS_INVALID_LOGICCLID != lclid &&
                INVALID_CL_MB_ID != mbid;
      }

      INT32 compare(UINT32 lclid,
                    const lobChunkKey &key,
                    UINT16 chainPos)const
      {
         SDB_ASSERT(DMS_INVALID_LOGICCLID != lclid, "can not be invalid");
         SDB_ASSERT(key.isValid(), "can not be invalid");
         SDB_ASSERT(isValid(), "can not be invalid");
         INT32 res = 0;
         if (this->lclid < lclid)
         {
            res = -1;
         }
         else if (this->lclid > lclid)
         {
            res = 1;
         }
         else
         {
            res = oid.compare(key.getOid());
            if (0 == res)
            {
               if (chunkId < key.getChunkId())
               {
                  res = -1;
               }
               else if (chunkId > key.getChunkId())
               {
                  res = 1;
               }
               else
               {
                  res = static_cast<INT32>(this->chainPos) -
                        static_cast<INT32>(chainPos);
               }
            }
         }

         return res;
      }

      INT32 compare(UINT32 lclid,
                    const lobChunkKey &key)const
      {
         SDB_ASSERT(DMS_INVALID_LOGICCLID != lclid, "can not be invalid");
         SDB_ASSERT(key.isValid(), "can not be invalid");
         SDB_ASSERT(isValid(), "can not be invalid");
         INT32 res = 0;
         if (this->lclid < lclid)
         {
            res = -1;
         }
         else if (this->lclid > lclid)
         {
            res = 1;
         }
         else
         {
            res = oid.compare(key.getOid());
            if (0 == res)
            {
               if (chunkId < key.getChunkId())
               {
                  res = -1;
               }
               else if (chunkId > key.getChunkId())
               {
                  res = 1;
               }
            }
         }

         return res;
      }

      INT32 compare(const lobExtentMetaBlock &o)const
      {
         SDB_ASSERT(isValid() && o.isValid(), "can not be invalid");
         INT32 res = 0;
         if (this->lclid < o.lclid)
         {
            res = -1;
         }
         else if (this->lclid > o.lclid)
         {
            res = 1;
         }
         else
         {
            res = oid.compare(o.oid);
            if (0 == res)
            {
               if (chunkId < o.chunkId)
               {
                  res = -1;
               }
               else if (chunkId > o.chunkId)
               {
                  res = 1;
               }
               else
               {
                  res = static_cast<INT32>(chainPos) -
                        static_cast<INT32>(o.chainPos);
               }
            }
         }

         return res;
      }

      static constexpr UINT8 FLAG_CHAIN_TAIL = 0x01;

      OSS_INLINE BOOLEAN isChainTail()const
      {
         SDB_ASSERT(isValid(), "can not be invald");
         return 0 != OSS_BIT_TEST(flags, FLAG_CHAIN_TAIL);
      }

      OSS_INLINE lextentDescriptor getExtentDesc()const
      {
         lextentDescriptor desc;
         desc.pcnt = pcnt;
         desc.pid = pid;
         desc.psv = psv;
         desc.size = size;
         return desc;
      }

      OSS_INLINE UINT32 hash()const
      {
         return ossHash((const BYTE * )(oid.getData()), sizeof(oid),
                        (const BYTE *)(&chunkId), sizeof(chunkId));
      }

      void init(UINT32 lclid,
                CL_MB_ID mbid,
                const lobChunkKey &key,
                const lextentDescriptor &desc,
                UINT16 chainPos = 0,
                BOOLEAN chainTail=TRUE)
      {
         version = LOB_EXTENT_META_BLOCK_VERSION;
         if (chainTail)
         {
            OSS_BIT_SET(flags, FLAG_CHAIN_TAIL);
         }
         pcnt = desc.pcnt;
         pid = desc.pid;
         psv = desc.psv;
         size = desc.size;
         this->lclid = lclid;
         this->mbid = mbid;
         this->chainPos = chainPos;
         oid = key.getOid();
         this->chunkId = key.getChunkId();
         ossMemset(reserved, 0x00, sizeof(reserved));
      }

      OSS_INLINE void setAsTail()
      {
         OSS_BIT_SET(flags, FLAG_CHAIN_TAIL);
      }

      OSS_INLINE void clearTail()
      {
         OSS_BIT_CLEAR(flags, FLAG_CHAIN_TAIL);
      }

      ossPoolString toString()const
      {
         bson::StringBuilder str(64);
         str << lclid << ':' << oid.toString()
             << ':' << chunkId << ':' << chainPos;
         return std::move(str.poolStr());
      }

      UINT8 version = 0;
      UINT8 flags = 0;
      UINT16 mbid = INVALID_CL_MB_ID;
      UINT32 lclid = DMS_INVALID_LOGICCLID;
      bson::OID oid;
      UINT32 chunkId = 0;
      UINT16 chainPos = 0;
      UINT16 pcnt = 0;
      UINT32 pid = INVALID_PAGE_ID;
      UINT32 psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT32 size = 0;
      CHAR reserved[24] = {};
   };//class lobExtentMetaBlock
   constexpr UINT32 LOB_EXTENT_META_BLOCK_SIZE = sizeof(lobExtentMetaBlock);

   static_assert(64 == LOB_EXTENT_META_BLOCK_SIZE, "must be 64 bytes");

#pragma pack()
} // namespace vessel

} // namespace engine

#endif//VESSEL_LOB_EXTENT_META_BLOCK_H_