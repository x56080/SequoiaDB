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

#include "vessel/indexScanCursor.h"
#include "vessel/dataScanRow.h"

namespace engine
{
namespace vessel
{
   indexScanCursor::~indexScanCursor()
   {
      
   }

   INT32 indexScanCursor::getNextRow(IExecutor *executor,
                                     cursorRow *row)
   {
      INT32 rc = SDB_OK;
      slice content;
      const recordID *rid = NULL;
      const DPS_TRANS_ID *transID = NULL;
      slice record;
      dataScanRow *dsr = NULL;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == row ||
                             CURSOR_ROW_TYPE_SCAN != row->getType()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      dsr = static_cast<dataScanRow *>(row);

      rc = cursorKernal::getNext(executor, content);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (content.getSize() < dataScanRow::MIN_CONTENT_SIZE)
      {
         PD_LOG(PDERROR, "invalid content len:%d", content.getSize());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      rid = (const recordID *)(content.data());
      transID = (const DPS_TRANS_ID *)((ossValuePtr)(content.data()) + sizeof(recordID));
      record.reset(content.getSize() - dataScanRow::MIN_CONTENT_SIZE,
                      (const CHAR *)((ossValuePtr)(content.data()) +
                       dataScanRow::MIN_CONTENT_SIZE));
      dsr->shallowCopy(*rid, *transID, record);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexScanCursor::saveEntry(const slice &entryData)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!entryData.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _entryData.copy(entryData.getSize(), entryData.data());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to copy entry data:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
} // namespace vessel

} // namespace engine
