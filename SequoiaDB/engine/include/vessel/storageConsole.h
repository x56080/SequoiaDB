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

   Source File Name = storageConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STORAGE_CONSOLE_H_
#define VESSEL_STORAGE_CONSOLE_H_

#include "ossMemPool.hpp"
#include "vessel/vesselIdDef.h"
#include "ossLatch.hpp"
#include "vessel/storageUnit.h"
#include "vessel/mmapPagePointer.h"

namespace engine
{
namespace vessel
{
   class storageUnit;
   class storageFileCluster;
   class requestContext;

   class storageConsole : public SDBObject
   {
      public:
         storageConsole();
         ~storageConsole();
         storageConsole(const storageConsole &) = delete;
         storageConsole &operator=(const storageConsole &) = delete;
      
      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return _isOpen;
         }

      public:
         INT32 open(requestContext *context);
         void close();

      public:
         PAGE_SNAPSHOT_VERION getOnlinePageSnapshotVersion();
         INT32 allocateFreeSpaceID(SPACE_ID &sid);
         INT32 releaseSpaceID(SPACE_ID sid);
         INT32 getStorageUnit(SPACE_ID sid, storageUnit **su);
         INT32 getLogicalPageSpace(SPACE_ID sid,
                                   SPACE_TYPE type,
                                   logicalPageSpace **lps);

         INT32 isSnapshotEffective(SPACE_ID sid,
                                   PAGE_SNAPSHOT_VERION psv,
                                   BOOLEAN &effective);

      public:
         ///WARNING: User must be sure that space exists and
         /// not be released during accessing.
         INT32 getMmapPagePtr(const GLOBAL_PAGE_ID &gpid,
                              mmapPagePointer &ptr)const;

         INT32 getPageSize(SPACE_ID sid,
                           SPACE_TYPE spaceType,
                           FILE_TYPE fileType,
                           UINT32 &pageSize)const;

      private:
         INT32 loadStorageUnitsOnDisk(requestContext *context);
         INT32 loadStorageUnit();

      private:
         typedef ossPoolList<SPACE_ID> _SPACE_ID_POOL;

      private:
         BOOLEAN _isOpen = FALSE;
         ossSpinXLatch _latch;
         SPACE_ID _minSidNotInPool = INVALID_SPACE_ID;
         _SPACE_ID_POOL _free;
         _SPACE_ID_POOL _workshop;
         _SPACE_ID_POOL _removedButSnapshoted;
         _SPACE_ID_POOL _abnormalSids;
   };
}//namespace vessel
}//namespace engine

#endif//VESSEL_STORAGE_CONSOLE_H_