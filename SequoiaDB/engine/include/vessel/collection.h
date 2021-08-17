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

#include "vessel/collectionRecordPage.h"
#include "vessel/recordID.h"
#include "vessel/strSlice.h"
#include "vessel/vesselOptions.h"
#include "vessel/recordID.h"
#include "vessel/listCollectionsDef.h"
#include "utilInsertResult.hpp"
#include "vessel/freeSpaceMap.h"
#include "vessel/indexOptions.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/collectionOptions.h"
#include "vessel/indexParameters.h"
#include "vessel/unstableIndexContext.h"

namespace engine
{
   class _dpsLogRecord;
   class _dmsIxmKeySorter;
namespace vessel
{
   class collectionSpace;
   class requestContext;
   class insertContext;
   class scanCLCursor;
   class IQueryFilter;

   class collection: public SDBObject
   {
      public:
         collection();
         ~collection();

      public:
         OSS_INLINE BOOLEAN isOpen()const
         {
            return NULL != _collectionSpace;
         }
         OSS_INLINE const collectionRecord &getRecord()const
         {
            return _record;
         }
         OSS_INLINE const CHAR *getName()const
         {
            return _record.name;
         }
         OSS_INLINE UINT32 getLogicalID()const
         {
            return _record.logicalCLID;
         }
         OSS_INLINE CL_MB_ID getMBID()const
         {
            return _record.mbID;
         }
         OSS_INLINE utilCLInnerID getInnerID()const
         {
            return _record.innerID;
         }
         OSS_INLINE UTIL_COMPRESSOR_TYPE getCompressionType()const
         {
            return (UTIL_COMPRESSOR_TYPE)(_record.compressionType);
         }

         INT32 create(requestContext *context,
                      const strSlice &clName,
                      utilCLInnerID innerID,
                      UINT32 logicalID,
                      collectionSpace *cs,
                      const createCLOptions &options);

         /// init when startup
         INT32 initWhenOpen(requestContext *context,
                            const collectionRecord &record,
                            collectionSpace *cs);

         void fini();

      public:
         INT32 createIndex(requestContext *context,
                           const strSlice &indexName,
                           const indexKeyPattern &keyPattern,
                           const indexParameters &params,
                           const createIndexOptions &options);

         INT32 listIndexes(requestContext *context,
                           ossPoolVector<bson::BSONObj> &indexes);

      public:
         INT32 dump(requestContext *context,
                    bson::BSONObj &record);

         INT32 insert(insertContext *context,
                      utilInsertResult &res);

         INT32 getMoreWhenScan(requestContext *context,
                               scanCLCursor *cursor);

         INT32 getTotalCountInRdpHead(requestContext *context,
                                      UINT64 &count);

      private:/// Used only when openning/creating.
         INT32 initPageSequenceWhenOpen(requestContext *context);
         INT32 initPageSequenceByRootLvL2(requestContext *context);
         INT32 initPageSequenceByRootLvL1(requestContext *context,
                                          UINT32 rootNo);
         INT32 initPageSequenceByRootLvL0(requestContext *context);

         /// User should always validate count and element when return SDB_OK.
         /// Count never shrink but element may be removed.
         INT32 getCountAndLastEleInRoutePage(requestContext *context,
                                             UINT32 capacity,
                                             PAGE_ID lpid,
                                             INT32 lvl,
                                             UINT32 &count,
                                             PAGE_ID &element);

         INT32 saveOnDiskWhenCreating(requestContext *context,
                                      const createCLOptions &options);

      private:
         INT32 getMoreFromPageInCursor(requestContext *context,
                                       scanCLCursor *cursor);

         INT32 getRecordCountInPageHead(requestContext *context,
                                        PAGE_ID lpid,
                                        UINT32 &count);

      private:
         INT32 insertNonBigRecord(insertContext *context);

         INT32 insertNonBigRecordToPage(insertContext *context,
                                        PAGE_ID lpid);

      private:
         INT32 findCandidate(requestContext *context,
                             INT32 lvl,
                             STRIPING_ID striping,
                             fsmCandidate &candidate);
         
         /// user should hold _newPageLatch first
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

         
   
      private:
         UINT32 getDataPageSize()const;

      private:

         INT32 _createIndex(requestContext *context,
                            const strSlice &indexName,
                            const indexKeyPattern &pattern,
                            const indexParameters &params,
                            INT32 &indexSlot);

         /// unstable context must created first.
         INT32 onlineBuildIndex(requestContext *context,
                                 INT32 indexSlot,
                                 UINT32 sortBufferSize);

      private: 
         INT32 testIfIndexDuplicated(requestContext *context,
                                     const strSlice &indexName,
                                     const indexKeyPattern &pattern,
                                     BOOLEAN &duplicated);

         /// get x latch first
         INT32 endToBuildIndex(requestContext *context,
                               INT32 indexSlot);

         INT32 buildIndexBySortingAndUpdateContext(requestContext *context,
                                                   unstableIndexContext *uic,
                                                   UINT32 maxRdpCount,
                                                   memoryBlock &sortBuffer);

         INT32 fillSorterAndUpdateEntry(requestContext *context,
                                        _dmsIxmKeySorter *sorter,
                                        UINT32 maxRdpCount,
                                        unstableIndexContext *uic);

         INT32 mergeSorterAndContextIntoIndex(requestContext *context,
                                              _dmsIxmKeySorter *sorter,
                                              unstableIndexContext *uic);

      private:
         typedef ossPoolMap<INT32, unstableIndexContext*> _UNSTABLE_INDEXES;
         
      private:
         //ossSpinSLatch _recordLatch;
         collectionRecord _record;
         collectionSpace *_collectionSpace = NULL;
         UINT32 _totalLvl0Count = 0;
         UINT32 _totalRdpCount = 0;
         freeSpaceMap _fsm;

         unstableIndexContextMap _unstableIndexes;

         ossRWMutex _dmlLatch;
         ossSpinXLatch _extendingLatch;
   };//class collection
}//namespace vessel
}//namespace engine

#endif // VESSEL_COLLECTION_H_