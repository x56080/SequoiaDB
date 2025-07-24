/*******************************************************************************

   Copyright (C) 2011-Present SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = pageInitializer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/pageInitializer.h"
#include "dpsLogRecordDef.hpp"
#include "dpsWriteReqBuilder.hpp"
#include "vessel/outerResource.h"
#include "vessel/requestContext.h"

namespace engine
{
namespace vessel
{
   INT32 pageInitializer::writeJournal(requestContext *context,
                                       const GLOBAL_PAGE_ID &gpid,
                                       PAGE_TYPE type,
                                       const slice &adjunct,
                                       DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      IDataJournal *journal = context->getOuterResource()->journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_VESSEL_PAGE_INIT);

      rc = jpad.append(DPS_LOG_PUBLIC_VESSEL_GPID,
                       GLOBAL_PAGE_ID_SIZE,
                       &gpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failedt append gpid:%d", rc);
         goto error;
      }

      rc = jpad.appendInt32(DPS_LOG_VESSEL_PAGE_INIT_PAGE_TYPE, type);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append page type:%d", rc);
         goto error;
      }

      if (0 < adjunct.getSize())
      {
         rc = jpad.append(DPS_LOG_VESSEL_PAGE_INIT_ADJUNCT,
                          adjunct.getSize(), adjunct.data());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append adjunct:%d", rc);
            goto error;
         }
      }

      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(), jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      lsn = jres._lsn;
   done:
      return rc;
   error:
      goto done;
   }
}//class vessel
}//class vessel