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
#include "dpsJournalPad.hpp"
#include "vessel/instanceEnv.h"

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

   INT32 commitCreateIndexLog(requestContext *context,
                              const strSlice &fullName,
                              UINT32 indexId,
                              INT32 indexSlot,
                              const slice &indexDef)
   {
      INT32 rc = SDB_OK;

      IDataJournal *journal = nullptr;
      dpsStackJournalPad jpad;
      dpsPackedRequest jrequest;

      if (NULL == context ||
          fullName.empty() ||
          !indexDef.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      journal = context->getEnv()->resource.journal;
      jpad.setType(LOG_TYPE_IX_CRT);
      jpad.setFlag(DPS_LOG_FLAG_VESSEL);
      rc = jpad.append(DPS_LOG_PUBLIC_FULLNAME,
                       fullName.strLen() + 1,
                       fullName.str());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append full name:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_IXCRT_IX_DEF_OBJ,
                       indexDef.getSize(),
                       indexDef.getData());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append index def:%d", rc);
         goto error;
      }

      jrequest = jpad.done();

      rc = journal->write(jrequest, dpsWriteOptions(), nullptr);
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

   INT32 commitCreateIndexEndLog(requestContext *context,
                                 const strSlice &fullName,
                                 const strSlice &indexName,
                                 UINT32 indexId,
                                 INT32 indexSlot,
                                 INT32 result)
   {
      INT32 rc = SDB_OK;
      IDataJournal *journal = nullptr;
      dpsStackJournalPad jpad;
      dpsPackedRequest jrequest;

      if (NULL == context ||
          fullName.empty() ||
          indexName.empty())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      journal = context->getEnv()->resource.journal;

      jpad.setType(LOG_TYPE_IX_CRT);
      jpad.setFlag(DPS_LOG_FLAG_VESSEL);

      jrequest = jpad.done();
      rc = journal->write(jrequest, dpsWriteOptions(), nullptr);
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

   INT32 commitReleasingPagesLog(requestContext *context,
                                 const ossPoolVector<PAGE_ID> &lpids,
                                 const bson::BSONObj &adjunct)
   {
      INT32 rc = SDB_OK;
      IDataJournal *journal = context->getEnv()->resource.journal;
      dpsStackJournalPad jpad;
      dpsPackedRequest jrequest;

      jpad.setType(LOG_TYPE_VESSEL_ROUTE_PAGE_UPDATE);
      jpad.setFlag(DPS_LOG_FLAG_VESSEL);
      jrequest = jpad.done();
      rc = journal->write(jrequest, dpsWriteOptions(), nullptr);
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

/////////////logicalPageSapceLogUtil begin
   INT32 lpsLogUtil::commit(requestContext *context,
                             SPACE_ID sid,
                             SPACE_TYPE spaceType,
                             FILE_TYPE fileType,
                             const deltaLogRecord &dlr,
                             DPS_LSN_OFFSET &lsn)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(nullptr != context, "can not be invalid");
      SDB_ASSERT(dlr.isValid(), "can not be invalid");
      IDataJournal *journal = context->getEnv()->resource.journal;
      dpsStackJournalPad jpad;
      dpsPackedRequest jrequest;
      dpsLogRecordHeader jres;
      UINT32 packedValue = packSidAndType(sid, spaceType, fileType);

      jpad.setType(LOG_TYPE_VESSEL_LPS_PAGE_MANAGEMENT);
      jpad.setFlag(DPS_LOG_FLAG_VESSEL);
      rc = jpad.appendInt32(DPS_LOG_VESSEL_LPS_PM_SID_AND_TYPE, packedValue);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append sid and type:%d", rc);
         goto error;
      }

      rc = jpad.append(DPS_LOG_VESSEL_LPS_PM_DELTA_LOG,
                       dlr.getLogHead()->_size,
                       dlr.getLogHead());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to append delta log:%d", rc);
         goto error;
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
/////////////logicalPageSapceLogUtil end
}//namespace vessel
}//namespace engine