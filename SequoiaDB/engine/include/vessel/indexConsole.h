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

   Source File Name = indexConsole.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_CONSOLE_H_
#define VESSEL_INDEX_CONSOLE_H_

#include "vessel/indexOptions.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/slice.h"
#include "vessel/strSlice.h"
#include "ixmKey.hpp"
#include "vessel/recordID.h"
#include "vessel/indexObject.h"
#include "vessel/indexSpace.h"
#include "vessel/dmlIndexRequest.h"
#include "vessel/lsm/lsmInsertBatch.h"
#include "vessel/btreeIndexIterator.h"
#include "vessel/lsm/lsmIndexIterator.h"

namespace engine
{
   class dmsRBSOffset;
namespace vessel
{
   class requestContext;
   class indexContextMap;
   class dmlContext;
   struct indexEntryPageHead;

   class indexConsole : public SDBObject
   {
      public:
         indexConsole(){}
         indexConsole(const indexConsole &) = delete;
         indexConsole &operator=(const indexConsole &) = delete;
         ~indexConsole(){}

      public:
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return INVALID_CL_MB_ID != _mbID;
         }
      public:
         void init(CL_MB_ID mbID, indexSpace *is);
         void fini();

      public:
         INT32 createIndex(requestContext *context,
                           INT32 indexSlot,
                           UINT32 indexId,
                           const slice &defObj,
                           PAGE_ID &lpid)const;

         INT32 releaseIndexDefPage(requestContext *context,
                                   INT32 indexSlot);

         INT32 truncateIndex(requestContext *context,
                             indexContext *ic);

         INT32 updateIndexStatus(requestContext *context,
                                 UINT32 indexId,
                                 PAGE_ID lpid,
                                 INDEX_STATUS status);

         INT32 getOwnedIndexObj(requestContext *context,
                                INT32 indexSlot,
                                indexObject &obj,
                                indexEntryPageHead *head=NULL);

         INT32 loadIndexesWhenStartup(requestContext *context,
                                      indexContextMap *indexes);

      public:
         INT32 insert(requestContext *context,
                      indexContext *ic,
                      const ixmKey &key,
                      const recordID &rid,
                      const DPS_TRANS_ID &transID);

         INT32 dmlInsert(dmlContext *context,
                         const dmlIndexRequestArray &ra);

         /// Must hold unique key latch first.
         INT32 checkUniqueConstraint(requestContext *context,
                                     indexContext *ic,
                                     const bson::BSONObj &key,
                                     recordID &rid);
         
      private:
         INT32 checkUniqueConstraintByIterator(requestContext *context,
                                               indexContext *ic,
                                               indexIterator *iterator,
                                               const bson::BSONObj &key,
                                               recordID &rid)const;

         INT32 lsmInsert(requestContext *context,
                         indexContext *ic,
                         const ixmKey &key,
                         const recordID &rid,
                         const DPS_TRANS_ID &transID);

         INT32 lsmTruncate(requestContext *context,
                           const indexObject &obj);

         INT32 createLsmBatch(dmlContext *context,
                              const dmlIndexRequestArray &ra,
                              lsmInsertBatch &lsmBatch);

      private:
         
         INT32 btreeInsert(dmlContext *context,
                           const dmlIndexRequestArray &ra);

         INT32 btreeInsert(requestContext *context,
                           indexContext *ic,
                           const ixmKey &key,
                           const recordID &rid,
                           const DPS_TRANS_ID &transID);
      private:
         INT32 createDirectMappedIndex(requestContext *context,
                                       INT32 indexSlot,
                                       UINT32 indexId,
                                       const slice &defObj,
                                       PAGE_ID &out)const;

         INT32 createDoubleMappedIndex(requestContext *context,
                                       INT32 indexSlot,
                                       UINT32 indexId,
                                       const slice &defObj,
                                       PAGE_ID &out)const;

      private:
         CL_MB_ID _mbID = INVALID_CL_MB_ID;
         indexSpace *_is = NULL;
   };//class indexConsole
}//namespace vessel
}//namespace engine

#endif//VESSEL_INDEX_CONSOLE_H_