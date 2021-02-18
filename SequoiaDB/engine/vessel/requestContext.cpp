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

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains functions for agent processing.

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

namespace engine
{
namespace vessel
{
   INT32 requestContext::open(ISession *session,
                              instanceEnv *env,
                              outerResource *outer)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error; 
      }
      else if (OSS_UNLIKELY(NULL == session ||
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

   INT32 requestContext::close()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_spaceIDLocked)
      {
         _env->spaceLocker.unlock(_spaceID, _spaceIDLockMode);
      }

      reset();
   done:
      return rc;
   error:
      goto done;
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
         buf = (CHAR*)SDB_OSS_MALLOC(size);
      }
      
   done:
      return buf;
   }
   
   void requestContext::releaseBuffer(CHAR *buffer, UINT32 size)
   {
      if (buffer < _staticBuf || (_staticBuf + CONTEXT_DEFAULT_BUFFER_POOL_SIZE) <= buffer)
      {
         SDB_OSS_FREE(buffer);
      }
      else if (buffer + size == _staticBuf + _bufAllocated)
      {
         _bufAllocated -= size;
      }
      return;
   }

   INT32 requestContext::lockSpaceID(SPACE_ID sid, OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      spaceIDLocker *locker = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      locker = &(getEnv()->spaceLocker);
      locker->lock(sid, mode);
      _spaceID = sid;
      _spaceIDLockMode = mode;
      _spaceIDLocked = TRUE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::unlockSpaceID()
   {
      INT32 rc = SDB_OK;
      spaceIDLocker *locker = NULL;
      if (OSS_UNLIKELY(!isOpen()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(!_spaceIDLocked))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      locker = &(_env->spaceLocker);
      SDB_ASSERT(INVALID_SPACE_ID != _spaceID, "impossible");
      locker->unlock(_spaceID, _spaceIDLockMode);
      _spaceID = INVALID_SPACE_ID;
      _spaceIDLockMode = SHARED;
      _spaceIDLocked = FALSE;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::attachMB(CL_MB_ID mbID, ossSpinSLatch *clLatch)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_CL_MB_ID == mbID || NULL == clLatch))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (INVALID_CL_MB_ID != _mbID || NULL != _clLatch)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _mbID = mbID;
      _clLatch = clLatch;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::detachMB()
   {
      INT32 rc = SDB_OK;
      if (INVALID_CL_MB_ID == _mbID || NULL == _clLatch)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (mbLocked())
      {
         unlockMB();
      }

      _mbID = INVALID_CL_MB_ID;
      _clLatch = NULL;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::lockMB(OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (INVALID_CL_MB_ID == _mbID || NULL == _clLatch)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (SHARED == mode)
      {
         _clLatch->get_shared();
      }
      else
      {
         _clLatch->get();
      }

      _mbIDLocked = TRUE;
      _mbIDLockMode = mode;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::unlockMB()
   {
      INT32 rc = SDB_OK;
      if (INVALID_CL_MB_ID == _mbID || NULL == _clLatch)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!mbLocked())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (SHARED == _mbIDLockMode)
      {
         _clLatch->release_shared();
      }
      else
      {
         _clLatch->release();
      }

      _mbIDLocked = FALSE;
      _mbIDLockMode = SHARED;
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 requestContext::lockLpid(SPACE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;

      if (OSS_UNLIKELY(!getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _lpidContext.lock(type, lpid, mode);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 requestContext::unlockLpid(SPACE_TYPE type, PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!getSpaceIDLocked()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _lpidContext.unlock(type, lpid);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN requestContext::testLpidLockMode(SPACE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode)const
   {
      return _lpidContext.testLockMode(type, lpid, mode);
   }
   
}//namespace vessel
}//namespace engine