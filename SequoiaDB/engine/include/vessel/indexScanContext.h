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

   Source File Name = indexScanContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SCAN_CONTEXT_H_
#define VESSEL_INDEX_SCAN_CONTEXT_H_

#include "vessel/requestContext.h"
#include "vessel/indexHandle.h"
#include "rtnPredicate.hpp"
#include "vessel/unorderedRidSet.h"
#include "vessel/collectionOptions.h"
#include "vessel/slice.h"
#include "vessel/indexScanEntryBatch.h"
#include "vessel/indexContext.h"

namespace engine
{
namespace vessel
{
   class indexEntryBuffer;
   class indexScanCursor;

   class indexScanContext : public requestContext
   {
      public:
         indexScanContext(){}
         ~indexScanContext(){}

      public:
         const indexHandle &getHandle()const;
         rtnPredicateListIterator *getPredicate()const;
         UNORDERED_RID_SET *getRidSet()const;
         const indexScanOptions &getOptions()const;
         const indexScanCursor *getCursor()const
         {
            return _cursor;
         }
         indexScanCursor *getCursor()
         {
            return _cursor;
         }
         OSS_INLINE BOOLEAN isCursorAttached()const
         {
            return NULL != _cursor;
         }
      public:
         void attachIndexScanCursor(indexScanCursor *cursor);
         virtual void close();
         void clearBatchAndRidLatch();

         OSS_INLINE indexScanEntryBatch &getBatch()
         {
            return _batch;
         }
         OSS_INLINE const indexScanEntryBatch &getBatch()const
         {
            return _batch;
         }
      private:
         indexScanCursor *_cursor = NULL;
         indexScanEntryBatch _batch;
   };//class indexScanContext
} // namespace vessel

} // namespace engine

#endif//VESSEL_INDEX_SCAN_CONTEXT_H_