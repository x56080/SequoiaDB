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

   Source File Name = collectionHandler.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/collectionHandler.h"
#include "vessel/insertOptions.h"
#include "vessel/vesselImpl.h"
#include "vessel/IQueryFilter.h"
#include "vessel/scanCLCursor.h"

namespace engine
{
namespace vessel
{
   INT32 collectionHandler::insert(ISession *session,
                                   const recordData &record,
                                   const DPS_TRANS_ID &transID,
                                   STRIPING_ID striping,
                                   const insertOptions *options,
                                   utilInsertResult &res)
   {
      INT32 rc = SDB_OK;
      if (!isOpen())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (NULL == session ||
               !record.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _db->insert(session, _handle, record, transID, striping, options, res);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionHandler::openScanCursor(ISession *session,
                                           IQueryFilter *filter,
                                           const scanCLOptions *scanOptions,
                                           const cursorOptions *cursorOptions,
                                           cursorHandler &cursor)
   {
      INT32 rc = SDB_OK;
      scanCLCursor *kernal = NULL;

      if (OSS_UNLIKELY(NULL == session))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isOpen())
      {
         rc = SDB_INVALIDARG;
      }

      kernal = SDB_OSS_NEW scanCLCursor();
      if (NULL == kernal)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to allocate mem");
         goto error;
      }

      rc = kernal->open(_db, filter, cursorOptions);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to open cursor:%d", rc);
         goto error;
      }

      kernal->resetToScan(_handle, scanOptions);

      cursor = cursorHandler(kernal);
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(kernal);
      goto done;
   }
}//namespace vessel
}//namespace engine