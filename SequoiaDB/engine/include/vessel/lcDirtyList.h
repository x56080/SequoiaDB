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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
#include "vessel/latch.h"
#include "vessel/lcExtentTagHolder.h"
#include "ossLatch.hpp"


namespace engine
{
namespace vessel
{
   class lcExtentTag;
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
         INT32 insert(lcExtentTagHolder &holder);

         /// under w lock
         INT32 remove(lcExtentTagHolder &holder);

         INT32 setPendingWrite(requestContext *context,
                               UINT32 scanDepth,
                               UINT64 minLSN,
                               diskIOJob *job);

         INT32 setWholeListPendingWrite(requestContext *context,
                                        diskIOJob *job);

         INT32 setWholeListPendingWrite(diskIOJob *job);

         UINT32 size();

         UINT64 getMinDirtyLSN();

         /// WARNING: incorrect use can cause wrong checkpoints.
         /// only after all dirty pages are persisted to disk,
         /// min lsn in the cache can be deleted.
         INT32 cacheMinDirtyLSN();
         void removeCachedMinDirtyLSN();

      private:
         void insertIntoSortedList(lcExtentTag *tag);

         void remove(lcExtentTag *tag);

      private:
         ossSpinSLatch _latch;
         UINT32 _size;
         lcExtentTag *_head;
         lcExtentTag *_tail;
         UINT64 _cachedMinDirtyLSN;
   };

} /// end of namespace vessel
} /// end of namespace engine

#endif
