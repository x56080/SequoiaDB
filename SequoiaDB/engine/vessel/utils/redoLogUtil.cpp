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

   Source File Name = redoLogUtil.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/redoLogUtil.h"
#include "dpsLogRecord.hpp"
#include "ossLikely.hpp"
#include "dpsLogRecordDef.hpp"
#include "vessel/vesselOptions.h"
#include "vessel/strSlice.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"
#include "vessel/atomicOperationList.h"
#include "dpsWriteReqBuilder.hpp"
#include "vessel/instanceEnv.h"
#include "vessel/threadContext.h"

namespace engine
{
namespace vessel
{
   UINT32 packSidAndType(SPACE_ID sid,
                            SPACE_TYPE spaceType,
                              FILE_TYPE fileType)
   {
      UINT32 value = ((UINT32)fileType << 24);
      value |= ((UINT32)spaceType << 16);
      value |= (UINT32)sid;
      return value;
   }

   void unpackSidAndType(UINT32 value,
                         SPACE_ID &sid,
                         SPACE_TYPE &spaceType,
                         FILE_TYPE &fileType)
   {
      sid = value;
      spaceType = (value >> 16);
      fileType = (value >> 24);
      return;
   }

   INT32 commitCreateIndexLog(const ossPoolString &fullName,
                              UINT32 indexLogicalId,
                              const bson::BSONObj &obj,
                              DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      IDataJournal *journal = tc->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      lsn = DPS_INVALID_LSN_OFFSET;

      if (fullName.empty() ||
          !obj.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      jpad.setType(LOG_TYPE_IX_CRT);
      
      rc = jpad.append(DPS_LOG_PUBLIC_FULLNAME,
                       fullName.size() + 1,
                       fullName.c_str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append full name:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_IXCRT_IX_DEF_OBJ,
                       obj.objsize(),
                       obj.objdata());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append index def:%d", rc);
         goto error;
      }

      jrequest = jpad.reap();

      rc = journal->write(tc->getExecutor(), jrequest, dpsWriteOptions(), &jres);
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

   INT32 commitCreateIndexEndLog(const ossPoolString &fullName,
                                 const std::string &indexName,
                                 UINT32 indexId,
                                 INT32 result,
                                 DPS_LSN_OFFSET *lsn)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      IDataJournal *journal = tc->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      if (nullptr != lsn)
      {
         *lsn = DPS_INVALID_LSN_OFFSET;
      }

      if (fullName.empty() ||
          indexName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      jpad.setType(LOG_TYPE_IX_CRT);

      jrequest = jpad.reap();
      rc = journal->write(tc->getExecutor(),  jrequest, dpsWriteOptions(), &jres);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }

      if (nullptr != lsn)
      {
         *lsn = jres._lsn;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 commitRemoveIndexLog(const ossPoolString &fullName,
                              const std::string &indexName,
                              UINT32 indexId,
                              DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      IDataJournal *journal = tc->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;

      if (fullName.empty() ||
          indexName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      jpad.setType(LOG_TYPE_IX_DELETE);
      

      jrequest = jpad.reap();
      rc = journal->write(tc->getExecutor(),  jrequest, dpsWriteOptions(), &jres);
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

   INT32 commitReleasingPagesLog(requestContext *context,
                                 const ossPoolVector<PAGE_ID> &lpids,
                                 const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      IDataJournal *journal = context->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;

      jpad.setType(LOG_TYPE_VESSEL_ROUTE_PAGE_UPDATE);
      
      jrequest = jpad.reap();
      rc = journal->write(context->getExecutor(),  jrequest, dpsWriteOptions(), nullptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to write journal:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 commitTruncateCLLog(const ossPoolString &fullName,
                             DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      IDataJournal *journal = tc->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;
      jpad.setType(LOG_TYPE_CL_TRUNC);
      
      jrequest = jpad.reap();
      rc = journal->write(tc->getExecutor(),  jrequest, dpsWriteOptions(), &jres);
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

   INT32 commitRemoveCLLog(const ossPoolString &fullName,
                           DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      IDataJournal *journal = tc->getEnv()->resource.journal;
      dpsWriteReqBuilder jpad;
      dpsWriteRequest jrequest;
      dpsLogRecordHeader jres;
      jpad.setType(LOG_TYPE_CL_DELETE);
      
      jrequest = jpad.reap();
      rc = journal->write(tc->getExecutor(),  jrequest, dpsWriteOptions(), &jres);
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
/////////////logicalPageSapceLogUtil end
}//namespace vessel
}//namespace engine