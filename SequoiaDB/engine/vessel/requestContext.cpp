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

namespace engine
{
namespace vessel
{
/////////////////////////requestContext

   void requestContext::open(IExecutor *executor,
                              instanceEnv *env,
                              outerResource *outer)
   {
      SDB_ASSERT(NULL != executor, "can not be null");
      SDB_ASSERT(NULL != env, "can not be null");
      SDB_ASSERT(NULL != outer, "can not be null");
      SDB_ASSERT(NULL == _executor, "do not reinit");
      _close();

      _executor = executor;
      _env = env;
      _outerResource = outer;
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
      SDB_ASSERT(0 == _bufAllocated, "memory leak");
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(_ridLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(NULL == _oplist, "detaching missed");

      _unlockAndClear(_UNLOCK_LVL_SPACE);

      _executor = NULL;
      _env = NULL;
      _outerResource = NULL;
      
      _bufAllocated = 0;
      _oplist = NULL;
      }
      
   done:
     return;
   }

   CHAR *requestContext::allocateBuffer(UINT32 size)
   {
      CHAR *buf = NULL;
      if (size <= CONTEXT_DEFAULT_BUFFER_POOL_SIZE - _bufAllocated)
      {
         buf = _staticBuf + _bufAllocated;
         _bufAllocated += size;
      }
      else
      {
         buf = (CHAR*)SDB_THREAD_ALLOC(size);
      }
      
   done:
      return buf;
   }
   
   void requestContext::releaseBuffer(CHAR *buffer, UINT32 size)
   {
      if (OSS_LIKELY(NULL != buffer))
      {
         if (buffer < _staticBuf || (_staticBuf + CONTEXT_DEFAULT_BUFFER_POOL_SIZE) <= buffer)
         {
            SDB_THREAD_FREE(buffer);
         }
         else if (buffer + size == _staticBuf + _bufAllocated)
         {
            _bufAllocated -= size;
         }
      }
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
      else if (OSS_UNLIKELY(INVALID_SPACE_ID != _spaceContext.sid))
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

      _spaceContext.sid = sid;
      _spaceContext.mode = mode;
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
      else if (OSS_UNLIKELY(INVALID_SPACE_ID != _spaceContext.sid))
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
         _spaceContext.sid = sid;
         _spaceContext.mode = mode;
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
      SDB_ASSERT(_ridLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(!_clContext.isLocking(), "unlocking missed");
      SDB_ASSERT(NULL == _clContext.mutex, "mb unlocking missed");
      _unlockAndClear(_UNLOCK_LVL_SPACE);
      return;
   }

   BOOLEAN requestContext::isSpaceIdLocked(OSS_LATCH_MODE *mode)const
   {
      BOOLEAN r = _spaceContext.isLocking();
      if (r && NULL != mode)
      {
         *mode = _spaceContext.mode;
      }
      return r;
   }

   void requestContext::cacheSpaceInfo(UINT32 logicalId,
                                       utilCSUniqueID uniqueId,
                                       const CHAR *name)
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(_spaceContext.isLocking(), "must be locked");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != logicalId, "can not be invalid");
      SDB_ASSERT(NULL != name, "can not be null");
      _spaceContext.lid =logicalId;
      _spaceContext.uniqueId = uniqueId;
      _spaceContext.name.reset(name);
      return;
   }

   void requestContext::lockMB(CL_MB_ID mbID,
                                ossRWMutex *mutex,
                                OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_spaceContext.isLocking(), "lock sid first");
      SDB_ASSERT(!_clContext.isLocking(), "do not relock");
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
      _clContext.mbID = mbID;
      _clContext.mutex = mutex;
      _clContext.mode = mode;
      return;
   }

   BOOLEAN requestContext::tryLockMB(CL_MB_ID mbID,
                                     ossRWMutex *mutex,
                                     OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_spaceContext.isLocking(), "lock sid first");
      SDB_ASSERT(!_clContext.isLocking(), "do not relock");
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
         _clContext.mbID = mbID;
         _clContext.mutex = mutex;
         _clContext.mode = mode;
      }
      return r;
   }

   void requestContext::cacheMbInfo(UINT32 logicalId,
                                    utilCLInnerID innerId,
                                    const CHAR *name)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_spaceContext.isLocking(), "must lock sid first");
      SDB_ASSERT(_clContext.isLocking(), "must lock mb first");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalId, "can not be invalid");
      SDB_ASSERT(NULL != name, "can not be null");
      _clContext.lid = logicalId;
      _clContext.innerId = innerId;
      _clContext.name.reset(name);
      return;
   }

   BOOLEAN requestContext::isMbLocked(OSS_LATCH_MODE *mode)const
   {
      return _clContext.isLocking();
   }

   void requestContext::unlockMB()
   {
      SDB_ASSERT(_ridLatchContext.isEmpty(), "unlocking missed");
      _unlockAndClear(_UNLOCK_LVL_MB);
      return;
   }

   globalCollectionId requestContext::getGlobalCollectionId()const
   {
      globalCollectionId gcid;
      utilCLUniqueID uid = utilBuildCLUniqueID(_spaceContext.uniqueId,
                                               _clContext.innerId);
      gcid.reset(_spaceContext.lid, _clContext.lid,
                 uid, _spaceContext.sid, _clContext.mbID);
      return gcid;
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

      key = logicalPidLatchKey(_spaceContext.sid, type, lpid);

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

      key = logicalPidLatchKey(_spaceContext.sid, type, lpid);

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

      key = logicalPidLatchKey(_spaceContext.sid, type, lpid);
      lh.autoUnlock(_env->lpidLatchMap, _lpidLatchContext, key);
   done:
      return;
   }

   BOOLEAN requestContext::testLpidLocked(SPACE_TYPE type,
                                          PAGE_ID lpid,
                                          ossSharedLatchMode *mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_spaceContext.isLocking(), "must be locking");
      logicalPidLatchKey key(_spaceContext.sid, type, lpid);
      return _lpidLatchContext.test(key, mode);
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
      if (OSS_UNLIKELY(!_spaceContext.isLocking()))
      {
         SDB_ASSERT(FALSE, "lock space first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _blocker.block(_spaceContext.sid, type, mutex);
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
      if (OSS_UNLIKELY(!_spaceContext.isLocking()))
      {
         SDB_ASSERT(FALSE, "lock space first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _blocker.tryToBlock(_spaceContext.sid, type, mutex, blocked);
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

      if (OSS_UNLIKELY(!rid.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isSpaceIdLocked() ||
               !isMbLocked())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      key = recordIdLatchKey(getLogicalCSID(),
                             getLogicalCLID(),
                             rid);
      rc = lh.lock(_env->ridLatchMap, _ridLatchContext, key, mode);
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
      else if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isSpaceIdLocked() ||
               !isMbLocked())
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      key = recordIdLatchKey(getLogicalCSID(),
                             getLogicalCLID(),
                             rid);
      rc = lh.tryLock(_env->ridLatchMap, _ridLatchContext,
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
      SDB_ASSERT(isSpaceIdLocked() && isMbLocked(), "must be locked");
      SDB_ASSERT(rid.isValid(), "can not be invalid");
      objectLatchHelper<recordIdLatchKey> lh;
      recordIdLatchKey key(getLogicalCSID(),
                           getLogicalCLID(),
                           rid);
      lh.autoUnlock(_env->ridLatchMap, _ridLatchContext, key);
   }

   void requestContext::unlockRids()
   {
      SDB_ASSERT(isOpen(), "can not be closed");
      SDB_ASSERT(isSpaceIdLocked() && isMbLocked(), "must be locked");
      _unlockAndClear(_UNLOCK_LVL_RID);
   }

   void requestContext::_unlockAndClear(_UNLOCK_LVL lvl)
   {
      SDB_ASSERT(isOpen(), "must be open");
      if (_UNLOCK_LVL_RID <= lvl && !_ridLatchContext.isEmpty())
      {
         objectLatchHelper<recordIdLatchKey>().releaseAll(_env->ridLatchMap,
                                                          _ridLatchContext);
         SDB_ASSERT(_ridLatchContext.isEmpty(), "must be empty");
      }

      if (_UNLOCK_LVL_MB <= lvl && _clContext.isLocking())
      {
         if (SHARED == _clContext.mode)
         {
            _clContext.mutex->release_r();
         }
         else
         {
            _clContext.mutex->release_w();
         }
         _clContext.reset();
      }

      if (_UNLOCK_LVL_SPACE <= lvl)
      {
         if (!_lpidLatchContext.isEmpty())
         {
            objectLatchHelper<logicalPidLatchKey>().releaseAll(_env->lpidLatchMap,
                                                               _lpidLatchContext);
            SDB_ASSERT(_lpidLatchContext.isEmpty(),"must be empty");
         }
         while (_blocker.isBlocking())
         {
            _blocker.unblock();
         }
         _blocker.fini();

         if (_spaceContext.isLocking())
         {
            _env->spaceLocker.unlock(_spaceContext.sid,
                                      _spaceContext.mode);
            _spaceContext.reset();
         }
      }
   }
}//namespace vessel
}//namespace engine