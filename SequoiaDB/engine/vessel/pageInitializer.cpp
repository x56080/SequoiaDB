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
#include "dpsLogRecordDef.hpp"
#include "dpsJournalPad.hpp"
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
      dpsStackJournalPad jpad;
      dpsPackedRequest jrequest;
      dpsLogRecordHeader jres;

      jpad.setType(LOG_TYPE_VESSEL_PAGE_INIT);
      jpad.setFlag(DPS_LOG_FLAG_VESSEL);

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

      jrequest = jpad.done();
      rc = journal->write(jrequest, dpsWriteOptions(), &jres);
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