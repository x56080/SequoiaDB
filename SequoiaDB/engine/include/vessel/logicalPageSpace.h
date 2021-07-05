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
#include "vessel/storageFileMap.h"
#include "vessel/deltaLogConsole.h"
#include "vessel/storageFileCreater.h"
#include "vessel/logicalPageIdCache.h"
#include "vessel/lpsCheckpointContext.h"
#include "vessel/storageFileLoader.h"
#include "vessel/logicalPageBuffer.h"

namespace engine
{
namespace vessel
{
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
         virtual void close();
         virtual void destroy();
         virtual SPACE_TYPE getSpaceType()const = 0;

      public:

         INT32 create(requestContext *context,
                      const createLogicalPageSpaceOptions &o);

         INT32 open(requestContext *context,
                    SPACE_ID sid,
                    const CHAR *dirPath);

         INT32 getLogicalPageBuffer(requestContext *context,
                                    PAGE_ID lpid,
                                    ossSharedLatch::mode mode,
                                    logicalPageBuffer &lpb);

         /// Make buffer from "getLogicalPageBuffer" writable.
         /// Buffer with shared locking can not be writable.
         INT32 makeBufferWritable(requestContext *context,
                                  logicalPageBuffer &lpb);

         /// lpid must be in reserved imp.
         /// Page will be created if lpid unmapped and initer is valid.
         /// Will always get upgrade lock first.
         /// If just want to read a reserved page ,jsut use "getLogicalPageBuffer".
         INT32 ensureReservedPage(requestContext *context,
                                  PAGE_ID lpid,
                                  pageInitializer *initer,
                                  logicalPageBuffer &lpb);

         INT32 allocatePages(requestContext *context,
                             const pageInitializer *initer,
                             UINT32 count,
                             PAGE_ID *lpids);

         INT32 releasePages(requestContext *context,
                            UINT32 count,
                            const PAGE_ID *lpids, 
                            atomicOperationList *oplist);
      public:

         INT32 getStorageCoreArgs(FILE_TYPE type, storageCoreArgs &args)const;

         INT32 blockCheckpoint(requestContext *context);
         INT32 tryToBlockCheckpoint(requestContext *context, BOOLEAN &blocked);
  
      protected:
         OSS_INLINE const storageFileCreater &getCreater()const
         {
            return _creater;
         }

         INT32 preallocate(requestContext *context,
                           UINT32 count,
                           PAGE_ID *lpids,
                           PAGE_ID *pids);

         void releasePreallocated(requestContext *context,
                                  UINT32 count,
                                  const PAGE_ID *lpids,
                                  const PAGE_ID *pids);

         INT32 validateLpidBeforeGet(PAGE_ID lpid)const;

         BOOLEAN isReservedLpid(PAGE_ID lpid)const;

      protected:
         class _runtimePageBufferIniter : public SDBObject
         {
            public:
               _runtimePageBufferIniter(){}
               ~_runtimePageBufferIniter(){}
            
            public:
               INT32 initWithLiteCache(requestContext *context,
                                       const GLOBAL_PAGE_ID &gpid,
                                       ossSharedLatch::mode mode,
                                       const runtimePageBuffer::options &options,
                                       UINT32 pageSize,
                                       runtimePageBuffer &rpb);

               INT32 initWithMmap(requestContext *context,
                                 const GLOBAL_PAGE_ID &gpid,
                                 const runtimePageBuffer::options &options,
                                 UINT32 pageSize,
                                 const mmapPagePointer &ptr,
                                 runtimePageBuffer &rpb);
         };//class _runtimePageBufferIniter

      private:
         virtual UINT32 getReservedImpCount()const = 0;
         virtual UINT32 getFreeBoundOfPageStorage()const = 0;
         virtual UINT32 getFreeBoundOfLpidAllocator()const = 0;

         virtual UINT32 getIdMapFileHeadFlags()const = 0;
         virtual BOOLEAN validateIdMapFileHeadFlags(UINT32 flags)const = 0;
         virtual BOOLEAN isStandardPage()const{return TRUE;}
         

      private:/// for data storage.
         virtual dataPageCluster *allocateStorageObject();
         virtual INT32 getRuntimePageBuffer(requestContext *context,
                                            PAGE_ID pid,
                                            ossSharedLatch::mode mode,
                                            const runtimePageBuffer::options &o,
                                            runtimePageBuffer &rpb) = 0;

      private:/// for openning/creating
         virtual INT32 _create(requestContext *context){return SDB_OK;}
         virtual INT32 _open(requestContext *context){return SDB_OK;}

      private:
         void _close();
         void _destroy();
         INT32 createFirstIdMapFile();
         INT32 openIdMapFiles(SPACE_ID sid,
                              const strSlice &dir,
                              const storageFileLoader &loader);

         INT32 validateIdMapFileMap();

         INT32 restoreAllocatorByBaseFile(const idMapFile *base);
         INT32 restoreAllocatorByReservedImp(const idMapFile *base, PAGE_ID pid);
         INT32 restoreAllocatorByImp(const idMapFile *base, PAGE_ID pid);

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

         INT32 createPageAndCompleteBuffer(requestContext *context,
                                           pageInitializer *initer,
                                           logicalPageBuffer &lpb);

      private:
         virtual INT32 map(requestContext *context,
                           PAGE_SNAPSHOT_VERION psv,
                           UINT32 count,
                           const PAGE_ID *lpids,
                           const PAGE_ID *pids) = 0;

         INT32 remap(requestContext *context,
                     PAGE_SNAPSHOT_VERION psv,
                     UINT32 count,
                     const PAGE_ID *lpids,
                     const PAGE_ID *newPids,
                     const PAGE_ID *oldPids,
                     BOOLEAN releaseOld);

         INT32 unmap(requestContext *context,
                     UINT32 count,
                     const PAGE_ID *lpids,
                     const PAGE_ID *pids,
                     BOOLEAN releaseOld);

         INT32 replicatedRemmap(requestContext *context,
                                PAGE_SNAPSHOT_VERION psv,
                                UINT32 count,
                                const PAGE_ID *lpids,
                                const PAGE_ID *newPids,
                                const PAGE_ID *oldPids,
                                DPS_LSN_OFFSET *lsn);

      private:
         INT32 replayDeltaLogWhenOpen(UINT64 beginOffset);
         INT32 replayLogRecord(const deltaLogRecord &dlr);
         INT32 replayMappingLogRecord(const deltaLogRecord &dlr);
         INT32 replayRemappingLogRecord(const deltaLogRecord &dlr);
         INT32 replayUnmappingLogRecord(const deltaLogRecord &dlr);
         INT32 replayReleasingLogRecord(const deltaLogRecord &dlr);

      private:
         storageFileCreater _creater;
         storageFileMap _idMapFiles;
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