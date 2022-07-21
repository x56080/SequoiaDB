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
#include "rtnPredicate.hpp"
#include "vessel/strSlice.h"
#include "vessel/unorderedRidSet.h"
#include "vessel/slice.h"
#include "vessel/objectIdentifier.h"
#include "dmsEngineOptions.hpp"
#include "../bson/util/builder.h"
#include "vessel/indexEntryLocation.h"

namespace engine
{
namespace vessel
{
   class indexScanCursor : public cursorKernal
   {
      public:
         indexScanCursor() = delete;
         indexScanCursor(const dmsIndexScanOptions &o,
                         const rtnPredicateList &predicateList,
                         const globalCollectionId &gcid,
                         const indexIdentifier &indexId):
         _o(o),
         _predicate(predicateList),
         _gcid(gcid),
         _indexId(indexId)
         {}

         virtual ~indexScanCursor() = default;

      public:
         virtual const CHAR *getName()const override
         {
            return "vessel.indexScanCursor";
         }
         virtual slice getDataSlice()const override
         {
            constexpr UINT32 _SIZE = sizeof(dmsRecordID) + sizeof(DPS_TRANS_ID);
            slice s;
            slice raw = cursorKernal::getRawData();
            if (_SIZE < raw.getSize())
            {
               s = raw.getSlice(_SIZE, raw.getSize() - _SIZE);
            }
            return s;
         }

      public:
         virtual CURSOR_TYPE getType()const
         {
            return CURSOR_TYPE_INDEX_SCAN;
         }
   
      public:
         OSS_INLINE const indexIdentifier &getIndexId()const
         {
            return _indexId;
         }
         OSS_INLINE const globalCollectionId &getCollectionId()const
         {
            return _gcid;
         }

         OSS_INLINE rtnPredicateListIterator *getPredicate()
         {
            return &_predicate;
         }
         OSS_INLINE const rtnPredicateListIterator *getPredicate()const
         {
            return &_predicate;
         }
         OSS_INLINE const dmsIndexScanOptions &getOptions()const
         {
            return _o;
         }

         BOOLEAN markRidScanned(const recordID &rid);
         BOOLEAN testRidScanned(const recordID &rid)const;

         void resetEntryLocation()
         {
            _location.reset();
         }
         OSS_INLINE BOOLEAN hasLocation()const {return !!_location;}
         OSS_INLINE IDX_ENTRY_LOCATION_UPTR &getLocation() {return _location;}

      private:
         dmsIndexScanOptions _o;
         rtnPredicateListIterator _predicate;
         globalCollectionId _gcid;
         indexIdentifier _indexId;
         UNORDERED_RID_SET _scanned;
         IDX_ENTRY_LOCATION_UPTR _location;
   };//class indexScanCursor
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_CURSOR_H_