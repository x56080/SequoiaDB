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

   Source File Name = pageInitializer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/pageInitializer.h"
#include "vessel/logRecordContext.h"
#include "dpsLogRecordDef.hpp"

namespace engine
{
namespace vessel
{
   INT32 pageInitializer::prepareInitLog(requestContext *context,
                                          UINT32 adjunctSize,
                                          const runtimePageBuffer *rpb,
                                          logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      rc = pageAccessor::prepareLog(context, rpb, LOG_TYPE_VESSEL_PAGE_INIT,
                                    TRUE, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      lrc->prepush(sizeof(GLOBAL_PAGE_ID));
      lrc->prepush(sizeof(PAGE_ID));
      lrc->prepush(sizeof(PAGE_SNAPSHOT_VERION));
      lrc->prepush(sizeof(PAGE_TYPE));
      if (0 < adjunctSize)
      {
         lrc->prepush(adjunctSize);
      }

      rc = pageAccessor::prepareLogDone(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare done log:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 pageInitializer::commitInitLog(requestContext *context,
                                        const GLOBAL_PAGE_ID &gpid,
                                        PAGE_ID lpid,
                                        PAGE_SNAPSHOT_VERION psv,
                                        PAGE_TYPE type,
                                        const slice &adjunct,
                                        logRecordContext *lrc)
   {
      INT32 rc = SDB_OK;
      rc = pageAccessor::pushElement(context, DPS_LOG_PUBLIC_VESSEL_GPID,
                                     sizeof(GLOBAL_PAGE_ID),
                                     &gpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_PAGE_INIT_LPID,
                                     sizeof(PAGE_ID),
                                     &lpid, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_PAGE_INIT_PSV,
                                     sizeof(PAGE_SNAPSHOT_VERION),
                                     &psv, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_PAGE_INIT_PAGE_TYPE,
                                     sizeof(PAGE_TYPE),
                                     &type, lrc);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (0 < adjunct.len())
      {
         rc = pageAccessor::pushElement(context, DPS_LOG_VESSEL_PAGE_INIT_ADJUNCT,
                                        adjunct.len(), adjunct.data(), lrc);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      rc = pageAccessor::commitLog(context, lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc->getLsn(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//class vessel
}//class vessel