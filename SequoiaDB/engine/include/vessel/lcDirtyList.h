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

   Source File Name = lcDirtyList.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LC_DIRTY_LIST_H_
#define VESSEL_LC_DIRTY_LIST_H_

#include "ossTypes.h"
#include "vessel/lcPageTagHolder.h"
#include "ossLatch.hpp"

namespace engine
{
namespace vessel
{
   class diskIOJob;
   class requestContext;

   class lcDirtyList : public SDBObject
   {
      public:
         lcDirtyList();
         ~lcDirtyList();

      public:
         INT32 init();

         INT32 fini();

         /// under w lock
         INT32 upsert(DPS_LSN_OFFSET lsn, lcPageTagHolder &holder);

         /// under w lock
         INT32 remove(lcPageTagHolder &holder);

         INT32 setPendingWrite(requestContext *context,
                               UINT32 scanDepth,
                               UINT64 minLSN,
                               diskIOJob *job);

         UINT32 size(BOOLEAN lock=TRUE);

         UINT64 getMinDirtyLSN(BOOLEAN lock=TRUE);

         /// Update min dirty lsn to current
         /// min lsn in list.
         void updateMinDirtyLsn();

         void removeCachedMinDirtyLSN();

      private:
         void insertIntoSortedList(liteCachePageTag *tag);

         void remove(liteCachePageTag *tag);

      private:
         ossSpinXLatch _latch;
         UINT32 _size = 0;
         liteCachePageTag *_head = NULL;
         liteCachePageTag *_tail = NULL;
         DPS_LSN_OFFSET _minDirtyLsn = DPS_INVALID_LSN_OFFSET;
   };

} /// end of namespace vessel
} /// end of namespace engine

#endif
