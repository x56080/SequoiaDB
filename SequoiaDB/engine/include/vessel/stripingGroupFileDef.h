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

   Source File Name = stripingGroupFileDef.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STRIPING_GROUP_FILE_DEF_H_
#define VESSEL_STRIPING_GROUP_FILE_DEF_H_

#include "vessel/extentDef.h"

namespace engine
{
namespace vessel
{
   const static UINT16 SG_PAGE_INVALID_VERSION = 0;
   const static UINT16 SG_PAGE_VERSION = 1;

   struct stripingGroupFilePageHead
   {

   };//struct stripingGroupFilePageHead

   struct stripingGroupPageHead
   {
      stripingGroupPageHead():
      version(SG_PAGE_INVALID_VERSION),
      mbid(INVALID_CL_MB_ID),
      clLogicalID(DMS_INVALID_LOGICCLID),
      flags(0),
      maxSGCount(0),
      currentSGCount(0),
      minStripingCountPerSG(0),
      minStripingId(INVALID_STRIPING_ID),
      maxStripingId(INVALID_STRIPING_ID),
      lsn(0)
      {}

      OSS_INLINE BOOLEAN crashed(UINT64 lsn)const
      {
         return SG_PAGE_VERSION != version ||
                this->lsn != lsn ||
                maxStripingId < minStripingId;
      }

      OSS_INLINE BOOLEAN isSame(UINT16 mbid, UINT32 logicalID)const
      {
         return this->mbid == mbid &&
                this->clLogicalID == logicalID;
      }

      UINT16 version;
      UINT16 mbid;
      UINT32 clLogicalID;
      UINT16 flags;
      UINT16 maxSGCount;
      UINT16 currentSGCount;
      UINT16 minStripingCountPerSG;
      UINT16 minStripingId;
      UINT16 maxStripingId;
      UINT64 lsn;
   };//struct stripingGroupPageHead

   const static UINT32 SG_PAGE_HEAD_SIZE = sizeof(stripingGroupPageHead);
   const static UINT32 SG_PAGE_SIZE = 512;
   const static UINT32 MAX_SG_COUNT = 32;

   struct stripingGroupOnDisk
   {
      OSS_INLINE stripingGroupOnDisk():
      minStripingId(INVALID_STRIPING_ID),
      maxStripingId(INVALID_STRIPING_ID),
      extentCount(0),
      lastExtent(INVALID_PAGE_ID)
      {
   
      }

      OSS_INLINE ~stripingGroupOnDisk()
      {}

      OSS_INLINE BOOLEAN operator==(const stripingGroupOnDisk &o)const
      {
         return minStripingId == o.minStripingId &&
                maxStripingId == o.maxStripingId &&
                extentCount == o.extentCount &&
                lastExtent == o.lastExtent;
      }

      OSS_INLINE stripingGroupOnDisk &operator=(const stripingGroupOnDisk &o)
      {
         minStripingId = o.minStripingId;
         maxStripingId = o.maxStripingId;
         extentCount = o.extentCount;
         lastExtent = o.lastExtent;
         return *this;
      }

      OSS_INLINE BOOLEAN valid()const
      {
         return INVALID_STRIPING_ID != minStripingId &&
                INVALID_STRIPING_ID != maxStripingId &&
                minStripingId <= maxStripingId;
      }

      OSS_INLINE void reset()
      {
         minStripingId = INVALID_STRIPING_ID;
         maxStripingId = INVALID_STRIPING_ID;
         extentCount = 0;
         lastExtent = INVALID_PAGE_ID;
         return;
      }

      UINT16 minStripingId;
      UINT16 maxStripingId;
      UINT32 extentCount;
      UINT32 lastExtent;
   };//struct stripingGroupOnDisk

   struct stripingGroup
   {
      OSS_INLINE stripingGroup():
      slot(0)
      {}

      OSS_INLINE ~stripingGroup()
      {}

      OSS_INLINE BOOLEAN operator==(const stripingGroup &o)const
      {
         return slot == o.slot &&
                sgOnDisk == o.sgOnDisk;
      }

      OSS_INLINE stripingGroup &operator=(const stripingGroup &o)
      {
         slot = o.slot;
         sgOnDisk = o.sgOnDisk;
         return *this;
      }

      OSS_INLINE BOOLEAN valid()const
      {
         return sgOnDisk.valid();
      }

      OSS_INLINE void reset()
      {
         sgOnDisk.reset();
         slot = 0;
         return;
      }

      stripingGroupOnDisk sgOnDisk;
      UINT16 slot;
   };//struct stripingGroup
}//namespace vessel
}//namespace engine

#endif//VESSEL_STRIPING_GROUP_FILE_DEF_H_