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
#include "vessel/ISession.h"
#include "vessel/spaceIDLocker.h"
#include "vessel/instanceEnv.h"
#include "vessel/atomicOperationList.h"

namespace engine
{
namespace vessel
{
   INT32 requestContext::open(ISession *session,
                              instanceEnv *env,
                              outerResource *outer)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(!isOpen(), "do not reinit");
      
      if (OSS_UNLIKELY(NULL == session ||
                            NULL == env ||
                            NULL == outer))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _session = session;
      _env = env;
      _outerResource = outer;
   done:
      return rc;
   error:
      goto done;
   }

   requestContext::~requestContext()
   {
      _close();
   }

   void requestContext::_close()
   {
      SDB_ASSERT(OSS_SHARED_LATCH_MODE_NONE == _mbLockMode, "unlocking missed");
      SDB_ASSERT(OSS_SHARED_LATCH_MODE_NONE == _sidLockedMode, "unlocking missed");
      SDB_ASSERT(0 == _bufAllocated, "memory leak");
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(NULL == _oplist, "detaching missed");
      OSS_SHARED_LATCH_MODE mode = OSS_SHARED_LATCH_MODE_NONE;
      LOGICAL_ID_LATCH_MAP::object lpidLatchObj;

      unlockMB();

      while (_lpidLatchContext.pop(lpidLatchObj, mode))
      {
         lpidLatchObj.getValue().unlockWith(mode);
         _env->_lpidLatchMap.release(lpidLatchObj);
      }

      _blocker.fini();
      
      unlockSpaceID();

      _session = NULL;
      _env = NULL;
      _outerResource = NULL;
      _lpidLatchContext.fini();
      _bufAllocated = 0;
      _oplist = NULL;
      
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
      spaceIDLocker *locker = NULL;
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

      locker = &(getEnv()->spaceLocker);
      rc = locker->lock(sid, mode);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _sid = sid;
      _sidLockedMode = mode;
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
      spaceIDLocker *locker = NULL;
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

      locker = &(getEnv()->spaceLocker);
      rc = locker->lock(sid, mode);
      rc = locker->tryLock(sid, mode, locked);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (locked)
      {
         _sid = sid;
         _sidLockedMode = mode;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void requestContext::unlockSpaceID()
   {  
      if (isSpaceIdLocked())
      {
         _env->spaceLocker.unlock(_sid, _sidLockedMode);
         _sid = INVALID_SPACE_ID;
         _sidLockedMode = SHARED;
      }
      return;
   }

   INT32 requestContext::lockMB(CL_MB_ID mbID,
                                ossRWMutex *latch,
                                OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID ||
                       NULL == latch))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (isMbLocked())
      {
         SDB_ASSERT(FALSE, "unlocking missed");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      
      if (SHARED == mode)
      {
         latch->lock_r();
      }
      else
      {
         latch->lock_w();
      }
      _mbID = mbID;
      _mbLatch = latch;
      _mbLockMode = mode;

   done:
      return rc;
   error:
      goto done;
   }

   void requestContext::unlockMB()
   {
      if (isMbLocked())
      {
         if (SHARED == _mbLockMode)
         {
            _mbLatch->release_r();
         }
         else
         {
            _mbLatch->release_w();
         }

         _mbID = INVALID_CL_MB_ID;
         _mbLockMode = SHARED;
         _mbLatch = NULL;
      }
      return;
   }


   INT32 requestContext::lockLpid(SPACE_TYPE type,
                                  PAGE_ID lpid,
                                  OSS_SHARED_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      logicalIdLatchKey key(_sid, type, lpid);
      LOGICAL_ID_LATCH_MAP::object obj;
      OSS_SHARED_LATCH_MODE m = OSS_SHARED_LATCH_MODE_NONE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid ||
                            OSS_SHARED_LATCH_MODE_NONE == mode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      if (_lpidLatchContext.test(key, NULL))
      {
         PD_LOG(PDERROR, "[%d,%d,%d] already locked:%d", 
                key._sid, key._type, key._lpid, m);
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _env->_lpidLatchMap.ensure(key, obj);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to ensure latch obj[%d,%d,%d], rc:%d",
                key._sid, key._type, key._lpid, rc);
         goto error;
      }

      rc = _lpidLatchContext.push(obj, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to push latch obj[%d,%d,%d], rc:%d",
                key._sid, key._type, key._lpid, rc);
         goto error;
      }

      obj.getValue().lockWith(mode);
   done:
      return rc;
   error:
      if (obj.isValid())
      {
         _env->_lpidLatchMap.release(obj);
      }
      goto done;
   }
   
   void requestContext::unlockLpid(SPACE_TYPE type, PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      logicalIdLatchKey key(_sid, type, lpid);
      LOGICAL_ID_LATCH_MAP::object obj;
      OSS_SHARED_LATCH_MODE mode = OSS_SHARED_LATCH_MODE_NONE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         SDB_ASSERT(FALSE, "not open");
         goto done;
      }
      else if (!key.isValid())
      {
         SDB_ASSERT(FALSE, "invalid key");
         goto done;
      }

      rc = _lpidLatchContext.pop(key, obj, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to pop[%d,%d,%d] in context, rc:%d",
                key._sid, key._type, key._lpid, rc);
         goto done;
      }

      obj.getValue().unlockWith(mode);
      _env->_lpidLatchMap.release(obj);

   done:
      return;
   }

   BOOLEAN requestContext::testLpidLocked(SPACE_TYPE type,
                                          PAGE_ID lpid,
                                          OSS_SHARED_LATCH_MODE *mode)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(isOpen(), "must be open");
      logicalIdLatchKey key(_sid, type, lpid);
      return _lpidLatchContext.test(key, mode);
   }

   INT32 requestContext::unlockUpgradeLpidAndLock(SPACE_TYPE type,
                                                  PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      logicalIdLatchKey key(_sid, type, lpid);
      LOGICAL_ID_LATCH_MAP::object obj;

      if (OSS_UNLIKELY(!isOpen()))
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
      else if (OSS_UNLIKELY(!isSpaceIdLocked()))
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      rc = _lpidLatchContext.findUpgradeAndSetExclusive(key, obj);
      if (SDB_OK != rc)
      {
         goto error;
      }

      obj.getValue().unlockUpgradeAndLock();
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
      SDB_ASSERT(NULL != oldOplist, "can not be null");
      *oldOplist = _oplist;
      _oplist = newOplist;
      return;
   }

   INT32 requestContext::blockCheckpoint(SPACE_TYPE type,
                                         ossRWMutex *mutex)
   {
      INT32 rc = SDB_OK;
      if (INVALID_SPACE_ID == _sid)
      {
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
      if (INVALID_SPACE_ID == _sid)
      {
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
   
}//namespace vessel
}//namespace engine