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
   INT32 indexObject::shallowInit(UINT32 indexId,
                                  const strSlice &indexName,
                                  const indexKeyPattern &pattern,
                                  const indexParameters &params,
                                  PAGE_ID btreeRoot,
                                  UINT32 btreeRootUpdatedTimes)
   {
      INT32 rc = SDB_OK;
      fini();

      if (INVALID_LOGICAL_INDEX_ID == indexId ||
          indexName.empty() ||
          !pattern.isValid() ||
          !params.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _indexId = indexId;
      _nameSlice = indexName;
      _pattern = pattern;
      _params = params;
      _btreeRootUpdatedTimes = btreeRootUpdatedTimes;
      _btreeRoot = btreeRoot;
   done:
      return rc;
   error:
      goto done;
   }

   void indexObject::shallowCopy(const indexObject &o)
   {
      fini();
      if (o.isValid())
      {
         _indexId = o._indexId;
         _nameSlice = o._nameSlice;
         _pattern = o._pattern;
         _params = o._params;
         _btreeRootUpdatedTimes = o._btreeRootUpdatedTimes;
         _btreeRoot = o._btreeRoot;
      }
      return;
   }

   BOOLEAN indexObject::isOwned()const
   {
      return !_indexName.empty();
   }

   void indexObject::getOwned()
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      if (isValid() && !isOwned())
      {
         _indexName.assign(_nameSlice.str(), _nameSlice.strLen());
         _nameSlice.reset(_indexName.c_str(), _indexName.size());
         _pattern.getOwned();
      }
      return;
   }

   void indexObject::fini()
   {
      _indexId = INVALID_LOGICAL_INDEX_ID;
      _nameSlice.reset();
      _indexName.clear();
      _pattern.reset();
      _params = indexParameters();
      _btreeRootUpdatedTimes = 0;
      _btreeRoot = INVALID_PAGE_ID;
      return;
   }

   void indexObject::updateBtreeRoot(PAGE_ID root,
                                     const UINT32 *updatedTimes)
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != root, "can not be invalid");
      SDB_ASSERT(INDEX_TYPE_BTREE == _params.type, "must be btree");
      _btreeRoot = root;
      if (NULL == updatedTimes)
      {
         ++_btreeRootUpdatedTimes;
      }
      else
      {
         _btreeRootUpdatedTimes = *updatedTimes;
      }
      return;
   }

   BOOLEAN indexObject::hasBtreeRoot()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(INDEX_TYPE_BTREE == _params.type, "must be btree");
      return INVALID_PAGE_ID != _btreeRoot;
   }
}//namespace vessel
}//namespace engine