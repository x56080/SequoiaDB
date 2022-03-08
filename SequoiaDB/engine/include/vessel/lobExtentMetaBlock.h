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

   Source File Name = lobExtentMetaBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_EXTENT_META_BLOCK_H_
#define VESSEL_LOB_EXTENT_META_BLOCK_H_

#include "vessel/lobChunkKey.h"
#include "vessel/vesselIdDef.h"
#include "dms.hpp"

namespace engine
{
namespace vessel
{
   constexpr UINT32 LOB_EXTENT_META_BLOCK_VERSION = 1;

#pragma pack(4)
   struct lobExtentMetaBlock
   {
      lobExtentMetaBlock(){}
      ~lobExtentMetaBlock(){}
      lobExtentMetaBlock(const lobExtentMetaBlock &o)
      {
         ossMemcpy(this, &o, sizeof(lobExtentMetaBlock));
      }
      lobExtentMetaBlock &operator=(const lobExtentMetaBlock &o)
      {
         ossMemcpy(this, &o, sizeof(lobExtentMetaBlock));
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return LOB_EXTENT_META_BLOCK_VERSION == (UINT32)version &&
                0 != pcnt &&
                INVALID_PAGE_ID != pid &&
                INVALID_PAGE_SNAPSHOT_VERSION != psv &&
                DMS_INVALID_LOGICCLID != lclid &&
                INVALID_CL_MB_ID != mbid &&
                INVALID_LOB_CHUNK_ID != chunkId;
      }

      UINT8 version = 0;
      UINT8 flags = 0;
      UINT16 pcnt = 0;
      UINT32 pid = INVALID_PAGE_ID;
      UINT32 psv = INVALID_PAGE_SNAPSHOT_VERSION;
      UINT32 size = 0;
      UINT32 lclid = DMS_INVALID_LOGICCLID;
      UINT16 mbid = INVALID_CL_MB_ID;
      UINT16 chainPos = 0;
      CHAR oid[12] = {};
      UINT32 chunkId = INVALID_LOB_CHUNK_ID;
   };//class lobExtentMetaBlock
   constexpr UINT32 LOB_EXTENT_META_BLOCK_SIZE = sizeof(lobExtentMetaBlock);

   struct lobExtentMetaBlockOnDisk
   {
      lobExtentMetaBlock data;
      CHAR reserved[24] = {};
   };
   constexpr UINT32 LOB_EXTENT_DISK_MB_SIZE = sizeof(lobExtentMetaBlockOnDisk);
   static_assert(64 == LOB_EXTENT_DISK_MB_SIZE, "must be 64 bytes");

#pragma pack()
} // namespace vessel

} // namespace engine

#endif//VESSEL_LOB_EXTENT_META_BLOCK_H_