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

   Source File Name = rdpIniter.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/rdpIniter.h"
#include "vessel/runtimePageBuffer.h"
#include "vessel/logRecordContext.h"
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
      logRecordContext lrc;
      UINT64 lidAndSeq = 0;
      slice adjunct;

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
               INVALID_CL_PAGE_SEQ == _sequence ||
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

      rc = pageInitializer::prepareInitLog(context, sizeof(UINT64),
                                           rpb, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare log:%d", rc);
         goto error;
      }

      if (!initRecordDataPage(rpb->getPageSize(),
                              rpb->getGlobalPid().page(),
                              lpid, psv, _logicalId,
                              _sequence + i, rpb->getBuffer()))
      {
         PD_LOG(PDERROR, "failed to init record data page");
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }

      lidAndSeq = pack(_logicalId, _sequence + i);
      adjunct.reset(sizeof(UINT64), &lidAndSeq);
      rc = pageInitializer::commitInitLog(context, rpb->getGlobalPid(),
                                          lpid, psv, PAGE_TYPE_RECORD,
                                          adjunct, &lrc);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to commit log[%lld], rc:%d", lrc.getLsn(), rc);
         ossPanic();
         goto error;
      }

      rpb->commit(lrc.getLsn());
   done:
      return rc;
   error:
      goto done;
   }

   UINT64 rdpIniter::pack(UINT32 logicalId, UINT32 sequence)const
   {
      UINT32 v = logicalId;
      v <<= 32;
      v |= sequence;
      return v;
   }
}//namesapce vessel
}//namespace engine