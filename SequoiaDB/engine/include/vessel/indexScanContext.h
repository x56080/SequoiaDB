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
#include "vessel/requestContext.h"
#include "rtnPredicate.hpp"
#include "vessel/unorderedRidSet.h"

namespace engine
{
namespace vessel
{
   class indexEntryBuffer;

   class indexScanContext : public requestContext
   {
      public:
         indexScanContext(){}
         ~indexScanContext();

      public:
         OSS_INLINE const indexHandle &getHandle()const
         {
            return _handle;
         }
         OSS_INLINE indexEntryBuffer *getEntryBuffer()const
         {
            return _entryBuffer;
         }
         OSS_INLINE _rtnPredicateListIterator *getPredicate()const
         {
            return _predicate;
         }
         OSS_INLINE BOOLEAN isForward()const
         {
            return _forward;
         }
         OSS_INLINE UNORDERED_RID_SET *getRidSet()const
         {
            return _ridSet;
         }

         OSS_INLINE BOOLEAN isScanning()const
         {
            return _handle.isValid();
         }
      public:
         INT32 openIndexScan(const indexHandle &handle,
                             _rtnPredicateListIterator *predicate,
                             indexEntryBuffer *entryBuffer,
                             UNORDERED_RID_SET *ridSet,
                             BOOLEAN forward);

         void closeIndexScan();

         virtual void close();

      private:
         
      private:
         indexHandle _handle;
         indexEntryBuffer *_entryBuffer = NULL;
         _rtnPredicateListIterator *_predicate = NULL;
         UNORDERED_RID_SET *_ridSet = NULL;
         BOOLEAN _forward = TRUE;
   };//class indexScanContext
} // namespace vessel

} // namespace engine

#endif//VESSEL_INDEX_SCAN_CONTEXT_H_