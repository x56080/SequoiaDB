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

   Source File Name = copyOnWriteSpaceCheckpoint.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COW_SPACE_CHECKPOINT_H_
#define VESSEL_COW_SPACE_CHECKPOINT_H_

#include "dpsDef.hpp"
#include "vessel/vesselFileDef.h"
#include "vessel/storageFileDef.h"

namespace engine
{
namespace vessel
{
   static const UINT32 COW_SPACE_CHECKPOINT_VERSION = 1;
#pragma pack(4)
   struct copyOnWriteSpaceCheckpoint
   {
      OSS_INLINE copyOnWriteSpaceCheckpoint(){}
      OSS_INLINE ~copyOnWriteSpaceCheckpoint(){}
      OSS_INLINE copyOnWriteSpaceCheckpoint(const copyOnWriteSpaceCheckpoint &o):
      version(o.version),
      flags(o.flags),
      lsn(o.lsn),
      minDeltaOffset(o.minDeltaOffset),
      maxDeltaOffset(o.maxDeltaOffset),
      minIdxDataSequence(o.minIdxDataSequence),
      maxIdxDataSequence(o.maxIdxDataSequence),
      idxMFileFullyFlushedTimes(o.idxMFileFullyFlushedTimes),
      idxMFilePageCount(o.idxMFilePageCount)
      {}

      OSS_INLINE copyOnWriteSpaceCheckpoint &operator=(const copyOnWriteSpaceCheckpoint &o)
      {
         version = o.version;
         flags = o.flags;
         lsn = o.lsn;
         minDeltaOffset = o.minDeltaOffset;
         maxDeltaOffset = o.maxDeltaOffset;
         minIdxDataSequence = o.minIdxDataSequence;
         maxIdxDataSequence = o.maxIdxDataSequence;
         idxMFileFullyFlushedTimes = o.idxMFileFullyFlushedTimes;
         idxMFilePageCount = o.idxMFilePageCount;
         return *this;
      }

      OSS_INLINE BOOLEAN operator==(const copyOnWriteSpaceCheckpoint &o)const
      {
         return 0 == ossMemcmp(this, &o, sizeof(copyOnWriteSpaceCheckpoint));
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return COW_SPACE_CHECKPOINT_VERSION == version &&
                DPS_INVALID_LSN_OFFSET != lsn;
      }

      UINT32 version = 0;
      UINT32 flags = 0;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      UINT64 minDeltaOffset = DPS_INVALID_LSN_OFFSET;
      UINT64 maxDeltaOffset = DPS_INVALID_LSN_OFFSET;
      UINT64 minIdxDataSequence = STORAGE_FILE_INVALID_SEQUENCE;
      UINT64 maxIdxDataSequence = STORAGE_FILE_INVALID_SEQUENCE;
      UINT64 idxMFileFullyFlushedTimes = 0;
      UINT32 idxMFilePageCount = 0;
   };//struct copyOnWriteSpaceCheckpoint

   static const UINT32 COW_SPACE_CHECKPOINT_OBJ_SIZE = sizeof(copyOnWriteSpaceCheckpoint);
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_COW_SPACE_CHECKPOINT_H_