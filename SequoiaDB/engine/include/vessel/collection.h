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

   Source File Name = collection.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_H_
#define VESSEL_COLLECTION_H_

#include "vessel/clMetaBlockPage.h"
#include "vessel/recordID.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "vessel/recordID.h"
#include "vessel/listCollectionsDef.h"
#include "utilInsertResult.hpp"
#include "vessel/freeSpaceMap.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/collectionOptions.h"
#include "vessel/indexDescription.h"
#include "vessel/indexObjectMap.h"
#include "vessel/dmlIndexRequest.h"
#include "vessel/btreeRebuildingSortElement.h"
#include "vessel/modifyRecordContext.h"
#include "vessel/shallowPointer.hpp"
#include "vessel/dmlRequest.h"
#include "vessel/objectIdentifier.h"
#include "vessel/lobChunkKey.h"
#include "vessel/listLobChunkCursor.h"
#include "vessel/clEntryBlock.h"

#include <atomic>

namespace engine
{
   class _dpsLogRecord;
   class IRecordUpdater;
   class dmsLobChunkProfile;

namespace vessel
{
   class collectionSpace;
   class requestContext;
   class insertContext;
   class scanCLCursor;
   class dmlContext;
   class buildingIndexContext;
   class indexScanContext;
   class indexScanCursor;
   class bigRecordStream;
   
   class collection: public SDBObject
   {
      public:
         collection() = default;
         ~collection();
         collection(const collection &) = delete;
         collection &operator=(const collection &) = delete;

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return nullptr != _cs;
         }

         OSS_INLINE const collectionId &getId()const
         {
            return _entryBlock._properties.clid;
         }

         globalCollectionId getGlobalId()const;

         OSS_INLINE const collectionId &getCollectionId()const
         {
            return _entryBlock._properties.clid;
         }

         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _entryBlock._properties.clid.getMbId();
         }

         OSS_INLINE UINT32 getLogicalID()const
         {
            return _entryBlock._properties.clid.getLid();
         }

         OSS_INLINE utilCLInnerID getInnerID()const
         {
            return _entryBlock._properties.clid.getInnerId();
         }

         OSS_INLINE const CHAR *getName()const
         {
            return _entryBlock._properties.name.c_str();
         }

         OSS_INLINE const collectionProperties *getProperties()const
         {
            return _entryBlock.getProperties();
         }

      public:

         INT32 create(requestContext *context,
                      const strSlice &clName,
                      utilCLInnerID innerID,
                      UINT32 logicalID,
                      collectionSpace *cs,
                      const createCLOptions &options);

         /// init when startup
         INT32 initWhenOpen(requestContext *context,
                            const clMetaBlock &block,
                            collectionSpace *cs);

         INT32 destroy(requestContext *context);

         void fini();

         INT32 truncate(requestContext *context);

      public:
         INT32 createIndex(requestContext *context,
                           const dmsBuildIndexOptions &o,
                           const bson::BSONObj &adjunct);

         INT32 listIndexes(requestContext *context,
                           ossPoolVector<bson::BSONObj> &indexes);

         INT32 removeIndex(requestContext *context,
                           const strSlice &indexName);

         INT32 testNormalIndex(requestContext *context,
                               const strSlice &indexName,
                               indexIdentifier &indexId);

      public:
         INT32 dump(requestContext *context,
                    bson::BSONObj &record);

         INT32 getMoreWhenScan(requestContext *context,
                               scanCLCursor *cursor);

         INT32 getTotalRecordCount(requestContext *context,
                                   UINT64 &count);

         INT32 getMoreWhenIndexScan(indexScanContext *context);

      public:
         INT32 insert(dmlContext *context,
                      const dmlInsertRequest &request,
                      utilInsertResult *res);

         INT32 update(dmlContext *context,
                      const dmlUpdateRequest &request,
                      IRecordUpdater *updater,
                      utilUpdateResult *res);

         INT32 remove(dmlContext *context,
                      const dmlRemoveRequest &request,
                      utilDeleteResult *res);

      public:
         INT32 insertLobChunk(requestContext *context,
                              const lobChunkKey &key,
                              UINT32 offset,
                              const slice &data);

         INT32 readLobChunk(requestContext *context,
                            const lobChunkKey &key,
                            UINT32 offset,
                            UINT32 size,
                            CHAR *data,
                            UINT32 &readSize);

         INT32 removeLobChunk(requestContext *context,
                              const lobChunkKey &key);

         INT32 updateLobChunk(requestContext *context,
                              const lobChunkKey &key,
                              UINT32 offset,
                              const slice &data,
                              BOOLEAN createIfNotExists);

         INT32 truncateLobChunk(requestContext *context,
                                const lobChunkKey &key,
                                UINT32 size,
                                UINT32 &tsize);

         INT32 listLobChunks(requestContext *context,
                             listLobChunkCursor *cursor);

         INT32 testLobChunk(requestContext *context,
                            const lobChunkKey &key,
                            dmsLobChunkProfile *profile);

      private:
         INT32 _getMoreWhenIndexScan(indexScanContext *context,
                                     indexObject *obj);

      private:
         INT32 buildDmlIndexRequests(requestContext *context,
                                     const slice &record,
                                     dmlIndexRequestArray &requests)const;

         INT32 buildUpdateIndexRequests(requestContext *context,
                                        const slice &oldRecord,
                                        IRecordUpdater *updater,
                                        dmlIndexRequestArray &ra);

         INT32 buildRemoveIndexRequest(requestContext *context,
                                       const slice &oldRecord,
                                       dmlIndexRequestArray &ra);

         INT32 constraintCheck(dmlContext *context,
                               const dmlIndexRequestArray &ra,
                               BOOLEAN &duplicated,
                               utilInsertResult *res)const;

         INT32 insertIndexRequests(dmlContext *context,
                                   const dmlIndexRequestArray &ra);

         INT32 mergeIntoBuildingContext(dmlContext *context,
                                        dmlIndexRequestArray &ra);

         INT32 lockAndFetchRecordToModify(dmlContext *context, const recordID &rid);

      private:/// Used only when openning/creating.
         INT32 initPageSequenceWhenOpen(requestContext *context);
         INT32 initPageSequenceByRootLvL2(requestContext *context);
         INT32 initPageSequenceByRootLvL1(requestContext *context,
                                          UINT32 rootNo);
         INT32 initPageSequenceByRootLvL0(requestContext *context);

         /// User should always validate count and element when return SDB_OK.
         /// Count never shrink but element may be removed.
         INT32 getCountAndLastEleInRoutePage(requestContext *context,
                                             PAGE_ID lpid,
                                             INT32 lvl,
                                             UINT32 &count,
                                             PAGE_ID &element);

         INT32 initCLMetaBlockOnDisk(requestContext *context,
                                     const clMetaBlock &mb,
                                     const createCLOptions &options);

         INT32 removeCLMetaBlockOnDisk(requestContext *context);

         INT32 resetRouteRootOnDisk(requestContext *context);

      private:
         INT32 getMoreFromPageInCursor(requestContext *context,
                                       scanCLCursor *cursor,
                                       UINT32 &count);

         INT32 getRecordCountInPage(requestContext *context,
                                    PAGE_ID lpid,
                                    UINT32 &count);

      private:
         INT32 insertRecordData(dmlContext *context, 
                                const dmlInsertRequest &request);

         INT32 insertNormalRecord(dmlContext *context,
                                  const slice &record);

         INT32 insertBigRecord(dmlContext *context,
                               const slice &record);

         INT32 insertBigRecordSlices(dmlContext *context,
                                     bigRecordStream &recordStream);
         
         INT32 overflowBigRecord(dmlContext *context,
                                 const recordID &overflowAddr);

         INT32 insertInvisibleRecord(dmlContext *context,
                                     const slice &newRowData,
                                     recordID &rid);

         INT32 insertAndUpdateCandidate(dmlContext *context,
                                        const slice &record,
                                        fsmCandidate &candidate,
                                        BOOLEAN &outOfSpace);
         
         INT32 insertInvisiblyAndUpdateCandidate(dmlContext *context,
                                                 const slice &newRowData,
                                                 fsmCandidate &candidate,
                                                 BOOLEAN &outOfSpace,
                                                 recordID &rid);

         INT32 overflowBigRecordAndUpdateCandidate(dmlContext *context,
                                                   const recordID &overflowAddr,
                                                   fsmCandidate &candidate,
                                                   BOOLEAN &outOfSpace);

         INT32 insertBigRecordSlice(dmlContext *context,
                                    bigRecordStream &recordStream,
                                    const fsmCandidate &candidate);

         INT32 updateRecordData(dmlContext *context,
                                const slice &newRecord);

         INT32 updateNormalRecord(dmlContext *context,
                                  const slice &newRecord);

         INT32 updateOverflowedRecord(dmlContext *context,
                                      const slice &newRecord);
         
         INT32 updateBigRecord(dmlContext *context,
                               const slice &newRecord);

         INT32 overflowRecord(dmlContext *context,
                              const slice &newRecord);

         INT32 reoverflowRecord(dmlContext *context,
                                const slice &newRecord);

         INT32 removeRecordData(dmlContext *context);

         INT32 removeNormalRecord(dmlContext *context);

         INT32 removeOverflowedRecord(dmlContext *context);

         INT32 removeBigRecord(dmlContext *context);

         INT32 removeBigRecordSlices(dmlContext *context);

      private:
         INT32 findCandidate(requestContext *context,
                             INT32 lvl,
                             const dmsStripingId &striping,
                             fsmCandidate &candidate);
         
         INT32 findCandidateExclusively(requestContext *context,
                                        INT32 lvl,
                                        fsmCandidate &candidate);
         
         /// user should hold _extendingLatch first
         INT32 allocateNewRecordDataPages(requestContext *context,
                                          UINT32 count,
                                          UINT32 &firstSeq,
                                          PAGE_ID *lpids);
      private:/// route page

         INT32 getLpidBySequence(requestContext *context,
                                 UINT32 sequence,
                                 PAGE_ID &lpid);

         /// create new lvl0 page and insert into map.
         INT32 extendRoutePageMap(requestContext *context);

         INT32 ensureRootRoutePage(requestContext *context,
                                   UINT32 rootSlot);

         INT32 createNewRoutePage(requestContext *context,
                                  UINT32 lvl,
                                  PAGE_ID &lpid);

         /// Create new route page and insert into father.
         INT32 createNonRootRoutePage(requestContext *context,
                                      PAGE_ID father,
                                      UINT32 pageLvl,
                                      PAGE_ID &out);

         /// Get the position by _totalLvl0Count and then create
         /// lvl1 if necessary.
         /// Root lvl2 should be created first.
         INT32 ensureNonRootLvl1RoutePage(requestContext *context,
                                          PAGE_ID &lpid);

         OSS_INLINE UINT32 getMaxLvl0Cnt(UINT32 capacity)const
         {
            return getMaxLvL0RoutePageCountLteRoot(capacity,
                                                   COLLECTION_ROOT_LVL2);
         }

         /// Return total lvl0 count 
         UINT32 getMaxLvL0RoutePageCountLteRoot(UINT32 capacity,
                                                UINT32 maxRoot)const;

         INT32 getLvl0RoutePage(requestContext *context,
                                UINT32 capacity,
                                UINT32 lvl0No,
                                PAGE_ID &lpid);

         INT32 getLpidFromRoutePage(requestContext *context,
                                    PAGE_ID routePgaeLpid,
                                    UINT32 pos,
                                    PAGE_ID &lpid);

         INT32 releaseAllRdps(requestContext *context);

         INT32 loopReleaseRdpsInLvl0(requestContext *context);

         INT32 loopReleaseRoutePages(requestContext *context);
      private:
         UINT32 getDataPageSize()const;

      private:

         INT32 _createIndex(requestContext *context,
                            const indexDescription &desc,
                            indexIdentifier &indexId);

         INT32 rollbackCreatingIndex(requestContext *context,
                                     const indexIdentifier &indexId,
                                     INT32 reason);

         INT32 buildIndexInContext(requestContext *context,
                                   const indexIdentifier &indexId,
                                   INDEX_TYPE type,
                                   const dmsBuildIndexOptions &o);

         /// unstable context must be created first.
         INT32 onlineBuildIndex(requestContext *context,
                                const indexIdentifier &indexId);

         /// unstable context must be created first.
         INT32 onlineBuildIndexBySorting(requestContext *context,
                                         const indexIdentifier &indexId,
                                         UINT64 sortBufferSize);

         /// mark index removing and return index Id;
         /// if index is building, building thread will be terminated.
         /// if index is neither normal nor building, return error. 
         INT32 markIndexRemovingByName(requestContext *context,
                                       const strSlice &indexName,
                                       indexIdentifier &indexId);

         /// always mark index removing first and truncate it.
         /// at last, release all resources.
         INT32 releaseIndexObjectAndEntryPage(requestContext *context,
                                              const indexIdentifier &indexId);

         INT32 markIndexRemovingBySlot(requestContext *context,
                                       const indexIdentifier &indexId);
                                    
         INT32 truncateIndex(requestContext *context,
                             const indexIdentifier &indexId);

         INT32 removeAllIndexes(requestContext *context);

         INT32 truncateAllIndexes(requestContext *context);

      private:/// need protection by dml latch

         INT32 testIfIndexDuplicated(requestContext *context,
                                     const strSlice &indexName,
                                     const indexKeyPattern &pattern,
                                     BOOLEAN &duplicated);

         /// get x latch first
         INT32 indexBuildDone(requestContext *context,
                              indexObject *obj);

         INT32 buildIndexBySortingAndUpdateContext(requestContext *context,
                                                   indexObject *obj,
                                                   UINT32 maxRdpCount,
                                                   memoryBlock &sortBuffer);

         INT32 buildIndexAndUpdateContext(requestContext *context,
                                          indexObject *obj,
                                          UINT32 maxRdpCount);

         INT32 fillSorterAndUpdateEntry(requestContext *context,
                                        indexObject *obj,
                                        BTREE_SORTOR *sortor,
                                        UINT32 maxRdpCount);

         INT32 mergeSorterAndContextIntoIndex(requestContext *context,
                                              indexObject *obj,
                                              BTREE_SORTOR *sorter);

         INT32 endToBuildCurrentRange(requestContext *context,
                                      buildingIndexContext *buildingContext);

         INT32 initIndexesWhenOpen(requestContext *context);

         INT32 fixUnstatbleIndexesWhenOpen(requestContext *context);

      private:
         void initProperties(const clMetaBlock &block);
         void initRouteMapInBlock(const clMetaBlock &block);
         OSS_INLINE atomic_uint &_getRdpCount() {return _entryBlock._rdpCount;}
         OSS_INLINE atomic_uint &_getLvl0Count() {return _entryBlock._lvl0Count;}

         void _exportMetaBlock(clMetaBlock &mb)const;
         
      private:
         collectionSpace *_cs = nullptr;
         clEntryBlock _entryBlock;
   };//class collection

   typedef shallowPointer<collection> COLLECTION_PTR;
}//namespace vessel
}//namespace engine

#endif // VESSEL_COLLECTION_H_