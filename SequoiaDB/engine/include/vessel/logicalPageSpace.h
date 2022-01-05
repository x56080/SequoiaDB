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
#include "vessel/inMemBitmap.h"
#include "ossLatch.hpp"
#include "vessel/slice.h"
#include "vessel/storageUnitDef.h"
#include "vessel/vesselFileName.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/deltaLogConsole.h"
#include "vessel/logicalPageIdCache.h"
#include "vessel/lpsCheckpointContext.h"
#include "vessel/storageFileLoader.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/sortedStorageFileList.h"
#include "vessel/shallowPointer.hpp"
#include "vessel/dataPageCluster.h"
#include "vessel/containerUtils.h"
#include "vessel/pidBatchList.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class idMapFile;
   class atomicOperationList;
   class pageInitializer;

   class logicalPageSpace : public SDBObject
   {
      public:
         logicalPageSpace(){}
         virtual ~logicalPageSpace();
         logicalPageSpace(const logicalPageSpace &) = delete;
         logicalPageSpace &operator=(const logicalPageSpace &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return INVALID_SPACE_ID != _sid;
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _sid;
         }

         static constexpr UINT32 MAX_LPID_COUNT = ID_MAP_PAGE_CAPACITY * ID_MAP_FILE_MAX_PAGE_COUNT;
      public:
         virtual SPACE_TYPE getSpaceType()const = 0;
         virtual BOOLEAN isCopyOnWrite()const = 0;

      public:
         void close();

         void destroy(requestContext *context);

         INT32 create(requestContext *context,
                      const createLogicalPageSpaceOptions &o);

         INT32 open(requestContext *context,
                    const storageFileLoader &loader);

         INT32 getLogicalPageBuffer(requestContext *context,
                                    PAGE_ID lpid,
                                    const ossSharedLatchMode &mode,
                                    logicalPageBuffer &lpb);

         INT32 tryToGetLogicalPageBuffer(requestContext *context,
                                         PAGE_ID lpid,
                                         const ossSharedLatchMode &mode,
                                         logicalPageBuffer &lpb);

         INT32 isLogicalPageMapped(requestContext *context,
                                   PAGE_ID lpid,
                                   BOOLEAN &mapped);

         /// Make buffer from "getLogicalPageBuffer" writable.
         /// Buffer with shared locking can not be writable.
         INT32 makeBufferWritable(logicalPageBuffer &lpb);

         /// lpid must be in reserved imp.
         /// Page will be created if lpid unmapped.
         /// If lpid has already been mapped, will do nothing.
         /// Should always get exclusive latch out side.
         /// If just want to read a reserved page ,jsut use "getLogicalPageBuffer".
         INT32 ensureReservedPageMapped(requestContext *context,
                                        PAGE_ID lpid,
                                        pageInitializer *initer);

         INT32 allocatePages(requestContext *context,
                             pageInitializer *initer,
                             UINT32 count,
                             PAGE_ID *lpids);

         INT32 allocatePage(requestContext *context,
                            pageInitializer *initer,
                            PAGE_ID &lpid)
         {
            return allocatePages(context, initer, 1, &lpid);
         }

         INT32 releasePages(requestContext *context,
                            UINT32 count,
                            const PAGE_ID *lpids);
         INT32 releasePage(requestContext *context,
                           PAGE_ID lpid)
         {
            return releasePages(context, 1, &lpid);
         }

      public:
         /// WARNING: Can not guarantee data consistency!
         /*
         INT32 getPageMappingAtNonruntime(requestContext *context,
                                          PAGE_ID lpid,
                                          PAGE_ID &pid,
                                          PAGE_SNAPSHOT_VERION &psv,
                                          mmapPagePointer &ptr);*/

      public:
         FILE_TYPE getStorageFileType()const;
         const storageCoreArgs &getStorageCoreArgs()const;

         INT32 fsyncSegment(UINT32 segment)const;

         virtual INT32 getPagePtr(FILE_TYPE type,
                                  PAGE_ID pid,
                                  mmapPagePointer &ptr)const;
      public:
         INT32 createCheckpoint(requestContext *context,
                                BOOLEAN forceFullCheckpoint);

         INT32 blockCheckpoint(requestContext *context);
         INT32 tryToBlockCheckpoint(requestContext *context, BOOLEAN &blocked);
         void waitCheckpoint();
      protected:
         OSS_INLINE ossSpinXLatch *getMappingLatch()
         {
            return &_mappingLatch;
         }
         OSS_INLINE lpsCheckpointContext &getCheckpointContext()
         {
            return _checkpointContext;
         }
         OSS_INLINE deltaLogConsole &getLogConsole()
         {
            return _logConsole;
         }
         OSS_INLINE logicalPageIdCache &getCache()
         {
            return _lpidCache;
         }

         INT32 reservePagesInMem(requestContext *context,
                                 LPID_MAPPING_ARRAY pages);

         void freePagesInMem(requestContext *context,
                             const LPID_MAPPING_ARRAY &pages);

         INT32 validateLpidBeforeGet(PAGE_ID lpid)const;

         BOOLEAN isReservedLpid(PAGE_ID lpid)const;

         const sortedStorageFileList &getIdMapFileList()
         {
            return _idMapFiles;
         }
      protected:
         class _runtimePageBufferIniter : public SDBObject
         {
            public:
               _runtimePageBufferIniter(){}
               ~_runtimePageBufferIniter(){}
            
            public:
               ///WARNING: tuple will be invalid after init.
               INT32 initWithCache(const GLOBAL_PAGE_ID &gpid,
                                   UINT32 pageSize,
                                   liteCacheTuple &tuple,
                                   runtimePageBuffer &rpb);

               INT32 initWithMmap(const GLOBAL_PAGE_ID &gpid,
                                  UINT32 pageSize,
                                  const mmapPagePointer &ptr,
                                  runtimePageBuffer &rpb);
         };//class _runtimePageBufferIniter

      protected:
         virtual dataPageCluster *getDataStorageObj() = 0;

      private:
         virtual UINT32 getReservedImpCount()const {return 0;}
         virtual dataPageCluster::options getStorageOptions()const
         {
            return dataPageCluster::options();
         }

      protected:/// page management
         INT32 map(requestContext *context,
                   PAGE_SNAPSHOT_VERION psv,
                   const LPID_MAPPING_ARRAY &mapping);

         INT32 remap(requestContext *context,
                     PAGE_SNAPSHOT_VERION psv,
                     const LPID_MAPPING_ARRAY &mapping,
                     const PID_ARRAY &oldPids,
                     BOOLEAN releaseOld);

         INT32 unmap(requestContext *context,
                     const LPID_MAPPING_ARRAY &mapping,
                     BOOLEAN releasePid);

      private:/// for data storage.
         virtual INT32 getRuntimePageBuffer(requestContext *context,
                                            PAGE_ID pid,
                                            const ossSharedLatchMode &mode,
                                            runtimePageBuffer &rpb) = 0;

         /// rpb must be writable at last
         virtual INT32 getRuntimePageBufferToReset(requestContext *context,
                                                   PAGE_ID pid,
                                                   runtimePageBuffer &rpb) = 0;

         /// rpb must be writable at last
         virtual INT32 copyPageAndReinitBuffer(requestContext *context,
                                               PAGE_SNAPSHOT_VERION psv,
                                               PAGE_ID newPid,
                                               runtimePageBuffer &rpb) = 0;

      private:/// for openning/creating
         virtual INT32 _create(requestContext *context) = 0;
         virtual INT32 _open(requestContext *context,
                             const storageFileLoader &loader) = 0;
         virtual void _close() = 0;
         virtual void _destroy(requestContext *context) = 0;

         virtual UINT32 createIdMapFileHeadFlags()const;
         virtual BOOLEAN validateIdMapFileHeadFlags(UINT32 flags)const;

      private:
         void fini();
         INT32 createFirstIdMapFile(requestContext *context,
                                    const createLogicalPageSpaceOptions &o);
         INT32 openIdMapFiles(requestContext *context,
                              const storageFileLoader &loader);

         INT32 validateIdMapFileMap();

         INT32 restoreAllocatorByBaseFile(requestContext *context,
                                          const idMapFile *base);

         INT32 restoreAllocatorByImp(requestContext *context,
                                     PAGE_ID impPid,
                                     const CHAR *page);

         INT32 rebaseWhenCreatingCheckpoint(requestContext *context,
                                            const LPS_CHECKPOINT &checkpoint);

         INT32 flushSegmentsAtCheckpoint(requestContext *context,
                                         const ossPoolSet<UINT32> &segments)const;

         void extractDirtySegmentsInCache(ossPoolSet<UINT32> &segments);

      private:
         virtual INT32 getMinUncompletedLSN(requestContext *context,
                                            DPS_LSN_OFFSET &lsn);

         virtual void endToCreateCheckpoint(requestContext *context){return;}

         BOOLEAN needFullCheckpoint();

         void postCheckpointIfNecessary(requestContext *context);
         INT32 createDeltaCheckpoint(requestContext *context);
         INT32 createFullCheckpoint(requestContext *context);

         INT32 resumeToLatestCheckpoint(requestContext *context);
      private:
         INT32 reserveLpidsInMem(requestContext *context,
                                 PID_ARRAY lpids);

         void freeLpidInMem(requestContext *context,
                            PAGE_ID lpid);
         void freeLpidsInMem(requestContext *context,
                              const PID_ARRAY &lpids);

         INT32 remapBufferToNewDataPage(requestContext *context,
                                        BOOLEAN releaseOld,
                                        logicalPageBuffer &lpb);

         INT32 ensureLogicalPidSpace(PAGE_ID lpid);

         INT32 initAndMapPages(requestContext *context,
                               pageInitializer *initer,
                               const LPID_MAPPING_ARRAY &mapping);

      private:
         INT32 replayDeltaLog(requestContext *context);
         INT32 replayMappingDeltaLog(requestContext *context, const deltaLogRecord &dlr);
         INT32 replayUnmappingDeltaLog(requestContext *context, const deltaLogRecord &dlr);
         INT32 replayRemmapingDeltaLog(requestContext *context, const deltaLogRecord &dlr);

      private:
         void pushIntoWaitingFreeList(const PID_ARRAY &pids);
         void mergeWaitingListIntoReadyList();
         void freePidsInReadyList();

      private:
         SPACE_ID _sid = INVALID_SPACE_ID;
         sortedStorageFileList _idMapFiles;
         inMemBitmap _allocator;
         ossSpinXLatch _mappingLatch;
         deltaLogConsole _logConsole;
         logicalPageIdCache _lpidCache;
         dataPageCluster *_dpc = NULL;
         lpsCheckpointContext _checkpointContext;

         ossSpinXLatch _freeLatch;
         pidBatchList _waitingToFree;
         pidBatchList _readyToFree;
   };//class logicalPageSpace

   typedef shallowPointer<logicalPageSpace> LPS_OBJ_PTR;
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGIAL_PAGE_SPACE_H_