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

   Source File Name = requestContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/requestContext.h"
#include "vessel/spaceIDLocker.h"
#include "vessel/instanceEnv.h"
#include "vessel/atomicOperationList.h"
#include "vessel/objectLatchHelper.hpp"
#include "vessel/collectionProperties.h"

namespace engine
{
namespace vessel
{
   requestContext::~requestContext()
   {
      _close();
   }

   void requestContext::close()
   {
      _onClose();
      _close();
   }

   void requestContext::_close()
   {
      SDB_ASSERT(_lpidLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(_ridLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(nullptr == _oplist, "detaching missed");

      _oplist = nullptr;

      if (!_lpidLatchContext.isEmpty())
      {
         objectLatchHelper<logicalPidLatchKey>().releaseAll(getEnv()->latchEnv.lpidLatchMap,
                                                            _lpidLatchContext);
         SDB_ASSERT(_lpidLatchContext.isEmpty(), "must be empty");
      }

      if (!_ridLatchContext.isEmpty())
      {
         objectLatchHelper<recordIdLatchKey>().releaseAll(getEnv()->latchEnv.ridLatchMap,
                                                          _ridLatchContext);
         SDB_ASSERT(_ridLatchContext.isEmpty(), "must be empty");
      }

      if (isMbLocked())
      {
         unlockMB();
      }

      if (isSpaceIdLocked())
      {
         getEnv()->spaceLocker.unlock(_sid, _sidMode);
         _sid = INVALID_SPACE_ID;
         _sidMode = SHARED;
      }

      return;
   }

   IExecutor *requestContext::getExecutor()const
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->getExecutor();
   }

   instanceEnv *requestContext::getEnv()const
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->getEnv();
   }

   DPS_TRANS_ID requestContext::getOrigTransId()const
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->getExecutor()->getTransID().getOrigTransID();
   }

   outerResource *requestContext::getOuterResource()const
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return &(tc->getEnv()->resource);
   }

   CHAR *requestContext::allocateBuffer(UINT32 size)
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      return tc->allocateBuffer(size);
   }
   
   void requestContext::releaseBuffer(void *buffer)
   {
      THREAD_CONTEXT *tc = GET_THREAD_CONTEXT();
      SDB_ASSERT(nullptr != tc, "can not be null");
      tc->releaseBuffer(buffer);
      return;
   }

   INT32 requestContext::lockSpaceID(SPACE_ID sid,
                                     OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "do not relock sid");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = getEnv()->spaceLocker.lock(sid, mode);
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
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(isSpaceIdLocked()))
      {
         SDB_ASSERT(FALSE, "do not relock sid");
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      rc = getEnv()->spaceLocker.tryLock(sid, mode, locked);
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

   BOOLEAN requestContext::isSpaceIdLocked(OSS_LATCH_MODE *mode)const
   {
      BOOLEAN r = INVALID_SPACE_ID != _sid;
      if (r && nullptr != mode)
      {
         *mode = _sidMode;
      }
      return r;
   }

   void requestContext::lockMB(CL_MB_ID mbID,
                               ossRWMutex *mutex,
                               OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(isSpaceIdLocked(), "lock sid first");
      SDB_ASSERT(!isMbLocked(), "do not relock");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(nullptr != mutex, "can not be null");
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
      SDB_ASSERT(isSpaceIdLocked(), "lock sid first");
      SDB_ASSERT(!isMbLocked(), "do not relock");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(nullptr != mutex, "can not be null");

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
      BOOLEAN r = nullptr != _mbMutex;
      if (r && nullptr != mode)
      {
         *mode = _mbMode;
      }
      return r;
   }

   void requestContext::unlockMB()
   {
      if (isMbLocked())
      {
         resetClProperties();
         
         if (SHARED == _mbMode)
         {
            _mbMutex->release_r();
         }
         else
         {
            _mbMutex->release_w();
         }

         _mbID = INVALID_CL_MB_ID;
         _mbMutex = nullptr;
         _mbMode = SHARED;
      }
      return;
   }

   void requestContext::setClProperties(const collectionProperties *properties)
   {
      SDB_ASSERT(nullptr != properties, "can not be invalid");
      SDB_ASSERT(isMbLocked(), "must be locked");
      SDB_ASSERT(properties->clid.getMbId() == _mbID, "must be same");
      _clProperties = properties;
      return;
   }

   UINT32 requestContext::getLogicalClId()const
   {
      return nullptr == _clProperties ?
             DMS_INVALID_LOGICCLID : _clProperties->clid.getLid();
   }

   INT32 requestContext::lockLpid(SPACE_TYPE type,
                                  PAGE_ID lpid,
                                  const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      logicalPidLatchKey key;
      objectLatchHelper<logicalPidLatchKey> lh;

      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                       INVALID_PAGE_ID == lpid ||
                       mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      key = logicalPidLatchKey(_sid, type, lpid);

      rc = lh.lock(getEnv()->latchEnv.lpidLatchMap, _lpidLatchContext, key, mode);
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

      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                       INVALID_PAGE_ID == lpid ||
                       mode.isNone()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_INVALID_OPERATION;
         goto error;
      }

      key = logicalPidLatchKey(_sid, type, lpid);

      rc = lh.tryLock(getEnv()->latchEnv.lpidLatchMap, _lpidLatchContext,
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

      if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
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
      lh.autoUnlock(getEnv()->latchEnv.lpidLatchMap, _lpidLatchContext, key);
   done:
      return;
   }

   BOOLEAN requestContext::testLpidLocked(SPACE_TYPE type,
                                          PAGE_ID lpid,
                                          ossSharedLatchMode *mode)
   {
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
      if (OSS_UNLIKELY(!isSpaceIdLocked()))
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
      if (OSS_UNLIKELY(!isSpaceIdLocked()))
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
      if (OSS_UNLIKELY(!isSpaceIdLocked()))
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
      return nullptr != _oplist && !(_oplist->isReadonly());
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
      return SDB_OK;
   }

   INT32 requestContext::tryToBlockCheckpoint(SPACE_TYPE type,
                                              ossRWMutex *mutex,
                                              BOOLEAN &blocked)
   {
      blocked = TRUE;
      return SDB_OK;
   }

   void requestContext::unblockCheckpoint()
   {
      return;
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
      else if (OSS_UNLIKELY(!isMbLocked()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      key = recordIdLatchKey(_sid, _mbID, rid);
      rc = lh.lock(getEnv()->latchEnv.ridLatchMap, _ridLatchContext, key, mode);
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

      key = recordIdLatchKey(_sid, _mbID, rid);
      rc = lh.tryLock(getEnv()->latchEnv.ridLatchMap,
                      _ridLatchContext,
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
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      objectLatchHelper<recordIdLatchKey> lh;
      recordIdLatchKey key(_sid, _mbID, rid);
      lh.autoUnlock(getEnv()->latchEnv.ridLatchMap, _ridLatchContext, key);
   }

   void requestContext::unlockRids()
   {
      objectLatchHelper<recordIdLatchKey> lh;
      lh.releaseAll(getEnv()->latchEnv.ridLatchMap, _ridLatchContext);
   }

   BOOLEAN requestContext::testRidLocked(const recordID &rid,
                                         ossSharedLatchMode *mode)
   {
      recordIdLatchKey key(_sid, _mbID, rid);
      return _ridLatchContext.test(key, mode);
   }

   void requestContext::waitRid(const recordID &rid,
                                const ossSharedLatchMode &mode)
   {
      SDB_ASSERT(!mode.isNone(), "can not be none");
      recordIdLatchKey key(_sid, _mbID, rid);
      objectLatchHelper<recordIdLatchKey> lh;
#if defined (_DEBUG)
      SDB_ASSERT(!_ridLatchContext.test(key, nullptr), "invalid waiting");
#endif//_DEBUT
      lh.testNotExistsOrWait(getEnv()->latchEnv.ridLatchMap, key, mode);
   }

   INT32 requestContext::acquireTransLock(const recordID &rid,
                                          const DPS_TRANSLOCK_TYPE &mode)
   {
      INT32 rc = SDB_OK;
      ITransLockConsole *console = nullptr;
      dpsTransLockId lockId;
      dmsRecordID dmsRid;

      if (OSS_UNLIKELY(!rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isMbLocked()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      console = getOuterResource()->transLockConsole;
      dmsRid = rid.toDMSRid();
      lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      rc = console->acquire(getExecutor(), lockId, mode, nullptr, nullptr, nullptr);
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
      ITransLockConsole *console = nullptr;
      dpsTransLockId lockId;
      dmsRecordID dmsRid;
      locked = FALSE;

      if (OSS_UNLIKELY(!rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isMbLocked()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      console = getOuterResource()->transLockConsole;
      dmsRid = rid.toDMSRid();
      lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      rc = console->tryAcquire(getExecutor(), lockId, mode, nullptr, nullptr);
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
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      dpsTransLockId lockId;
      dmsRecordID dmsRid = rid.toDMSRid();
      lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      getOuterResource()->transLockConsole->release(getExecutor(), lockId, FALSE, nullptr);
   }

   void requestContext::releaseAllTransLock()
   {
      getOuterResource()->transLockConsole->releaseAll(getExecutor(), nullptr);
   }

   INT32 requestContext::waitTransLock(const recordID &rid,
                                       const DPS_TRANSLOCK_TYPE &mode)
   {
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      dmsRecordID dmsRid = rid.toDMSRid();
      dpsTransLockId lockId = dpsTransLockId(_sid, _mbID, &dmsRid);
      ITransLockConsole *console = getOuterResource()->transLockConsole;
      INT32 rc = console->acquire(getExecutor(), lockId, mode, nullptr, nullptr, nullptr);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock rid:%s, rc:%d", rid.toString().c_str(), rc);
         goto error;
      }
      
      console->release(getExecutor(), lockId, FALSE, nullptr);

   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine