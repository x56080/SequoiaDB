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

   Source File Name = dataScanContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/dataScanContext.h"
#include "vessel/objectLatchHelper.hpp"
#include "vessel/instanceEnv.h"

namespace engine
{
namespace vessel
{
   dataScanContext::dataScanContext()
   {}

   dataScanContext::~dataScanContext()
   {
      unlockAllRids();
   }

   void dataScanContext::close()
   {
      unlockAllRids();
      requestContext::close();
   }

   INT32 dataScanContext::tryLockRid(ossSharedLatchMode mode,
                                     const recordID &rid,
                                     BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;
      collectionHandle clHandle;

      if (OSS_UNLIKELY(!requestContext::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(mode.isNone() ||
                            !rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      clHandle = requestContext::getCollectionHandle();
      if (!clHandle.isValid())
      {
         PD_LOG(PDERROR, "collection handle is not valid yet");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
         recordIdLatchKey key(clHandle.getSpaceID(),
                              clHandle.getMbId(),
                              rid);
         RECORD_ID_LATCH_MAP &lm = requestContext::getEnv()->ridLatchMap;
         objectLatchHelper<recordIdLatchKey> lh;
         rc = lh.tryLock(lm, _rlc, key, mode, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to try lock rid[%d,%d], rc:%d",
                   rid.getPageID(), rid.getSlotID(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      locked = FALSE;
      goto done;
   }

   INT32 dataScanContext::lockRid(ossSharedLatchMode mode,
                                  const recordID &rid)
   {
      INT32 rc = SDB_OK;
      collectionHandle clHandle;

      if (OSS_UNLIKELY(!requestContext::isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(mode.isNone() ||
                            !rid.valid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      clHandle = requestContext::getCollectionHandle();
      if (!clHandle.isValid())
      {
         PD_LOG(PDERROR, "collection handle is not valid yet");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      {
         recordIdLatchKey key(clHandle.getSpaceID(),
                              clHandle.getMbId(),
                              rid);
         RECORD_ID_LATCH_MAP &lm = requestContext::getEnv()->ridLatchMap;
         objectLatchHelper<recordIdLatchKey> lh;
         rc = lh.lock(lm, _rlc, key, mode);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to lock rid[%d,%d], rc:%d",
                   rid.getPageID(), rid.getSlotID(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void dataScanContext::unlockAllRids()
   {
      if (!_rlc.isEmpty())
      {
         RECORD_ID_LATCH_MAP &lm = requestContext::getEnv()->ridLatchMap;
         objectLatchHelper<recordIdLatchKey> lh;
         lh.releaseAll(lm, _rlc);
      }
      return;
   }

   void dataScanContext::unlockRid(const recordID &rid)
   {
      SDB_ASSERT(rid.valid(), "must be valid");
      if (!_rlc.isEmpty())
      {
         recordIdLatchKey key(requestContext::getSpaceID(),
                              requestContext::getMBID(),
                              rid);
         RECORD_ID_LATCH_MAP &lm = requestContext::getEnv()->ridLatchMap;
         objectLatchHelper<recordIdLatchKey> lh;
         lh.autoUnlock(lm, _rlc, key);
      }
      else
      {
         SDB_ASSERT(FALSE, "impossible");
      }
      return;
   }
} // namespace vessel

} // namespace engine
