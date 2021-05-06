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

   Source File Name = logicalPageSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOGIAL_PAGE_SPACE_H_
#define VESSEL_LOGIAL_PAGE_SPACE_H_

#include "vessel/vesselIdDef.h"
#include "vessel/pageDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/inMemBitMap.h"
#include "ossLatch.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class storageUnit;

   class logicalPageSpace : public SDBObject
   {
      public:
         logicalPageSpace(){}
         virtual ~logicalPageSpace();
         logicalPageSpace(const logicalPageSpace &) = delete;
         logicalPageSpace &operator=(const logicalPageSpace &) = delete;

      public:
         OSS_INLINE storageUnit *getSU()
         {
            return _su;
         }
         OSS_INLINE const storageUnit *getSU()const
         {
            return _su;
         }

      public:
         BOOLEAN isOpen()const;
         SPACE_ID getSpaceID()const;
         INT32 getDataPageSize(UINT32 &pageSize)const;
         INT32 getMetaPageSize(UINT32 &pageSize)const;

      public:
         
         INT32 open(requestContext *context,
                    storageUnit *su);
         void close();

         INT32 preallocatePages(requestContext *context,
                                UINT32 count,
                                PAGE_ID *lpids,
                                PAGE_ID *pids);

         void releasePagesPreallocated(requestContext *context,
                                       UINT32 count,
                                       PAGE_ID *lpids,
                                       PAGE_ID *pids);

         INT32 preallocatePhysicalPids(requestContext *context,
                                       UINT32 count,
                                       PAGE_ID *pids);

         void releasePhysicalPidsPreallocated(requestContext *context,
                                              UINT32 count,
                                              const PAGE_ID *pids);

      public:
         /// allocate physical pids and map them to logical pids on disk.
         /// both pids and lpids should be preallocated first.
         virtual INT32 allocatePages(requestContext *context,
                                     PAGE_TYPE pageType,
                                     UINT32 count,
                                     const PAGE_ID *lpids,
                                     const PAGE_ID *pids,
                                     const slice &args,
                                     DPS_LSN_OFFSET *oplist) = 0;

         /// releasePages should also release lpids in memory.
         /// Physical pids' releasing depends on snapshot version.
         /// If physical pid can be recycled, it will be released
         /// in memory also.
         /// In another word, you do not need to care about "releaseXXPrealloated"
         /// any more if releasePages returns ok.
         /// But if it returns error and you are rollbacking oplist, remember to
         /// abort oplist by logger's abortOplist.
         virtual INT32 releasePages(requestContext *context,
                                    UINT32 count,
                                    const PAGE_ID *lpids,
                                    DPS_LSN_OFFSET oplist) = 0;

         /// pages only preallocated not included. 
         virtual INT32 getPhysicalPid(requestContext *context,
                                      PAGE_ID lpid,
                                      PAGE_ID &pid,
                                      SNAPSHOT_ID *snap) = 0;

         /// The query on imp should be excuted automatically
         /// if oldPid is invalid.
         virtual INT32 copyOnWirte(requestContext *context,
                                   PAGE_ID lpid,
                                   PAGE_ID oldPid,
                                   PAGE_ID &newPid,
                                   SNAPSHOT_ID *snap) = 0;

         /// copyOnWirte first if necessary and then return.
         virtual INT32 getPhysicalPidToWrite(requestContext *context,
                                             PAGE_ID lpid,
                                             PAGE_ID &pid,
                                             SNAPSHOT_ID *snap) = 0;
      protected:
         PAGE_ID getImpPidOfLpid(PAGE_ID lpid)const;
         PAGE_ID getSMPPIdOfImp(PAGE_ID pid)const;

      protected:
         INT32 preallocateLogicalPids(requestContext *context,
                                      UINT32 count,
                                      PAGE_ID *lpids);
         void releaseLogicalPidsPreallocated(requestContext *context,
                                             UINT32 count,
                                             const PAGE_ID *lpids);

         /// extending will not be perfermed
         /// if pageInPoolBeforeExtending is lower than current count.
         INT32 extendLogicalPidSpace(requestContext *context,
                                     const UINT32 *pageInPoolBeforeExtending);

         INT32 extendPhysicalPidSpace(requestContext *context,
                                      const UINT32 *pageInPoolBeforeExtending);

         INT32 mapImpPageToBitmap(requestContext *context,
                                  UINT32 impPageSize,
                                  UINT32 impCapacity,
                                  PAGE_ID impPid,
                                  FILE_TYPE type,
                                  storageUnit *su,
                                  inMemBitMap &bitmap);

         INT32 mapSMPPageToBitmap(requestContext *context,
                                  UINT32 pageSize,
                                  UINT32 capacity,
                                  PAGE_ID pid,
                                  FILE_TYPE type,
                                  storageUnit *su,
                                  inMemBitMap &bitmap);

      private:
         INT32 initInMemPpidPoolFromDisk(requestContext *context);
         INT32 initInMemLpidPoolFromDisk(requestContext *context);

      private:
         /// The format of meta file should always be:
         /// | smp * n | system pages | reserved id map pages | id map pages|
         virtual UINT32 getSystemPageCount()const = 0;
         virtual UINT32 getReservedImpCount()const = 0;
         virtual FILE_TYPE getTypeOfMetaFile()const = 0;
         virtual FILE_TYPE getTypeOfDataFile()const = 0;
         virtual UINT32 getFreeBoundOfLpidPool()const = 0;
         virtual UINT32 getFreeBoundOfPpidPool()const = 0;
      private:
         virtual INT32 allocateIdMapPagesOnDisk(requestContext *context,
                                                PAGE_ID first,
                                                UINT32 count) = 0;

         virtual INT32 createDataFile(requestContext *context,
                                      UINT64 sequence) = 0;

         virtual UINT32 getDataFileCount() = 0;
         virtual INT32 getDataSMPOfFile(UINT32 sequence, UINT32 i, PAGE_ID &pid) = 0;

      private:
         ossSpinXLatch _extendingLatch;
         storageUnit *_su = NULL;
         inMemBitMap _lpidPool;
         UINT32 _pageCountInMeta = 0;
         inMemBitMap _ppidPool;
         UINT32 _dataFileCount = 0;
      
   };//class logicalPageSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGIAL_PAGE_SPACE_H_