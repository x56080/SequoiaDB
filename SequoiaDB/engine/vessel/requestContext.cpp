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

   Source File Name = requestContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/requestContext.h"
#include "vessel/spaceIDLocker.h"
#include "vessel/instanceEnv.h"
#include "vessel/atomicOperationList.h"
#include "vessel/objectLatchHelper.hpp"
#include "vessel/runtimeMbContext.h"

namespace engine
{
namespace vessel
{
/////////////////////////requestContext
   requestContext::requestContext():
   _sba(_staticBuf, CONTEXT_DEFAULT_BUFFER_POOL_SIZE)
   {

   }

   void requestContext::open(IExecutor *executor,
                              instanceEnv *env)
   {
      SDB_ASSERT(NULL != executor, "can not be null");
      SDB_ASSERT(NULL != env, "can not be null");
      SDB_ASSERT(env->resource.isValid(), "can not be invalid");
      SDB_ASSERT(NULL == _executor, "do not reinit");
      _close();

      _executor = executor;
      _env = env;
      return;
   }

   requestContext::~requestContext()
   {
      _close();
   }

   void requestContext::_close()
   {
      if (!isOpen())
      {
         goto done;
      }

      {
      SDB_ASSERT(!_sba.hasUnfreeBuffer(), "memory leak");
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(NULL == _oplist, "detaching missed");

      _unlockAll();

      _executor = NULL;
      _env = NULL;
      
      _sba.clearBufferAllocated();
      _oplist = NULL;
      }
      
   done:
     return;
   }

   outerResource *requestContext::getOuterResource()const
   {
      SDB_ASSERT(NULL != _env, "can not be null");
      return &(_env->resource);
   }

   CHAR *requestContext::allocateBuffer(UINT32 size)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      return _sba.allocate(size);
   }
   
   void requestContext::releaseBuffer(void *buffer)
   {
      _sba.release((CHAR *)buffer);
      return;
   }

   INT32 requestContext::lockSpaceID(SPACE_ID sid,
                                     OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "do not relock sid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _env->spaceLocker.lock(sid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", sid, rc);
         goto error;
      }

      _sid = sid;
      _sidMode = mode;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryLockSpaceID(SPACE_ID sid,
                                        OSS_LATCH_MODE mode,
                                        BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;

      locked = FALSE;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "do not relock sid");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _env->spaceLocker.tryLock(sid, mode, locked);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (locked)
      {
         _sid = sid;
         _sidMode = mode;
      }
   done:
      return rc;
   error:
      goto done;
   }
   

   void requestContext::unlockSpaceID()
   {
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(!isMbLocked(), "unlocking missing");
      _unlockAll();
      return;
   }

   BOOLEAN requestContext::isSpaceIdLocked(OSS_LATCH_MODE *mode)const
   {
      BOOLEAN r = INVALID_SPACE_ID != _sid;
      if (r && NULL != mode)
      {
         *mode = _sidMode;
      }
      return r;
   }

   void requestContext::lockMB(CL_MB_ID mbID,
                               ossRWMutex *mutex,
                               OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(isSpaceIdLocked(), "lock sid first");
      SDB_ASSERT(!isMbLocked(), "do not relock");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != mutex, "can not be null");
      if (SHARED == mode)
      {
         mutex->lock_r();
      }
      else
      {
         mutex->lock_w();
      }
      _mbID = mbID;
      _mbMutex = mutex;
      _mbMode = mode;
      return;
   }

   BOOLEAN requestContext::tryLockMB(CL_MB_ID mbID,
                                     ossRWMutex *mutex,
                                     OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(isSpaceIdLocked(), "lock sid first");
      SDB_ASSERT(!isMbLocked(), "do not relock");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != mutex, "can not be null");

      BOOLEAN r = FALSE;
      if (SHARED == mode)
      {
         r = mutex->try_lock_r();
      }
      else
      {
         r = mutex->try_lock_w();
      }

      if (r)
      {
         _mbID = mbID;
         _mbMutex = mutex;
         _mbMode = mode;
      }
      return r;
   }

   BOOLEAN requestContext::isMbLocked(OSS_LATCH_MODE *mode)const
   {
      BOOLEAN r = NULL != _mbMutex;
      if (r && NULL != mode)
      {
         *mode = _mbMode;
      }
      return r;
   }

   void requestContext::unlockMB()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      if (isMbLocked())
      {
         if (SHARED == _mbMode)
         {
            _mbMutex->release_r();
         }
         else
         {
            _mbMutex->release_w();
         }

         _mbID = INVALID_CL_MB_ID;
         _mbMutex = NULL;
         _mbMode = SHARED;
      }
      return;
   }

   void requestContext::attachMbContext(runtimeMbContext *rmc)
   {
      SDB_ASSERT(NULL != rmc && rmc->isValid(), "can not be invalid");
      SDB_ASSERT(isMbLocked(), "lock mb first");
      SDB_ASSERT(rmc->getGlobalId().getMbId() == _mbID, "must be same mb");
      SDB_ASSERT(NULL == _rmc, "do not reattach");
      _rmc = rmc;
   }

   void requestContext::detachMbContext()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      if (NULL != _rmc)
      {
         if (!_rmc->getRidLatchContext().isEmpty())
         {
            unlockRids();
         }

         _rmc = NULL;
      }
   }

   INT32 requestContext::lockLpid(SPACE_TYPE type,
                                  PAGE_ID lpid,
                                  const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      logicalPidLatchKey key;
      objectLatchHelper<logicalPidLatchKey> lh;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      key = logicalPidLatchKey(_sid, type, lpid);

      rc = lh.lock(_env->lpidLatchMap, _lpidLatchContext, key, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryLockLpid(SPACE_TYPE type,
                                     PAGE_ID lpid,
                                     const ossSharedLatchMode &mode,
                                     BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      logicalPidLatchKey key;
      objectLatchHelper<logicalPidLatchKey> lh;
      locked = FALSE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid ||
                            mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      key = logicalPidLatchKey(_sid, type, lpid);

      rc = lh.tryLock(_env->lpidLatchMap, _lpidLatchContext,
                      key, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d], rc:%d", lpid, rc);
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }
   
   void requestContext::unlockLpid(SPACE_TYPE type, PAGE_ID lpid)
   {
      logicalPidLatchKey key;
      objectLatchHelper<logicalPidLatchKey> lh;

      if (OSS_UNLIKELY(!isOpen()))
      {
         SDB_ASSERT(FALSE, "not open");
         goto done;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid))
      {
         SDB_ASSERT(FALSE, "invalid key");
         goto done;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "impossible");
         goto done;
      }

      key = logicalPidLatchKey(_sid, type, lpid);
      lh.autoUnlock(_env->lpidLatchMap, _lpidLatchContext, key);
   done:
      return;
   }

   BOOLEAN requestContext::testLpidLocked(SPACE_TYPE type,
                                          PAGE_ID lpid,
                                          ossSharedLatchMode *mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(isSpaceIdLocked(), "must be locking");
      SDB_ASSERT(INVALID_SPACE_TYPE != type, "can not be invalid");
      SDB_ASSERT(INVALID_PAGE_ID != lpid, "can not be invalid");
      logicalPidLatchKey key(_sid, type, lpid);
      return _lpidLatchContext.test(key, mode);
   }

   INT32 requestContext::lockFromUpgradeToExclusive(SPACE_TYPE type,
                                                    PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen() || !isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         objectLatchHelper<logicalPidLatchKey> lh;
         logicalPidLatchKey key(_sid, type, lpid);
         rc = lh.lockFromUpgradeToExclusive(_lpidLatchContext, key);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update lpid[%s] from upgrade to exclusive:%d",
                   key.toString().c_str(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryLockFromUpgradeToExclusive(SPACE_TYPE type,
                                                       PAGE_ID lpid,
                                                       BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;
      if (OSS_UNLIKELY(!isOpen() || !isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         objectLatchHelper<logicalPidLatchKey> lh;
         logicalPidLatchKey key(_sid, type, lpid);
         rc = lh.tryLockFromUpgradeToExclusive(_lpidLatchContext, key, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update lpid[%s] from upgrade to exclusive:%d",
                   key.toString().c_str(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryLockFromSharedToExclusive(SPACE_TYPE type,
                                                      PAGE_ID lpid,
                                                      BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;
      if (OSS_UNLIKELY(!isOpen() || !isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      {
         objectLatchHelper<logicalPidLatchKey> lh;
         logicalPidLatchKey key(_sid, type, lpid);
         rc = lh.tryLockFromSharedToExclusive(_lpidLatchContext, key, locked);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to update lpid[%s] from upgrade to exclusive:%d",
                   key.toString().c_str(), rc);
            goto error;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN requestContext::isInProcessingOplist()const
   {
      return NULL != _oplist && !(_oplist->isReadonly());
   }

   void requestContext::swtichOplist(atomicOperationList *newOplist,
                                     atomicOperationList **oldOplist)
   {
      *oldOplist = _oplist;
      _oplist = newOplist;
      return;
   }

   INT32 requestContext::blockCheckpoint(SPACE_TYPE type,
                                         ossRWMutex *mutex)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "lock space first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _blocker.block(_sid, type, mutex);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryToBlockCheckpoint(SPACE_TYPE type,
                                              ossRWMutex *mutex,
                                              BOOLEAN &blocked)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "lock space first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _blocker.tryToBlock(_sid, type, mutex, blocked);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void requestContext::unblockCheckpoint()
   {
      _blocker.unblock();
   }

   INT32 requestContext::lockRid(const recordID &rid,
                                 const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      recordIdLatchKey key;
      objectLatchHelper<recordIdLatchKey> lh;

      if (OSS_UNLIKELY(!rid.isValid() ||
                       mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isMbContextAttached()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      key = recordIdLatchKey(_sid, _mbID, rid);
      rc = lh.lock(_env->ridLatchMap, _rmc->getRidLatchContext(), key, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock rid[%s], rc:%d", key.toString().c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryLockRid(const recordID &rid,
                                    const ossSharedLatchMode &mode,
                                    BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      recordIdLatchKey key;
      objectLatchHelper<recordIdLatchKey> lh;
      locked = FALSE;

      if (OSS_UNLIKELY(!rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isMbContextAttached()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      key = recordIdLatchKey(_sid, _mbID, rid);
      rc = lh.tryLock(_env->ridLatchMap,
                      _rmc->getRidLatchContext(),
                      key, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to try lock rid[%s], rc:%d", key.toString().c_str(), rc);
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
   
   void requestContext::unlockRid(const recordID &rid)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      SDB_ASSERT(isMbContextAttached(), "must be attached");
      objectLatchHelper<recordIdLatchKey> lh;
      recordIdLatchKey key(_sid, _mbID, rid);
      lh.autoUnlock(_env->ridLatchMap, _rmc->getRidLatchContext(), key);
   }

   void requestContext::unlockRids()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isMbContextAttached(), "must be attached");
      objectLatchHelper<recordIdLatchKey> lh;
      lh.releaseAll(_env->ridLatchMap, _rmc->getRidLatchContext());
   }

   BOOLEAN requestContext::testRidLocked(const recordID &rid,
                                         ossSharedLatchMode *mode)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isMbContextAttached(), "must be attached");
      recordIdLatchKey key(_sid, _mbID, rid);
      return _rmc->getRidLatchContext().test(key, mode);
   }

   void requestContext::waitRid(const recordID &rid,
                                const ossSharedLatchMode &mode)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isMbContextAttached(), "must be attached");
      SDB_ASSERT(!mode.isNone(), "can not be none");
      recordIdLatchKey key(_sid, _mbID, rid);
      objectLatchHelper<recordIdLatchKey> lh;
#if defined (_DEBUG)
      SDB_ASSERT(!_rmc->getRidLatchContext().test(key, NULL), "invalid waiting");
#endif//_DEBUT
      lh.testNotExistsOrWait(_env->ridLatchMap, key, mode);
   }

   void requestContext::_unlockAll()
   {
      SDB_ASSERT(isOpen(), "can not be closed");

      if (isMbContextAttached())
      {
         detachMbContext();
      }

      if (isMbLocked())
      {
         unlockMB();
      }

      if (!_lpidLatchContext.isEmpty())
      {
         objectLatchHelper<logicalPidLatchKey>().releaseAll(_env->lpidLatchMap,
                                                            _lpidLatchContext);
         SDB_ASSERT(_lpidLatchContext.isEmpty(), "must be empty");
      }

      if (_blocker.isBlocking())
      {
         _blocker.terminate();
      }

      if (isSpaceIdLocked())
      {
         _env->spaceLocker.unlock(_sid, _sidMode);
         _sid = INVALID_SPACE_ID;
         _sidMode = SHARED;
      }
      return;
   }

   INT32 requestContext::acquireTransLock(const recordID &rid,
                                          const DPS_TRANSLOCK_TYPE &mode)
   {
      INT32 rc = SDB_OK;
      ITransLockConsole *console = NULL;
      dpsTransLockId lockId;
      dmsRecordID dmsRid;

      if (OSS_UNLIKELY(!rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen() ||
                            !isMbContextAttached()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      console = getOuterResource()->transLockConsole;
      dmsRid = rid.toDMSRid();
      lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      rc = console->acquire(getExecutor(), lockId, mode, NULL, NULL, NULL);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock rid:%s, rc:%d", rid.toString().c_str(), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::tryAcquireTransLock(const recordID &rid,
                                             const DPS_TRANSLOCK_TYPE &mode,
                                             BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      ITransLockConsole *console = NULL;
      dpsTransLockId lockId;
      dmsRecordID dmsRid;
      locked = FALSE;

      if (OSS_UNLIKELY(!rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen() ||
                            !isMbContextAttached()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      console = getOuterResource()->transLockConsole;
      dmsRid = rid.toDMSRid();
      lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      rc = console->tryAcquire(getExecutor(), lockId, mode, NULL, NULL);
      if (SDB_DPS_TRANS_LOCK_INCOMPATIBLE == rc)
      {
         rc = SDB_OK;
         goto done;
      }
      else if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock rid:%s, rc:%d", rid.toString().c_str(), rc);
         goto error;
      }

      locked = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   void requestContext::releaseTransLock(const recordID &rid)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(rid.isValid(), "can not be closed");
      dpsTransLockId lockId;
      dmsRecordID dmsRid = rid.toDMSRid();
      lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      getOuterResource()->transLockConsole->release(getExecutor(), lockId, FALSE, NULL);
   }

   void requestContext::releaseAllTransLock()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      getOuterResource()->transLockConsole->releaseAll(getExecutor(), NULL);
   }
}//namespace vessel
}//namespace engine