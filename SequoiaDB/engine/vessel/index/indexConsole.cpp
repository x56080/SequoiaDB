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

   Source File Name = indexConsole.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexConsole.h"
#include "vessel/collectionRecordPage.h"
#include "vessel/collectionSpace.h"
#include "ossLikely.hpp"
#include "vessel/inMemIndexDefObj.h"
#include "vessel/indexDefPageAccessor.h"
#include "vessel/lpidLockHelper.h"
#include "vessel/indexSpace.h"

namespace engine
{
namespace vessel
{
   INT32 indexConsole::createIndex(requestContext *context,
                                   const strSlice &indexName,
                                   const indexKeyPattern &pattern,
                                   const createIndexOptions &options)
   {
      INT32 rc = SDB_OK;
      UINT32 indexID = INVALID_LOGICAL_INDEX_ID;
      INT32 slot = -1;

      if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isValidCreatingIndexArgs(indexName, pattern, options))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isInitialized())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      if (MAX_INDEX_SAVING_SIZE < inMemIndexDefObj::estimate(indexName, pattern))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = preallocateIndexIdAndSlot(indexID, slot);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = validateIfDuplicated(context, indexName, pattern);
      if (SDB_OK != rc)
      {
         goto error;
      }

      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::preallocateIndexIdAndSlot(UINT32 &logicalID, INT32 &slot)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isInitialized(), "must be inited");
      UINT32 currentIndexCount = _record->uniqueIndexCount + _record->nonUniqueIndexCount;

      if (MAX_INDEX_COUNT_PER_CL <= currentIndexCount)
      {
         PD_LOG(PDERROR, "hit the max index count in cl[%s]",
                _record->name);
         rc = SDB_DMS_MAX_INDEX;
         goto error;
      }
      else if (INVALID_LOGICAL_INDEX_ID == _record->nextIndexID)
      {
         PD_LOG(PDERROR, "hit the max logical index id in cl[%s]",
                _record->name);
         rc = SDB_DMS_MAX_INDEX;
         goto error;
      }

      logicalID = _record->nextIndexID;
      slot = (INT32)currentIndexCount;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexConsole::validateIfDuplicated(requestContext *context,
                                            const strSlice &indexName,
                                            const indexKeyPattern &pattern)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(NULL != context, "can not be null");
      SDB_ASSERT(!indexName.empty(), "can not be empty");
      SDB_ASSERT(pattern.isValid(), "must be valid");
      SDB_ASSERT(NULL != _record && NULL != _is, "can not be null");
      inMemIndexDefObj obj;
      indexDefPageAccessor accessor;
      UINT32 count = _record->uniqueIndexCount + _record->nonUniqueIndexCount;

      for (UINT32 i = 0; i < count; ++i)
      {
         lpidLockHelper lh;
         PAGE_ID lpid = _record->indexSlots[i];
         if (INVALID_PAGE_ID == lpid)
         {
            PD_LOG(PDERROR, "index slot[%d] is invalid", i);
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         rc = lh.lock(context, FILE_TYPE_IDX_D, lpid, SHARED);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
            goto error;
         }         
      }
   done:
      return rc;
   error:
      accessor.fini(context);
      goto done;
   }
}//namespace vessel
}//namespace engine