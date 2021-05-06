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

   Source File Name = logicalPageAccessor.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageAccessor.h"
#include "ossLikely.hpp"
#include "vessel/logicalPageSpace.h"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   logicalPageAccessor::logicalPageAccessor()
   {}

   logicalPageAccessor::~logicalPageAccessor()
   {
      SDB_ASSERT(!_lh.isLocked(), "fini missed");
      _lh.unlock();
   }

   INT32 logicalPageAccessor::init(requestContext *context,
                                   FILE_TYPE type,
                                   PAGE_ID lpid,
                                   const pageAccessor::options &o,
                                   logicalPageSpace *space,
                                   DPS_LSN_OFFSET oplist)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(INVALID_PAGE_ID == _lpid, "do not reinit");
      OSS_LATCH_MODE mode = o.readOnly ? SHARED : EXCLUSIVE;
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!context->getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = INVALID_PAGE_ID;
         goto error;
      }
      else if (OSS_UNLIKELY(NULL == space))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(context->getSpaceID() != space->getSpaceID()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _lpid = lpid;
      _space = space;
      rc = _lh.lock(context, type, lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      rc = _init(context, type, o.getFlags(), oplist, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      fini(context);
      goto done;
   }

   void logicalPageAccessor::fini(requestContext *context)
   {
      pageAccessor::fini(context);
      _lh.unlock();
      _lpid = INVALID_PAGE_ID;
      _snapshot = INVALID_SNAPSHOT_ID;
      _space = NULL;
      return;
   }

   INT32 logicalPageAccessor::_init(requestContext *context,
                                    FILE_TYPE type,
                                    UINT32 flags,
                                    DPS_LSN_OFFSET oplist,
                                    BOOLEAN toWrite)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(_lh.isLocked(), "must be locked");
      SDB_ASSERT(NULL != _space, "can not be null");
      PAGE_ID pid = INVALID_PAGE_ID;
      SNAPSHOT_ID snap = INVALID_SNAPSHOT_ID;
      if (toWrite)
      {
         rc = _space->getPhysicalPidToWrite(context, _lpid, pid, &snap);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get pid of lpid[%d], rc:%d",
                  _lpid, rc);
            goto error;
         }
      }
      else
      {
         rc = _space->getPhysicalPid(context, _lpid, pid, &snap);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to get pid of lpid[%d], rc:%d",
                  _lpid, rc);
            goto error;
         }
      }

      _snapshot = snap;

      rc = pageAccessor::initUniversally(context, type, pid, flags,
                                         _space->getSU(), oplist);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init accessor:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 logicalPageAccessor::prepareToWrite(requestContext *context)
   {
      INT32 rc = SDB_OK;
      OSS_LATCH_MODE mode = SHARED;
      SDB_ASSERT(!isReadOnly(), "impossible");
      SDB_ASSERT(NULL != _space, "can not be null");
      SDB_ASSERT(_lh.isLocked(&mode), "must locked");
      SDB_ASSERT(EXCLUSIVE == mode, "must be exclusive");
      SPACE_ID sid = getGPID().space();
      snapshotContainer &sc = context->getEnv()->snapContainer;

      if (isReadOnly())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      if (INVALID_SNAPSHOT_ID != _snapshot &&
          sc.contains(_snapshot, sid))
      {
         FILE_TYPE type = getGPID().type();
         UINT32 flags = getFlags();
         DPS_LSN_OFFSET oplist = getOplist();
         pageAccessor::fini(context);
         rc = _init(context, type, flags, oplist, TRUE);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to reinit accessor:%d", rc);
            goto error;
         }
      }

      rc = pageAccessor::prepareToWrite(context);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to prepare to write:%d", rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine