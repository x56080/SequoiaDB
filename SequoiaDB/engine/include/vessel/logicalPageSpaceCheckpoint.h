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

#include "dpsDef.hpp"
#include "vessel/vesselFileDef.h"
#include "vessel/storageFileDef.h"

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
      OSS_INLINE logicalPageSpaceCheckpoint(const logicalPageSpaceCheckpoint &o):
      version(o.version),
      flags(o.flags),
      lsn(o.lsn),
      preCheckpoint(o.preCheckpoint)
      {}

      OSS_INLINE logicalPageSpaceCheckpoint &operator=(const logicalPageSpaceCheckpoint &o)
      {
         version = o.version;
         flags = o.flags;
         lsn = o.lsn;
         preCheckpoint = o.preCheckpoint;
         return *this;
      }

      OSS_INLINE BOOLEAN isValid()const
      {
         return LPS_CHECKPOINT_VERSION == version &&
                DPS_INVALID_LSN_OFFSET != lsn &&
                DPS_INVALID_LSN_OFFSET != offset;
      }

      OSS_INLINE void init(UINT32 flags,
                           UINT64 lsn,
                           UINT64 offset,
                           UINT64 precheckpoint)
      {
         version = LPS_CHECKPOINT_VERSION;
         flags = flags;
         this->lsn = lsn;
         this->offset = offset;
         this->preCheckpoint = preCheckpoint;
         return;
      }

      std::string toString()const
      {
         std::stringstream ss;
         ss << "{version:" << version
            << ", flags:" << flags
            << ", lsn:" << lsn
            << ", offset:" << offset
            << ", preCheckpoint:" << preCheckpoint
            << "}";
         return ss.str();
      }

      UINT32 version = 0;
      UINT32 flags = 0;
      UINT64 lsn = DPS_INVALID_LSN_OFFSET;
      UINT64 offset = DPS_INVALID_LSN_OFFSET;
      UINT64 preCheckpoint = DPS_INVALID_LSN_OFFSET;
   };//struct logicalPageSpaceCheckpoint

   static const UINT32 LPS_CHECKPOINT_SIZE = sizeof(logicalPageSpaceCheckpoint);

   typedef class logicalPageSpaceCheckpoint LPS_CHECKPOINT;
#pragma pack()
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGICAL_PAGE_SPACE_CHECKPOINT_H_