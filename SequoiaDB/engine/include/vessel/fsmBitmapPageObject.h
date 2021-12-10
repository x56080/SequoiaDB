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

   Source File Name = fsmBitmapPageObject.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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

         INT32 init(UINT32 pageNo,
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
