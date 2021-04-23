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

namespace engine
{
namespace vessel
{
#pragma pack(4)
   struct copyOnWriteSpaceCheckpoint
   {
      OSS_INLINE copyOnWriteSpaceCheckpoint(){}
      OSS_INLINE ~copyOnWriteSpaceCheckpoint(){}
      OSS_INLINE copyOnWriteSpaceCheckpoint(const copyOnWriteSpaceCheckpoint &o):
      lsn(o.lsn),
      minDeltaOffset(o.minDeltaOffset),
      maxDeltaOffset(o.maxDeltaOffset),
      idxMetaSequence(o.idxMetaSequence),
      minIdxDataSequence(o.minIdxDataSequence),
      maxIdxDataSequence(o.maxIdxDataSequence){}
      OSS_INLINE copyOnWriteSpaceCheckpoint &operator=(const copyOnWriteSpaceCheckpoint &o)
      {
         lsn = o.lsn;
         minDeltaOffset = o.minDeltaOffset;
         maxDeltaOffset = o.maxDeltaOffset;
         idxMetaSequence = o.idxMetaSequence;
         minIdxDataSequence = o.minIdxDataSequence;
         maxIdxDataSequence = o.maxIdxDataSequence;
         return *this;
      }

      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      UINT64 minDeltaOffset = DPS_INVALID_LSN_OFFSET;
      UINT64 maxDeltaOffset = DPS_INVALID_LSN_OFFSET;
      UINT64 idxMetaSequence = INVALID_FILE_SEQUENCE;
      UINT64 minIdxDataSequence = INVALID_FILE_SEQUENCE;
      UINT64 maxIdxDataSequence = INVALID_FILE_SEQUENCE;
   };//struct copyOnWriteSpaceCheckpoint
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_COW_SPACE_CHECKPOINT_H_