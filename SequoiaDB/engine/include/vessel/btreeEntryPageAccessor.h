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

   Source File Name = btreeEntryPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BTREE_ENTRY_PAGE_ACCESSOR_H_
#define VESSEL_BTREE_ENTRY_PAGE_ACCESSOR_H_

#include "vessel/pageAccessor.h"
#include "vessel/indexKeyPattern.h"
#include "vessel/logicalPageBuffer.h"
#include "vessel/slice.h"
#include "vessel/indexDef.h"
#include "vessel/btreeEntryPage.h"
#include "vessel/btreeStatistics.h"

namespace engine
{
namespace vessel
{
   class btreeEntryPageAccessor : public pageAccessor
   {
      public:
         btreeEntryPageAccessor(UINT32 lid);
         ~btreeEntryPageAccessor() = default;

      public:
         INT32 load(logicalPageBuffer *lpb,
                    PAGE_ID &root,
                    UINT32 &transferTick,
                    btreeStatistics &stats);

         INT32 refill(requestContext *context,
                      PAGE_ID root,
                      UINT32 transferTick,
                      const btreeStatistics &stats,
                      logicalPageBuffer *lpb);
      private:
         UINT32 _indexLid = INVALID_LOGICAL_INDEX_ID;
   };//class btreeEntryPageAccessor 
}//namespace vessel
}//namespace engine

#endif//VESSEL_BTREE_ENTRY_PAGE_ACCESSOR_H_