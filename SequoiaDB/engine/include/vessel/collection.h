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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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
#include "vessel/recordData.h"
#include "utilInsertResult.hpp"
#include "vessel/freeSpaceMap.h"

namespace engine
{
   class _dpsLogRecord;
namespace vessel
{
   class collectionSpace;
   class requestContext;
   class insertContext;
   
   class collection: public SDBObject
   {
      public:
         collection();
         ~collection();

      public:
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
         INT32 initWhenOpen(const collectionRecord &record,
                            collectionSpace *cs);

         void fini();

      public:
         INT32 dump(requestContext *context,
                    listCollectionsRecord &record);

         INT32 insert(insertContext *context,
                      utilInsertResult &res);

      private:
         INT32 findFreePageForRecord(requestContext *context,
                                     UINT32 recordSize,
                                     STRIPING_ID striping,
                                     fsmCandidate &candidate);
         INT32 insertNonBigRecord(insertContext *context,
                                  utilInsertResult &res);


         /// user should hold _pageAllocLatch first
         INT32 allocateNewRecordDataPages(requestContext *context,
                                          UINT32 count,
                                          CL_PAGE_SEQ &firstSeq,
                                          PAGE_ID *lpids);

         INT32 initNewRecordDataPages(requestContext *context,
                                      UINT32 count,
                                      const PAGE_ID *lpids,
                                      const PAGE_ID *pids);
      private:
         INT32 extendRoutePageMap(requestContext *context,
                                  PAGE_ID *newLvl0=NULL);

         INT32 createRootRoutePage(requestContext *context,
                                   UINT32 rootSlot);

         INT32 createNonRootRoutePage(requestContext *context,
                                      PAGE_ID lpid,
                                      UINT32 slot,
                                      PAGE_ID &lpidOfRP);

         INT32 ensureNonRootLvl1RoutePage(requestContext *context,
                                          UINT32 slot,
                                          PAGE_ID &lpid);

         INT32 getLvl0RoutePage(requestContext *context,
                                UINT32 capacity,
                                UINT32 lvl0No,
                                PAGE_ID &lpid);
         
         INT32 createNewRoutePage(requestContext *context,
                                  PAGE_ID &lpidOfRP,
                                  DPS_LSN_OFFSET *lsn);

      private:
         INT32 saveOnDiskWhenCreating(requestContext *context);

         INT32 ensureCLRecordPageAllocated(requestContext *context,
                                           PAGE_ID lpid,
                                           PAGE_ID &pid);

         INT32 allocatePageForCLRecord(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_ID &pid);

         INT32 preallocateCLRecordPage(requestContext *context,
                                       PAGE_ID lpid,
                                       PAGE_ID &pid);

         INT32 saveCLRecordWhenCreating(requestContext *contex, PAGE_ID pid);

      private:
         OSS_INLINE UINT32 getMaxLvl0Cnt(UINT32 capacity)
         {
            return 1 + (capacity << 1) + capacity * capacity;
         }
      private:
         //ossSpinSLatch _recordLatch;
         collectionRecord _record;
         collectionSpace *_collectionSpace;

         UINT32 _maxPageCntInRoutePages;
         UINT32 _pageCntInRoutePages;

         freeSpaceMap _fsm;
         ossSpinSLatch _ddlSLatch;
         ossSpinXLatch _pageAllocLatch;
   };//class collection
}//namespace vessel
}//namespace engine

#endif // VESSEL_COLLECTION_H_