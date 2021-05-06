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

   Source File Name = indexDefPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexDefPageAccessor.h"

namespace engine
{
namespace vessel
{
   INT32 indexDefPageAccessor::getDefObj(requestContext *context,
                                         inMemIndexDefObj &obj)
   {
      INT32 rc = SDB_OK;
      const indexDefRecord *record = NULL;
      const CHAR *nameBuffer = NULL;
      const CHAR *patternBuffer = NULL;
      strSlice nameSlice;

      if (NULL == context)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rc = getReadableUserHeadPtr<indexDefRecord>(&record);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get def obj:%d", rc);
         goto error;
      }

      if (INDEX_DEF_RECORD_VERSION != record->version)
      {
         PD_LOG(PDERROR, "invalid record version");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }
      else if (record->indexNameLen <= 1||
               record->keyPatternLen <= 5)
      {
         PD_LOG(PDERROR, "invalid index name len or pattern len");
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      rc = getReadPtrOfPageBody(record->indexNameOffset,
                                record->indexNameLen,
                                &nameBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index name buffer[%d,%d], rc:%d",
                record->indexNameOffset, record->indexNameLen, rc);
         goto error;
      }
      nameSlice.reset(nameBuffer, record->indexNameLen - 1);

      rc = getReadPtrOfPageBody(record->keyPatternOffset,
                                record->keyPatternLen,
                                &patternBuffer);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get index pattern buffer[%d,%d], rc:%d",
                record->keyPatternOffset, record->keyPatternLen, rc);
         goto error;
      }

      rc = obj.set(*record, nameSlice, bson::BSONObj(patternBuffer, FALSE));
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine