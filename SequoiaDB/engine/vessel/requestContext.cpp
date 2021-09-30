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
#include "vessel/objectLatchHelper.hpp"

namespace engine
{
namespace vessel
{
/////////////////////////collectionSpaceContext

   collectionSpaceContext::~collectionSpaceContext()
   {
      SDB_ASSERT(!isOpen(), "closing missed");
   }

   INT32 collectionSpaceContext::lockSid(SPACE_ID sid,
                                         OSS_LATCH_MODE mode,
                                         spaceIDLocker *locker)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(isOpen()))
      {
         SDB_ASSERT(FALSE, "do not relock");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            NULL == locker))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = locker->lock(sid, mode);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", sid, rc);
         goto error;
      }

      _sid = sid;
      _mode = mode;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 collectionSpaceContext::tryLockSid(SPACE_ID sid,
                                            OSS_LATCH_MODE mode,
                                            spaceIDLocker *locker,
                                            BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;

      if (OSS_UNLIKELY(isOpen()))
      {
         SDB_ASSERT(FALSE, "do not relock");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                            NULL == locker))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = locker->tryLock(sid, mode, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock sid[%d], rc:%d", sid, rc);
         goto error;
      }

      if (locked)
      {
         _sid = sid;
         _mode = mode;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void collectionSpaceContext::close(spaceIDLocker *locker)
   {
      SDB_ASSERT(NULL != locker, "can not be null");
      if (isOpen())
      {
         _csName.reset();
         _lid = DMS_INVALID_LOGICCSID;
         locker->unlock(_sid, _mode);
         _sid = INVALID_SPACE_ID;
         _mode = SHARED;
      }
      return;
   }

   OSS_LATCH_MODE collectionSpaceContext::getLockingMode()const
   {
      SDB_ASSERT(isOpen(), "must be open");
      return _mode;
   }

/////////////////////////collectionSpaceContext END


/////////////////////////collectionContext

   collectionContext::~collectionContext()
   {
      close();
   }

   void collectionContext::lockMB(CL_MB_ID mbID,
                                  ossRWMutex *latch,
                                  OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(!isOpen(), "do not relock");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != latch, "can not be null");
      if (SHARED == mode)
      {
         latch->lock_r();
      }
      else
      {
         latch->lock_w();
      }
      _mbID = mbID;
      _mode = mode;
      _mbLatch = latch;
      return;
   }

   BOOLEAN collectionContext::tryLockMB(CL_MB_ID mbID,
                                        ossRWMutex *latch,
                                        OSS_LATCH_MODE mode)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(!isOpen(), "do not relock");
      SDB_ASSERT(INVALID_CL_MB_ID != mbID, "can not be invalid");
      SDB_ASSERT(NULL != latch, "can not be null");

      if (SHARED == mode)
      {
         r = latch->try_lock_r();
      }
      else
      {
         r = latch->try_lock_w();
      }

      if (r)
      {
         _mbID = mbID;
         _mode = mode;
         _mbLatch = latch;
      }
      return r;
   }

   BOOLEAN collectionContext::isMbIDLocked(OSS_LATCH_MODE *mode)const
   {
      BOOLEAN r = isOpen();
      if (r && NULL != mode)
      {
         *mode = _mode;
      }
      return r;
   }

   void collectionContext::close()
   {
      if (isOpen())
      {
         _name.reset();
         _lid = DMS_INVALID_LOGICCLID;
         if (SHARED == _mode)
         {
            _mbLatch->release_r();
         }
         else
         {
            _mbLatch->release_w();
         }
         _mbID = INVALID_CL_MB_ID;
         _mode = SHARED;
         _mbLatch = NULL;
      }
      return;
   }

/////////////////////////collectionContext END

/////////////////////////requestContext

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
      if (!isOpen())
      {
         goto done;
      }

      {
      SDB_ASSERT(0 == _bufAllocated, "memory leak");
      SDB_ASSERT(!_blocker.isBlocking(), "unblocking missed");
      SDB_ASSERT(_lpidLatchContext.isEmpty(), "unlocking missed");
      SDB_ASSERT(NULL == _oplist, "detaching missed");
      ossSharedLatchMode mode;
      LOGICAL_ID_LATCH_MAP::object lpidLatchObj;

      _clContext.close();

      while (_lpidLatchContext.pop(lpidLatchObj, mode))
      {
         lpidLatchObj.getValue().unlockWith(mode);
         _env->lpidLatchMap.release(lpidLatchObj);
      }
      _lpidLatchContext.fini();

      _blocker.fini();
      
      _spaceContext.close(&_env->spaceLocker);

      _session = NULL;
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
      spaceIDLocker *locker = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      locker = &(getEnv()->spaceLocker);
      rc = _spaceContext.lockSid(sid, mode, locker);
      if (SDB_OK != rc)
      {
         goto error;
      }
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

      locker = &(getEnv()->spaceLocker);
      rc = _spaceContext.tryLockSid(sid, mode, locker, locked);
      if (SDB_OK != rc)
      {
         goto error;
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
      SDB_ASSERT(!_clContext.isMbIDLocked(), "mb unlocking missed");
      if (_spaceContext.isOpen())
      {
         _spaceContext.close(&_env->spaceLocker);
      }
   }

   BOOLEAN requestContext::isSpaceIdLocked(OSS_LATCH_MODE *mode)const
   {
      BOOLEAN r = _spaceContext.isOpen();
      if (r && NULL != mode)
      {
         *mode = _spaceContext.getLockingMode();
      }
      return r;
   }

   void requestContext::initSpaceContextUnderLock(UINT32 lcsid,
                                                 const strSlice &csName)
   {
      SDB_ASSERT(_spaceContext.isOpen(), "must be open");
      SDB_ASSERT(DMS_INVALID_LOGICCSID != lcsid, "can not be invalid");
      SDB_ASSERT(!csName.empty(), "can not be empty");
      _spaceContext.setCSName(csName);
      _spaceContext.setLogicalCSID(lcsid);
      return;
   }

   void requestContext::lockMB(CL_MB_ID mbID,
                                ossRWMutex *latch,
                                OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(_spaceContext.isOpen(), "must be open");
      _clContext.lockMB(mbID, latch, mode);
   }

   BOOLEAN requestContext::tryLockMB(CL_MB_ID mbID,
                                     ossRWMutex *latch,
                                     OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(_spaceContext.isOpen(), "must be open");
      return _clContext.tryLockMB(mbID, latch, mode);
   }

   void requestContext::initCollectionContextUnderLock(UINT32 logicalCLID,
                                                       const strSlice &clName)
   {
      SDB_ASSERT(_clContext.isOpen(), "must be open");
      SDB_ASSERT(DMS_INVALID_LOGICCLID != logicalCLID, "can not be invalid");
      SDB_ASSERT(!clName.empty(), "can not be empty");
      _clContext.setCLName(clName);
      _clContext.setLogicalCLID(logicalCLID);
      return;
   }

   BOOLEAN requestContext::isMbLocked(OSS_LATCH_MODE *mode)const
   {
      return _clContext.isMbIDLocked(mode);
   }

   void requestContext::unlockMB()
   {
      _clContext.close();
      return;
   }

   collectionHandle requestContext::getCollectionHandle()const
   {
      return collectionHandle(_spaceContext.getLogicalCSID(),
                              _clContext.getLogicalCLID(),
                              _spaceContext.getSpaceID(),
                              _clContext.getMBID());
   }


   INT32 requestContext::lockLpid(SPACE_TYPE type,
                                  PAGE_ID lpid,
                                  const ossSharedLatchMode &mode)
   {
      INT32 rc = SDB_OK;
      logicalIdLatchKey key;
      objectLatchHelper<logicalIdLatchKey> lh;

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
         SDB_ASSERT(_spaceContext.isOpen(), "lock space first");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      key = logicalIdLatchKey(_spaceContext.getSpaceID(), type, lpid);

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
      logicalIdLatchKey key;
      objectLatchHelper<logicalIdLatchKey> lh;
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
         SDB_ASSERT(_spaceContext.isOpen(), "lock space first");
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }

      key = logicalIdLatchKey(_spaceContext.getSpaceID(), type, lpid);

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
      logicalIdLatchKey key;
      objectLatchHelper<logicalIdLatchKey> lh;

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

      key = logicalIdLatchKey(_spaceContext.getSpaceID(), type, lpid);
      lh.autoUnlock(_env->lpidLatchMap, _lpidLatchContext, key);
   done:
      return;
   }

   BOOLEAN requestContext::testLpidLocked(SPACE_TYPE type,
                                          PAGE_ID lpid,
                                          ossSharedLatchMode *mode)
   {
      SDB_ASSERT(isOpen(), "must be open");
      SDB_ASSERT(_spaceContext.isOpen(), "must be open");
      logicalIdLatchKey key(_spaceContext.getSpaceID(), type, lpid);
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
      if (OSS_UNLIKELY(!_spaceContext.isOpen()))
      {
         SDB_ASSERT(FALSE, "lock space first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _blocker.block(_spaceContext.getSpaceID(), type, mutex);
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
      if (OSS_UNLIKELY(!_spaceContext.isOpen()))
      {
         SDB_ASSERT(FALSE, "lock space first");
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _blocker.tryToBlock(_spaceContext.getSpaceID(), type, mutex, blocked);
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