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
      SDB_ASSERT(ossSharedLatch::NONE == _mbLockMode, "unlocking missed");
      SDB_ASSERT(ossSharedLatch::NONE == _sidLockedMode, "unlocking missed");
      SDB_ASSERT(0 == _bufAllocated, "memory leak");
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(NULL == _oplist, "detaching missed");

      if (ossSharedLatch::NONE != _mbLockMode)
      {
         _mbLatch->unlockWith(_mbLockMode);
         _mbLockMode = ossSharedLatch::NONE;
         _mbLatch = NULL;
      }

      _lpidContext.reset();
      _blocker.fini();
      
      if (ossSharedLatch::NONE != _sidLockedMode)
      {
         _env->spaceLocker.unlock(_sid, _sidLockedMode);
         _sidLockedMode = ossSharedLatch::NONE;
      }

      _session = NULL;
      _env = NULL;
      _outerResource = NULL;
      _sid = INVALID_SPACE_ID;
      _mbID = INVALID_CL_MB_ID;
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
                                     ossSharedLatch::mode mode)
   {
      INT32 rc = SDB_OK;
      spaceIDLocker *locker = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            ossSharedLatch::NONE == mode))
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

   void requestContext::unlockSpaceID()
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(ossSharedLatch::NONE == _mbLockMode, "unlocking missed");
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(INVALID_SPACE_ID != _sid, "can not be invalid");
      SDB_ASSERT(ossSharedLatch::NONE != _sidLockedMode, "can not be invalid");
      
      _env->spaceLocker.unlock(_sid, _sidLockedMode);
      _sid = INVALID_SPACE_ID;
      _sidLockedMode = ossSharedLatch::NONE;
      return;
   }

   INT32 requestContext::lockMB(CL_MB_ID mbID,
                                ossSharedLatch *latch,
                                ossSharedLatch::mode mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID ||
                       NULL == latch ||
                       ossSharedLatch::NONE == mode))
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
      
      latch->lockWith(mode);
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
      if (OSS_LIKELY(isMbLocked()))
      {
         _mbLatch->unlockWith(_mbLockMode);
         _mbID = INVALID_CL_MB_ID;
         _mbLockMode = ossSharedLatch::NONE;
         _mbLatch = NULL;
      }
      else
      {
         SDB_ASSERT(FALSE, "not locking");
      }
      return;
   }

   INT32 requestContext::tryLockMB(CL_MB_ID mbID,
                                   ossSharedLatch *latch,
                                   ossSharedLatch::mode mode,
                                   BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID ||
                            NULL == latch ||
                            ossSharedLatch::NONE == mode))
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

      locked = latch->tryLockWith(mode);
      if (locked)
      {
         _mbID = mbID;
         _mbLockMode = mode;
         _mbLatch = latch;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::lockLpid(SPACE_TYPE type,
                                  PAGE_ID lpid,
                                  ossSharedLatch::mode mode)
   {
      INT32 rc = SDB_OK;
      logicalIdLatchKey key(_sid, type, lpid);
      LOGICAL_ID_LATCH_MAP::object obj;
      ossSharedLatch::mode m = ossSharedLatch::NONE;

      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_TYPE == type ||
                            INVALID_PAGE_ID == lpid ||
                            ossSharedLatch::NONE == mode))
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
      ossSharedLatch::mode mode = ossSharedLatch::NONE;

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
                                          ossSharedLatch::mode *mode)
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

   BOOLEAN requestContext::isInProcessingOplistAttached()const
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
   
}//namespace vessel
}//namespace engine