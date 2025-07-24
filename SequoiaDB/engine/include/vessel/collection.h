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

   Source File Name = collection.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
#include "vessel/dmlIndexRequest.h"
#include "vessel/btreeRebuildingSortElement.h"
#include "vessel/modifyRecordContext.h"
#include "vessel/shallowPointer.hpp"
#include "vessel/dmlRequest.h"
#include "vessel/objectIdentifier.h"
#include "vessel/lobChunkKey.h"
#include "vessel/listLobChunkCursor.h"
#include "vessel/clEntryBlock.h"
#include "vessel/lpsPteWriteBatch.h"

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
   class indexScanCursor;
   class bigRecordStream;
   class hitTransferTaskCtx;
   class spacePteAccessCtx;
   
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

         OSS_INLINE const std::string &getNameString()const
         {
            return _entryBlock._properties.name;
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

         INT32 truncate(requestContext *context,
                        LPS_PTE_WRITE_BATCH &batch);

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

         /// not thread-safe
         INT32 transferIndexEntries(requestContext *context,
                                    hitTransferTaskCtx *tc);

      public:
         INT32 dump(bson::BSONObj &record);

         INT32 getMoreWhenScan(requestContext *context,
                               scanCLCursor *cursor);

         INT32 getTotalRecordCount(requestContext *context,
                                   UINT64 &count);

         INT32 getMoreWhenIndexScan(requestContext *context,
                                    indexScanCursor *cursor);

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
         INT32 _getMoreWhenIndexScan(requestContext *context,
                                     indexObject *obj,
                                     indexScanCursor *cursor);

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
         INT32 _createNewIndex(requestContext *context,
                               const bson::BSONObj &adjunct,
                               UINT32 &logicalIndexId);

         INT32 _abortCreatingIndex(requestContext *context,
                                   UINT32 logicalIndexId,
                                   INT32 reason);

         /// building context must be created first.
         INT32 _buildIndex(requestContext *context,
                           UINT32 logicalIndexId,
                           const dmsBuildIndexOptions &o);

         INT32 _buildIndexOnline(requestContext *context,
                                 UINT32 logicalIndexId);

         /// mark index removing and return index Id;
         /// if index is building, building thread will be terminated.
         /// if index is neither normal nor building, return error. 
         INT32 _setIndexRemoving(requestContext *context,
                                 const strSlice &indexName,
                                 indexObject **out);
                                    
         INT32 _truncateIndex(requestContext *context,
                              indexObject *obj,
                              BOOLEAN removeEntryPage,
                              LPS_PTE_WRITE_BATCH &batch);

         INT32 _endToRemoveIndex(requestContext *context,
                                 UINT32 logicalIndexId);

         INT32 _removeAllIndexes(requestContext *context,
                                 DPS_LSN_OFFSET lsn);

         INT32 _truncateAllIndexes(requestContext *context,
                                   DPS_LSN_OFFSET lsn,
                                   LPS_PTE_WRITE_BATCH &batch);

         INT32 _transferIndexEntries(requestContext *context,
                                     hitTransferTaskCtx *taskCtx,
                                     indexObject *obj);

         INT32 _createBtreeEntryPage(requestContext *context,
                                     indexObject *obj,
                                     hitTransferTaskCtx *taskCtx,
                                     spacePteAccessCtx *ac);
         INT32 _rollbackUnstableBtreeEntryPage(requestContext *context,
                                               indexObject *obj,
                                               hitTransferTaskCtx *taskCtx);

      private:/// need protection by op lock
         INT32 _buildIndexInWindow(requestContext *context,
                                   buildingIndexContext *buildingCtx);

         INT32 _endToBuildCurrrentWindow(requestContext *context,
                                         buildingIndexContext *buildingCtx);

         /// hold x lock first
         INT32 _finishIndexBuilding(requestContext *context,
                                    UINT32 logicalIndexId);

         INT32 _initIndexesWhenOpen(requestContext *context);

         INT32 _fixUnstatbleIndexesWhenOpen(requestContext *context);

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