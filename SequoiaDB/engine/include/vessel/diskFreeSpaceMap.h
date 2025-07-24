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

   Source File Name = diskFreeSpaceMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

         void destroy();

         INT32 truncate();

         INT32 find(INT32 targetLvl,
                    BOOLEAN &found,
                    UINT32 &seq,
                    INT32 &lvl);

         INT32 incDataPageCount(UINT32 count);

         INT32 upgradePageSpaceLvl(UINT32 seq,
                                   INT32 lvl);

         INT32 downgradePgaeSpaceLvl(UINT32 seq,
                                     INT32 lvl);
      private:
         INT32 initBitmapPage(PAGE_ID pid);
         void initBitmapPageBuffer(ossValuePtr ptr,
                                   UINT32 logicalId,
                                   PAGE_ID pre=INVALID_PAGE_ID,
                                   PAGE_ID next=INVALID_PAGE_ID);
         INT32 initOwnerPage(PAGE_ID pid, PAGE_ID pre);
         INT32 updateEntrySlot(CL_MB_ID mbID, const fsmCLEntry &entry);
         INT32 destroyEntrySlot(CL_MB_ID mbID);
         INT32 readEntrySlot(CL_MB_ID mbID, UINT32 logicalID, fsmCLEntry &entry);
         INT32 buildBitmapObjs(PAGE_ID root, UINT32 totalPageCount);

      private:
         INT32 createSuperBitmap();
         INT32 ensureSuperBitmapSize(UINT32 bitmapPageCount);

         void clearInSuperBitmapGTELvL(INT32 lvl, UINT32 bitmapNo);
         BOOLEAN findBitmapFromSuperBitmap(INT32 targetLvl,
                                           UINT32 &bitmapNo)const;
         void atomicSetSuperBitmap(INT32 lvl, UINT32 bitmapNo);
         void atomicUnsetSuperBitmap(INT32 lvl, UINT32 bitmapNo);
         void atomicUnsetSuperBitmapFromLvl(INT32 lvl, UINT32 bitmapNo);
      private:
         INT32 getFsmPageHead(PAGE_ID pid, UINT16 type,
                              fsmPageHead **head);

         OSS_INLINE UINT32 getBitmapPageNo(UINT32 seq)const
         {
            return seq / FSM_BITMAP_PAGE_CAPACITY;
         }
         OSS_INLINE UINT32 getOwnerPageNo(UINT32 bitmapPageNo)const
         {
            SDB_ASSERT(0 < bitmapPageNo, "can not be root");
            return (bitmapPageNo - 1) / FSM_BITMAP_OWNER_PAGE_CAPACITY;
         }

      private:
         INT32 ensureOwnerPage(UINT32 count);

         INT32 createNewBitmapObj(UINT32 pageNo,
                                 fsmBitmapPageObject **out);

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
         CL_MB_ID _mbID = INVALID_CL_MB_ID;

         ossSpinSLatchPOSIX _latch;
         UINT32 _totalDataPageCount = 0;
         _BITMAP_OBJ_MAP _bitmaps;
         _BITMAP_OWNER_ARRAY _bitmapOwners;

         /// bitmap index of bitmap pages.
         memoryBlock _superBitmap[FSM_SPACE_LVL_COUNT];
   };//class diskFreeSpaceMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_DISK_FREE_SPACE_MAP_H_