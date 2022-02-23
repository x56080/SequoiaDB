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
#include "ossLikely.hpp"
#include "vessel/buildingIndexContext.h"
#include "ixm_common.hpp"
#include "msgDef.h"
#include "vessel/indexUtils.h"

namespace engine
{
namespace vessel
{
   indexObject::indexObject()
   {}

   indexObject::~indexObject()
   {
      fini();
   }

   INT32 indexObject::init(INT32 indexSlot,
                           UINT32 indexLid,
                           const indexDescription &desc,
                           INDEX_STATUS status,
                           PAGE_ID lpid,
                           PAGE_ID btreeRoot)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       INVALID_LOGICAL_INDEX_ID == indexLid ||
                       !desc.isValid() ||
                       INDEX_STATUS_INVALID == status||
                       INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _indexId.reset(indexSlot, indexLid, desc.getInnerID());
      _desc = desc;
      _entryLpid = lpid;
      _btreeRoot = btreeRoot;

      if (INDEX_STATUS_BUILDING == status)
      {
         _unstatbleContext = SDB_OSS_NEW buildingIndexContext();
         if (OSS_UNLIKELY(NULL == _unstatbleContext))
         {
            PD_LOG(PDERROR, "failed to alocate mem");
            rc = SDB_OOM;
            goto error;
         }
      }

      _status = status;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   void indexObject::fini()
   {
      _indexId.reset();
      _desc.reset();
      _status = INDEX_STATUS_INVALID;
      _entryLpid = INVALID_PAGE_ID;
      _btreeRoot = INVALID_PAGE_ID;
      _btreeRootSplitTimes = 0;
      SAFE_OSS_DELETE(_unstatbleContext);
      return;
   }

   void indexObject::removeUnstableContext()
   {
      SAFE_OSS_DELETE(_unstatbleContext);
   }

   void indexObject::updateStatus(INDEX_STATUS status)
   {
      SDB_ASSERT(INDEX_STATUS_INVALID != status, "can not be invalid");
      _status = status;
   }

   void indexObject::dump(bson::BSONObjBuilder &builder)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      builder.append(VESSEL_INDEX_FIELD_NAME_INDEX_SLOT, _indexId.getIndexSlot());
      builder.append(VESSEL_INDEX_FIELD_NAME_INDEX_ID, _indexId.getLogicalIndexId());
      _desc.exportToBson(builder);
      builder.append(VESSEL_INDEX_FIELD_NAME_STATUS, _status);
   }

   BOOLEAN indexObject::associates(const CHAR *fieldName)const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(NULL != fieldName, "can not be null");

      BOOLEAN r = FALSE;
      const bson::BSONObj pattern = _desc.getPattern().getPattern();
      bson::BSONObjIterator itr(pattern);
      while (itr.more())
      {
         const CHAR *patternFieldName = itr.next().fieldName();
         if (indexUtils::fieldNameAssociate(fieldName, patternFieldName))
         {
            r = TRUE;
            goto done;
         }
      }

   done:
      return r;
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

} // namespace vessel

}//namespace engine
