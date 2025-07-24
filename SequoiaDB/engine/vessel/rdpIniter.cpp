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

   Source File Name = rdpIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/rdpIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/recordDataPage.h"

namespace engine
{
namespace vessel
{
   INT32 rdpIniter::initPage(requestContext *context,
                             PAGE_ID lpid,
                             PAGE_SNAPSHOT_VERION psv,
                             runtimePageBuffer *rpb)
   {
      return initInTurns(context, 0, lpid, psv, rpb);
   }

   INT32 rdpIniter::initInTurns(requestContext *context,
                                UINT32 i,
                                PAGE_ID lpid,
                                PAGE_SNAPSHOT_VERION psv,
                                runtimePageBuffer *rpb)
   {
      INT32 rc = SDB_OK;
      UINT64 lidAndSeq = 0;
      slice adjunct;
      DPS_LSN_OFFSET lsn = DPS_INVALID_LSN_OFFSET;

      if (OSS_UNLIKELY(NULL == context ||
                      INVALID_PAGE_ID == lpid ||
                      INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                      NULL == rpb ||
                      !rpb->isWritingPrepared()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (DMS_INVALID_LOGICCLID == _logicalId ||
               0 == _batchCount)
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (_batchCount <= i)
      {
         rc = SDB_OUT_OF_BOUND;
         goto error;
      }

      if (!initRecordDataPage(rpb->getPageSize(),
                              rpb->getGlobalPid().page(),
                              lpid, psv, _logicalId,
                              _sequence + i, rpb->getWritableBuffer().getWPtr()))
      {
         PD_LOG(PDERROR, "failed to init record data page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      lidAndSeq = pack(_logicalId, _sequence + i);
      adjunct.reset(sizeof(UINT64), (const CHAR *)(&lidAndSeq));

      rc = pageInitializer::writeJournal(context, rpb->getGlobalPid(),
                                         PAGE_TYPE_RECORD, adjunct, lsn);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal, rc:%d", rc);
         goto error;
      }

      rpb->commit(lsn);
   done:
      return rc;
   error:
      goto done;
   }

   UINT64 rdpIniter::pack(UINT32 logicalId, UINT32 sequence)const
   {
      UINT64 v = logicalId;
      v <<= 32;
      v |= sequence;
      return v;
   }
}//namesapce vessel
}//namespace engine