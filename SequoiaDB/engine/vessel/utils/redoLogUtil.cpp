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

   Source File Name = redoLogUtil.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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