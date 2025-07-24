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

   Source File Name = indexScanContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_SCAN_CONTEXT_H_
#define VESSEL_INDEX_SCAN_CONTEXT_H_

#include "rtnPredicate.hpp"
#include "vessel/unorderedRidSet.h"
#include "vessel/indexEntryLocation.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   class indexScanContext : public SDBObject
   {
      public:
         indexScanContext(const rtnPredicateList &p):
         _predicates(p),
         _predicate(_predicates)
         {
            SDB_ASSERT(_predicates.isInitialized(), "can not be invalid");
         }
         ~indexScanContext() = default;
         indexScanContext(const indexScanContext &) = delete;
         indexScanContext &operator=(const indexScanContext &) = delete;

      public:
         const rtnPredicateList &getPredicates()const {return _predicates;}
         BOOLEAN isForward()const {return 0 <= _predicates.getDirection();}
         rtnPredicateListIterator *getPredicate() {return &_predicate;}
         BOOLEAN isPointGet()const {return _predicates.isPointGet();}
         IDX_ENTRY_LOCATION_UPTR &getLocation() {return _location;}
         BOOLEAN hasLocation()const {return !!_location;}
         void resetLocation() {_location.reset();}
         BOOLEAN testRidScanned(const recordID &rid)const {return 0 < _scanned.count(rid);}
         BOOLEAN markRidScanned(const recordID &rid) {return _scanned.insert(rid).second;}
      private:
         const rtnPredicateList &_predicates;
         rtnPredicateListIterator _predicate;
         UNORDERED_RID_SET _scanned;
         IDX_ENTRY_LOCATION_UPTR _location;
   };//class indexScanContext
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_CONTEXT_H_