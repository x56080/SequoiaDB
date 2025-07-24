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

   Source File Name = logicalPageSpaceCheckpoint.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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