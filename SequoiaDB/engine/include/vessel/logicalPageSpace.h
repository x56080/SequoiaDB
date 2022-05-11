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
#include "vessel/storageFileName.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/deltaLogConsole.h"
#include "vessel/lpsCheckpointContext.h"
#include "vessel/storageFileLoader.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/sortedStorageFileList.h"
#include "vessel/shallowPointer.hpp"
#include "vessel/containerUtils.h"
#include "vessel/pidBatchList.h"
#include "vessel/lpageMapping.h"
#include "ossMemPool.hpp"
#include "vessel/idMapPage.h"
#include "vessel/storageFileTrashCan.h"
#include "vessel/liteIOBuffer.h"
#include "vessel/storageManifest.h"
#include "vessel/storageFileCluster.h"

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
         logicalPageSpace(const storageUnitManifest *manifest);
         virtual ~logicalPageSpace();
         logicalPageSpace(const logicalPageSpace &) = delete;
         logicalPageSpace &operator=(const logicalPageSpace &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const {return nullptr != _baseMap;}
         OSS_INLINE const storageUnitManifest *getManifest()const {return _manifest;}
         OSS_INLINE SPACE_ID getSpaceID()const {return _manifest->sid;}
         static constexpr UINT32 MAX_LPID_COUNT = ID_MAP_PAGE_CAPACITY * ID_MAP_FILE_MAX_PAGE_COUNT;
      public:
         virtual SPACE_TYPE getSpaceType()const = 0;
         virtual BOOLEAN isCopyOnWrite()const = 0;

      public:
         void close();

         void destroy();

         INT32 create();

         INT32 open(const storageFileLoader &loader);

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
         storageFileCluster *getFileCluster() {return &_fcluster;}
         const storageFileCluster *getFileCluster()const {return &_fcluster;}
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
         OSS_INLINE lpageMapping &getMapping()
         {
            return _lpm;
         }

         INT32 reservePagesInMem(requestContext *context,
                                 LPID_MAPPING_ARRAY pages);

         void freePagesInMem(requestContext *context,
                             const LPID_MAPPING_ARRAY &pages);

         INT32 validateLpidBeforeGet(PAGE_ID lpid)const;

         BOOLEAN isReservedLpid(PAGE_ID lpid)const;

         INT32 _reserveClusterPids(UINT32 count, PAGE_ID *pids);

         void _releaseClusterPids(UINT32 count, const PAGE_ID *pids);
         
      protected:
         class _runtimePageBufferIniter : public SDBObject
         {
            public:
               _runtimePageBufferIniter(){}
               ~_runtimePageBufferIniter(){}
            
            public:
               ///WARNING: iob will be invalid after init.
               void initWithBuffer(const GLOBAL_PAGE_ID &gpid,
                                   UINT32 pageSize,
                                   liteIOBuffer &iob,
                                   runtimePageBuffer &rpb);

               void initWithMmap(const GLOBAL_PAGE_ID &gpid,
                                 UINT32 pageSize,
                                 const mmapPagePointer &ptr,
                                 runtimePageBuffer &rpb);
         };//class _runtimePageBufferIniter

      private:
         virtual UINT32 getReservedImpCount()const {return 0;}

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
         virtual UINT32 _getStorageFileCtlFlags()const
         {
            return storageFileCtlFlag::MMAP_DATA_SEGMENT;
         }
         virtual inMemBitmap::options _getFAllocatorOptions()const;
         virtual INT32 _create() {return SDB_OK;}
         virtual INT32 _open(const storageFileLoader &loader) {return SDB_OK;}
         virtual void _close() {}
         virtual void _destroy() {}

      private:
         void fini();
         INT32 createFirstIdMapFile();
         INT32 openIdMapFiles(const storageFileLoader &loader);

         INT32 restoreAllocatorByBaseFile(const idMapFile *base);

         INT32 restoreAllocatorByImp(PAGE_ID impPid,
                                     const CHAR *page);

         INT32 rebaseWhenCreatingCheckpoint(requestContext *context,
                                            const LPS_CHECKPOINT &checkpoint,
                                            storageFileTrashCan &trashCan);

         INT32 flushSegmentsAtCheckpoint(requestContext *context,
                                         const ossPoolSet<UINT32> &segments)const;

         INT32 mergeDataIntoNewBase(const idMapFile *base,
                                    const DELTA_PAGE_LIST &delta,
                                    idMapFile *newBase);

      private:
         virtual INT32 getMinUncompletedLSN(requestContext *context,
                                            DPS_LSN_OFFSET &lsn);

         virtual void endToCreateCheckpoint(requestContext *context){return;}

         BOOLEAN needFullCheckpoint();

         void postCheckpointIfNecessary(requestContext *context);
         INT32 createDeltaCheckpoint(requestContext *context);
         INT32 createFullCheckpoint(requestContext *context);

         INT32 resumeToLatestCheckpoint();
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

         INT32 _openFileCluster(const storageFileLoader *loader);

      private:
         INT32 replayDeltaLog();
         INT32 replayMappingDeltaLog(const deltaLogRecord &dlr);
         INT32 replayUnmappingDeltaLog(const deltaLogRecord &dlr);
         INT32 replayRemmapingDeltaLog(const deltaLogRecord &dlr);

      private:
         void pushIntoWaitingFreeList(const PID_ARRAY &pids);
         void mergeWaitingListIntoReadyList();
         void freePidsInReadyList();

      private:
         const storageUnitManifest *_manifest = nullptr;
         idMapFile *_baseMap = nullptr;
         inMemBitmap _allocator;
         ossSpinXLatch _mappingLatch;
         deltaLogConsole _logConsole;
         lpageMapping _lpm;

         std::mutex _fclusterMutex;
         storageFileCluster _fcluster;
         inMemBitmap _fallocator;

         lpsCheckpointContext _checkpointContext;
         ossPoolSet<UINT32> _dirtySegments;

         ossSpinXLatch _freeLatch;
         pidBatchList _waitingToFree;
         pidBatchList _readyToFree;
   };//class logicalPageSpace

   typedef shallowPointer<logicalPageSpace> LPS_OBJ_PTR;
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGIAL_PAGE_SPACE_H_