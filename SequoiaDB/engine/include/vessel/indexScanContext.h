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