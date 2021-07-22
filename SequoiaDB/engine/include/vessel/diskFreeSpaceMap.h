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

   Source File Name = diskFreeSpaceMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_DISK_FREE_SPACE_MAP_H_
#define VESSEL_DISK_FREE_SPACE_MAP_H_

#include "vessel/pageDef.h"
#include "vessel/fsmFile.h"
#include "ossLatch.hpp"
#include "vessel/freeSpaceMapDef.h"
#include "ossMemPool.hpp"
#include "vessel/fsmBitmapPageObject.h"
#include "vessel/memoryBlock.h"

namespace engine
{
namespace vessel
{
   class diskFreeSpaceMap : public SDBObject
   {
      public:
         diskFreeSpaceMap();
         ~diskFreeSpaceMap();
         diskFreeSpaceMap(const diskFreeSpaceMap &) = delete;
         diskFreeSpaceMap &operator=(const diskFreeSpaceMap &) = delete;
      public:
         BOOLEAN isOpen()const
         {
            return NULL != _fsmFile;
         }

         INT32 create(fsmFile *file,
                      CL_MB_ID mbID,
                      UINT32 logicalID);

         INT32 open(fsmFile *file,
                    CL_MB_ID mbID,
                    UINT32 logicalID,
                    UINT32 dataPageCount,
                    BOOLEAN autoRecreateEntry=TRUE);

         void close();

         INT32 find(INT32 targetLvl,
                    BOOLEAN &found,
                    UINT32 &seq,
                    INT32 &lvl);

         INT32 incDataPageCount(UINT32 count);

         INT32 upgradePageSpaceLvl(UINT32 seq,
                                   INT32 lvl);
      private:
         INT32 initBitmapPage(PAGE_ID pid);
         INT32 initOwnerPage(PAGE_ID pid, PAGE_ID pre);
         INT32 updateEntrySlot(CL_MB_ID mbID, const fsmCLEntry &entry);
         INT32 readEntrySlot(CL_MB_ID mbID, UINT32 logicalID, fsmCLEntry &entry);
         INT32 buildBitmapObjs(PAGE_ID root, UINT32 totalPageCount);

      private:
         INT32 createSuperBitmaps();
         INT32 ensureSuperBitmapSize(UINT32 bitmapPageCount);
         void clearInSuperBitmapAtLvL(INT32 lvl, UINT32 bitmapNo);
         void clearInSuperBitmapGTELvL(INT32 lvl, UINT32 bitmapNo);
         BOOLEAN findBitmapFromSuperBitmap(INT32 targetLvl,
                                           UINT32 &bitmapNo)const;
         void atomicSetSuperBitmap(INT32 lvl, UINT32 bitmapNo);

      private:
         PAGE_ID getEntryPid(CL_MB_ID mbID);
         fsmCLEntry *getEntryFromPagePtr(ossValuePtr ptr, CL_MB_ID mbID);
         INT32 getFsmPageHead(PAGE_ID pid, UINT16 type,
                              fsmPageHead *&head);

         OSS_INLINE UINT32 getBitmapPageNo(UINT32 seq)const
         {
            SDB_ASSERT(INVALID_CL_PAGE_SEQ != seq, "impossible");
            return seq / FSM_BITMAP_PAGE_CAPACITY;
         }
         OSS_INLINE UINT32 getOwnerPageNo(UINT32 bitmapPageNo)const
         {
            SDB_ASSERT(0 < bitmapPageNo, "can not be root");
            return (bitmapPageNo - 1) / FSM_BITMAP_OWNER_PAGE_CAPAITY;
         }

      private:
         INT32 ensureOwnerPage(UINT32 count);

         INT32 createNewBitmapObj(UINT32 pageNo,
                                 fsmBitmapPageObject **out);

         void removeBitmapObj(UINT32 pageNo);

         fsmBitmapPageObject *getBitmapPageObj(UINT32 i);

         INT32 buildBitmapObj(UINT32 pageNo,
                              PAGE_ID pid,
                              UINT32 dataPageCount,
                              fsmBitmapPage *page);

         INT32 createNewOwnerPage(PAGE_ID pre, PAGE_ID &pid);

         INT32 ensureBitmapObj(UINT32 bitmapNo,
                               fsmBitmapPageObject **out);

      private:
         typedef ossPoolVector<PAGE_ID> _BITMAP_OWNER_ARRAY;

         typedef ossPoolMap<UINT32, fsmBitmapPageObject *> _BITMAP_OBJ_MAP;

      private:
         fsmFile *_fsmFile = NULL;
         UINT32 _logicalId = DMS_INVALID_LOGICCLID;

         ossSpinSLatchPOSIX _latch;
         UINT32 _totalDataPageCount = 0;
         _BITMAP_OBJ_MAP _bitmaps;
         _BITMAP_OWNER_ARRAY _bitmapOwners;

         /// bitmap indexes of bitmap pages.
         memoryBlock _superBitmaps[FSM_SPACE_LVL_COUNT];
   };//class diskFreeSpaceMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_DISK_FREE_SPACE_MAP_H_