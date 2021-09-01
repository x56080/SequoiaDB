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

   Source File Name = indexScanCursor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexScanCursor.h".
#include "vessel/recordCursorRow.h"

namespace engine
{
namespace vessel
{
   INT32 indexScanCursor::getNextRow(ISession *session,
                                     cursorRow *row)
   {
      INT32 rc = SDB_OK;
      slice content;
      const recordID *rid = NULL;
      const DPS_TRANS_ID *transID = NULL;
      slice record;
      recordCursorRow *recordRow = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == row ||
                             CURSOR_ROW_TYPE_RECORD != row->getType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      recordRow = static_cast<recordCursorRow *>(row);
      if (OSS_UNLIKELY(NULL == recordRow))
      {
         PD_LOG(PDERROR, "failed to cast row ptr to record cursor row");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rc = cursorKernal::getNext(session, content);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (content.len() < recordCursorRow::MIN_CONTENT_SIZE)
      {
         PD_LOG(PDERROR, "invalid content len:%d", content.len());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rid = (const recordID *)(content.data());
      transID = (const DPS_TRANS_ID *)((ossValuePtr)(content.data()) + sizeof(recordID));
      if (!_o.indexCover)
      {
         record.reset(content.len() - recordCursorRow::MIN_CONTENT_SIZE,
                      (const CHAR *)((ossValuePtr)(content.data()) +
                       recordCursorRow::MIN_CONTENT_SIZE));
      }
      recordRow->shallowCopy(*rid, *transID, record);

   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
