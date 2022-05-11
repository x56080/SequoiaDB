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

   Source File Name = indexSpace.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/indexSpace.h"
#include "vessel/idMapPage.h"
#include "vessel/indexDef.h"
#include "vessel/requestContext.h"
#include "vessel/outerResource.h"

namespace engine
{
namespace vessel
{
   INT32 indexSpace::getMinUncompletedLSN(requestContext *context,
                                          DPS_LSN_OFFSET &lsn)
   {
      SDB_ASSERT(NULL != context, "can not be null");
      /// getMinUncompletedLSN is very expensive.
      lsn = context->getOuterResource()->getMinUncompletedLSN();
      return SDB_OK;
   }

   INT32 indexSpace::getRuntimePageBuffer(requestContext *context,
                                          PAGE_ID pid,
                                          const ossSharedLatchMode &mode,
                                          runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!rpb.isValid(), "can not be valid");
      storageFileCluster *fcluster = nullptr;
      logicalPageSpace::_runtimePageBufferIniter initer;
      GLOBAL_PAGE_ID gpid;
      UINT32 pageSize = 0;
      mmapPagePointer ptr;

      if (OSS_UNLIKELY(NULL == context ||
                      INVALID_PAGE_ID == pid ||
                      mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      fcluster = getFileCluster();
      SDB_ASSERT(nullptr != fcluster, "can not be null");

      rc = fcluster->getPageMmapPtr(pid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get page[%d], rc:%d", pid, rc);
         goto error;
      }

      gpid.reset(logicalPageSpace::getSpaceID(),
                 getSpaceType(),
                 FILE_TYPE_DATA_STORAGE,
                 pid);
      pageSize = getFileCluster()->getCoreArgs().pageSize;

      initer.initWithMmap(gpid, pageSize, ptr, rpb);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexSpace::getRuntimePageBufferToReset(requestContext *context,
                                                     PAGE_ID pid,
                                                     runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      ossSharedLatchMode mode(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);

      rc = this->getRuntimePageBuffer(context, pid,
                                      mode, /// usless actually
                                      rpb);
      if (SDB_OK != rc)
      {
         goto error;
      }

      rc = rpb.prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get rpb ready to write:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      rpb.fini();
      goto done;
   }

   INT32 indexSpace::copyPageAndReinitBuffer(requestContext *context,
                                                 PAGE_SNAPSHOT_VERION psv,
                                                 PAGE_ID newPid,
                                                 runtimePageBuffer &rpb)
   {
      INT32 rc = SDB_OK;
      UINT32 pageSize = getFileCluster()->getCoreArgs().pageSize;
      mmapPagePointer ptr;
      GLOBAL_PAGE_ID gpid;
      logicalPageSpace::_runtimePageBufferIniter initer;
      slice rs;

      if (OSS_UNLIKELY(NULL == context ||
                       INVALID_PAGE_SNAPSHOT_VERSION == psv ||
                       INVALID_PAGE_ID == newPid ||
                       !rpb.isValid() ||
                       rpb.isCacheBuffer()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      
      rs = rpb.getSlice();
      if (isPageCrashed((ossValuePtr)(rs.data()), pageSize))
      {
         PD_LOG(PDERROR, "page[%s] may be crashed", rpb.getGlobalPid().toString().c_str());
         rc = SDB_VESSEL_PAGE_CRASHED;
         goto error;
      }

      rc = getFileCluster()->getPageMmapPtr(newPid, ptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "faield to get new page[%d] ptr:%d", newPid, rc);
         goto error;
      }

      ossMemcpy((void *)(ptr.get()), rs.data(), pageSize);
      ((pageHead *)(ptr.get()))->pid = newPid;
      ((pageHead *)(ptr.get()))->psv = psv;

      rpb.fini();

      gpid.reset(getSpaceID(),
                 getSpaceType(),
                 FILE_TYPE_DATA_STORAGE,
                 newPid);

      initer.initWithMmap(gpid, pageSize, ptr, rpb);
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine