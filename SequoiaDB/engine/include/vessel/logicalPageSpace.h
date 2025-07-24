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

   Source File Name = logicalPageSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOGIAL_PAGE_SPACE_H_
#define VESSEL_LOGIAL_PAGE_SPACE_H_

#include "vessel/storageManifest.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/metaDataUberBlock.h"
#include "vessel/lpageMetaDataFile.h"
#include "vessel/fclusterSpaceManager.h"
#include "vessel/storageFileCluster.h"
#include "vessel/lpageMapping.h"
#include "vessel/storageFileLoader.h"
#include "vessel/lpageDescriptor.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/unitedBitmap.hpp"
#include "vessel/shallowPointer.hpp"
#include "vessel/sparseBitmap32.h"

namespace engine
{
namespace vessel
{
   class requestContext;
   class pageInitializer;

   class logicalPageSpace : public SDBObject
   {
      public:
         logicalPageSpace(const storageUnitManifest *manifest);
         virtual ~logicalPageSpace();
         logicalPageSpace(const logicalPageSpace &) = delete;
         logicalPageSpace &operator=(const logicalPageSpace &) = delete;

      private:
         typedef class unitedBitmap<lpageMapping::LPID_UNIT_SIZE> _LPID_ALLOCATOR;

      public:
         virtual SPACE_TYPE getSpaceType()const = 0;

      public:
         OSS_INLINE BOOLEAN isOpen()const {return _mfile.isOpen();}
         OSS_INLINE const storageUnitManifest *getManifest()const {return _manifest;}
         OSS_INLINE SPACE_ID getSpaceID()const
         {
            return nullptr == _manifest ? INVALID_SPACE_ID : _manifest->id.getSpaceId();
         }
         OSS_INLINE const storageFileCluster *getFileCluster()const {return &_fcluster;}
         OSS_INLINE storageFileCluster *getFileCluster() {return &_fcluster;}

      public:
         INT32 create();
         INT32 open(const storageFileLoader &loader);
         void close();
         void destroy();

      public:
         virtual INT32 getLogicalPageBuffer(requestContext *context,
                                            PAGE_ID lpid,
                                            const ossSharedLatchMode &mode,
                                            logicalPageBuffer &lpb);

         virtual INT32 tryToGetLogicalPageBuffer(requestContext *context,
                                                 PAGE_ID lpid,
                                                 const ossSharedLatchMode &mode,
                                                 logicalPageBuffer &lpb);

         ///WARNING: user should lock lpid out side if necessary.
         /// return ok but invalid desc if unmapped.
         virtual INT32 testLogicalPageMapping(PAGE_ID lpid,
                                              lpageDescriptor &desc);

         /// Make buffer from "getLogicalPageBuffer" writable.
         /// Buffer with shared locking can not be writable.
         virtual INT32 makeBufferWritable(logicalPageBuffer &lpb);

         /// lpid must be in reserved by _getReservedLpidUnits.
         /// Page will be created if lpid unmapped.
         /// If lpid has already been mapped, will do nothing.
         /// Should always get exclusive latch out side.
         /// If just want to read a reserved page ,jsut use "getLogicalPageBuffer".
         virtual INT32 ensureReservedPageMapped(requestContext *context,
                                                PAGE_ID lpid,
                                                pageInitializer *initer);

         virtual INT32 allocatePages(requestContext *context,
                                     pageInitializer *initer,
                                     UINT32 count,
                                     PAGE_ID *lpids);

         INT32 allocatePage(requestContext *context,
                            pageInitializer *initer,
                            PAGE_ID &lpid)
         {
            return allocatePages(context, initer, 1, &lpid);
         }

         virtual INT32 releasePages(requestContext *context,
                                    UINT32 count,
                                    const PAGE_ID *lpids);

         INT32 releasePage(requestContext *context,
                           PAGE_ID lpid)
         {
            return releasePages(context, 1, &lpid);
         }

      private:
         virtual INT32 _onCreationStarted() {return SDB_OK;}
         virtual INT32 _onCreationFinished() {return SDB_OK;}
         virtual INT32 _onOpenStarted(const storageFileLoader &loader) {return SDB_OK;}
         virtual INT32 _onOpenFinished(const storageFileLoader &loader) {return SDB_OK;}
         virtual void _onClosingStarted() {return;}
         virtual void _onClosingFinished() {return;}
         virtual void _onDestroyStarted() {return;}
         virtual void _onDestroyFinished() {return;}

      protected:
         virtual UINT32 _getReservedLpidUnits()const {return 0;}
         virtual UINT32 _getStorageFileCtlFlags()const
         {
            return storageFileCtlFlag::MMAP_DATA_SEGMENT;
         }

         /// the segment can be reused with the min free page count 
         virtual UINT32 _getSegmentPcntReused()const {return 8;}

         virtual INT32 _getRuntimePageBuffer(requestContext *context,
                                             PAGE_ID pid,
                                             const ossSharedLatchMode &mode,
                                             runtimePageBuffer &rpb);

         virtual INT32 _getRuntimePageBufferToReset(requestContext *context,
                                                    PAGE_ID pid,
                                                    runtimePageBuffer &rpb);

         virtual INT32 _copyPageAndReinitBuffer(requestContext *context,
                                                PAGE_SNAPSHOT_VERION psv,
                                                PAGE_ID newPid,
                                                runtimePageBuffer &rpb);

         class _runtimePageBufferIniter : public SDBObject
         {            
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

         class _logicalPageBufferIniter : public SDBObject
         {
            public:
               void init(PAGE_ID lpid,
                         ossSharedLatchMode mode,
                         requestContext *context,
                         logicalPageSpace *lps,
                         runtimePageBuffer &&rpb,
                         PAGE_SNAPSHOT_VERION psv,
                         logicalPageBuffer &lpb);
               runtimePageBuffer &getRpbRef(logicalPageBuffer &lpb)
               {
                  return lpb._rpb;
               }
         };////class _logicalPageBufferIniter
         
      private:
         INT32 _createMetaFile();
         INT32 _openMetaFile();
         INT32 _initPageMapping(BOOLEAN creating);
         INT32 _createUberBlock();
         INT32 _openFileCluster(const storageFileLoader *loader);
         INT32 _initSpaceManager();
         INT32 _initLpidAllocator();

      protected:
         INT32 _initAndCreateMapping(requestContext *context,
                                     pageInitializer *initer,
                                     UINT32 size,
                                     const PAGE_ID *lpids,
                                     const PAGE_ID *pids);

         INT32 _reserveLpids(UINT32 size, PAGE_ID *lpids, BOOLEAN autoExtendLPM=TRUE);
         void _freeLpids(UINT32 size, const PAGE_ID *lpids);
         void _freeLpid(PAGE_ID lpid);
         void _freeLpids(const sparseBitmap32 &bm);

         OSS_INLINE BOOLEAN _isReservedLpid(PAGE_ID lpid)const
         {
            return lpid < (_getReservedLpidUnits() * lpageMapping::LPID_UNIT_SIZE);
         }

         fclusterSpaceManager &_getSpaceMgr() {return _smgr;}
         lpageMapping &_getPageMapping() {return _lpm;}
         lpageMetaDataFile &_getMetaFile() {return _mfile;}

         INT32 _updateUberBlockOnDisk(BOOLEAN fsync);

      private:
         const storageUnitManifest *_manifest = nullptr;
         lpageMetaDataFile _mfile;
         lpageMapping _lpm;
         storageFileCluster _fcluster;
         fclusterSpaceManager _smgr;

         std::mutex _am;
         _LPID_ALLOCATOR _allocator;
         
   };//class logicalPageSpace

   typedef class shallowPointer<logicalPageSpace> LPS_OBJ_PTR;
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOGIAL_PAGE_SPACE_H_