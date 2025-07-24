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

   Source File Name = indexScanCursor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_SCAN_CURSOR_H_
#define VESSEL_INDEX_SCAN_CURSOR_H_

#include "vessel/cursorKernal.h"
#include "rtnPredicate.hpp"
#include "vessel/strSlice.h"
#include "vessel/unorderedRidSet.h"
#include "vessel/slice.h"
#include "vessel/objectIdentifier.h"
#include "dmsEngineOptions.hpp"
#include "vessel/indexScanContext.h"

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
         _gcid(gcid),
         _indexId(indexId),
         _ctx(predicateList)
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
         OSS_INLINE const dmsIndexScanOptions &getOptions()const
         {
            return _o;
         }
         OSS_INLINE indexScanContext &getCtx() {return _ctx;}
      private:
         dmsIndexScanOptions _o;
         globalCollectionId _gcid;
         indexIdentifier _indexId;
         indexScanContext _ctx;
   };//class indexScanCursor
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_SCAN_CURSOR_H_