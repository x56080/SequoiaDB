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

   Source File Name = collectionSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_SPACE_H_
#define VESSEL_COLLECTION_SPACE_H_

#include "vessel/vesselIdDef.h"
#include "vessel/vesselOptions.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/collectionAllocator.h"
#include "vessel/storageUnitDef.h"
#include "vessel/strSlice.h"
#include "vessel/inMemBitMap.h"
#include "vessel/listCollectionSpaceDef.h"
#include "vessel/listCollectionsDef.h"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class collection;
   class requestContext;
   class storageUnit;
   class listCLCursor;
   class fsmFile;

   class collectionSpace : public SDBObject
   {
      public:
         collectionSpace();
         ~collectionSpace();
         collectionSpace(const collectionSpace &o) = delete;
         collectionSpace &operator=(const collectionSpace &o) = delete;

      private:
         enum CS_IN_MEM_STATUS
         {
            CLOSED = 0,
            SU_LOADED = 1,
            OPEN = 2,
         };//enum CS_IN_MEM_STATUS
   
      public:
         OSS_INLINE BOOLEAN isClosed()const
         {
            return CLOSED == _status;
         }
         OSS_INLINE BOOLEAN suIsLoaded()const
         {
            return _status >= SU_LOADED;
         }
         OSS_INLINE BOOLEAN isOpen()const
         {
            return OPEN == _status;
         }

         OSS_INLINE const CHAR *getCSName()const
         {
            return _recordInMem.name;
         }
         OSS_INLINE UINT32 getUniqueID()const
         {
            return _recordInMem.uniqueID; 
         }

         OSS_INLINE UINT32 getStatus()const
         {
            return _recordInMem.status;
         }
         OSS_INLINE UINT32 getFlags()const
         {
            return _recordInMem.flags;
         }
         OSS_INLINE UINT32 getVersion()const
         {
            return _recordInMem.version;
         }
         OSS_INLINE storageUnit *getSU()
         {
            return _su;
         }
         OSS_INLINE BOOLEAN isOnline()const
         {
            return _recordInMem.isOnline();
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _recordInMem.csLogicalID;
         }
      public:
         INT32 create(requestContext *context,
                      const strSlice &name,
                      utilCSUniqueID uniqueID,
                      UINT32 logicalID,
                      const createCSOptions &options);

         /// opening collection space should always
         /// be in two steps: openSU and openAfterSULoaded
         INT32 openSU(requestContext *context,
                      const strSlice &dirName);

         INT32 openAfterSULoaded(requestContext *context);

         INT32 destroy(requestContext *context);

         void close(requestContext *context);

         INT32 createCL(requestContext *context,
                        const strSlice &clName, 
                        utilCLInnerID clInnerId,
                        const createCLOptions &options);

      public:
         INT32 getDataPageSize(UINT32 &pageSize);
      public:
         SPACE_ID getSpaceID()const;

         UINT32 getCollectionCount();

         INT32 listCollections(requestContext *context,
                               listCLCursor *cursor);

         INT32 dump(requestContext *context,
                    listCollectionSpaceRecord &record);

         INT32 getCollectionByName(requestContext *context,
                                   const strSlice &clName, 
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

         INT32 getCollectionByMBID(requestContext *context,
                                   CL_MB_ID mbID,
                                   UINT32 logicalID,
                                   OSS_LATCH_MODE mode,
                                   collection **obj);

         INT32 ensureFsmFile(requestContext *context,
                             fsmFile **file);

      public:
         INT32 getLpidOfClRecord(CL_MB_ID mbID, PAGE_ID &lpid)const;
         INT32 getDataPhyPidInIdMapToRead(requestContext *context,
                                          PAGE_ID lpid,
                                          PAGE_ID &pid);
         INT32 getDataPhyPidInIdMapToWrite(requestContext *context,
                                           PAGE_ID lpid,
                                           PAGE_ID &pid);

         /// cs will read phy pid automatically if toBeCow set as invalid value 
         INT32 copyOnWritePageInDFile(requestContext *context,
                                      PAGE_ID lpid,
                                      PAGE_ID toBeCow,
                                      PAGE_ID &pid);         


         /// preallocate physical pids in mem
         INT32 preallocatePhyPagesInDFile(requestContext *context,
                                          UINT32 count,
                                          PAGE_ID *pids);
         void releasePhyPagesPreallocated(requestContext *context,
                                           UINT32 count,
                                           const PAGE_ID *pids);

         /// preallocate logical and physical pids in mem
         INT32 preallocateDataPages(requestContext *context,
                                    UINT32 count,
                                    PAGE_ID *lpids,
                                    PAGE_ID *pids);

         void releaseDataPagesPreallocated(requestContext *context,
                                           UINT32 count,
                                           const PAGE_ID *lpids,
                                           const PAGE_ID *pids);

         /// preallocate logical pids in mem
         INT32 preallocateLpids(requestContext *context,
                                UINT32 count,
                                PAGE_ID *lpids);
         void releaseLpidsPreallocated(requestContext *context,
                                       UINT32 count,
                                       PAGE_ID *lpids);

         /// allocate physical pids and map them to logical pids in disk.
         INT32 allocateDataPages(requestContext *context,
                                 PAGE_TYPE type,
                                 UINT32 count,
                                 const PAGE_ID *lpids,
                                 const PAGE_ID *pids,
                                 const slice &args,
                                 DPS_LSN_OFFSET *oplist=NULL);

         /// releaseDataPages will also release lpids in memory.
         /// Physical pids' releasing depends on snapshot version.
         /// If physical pid can be recycled, it will be released
         /// in memory also.
         /// In another word, you do not need to care about "releaseXXPrealloated"
         /// any more if releaseDataPages returns ok.
         /// But if it returns error and you are rollbacking oplist, remember to
         /// abort oplist by logger's abortOplist.
         INT32 releaseDataPages(requestContext *context,
                                UINT32 count,
                                const PAGE_ID *lpids,
                                DPS_LSN_OFFSET oplist=DPS_INVALID_LSN_OFFSET);
      private:
         PAGE_ID getDataSMPPId(PAGE_ID pid);
         INT32 allocateDataPagesOnSMP(requestContext *context,
                                      PAGE_TYPE type,
                                      UINT32 count,
                                      const PAGE_ID *lpids,
                                      const PAGE_ID *pids,
                                      const slice &extArgs,
                                      DPS_LSN_OFFSET *oplist);
         INT32 releaseDataPagesOnSMP(requestContext *context,
                                     UINT32 count,
                                     const PAGE_ID *pids,
                                     DPS_LSN_OFFSET *oplist);
         INT32 mapNewLpids(requestContext *context,
                           UINT32 count,
                           const PAGE_ID *lpids,
                           const PAGE_ID *pids,
                           DPS_LSN_OFFSET oplist,
                           BOOLEAN oplistTail=FALSE);

         /// under lpid exclusive latch.
         /// you must remap lpid to a new physical page before writing if mustBeCow is TRUE.
         INT32 getDataPhyPidInIdMapToWrite(requestContext *context,
                                           PAGE_ID lpid,
                                           PAGE_ID &pid,
                                           BOOLEAN &mustBeCow);
      private:
         BOOLEAN addToCreatingIndex(const strSlice &clName,
                                    utilCLInnerID innerID);
         void rollbackCreatingIndex(const strSlice &clName,
                                     utilCLInnerID innerID);

         void moveToFormalIndex(collectionAllocator::collectionHolder *holder);

         BOOLEAN insertIntoFormalIndex(collectionAllocator::collectionHolder *holder);

         void eraseFromIndex(const strSlice &clName,
                             utilCLInnerID innerID);

         BOOLEAN existsInFormalIndex(const strSlice &clName,
                                     utilCLInnerID innerID);

         /// Should always use name in nameBuffer instead of
         /// name in cl obj to do next upper bound.
         BOOLEAN upperBoundCLName(const strSlice &clName,
                                  UINT32 bufferSize,
                                  CHAR *nameBuffer,
                                  UINT32 &logicalID,
                                  collectionAllocator::collectionHolder **holder);

         BOOLEAN findCollection(const strSlice &clName,
                                UINT32 &logicalID,
                                collectionAllocator::collectionHolder **holder);
         BOOLEAN findCollection(utilCLInnerID innerID,
                                UINT32 &logicalID,
                                collectionAllocator::collectionHolder **holder);

      private:
         void fini();
         INT32 initNecessaryPagesWhenCreating(requestContext *context);
         INT32 updateStatusToOnlineWhenCreating(requestContext *context);
         INT32 firstExtendMetaFile(requestContext *context);
         INT32 initGMP(requestContext *context,
                       const csMetaRecord &record);
         INT32 initIMP(requestContext *context,
                       PAGE_ID pid);

         INT32 initDataSMP(requestContext *context,
                           PAGE_ID pid);

         ///can be used only when start.
         INT32 cacheGlobalMetaData(requestContext *context);

         INT32 initCollectionsFromDisk(requestContext *context);

         INT32 initCollectionsFromOneDiskPage(requestContext *context,
                                              PAGE_ID pid);

         INT32 allocateCLLogicalID(requestContext *context,
                                   UINT32 &lid);
         PAGE_ID getDataIMPPid(PAGE_ID lpid)const;

         INT32 createNewDataFileAndExtendBitMap(requestContext *context,
                                                const UINT32 *oldPageCount);

         INT32 ensureNewIMPAndExtendPool(requestContext *context,
                                         UINT32 oldPageAllocated);

         INT32 cacheKeyParametersAboutStorage();
         
         BOOLEAN isInMemBitMapsReady()const;
         INT32 initInMemBitMaps(requestContext *context);
         INT32 initInMemLpidPool(requestContext *context);
         INT32 initInMemDataSMPBitMap(requestContext *context);

      private:
         struct comp
         {
            BOOLEAN operator()(const CHAR *l, const CHAR *r)const
            {
               return ossStrcmp(l, r) < 0;
            }
         };//struct comp

         struct _UID_HOLDER_PAIR
         {
            OSS_INLINE _UID_HOLDER_PAIR():
            holder(NULL),
            logicalID(DMS_INVALID_LOGICCLID)
            {}
            OSS_INLINE _UID_HOLDER_PAIR(collectionAllocator::collectionHolder *h, UINT32 lid):
            holder(h),
            logicalID(lid)
            {}
            OSS_INLINE ~_UID_HOLDER_PAIR()
            {
               holder = NULL;
               logicalID = DMS_INVALID_LOGICCLID;
            }
            OSS_INLINE _UID_HOLDER_PAIR(const _UID_HOLDER_PAIR &o):
            holder(o.holder),
            logicalID(o.logicalID)
            {}
            OSS_INLINE _UID_HOLDER_PAIR &operator=(const _UID_HOLDER_PAIR &o)
            {
               holder = o.holder;
               logicalID = o.logicalID;
               return *this;
            }
            OSS_INLINE BOOLEAN isValid()const
            {
               return NULL != holder && DMS_INVALID_LOGICCLID != logicalID;
            }

            collectionAllocator::collectionHolder *holder;
            UINT32 logicalID;
         };//struct _UID_HOLDER_PAIR

         ///At present, the holder will not be destroyed after being allocated.
         ///We can store it's pointer here.
         typedef ossPoolMap<const CHAR *, _UID_HOLDER_PAIR, comp> NAME_INDEX;
         typedef ossPoolMap<utilCLInnerID, _UID_HOLDER_PAIR> ID_INDEX;
         typedef ossPoolSet<ossPoolString> CREATING_NAME_INDEX;
         typedef ossPoolSet<utilCLInnerID> CREATING_ID_INDEX;

         
      private:
         CS_IN_MEM_STATUS _status;
         storageUnit *_su;
         csMetaRecord _recordInMem;

         UINT32 _capacityOfCLRecordPage;
         UINT32 _idMapCapacity;
         UINT32 _maxPageCountPerDataFile;
         UINT32 _maxPageCountPerMetaFile;

         inMemBitMap _inMemDataSMP;
         inMemBitMap _inMemLpidPool;
         UINT32 _pageAllocatedInMetaSMP;
         collectionAllocator _collectionAllocator;

         NAME_INDEX _clNameIndex;
         ID_INDEX _clIdIndex;
         CREATING_NAME_INDEX _creatingNameIndex;
         CREATING_ID_INDEX _creatingIdIndex;
         
         ossSpinXLatch _dataAndMetaSpaceLatch;
         ossSpinSLatch _indexLatch;
   };//class collectionSpace
}
}

#endif//VESSEL_COLLECTION_SPACE_H_