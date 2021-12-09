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

   Source File Name = lpidLockHelper.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpidLockHelper.h"
#include "vessel/requestContext.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   INT32 lpidLockHelper::lock(requestContext *context,
                              SPACE_TYPE type,
                              PAGE_ID lpid,
                              const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY((NULL == context ||
                       INVALID_SPACE_TYPE == type ||
                       INVALID_PAGE_ID == lpid ||
                       mode.isNone())))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(isLocked()))
      {
         SDB_ASSERT(FALSE, "should not be locked");
         unlock();
      }

      rc = context->lockLpid(type, lpid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      _context = context;
      _type = type;
      _lpid = lpid;
      _mode = mode;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpidLockHelper::tryLock(requestContext *context,
                                 SPACE_TYPE type,
                                 PAGE_ID lpid,
                                 const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;
      if (OSS_UNLIKELY(NULL == context))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(isLocked()))
      {
         SDB_ASSERT(FALSE, "should not be locked");
         unlock();
      }

      rc = context->tryLockLpid(type, lpid, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }

      if (locked)
      {
         _context = context;
         _type = type;
         _lpid = lpid;
         _mode = mode;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lpidLockHelper::unlock()
   {
      if (isLocked())
      {
         _context->unlockLpid(_type, _lpid);
         _context = NULL;
         _type = INVALID_SPACE_TYPE;
         _lpid = INVALID_PAGE_ID;
         _mode.setNone();
      }
      return;
   }

   void lpidLockHelper::lockExclusiveFromUpgrade()
   {
      SDB_ASSERT(_mode.isUpgrade(), "must holding upgrade");
      LPID_LATCH_CONTEXT &lc = _context->getLpidLatchContext();
      LOGICAL_PID_LATCH_MAP::object obj;
      ossSharedLatchMode *mode = NULL;
      logicalPidLatchKey key(_context->getSpaceID(), _type, _lpid);

      if (!lc.findToUpdate(key, obj, &mode))
      {
         PD_LOG(PDERROR, "failed to find latch obj[%s] in context",
                key.toString().c_str());
         ossPanic();
      }

      SDB_ASSERT(mode->isUpgrade(), "impossible");
      obj.getValue().unlockUpgradeAndLock();
      mode->setExclusive();
      _mode.setExclusive();
      return;
   }

   BOOLEAN lpidLockHelper::tryLockExclusiveFromUpgrade()
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(_mode.isUpgrade(), "must holding shared lock");
      objectSharedLatchContext<logicalPidLatchKey> &lc = _context->getLpidLatchContext();
      LOGICAL_PID_LATCH_MAP::object obj;
      ossSharedLatchMode *mode = NULL;
      logicalPidLatchKey key(_context->getSpaceID(), _type, _lpid);

      if (!lc.findToUpdate(key, obj, &mode))
      {
         PD_LOG(PDERROR, "failed to find latch obj[%s] in context",
                key.toString().c_str());
         ossPanic();
      }

      SDB_ASSERT(mode->isUpgrade(), "must be upgrade");
      r = obj.getValue().tryUnlockUpgradeAndLock();
      if (r)
      {
         mode->setExclusive();
         _mode.setExclusive();
      }

      return r;
   }

   BOOLEAN lpidLockHelper::tryLockExclusiveFromShared()
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(_mode.isShared(), "must holding shared lock");
      objectSharedLatchContext<logicalPidLatchKey> &lc = _context->getLpidLatchContext();
      LOGICAL_PID_LATCH_MAP::object obj;
      ossSharedLatchMode *mode = NULL;
      logicalPidLatchKey key(_context->getSpaceID(), _type, _lpid);

      if (!lc.findToUpdate(key, obj, &mode))
      {
         PD_LOG(PDERROR, "failed to find latch obj[%s] in context",
                key.toString().c_str());
         ossPanic();
      }

      SDB_ASSERT(mode->isShared(), "must be shared");
      r = obj.getValue().tryUnlockSharedAndLock();
      if (r)
      {
         mode->setExclusive();
         _mode.setExclusive();
      }

      return r;
   }
}//namespace vessel
}//namespace engine