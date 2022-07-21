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

   Source File Name = btreeEntryPageAccessor.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
         /// root may be invalid
         INT32 resetBtreeRoot(requestContext *context,
                              PAGE_ID root,
                              logicalPageBuffer *lpb)const;

         INT32 load(logicalPageBuffer *lpb,
                    PAGE_ID &root,
                    btreeStatistics &stats);
      private:
         UINT32 _indexLid = INVALID_LOGICAL_INDEX_ID;
   };//class btreeEntryPageAccessor 
}//namespace vessel
}//namespace engine

#endif//VESSEL_BTREE_ENTRY_PAGE_ACCESSOR_H_