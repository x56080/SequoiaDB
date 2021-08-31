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

   Source File Name = indexScanCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_SCAN_CURSOR_H_
#define VESSEL_INDEX_SCAN_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "vessel/collectionOptions.h"
#include "rtnPredicate.hpp"
#include "vessel/collectionHandle.h"
#include "vessel/indexHandle.h"
#include "vessel/strSlice.h"
#include "vessel/unorderedRidSet.h"
#include "vessel/indexEntryBuffer.h"

namespace engine
{
namespace vessel
{
   class indexScanCursor : public cursorKernal
   {
      public:
         indexScanCursor() = delete;
         indexScanCursor(const indexScanOptions &o,
                         const rtnPredicateList &predicateList,
                         const collectionHandle &clHandle,
                         const strSlice &indexName):
         _o(o),
         _predicate(predicateList),
         _clHandle(clHandle),
         _indexName(indexName)
         {}

         virtual ~indexScanCursor(){}

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_INDEX_SCAN;
         }

         virtual UINT32 getStepLengthInLoop()const
         {
            return _o.stepLength;
         }

      public:
         OSS_INLINE const strSlice &getIndexName()const
         {
            return _indexName;
         }

         OSS_INLINE const indexHandle &getIndexHandle()const
         {
            return _handle;
         }
         OSS_INLINE UINT32 getIndexId()const
         {
            return _handle.getIndexId();
         }

         OSS_INLINE void setIndexHandle(const indexHandle &h)
         {
            _handle = h;
         }
         OSS_INLINE const collectionHandle &getCLHandle()const
         {
            return _clHandle;
         }

         OSS_INLINE indexEntryBuffer *getEntryBuffer()
         {
            return &_entryBuffer;
         }

         OSS_INLINE UNORDERED_RID_SET *getScannedSet()
         {
            return &_scanned;
         }

         OSS_INLINE rtnPredicateListIterator *getPredicate()
         {
            return &_predicate;
         }
         OSS_INLINE const indexScanOptions &getOptions()const
         {
            return _o;
         }

      private:
         indexScanOptions _o;
         rtnPredicateListIterator _predicate;
         collectionHandle _clHandle;
         strSlice _indexName;
         indexHandle _handle;
         UNORDERED_RID_SET _scanned;
         indexEntryBuffer _entryBuffer;

   };//class indexScanCursor
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_CURSOR_H_