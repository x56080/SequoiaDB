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

   Source File Name = indexObject.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexObject.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   INT32 indexObject::init(INT32 indexSlot, 
                           UINT32 indexLid,
                           const indexDescription &desc,
                           PAGE_ID btreeRoot)
   {
      INT32 rc = SDB_OK;
      fini();

      if (!isValidIndexSlot(indexSlot) ||
          INVALID_LOGICAL_INDEX_ID == indexLid ||
          !desc.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(!(desc.isLsmIndex() && INVALID_PAGE_ID != btreeRoot), "impossible");

      _indexId.reset(indexSlot, indexLid, desc.getInnerID());
      _desc = desc;
      _btreeRoot = btreeRoot;
   done:
      return rc;
   error:
      goto done;
   }

   void indexObject::fini()
   {
      _indexId.reset();
      _desc.reset();
      _btreeRootSplitTimes = 0;
      _btreeRoot = INVALID_PAGE_ID;
      return;
   }

   void indexObject::updateBtreeRoot(PAGE_ID root,
                                     UINT32 splitTimes)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != root, "can not be invalid");
      SDB_ASSERT(INDEX_TYPE_BTREE == _desc.getType(), "must be btree");
      _btreeRoot = root;
      _btreeRootSplitTimes = splitTimes;
      
      return;
   }

   void indexObject::updateBtreeRootSplitTimes(UINT32 splitTimes)
   {
      _btreeRootSplitTimes = splitTimes;
   }

   BOOLEAN indexObject::hasBtreeRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INDEX_TYPE_BTREE == _desc.getType(), "must be btree");
      return INVALID_PAGE_ID != _btreeRoot;
   }

   void indexObject::removeBtreeRoot()
   {
      _btreeRoot = INVALID_PAGE_ID;
      _btreeRootSplitTimes = 0;
   }
}//namespace vessel
}//namespace engine