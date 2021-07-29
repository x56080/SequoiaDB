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
#include "vessel/storageFileCreater.h"
#include "vessel/logicalPageIdCache.h"
#include "vessel/lpsCheckpointContext.h"
#include "vessel/storageFileLoader.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/sortedStorageFileList.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class idMapFile;
   class atomicOperationList;
   class pageInitializer;
   class dataPageCluster;

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
            return _creater.isValid();
         }
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return _creater.getSpaceID();
         }

         static constexpr UINT32 MAX_LPID_COUNT = ID_MAP_PAGE_CAPACITY * ID_MAP_FILE_MAX_PAGE_COUNT;
      public:
         virtual SPACE_TYPE getSpaceType()const = 0;

      public:
         void close();

         void destroy();

         INT32 create(requestContext *context,
                      const createLogicalPageSpaceOptions &o);

         INT32 open(requestContext *context,
                    SPACE_ID sid,
                    const CHAR *dirPath);

         INT32 getLogicalPageBuffer(requestContext *context,
                                    PAGE_ID lpid,
                                    OSS_SHARED_LATCH_MODE mode,
                                    logicalPageBuffer &lpb);

         /// Make buffer from "getLogicalPageBuffer" writable.
         /// Buffer with shared locking can not be writable.
         INT32 makeBufferWritable(requestContext *context,
                                  logicalPageBuffer &lpb);

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

         INT32 releasePages(requestContext *context,
                            UINT32 count,
                            const PAGE_ID *lpids);

      public:
         /// WARNING: Can not guarantee data consistency!
         INT32 getPageMappingAtNonruntime(requestContext *context,
                                          PAGE_ID lpid,
                                          PAGE_ID &pid,
                                          PAGE_SNAPSHOT_VERION &psv,
                                          mmapPagePointer &ptr);

      public:
         FILE_TYPE getStorageFileType()const;
         const storageCoreArgs &getStorageCoreArgs()const;

         INT32 fsyncSegment(UINT32 segment)const;

         virtual INT32 getPagePtr(FILE_TYPE type,
                                  PAGE_ID pid,
                                  mmapPagePointer &ptr)const;
      public:
         INT32 createCheckpoint(requestContext *context);

         INT32 blockCheckpoint(requestContext *context);
         INT32 tryToBlockCheckpoint(requestContext *context, BOOLEAN &blocked);

      private:
         /// Under checkpoint x latch.
         /// Must resume x latch if released in func.
         virtual INT32 prepareToCreateCheckpoint(requestContext *context) = 0;

         virtual INT32 prepareToFlushSegments(requestContext *context,
                                              BOOLEAN isFullCheckpoint,
                                              ossPoolSet<UINT32> &segments) = 0;

         virtual INT32 flushWhenCreatingCheckpoint(requestContext *context,
                                                   const ossPoolSet<UINT32> &segments)
         {
            return SDB_VESSEL_INTERNAL_ERR;
         }
  
      protected:
         OSS_INLINE const storageFileCreater &getCreater()const
         {
            return _creater;
         }
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

         INT32 preallocate(requestContext *context,
                           UINT32 count,
                           mappedLogicalPageId *mpids);

         void releasePreallocated(requestContext *context,
                                  UINT32 count,
                                  const mappedLogicalPageId *mpids);

         INT32 validateLpidBeforeGet(PAGE_ID lpid)const;

         BOOLEAN isReservedLpid(PAGE_ID lpid)const;
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
                                   const runtimePageBuffer::options &o,
                                   liteCacheTuple &tuple,
                                   runtimePageBuffer &rpb);

               INT32 initWithMmap(const GLOBAL_PAGE_ID &gpid,
                                  UINT32 pageSize,
                                  const runtimePageBuffer::options &o,
                                  const mmapPagePointer &ptr,
                                  runtimePageBuffer &rpb);
         };//class _runtimePageBufferIniter

      protected:
         virtual dataPageCluster *getDataStorageObj() = 0;

      private:
         virtual UINT32 getReservedImpCount()const {return 0;}
         virtual UINT32 getFreeBoundOfPageStorage()const = 0;
         virtual UINT32 getFreeBoundOfLpidAllocator()const = 0;

      private:/// page management
         virtual INT32 getPageFromCache(PAGE_ID lpid,
                                        idMapSlot &slot,
                                        BOOLEAN &isMutable);

         virtual INT32 map(requestContext *context,
                           PAGE_SNAPSHOT_VERION psv,
                           UINT32 count,
                           const mappedLogicalPageId *mpids) = 0;

         virtual INT32 remap(requestContext *context,
                             PAGE_SNAPSHOT_VERION psv,
                             UINT32 count,
                             const mappedLogicalPageId *mpids,
                             const PAGE_ID *oldPids,
                             BOOLEAN releaseOld) = 0;

         virtual INT32 unmap(requestContext *context,
                             UINT32 count,
                             const mappedLogicalPageId *mpids,
                             BOOLEAN releasePid) = 0;

         virtual INT32 releasePids(requestContext *context,
                                   UINT32 count,
                                   const PAGE_ID *pids) = 0;

      private:/// for data storage.
         virtual INT32 getRuntimePageBuffer(requestContext *context,
                                            PAGE_ID pid,
                                            OSS_SHARED_LATCH_MODE mode,
                                            const runtimePageBuffer::options &o,
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
         virtual INT32 _create(requestContext *context){return SDB_OK;}
         virtual INT32 _open(requestContext *context,
                             const storageFileLoader &loader){return SDB_OK;}
         virtual void _close(){return;}
         virtual void _destroy(){return ;}
         virtual UINT32 getIdMapFileHeadFlags()const = 0;
         virtual BOOLEAN validateIdMapFileHeadFlags(UINT32 flags)const = 0;

      private:
         void fini();
         INT32 createFirstIdMapFile(const storageCoreArgs &dataArgs);
         INT32 openIdMapFiles(SPACE_ID sid,
                              const strSlice &dir,
                              const storageFileLoader &loader);

         INT32 validateIdMapFileMap();

         INT32 restoreAllocatorByBaseFile(const idMapFile *base);
         INT32 restoreAllocatorByReservedImp(const idMapFile *base, PAGE_ID pid);
         INT32 restoreAllocatorByImp(const idMapFile *base, PAGE_ID pid);

         INT32 rebaseWhenCreatingCheckpoint(UINT32 totalImpCount,
                                            UINT64 deltaLogOffset);

         INT32 removeHistoryIdMapAndDeltaLogFiles();

      private:
         INT32 preallocateLpids(requestContext *context,
                                UINT32 count,
                                PAGE_ID *lpids);
         void releaseLpidsPreallocated(requestContext *context,
                                       UINT32 count,
                                       const PAGE_ID *lpids);

         INT32 remapBufferToNewDataPage(requestContext *context,
                                        BOOLEAN releaseOld,
                                        logicalPageBuffer &lpb);

         INT32 ensureLogicalPidSpace(PAGE_ID lpid);

         INT32 initAndMapPages(requestContext *context,
                               pageInitializer *initer,
                               UINT32 count,
                               const mappedLogicalPageId *mpids);

      private:
         INT32 replayDeltaLogWhenOpen(UINT64 beginOffset);
         INT32 replayLogRecord(const deltaLogRecord &dlr);
         INT32 replayMappingLogRecord(const deltaLogRecord &dlr);
         INT32 replayRemappingLogRecord(const deltaLogRecord &dlr);
         INT32 replayUnmappingLogRecord(const deltaLogRecord &dlr);
         INT32 replayReleasingLogRecord(const deltaLogRecord &dlr);

      private:
         storageFileCreater _creater;
         sortedStorageFileList _idMapFiles;
         inMemBitmap _allocator;
         ossSpinXLatch _mappingLatch;
         deltaLogConsole _logConsole;
         logicalPageIdCache _lpidCache;
         dataPageCluster *_dpc = NULL;
         lpsCheckpointContext _checkpointContext;
   };//class logicalPageSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_LOGIAL_PAGE_SPACE_H_