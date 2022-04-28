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

   Source File Name = logicalPageSpaceCheckpoint.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGICAL_PAGE_SPACE_CHECKPOINT_H_
#define VESSEL_LOGICAL_PAGE_SPACE_CHECKPOINT_H_

#include "vessel/checkpointLSN.h"
#include "pdTrace.hpp"
#include "../../bson/util/builder.h"

namespace engine
{
namespace vessel
{
   constexpr UINT32 LPS_CHECKPOINT_VERSION = 1;
#pragma pack(4)
   struct logicalPageSpaceCheckpoint
   {
      OSS_INLINE logicalPageSpaceCheckpoint(){}
      OSS_INLINE ~logicalPageSpaceCheckpoint(){}
      OSS_INLINE logicalPageSpaceCheckpoint(const logicalPageSpaceCheckpoint &o) = delete;

      OSS_INLINE logicalPageSpaceCheckpoint &operator=(const logicalPageSpaceCheckpoint &o)
      {
         version = o.version;
         flags = o.flags;
         lsn = o.lsn;
         sequence = o.sequence;
         time = o.time;
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return LPS_CHECKPOINT_VERSION == version &&
                lsn.isValid();
      }

      void init(UINT32 flags,
                const checkpointLSN &lsn,
                UINT64 sequence,
                UINT64 time)
      {
         version = LPS_CHECKPOINT_VERSION;
         SDB_ASSERT(lsn.isValid(), "can not be invalid");
         this->flags = flags;
         this->lsn = lsn;
         this->sequence = sequence;
         this->time = time;
         return;
      }

      ossPoolString toString()const
      {
         bson::StringBuilder builder;
         builder << "{version:" << version
                 << ",flags:" << flags
                 << ",lsn:" << lsn._lsn
                 << ",minDirtyLsn:" << lsn._minDirtyLSN
                 << ",minUncompletedLsn:" << lsn._minUncompletedLSN
                 << ",sequence:" << sequence
                 << ",time:" << time
                 << "}";
         return std::move(builder.poolStr());
      }

      static const UINT32 FLAG_FULL_CHECKPOINT = 0x01;

      UINT32 version = 0;
      UINT32 flags = 0;
      checkpointLSN lsn;
   UINT64 sequence = 0;
      UINT64 time = 0;
   };//struct logicalPageSpaceCheckpoint

   static const UINT32 LPS_CHECKPOINT_SIZE = sizeof(logicalPageSpaceCheckpoint);

   typedef class logicalPageSpaceCheckpoint LPS_CHECKPOINT;
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_SPACE_CHECKPOINT_H_