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

   Source File Name = fsmBitmapPageObject.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_FSM_BITMAP_PAGE_OBJECT_H_
#define VESSEL_FSM_BITMAP_PAGE_OBJECT_H_

#include "vessel/freeSpaceMapDef.h"

namespace engine
{
namespace vessel
{
   class fsmBitmapPage;

   class fsmBitmapPageObject : public SDBObject
   {
      public:
         fsmBitmapPageObject();
         ~fsmBitmapPageObject();

         fsmBitmapPageObject(const fsmBitmapPageObject &) = delete;
         fsmBitmapPageObject &operator=(const fsmBitmapPageObject &) = delete;

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return INVALID_PAGE_ID != _pid;
         }
         OSS_INLINE PAGE_ID getPid()const
         {
            return _pid;
         }
         OSS_INLINE UINT32 getPageNo()const
         {
            return _pageNo;
         }

         void reset();

         INT32 initWhenCreate(UINT32 pageNo,
                              PAGE_ID pid,
                              fsmBitmapPage *page);

         INT32 initWhenOpen(UINT32 pageNo,
                            PAGE_ID pid,
                            UINT32 dataPageCount,
                            fsmBitmapPage *page,
                            UINT32 &abnormalCount);

         /// The size never shrink
         /// Not thread safe
         void resetSizeIfHigher(UINT32 size);

         INT32 findAndClear(INT32 targetLvl,
                            BOOLEAN &found,
                            UINT32 &seq,
                            INT32 &realLvl);

         INT32 getStats(INT32 lvl)const;

         INT32 upgradePageSpaceLvl(UINT32 seq,
                                   INT32 lvl,
                                   BOOLEAN &upgraded);

         INT32 downgradePageSpaceLvl(UINT32 seq,
                                     INT32 lvl);
                       
      private:
         BOOLEAN atomicFindAndClear(INT32 lvl,
                                    UINT32 &offset);

         /// Return false if already set.
         BOOLEAN atomicSetLvl(UINT32 bitsCount,
                              UINT32 offset,
                              INT32 lvl);

         BOOLEAN atomicUnsetLvl(UINT32 bitsCount,
                                UINT32 offset,
                                INT32 lvl);
      private:
         UINT32 _pageNo = 0;
         PAGE_ID _pid = INVALID_PAGE_ID;
         fsmBitmapPage *_page = NULL;
         UINT32 _size = 0;
         INT32 _stats[FSM_SPACE_LVL_COUNT] = {};
         
   };//class fsmBitmapPageObject
}//namespace vessel
}//namespace engine

#endif//VESSEL_FSM_BITMAP_PAGE_OBJECT_H_
