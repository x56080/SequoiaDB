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

   Source File Name = logicalPageBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/logicalPageSpace.h"

namespace engine
{
namespace vessel
{
   logicalPageBuffer::~logicalPageBuffer()
   {
      _fini();
   }

   logicalPageBuffer::logicalPageBuffer(logicalPageBuffer &&o):
   _lpid(o._lpid),
   _mode(o._mode),
   _context(o._context),
   _lps(o._lps),
   _rpb(std::move(o._rpb)),
   _psv(o._psv)
   {
      o._lpid = INVALID_PAGE_ID;/// reset lpid first to avoid unlocking.
      o._fini();
   }

   logicalPageBuffer &logicalPageBuffer::operator=(logicalPageBuffer &&o)
   {
      _fini();
      if (o.isValid())
      {
         _lpid = o._lpid;
         _mode = o._mode;
         _context = o._context;
         _lps = o._lps;
         _rpb = std::move(o._rpb);
         _psv = o._psv;

         o._lpid = INVALID_PAGE_ID;
         o._fini();
      }

      return *this;
   }  

   void logicalPageBuffer::fini()
   {
      _fini();
      return;
   }

   void logicalPageBuffer::_fini()
   {
      _rpb.fini();
      if (NULL != _lps && INVALID_PAGE_ID != _lpid && !_mode.isNone())
      {
         _context->unlockLpid(_lps->getSpaceType(), _lpid);
      }
      _lpid = INVALID_PAGE_ID;
      _mode.setNone();
      _context = NULL;
      _lps = NULL;
      _psv = INVALID_PAGE_SNAPSHOT_VERSION;
   }

   INT32 logicalPageBuffer::prepareToWrite()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }

      rc = _lps->makeBufferWritable(*this);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void logicalPageBuffer::commit(DPS_LSN_OFFSET lsn)
   {
      if (OSS_LIKELY(isValid()))
      {
         _rpb.commit(lsn);
      }
   }

   INT32 logicalPageBuffer::validatePage(PAGE_TYPE type)const
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(INVALID_PAGE_TYPE != type, "can not be invalid");

      rc = ::engine::vessel::validatePage((ossValuePtr)_rpb.getPageHead(),
                                          type, _rpb.getPageSize(),
                                          _rpb.getGlobalPid().page(),
                                          getLogicalPid(),
                                          _psv);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN logicalPageBuffer::tryLockExclusiveFromShared()
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(_mode.isShared(), "must be upgrade");
      BOOLEAN locked = FALSE;
      INT32 rc = _context->tryLockFromSharedToExclusive(_lps->getSpaceType(), _lpid, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d] from shared to exclusive:%d", rc);
         ossPanic();
      }
      if (locked)
      {
         _mode.setExclusive();
      }

      return locked;
   }

   BOOLEAN logicalPageBuffer::tryLockExclusiveFromUpgrade()
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(_mode.isUpgrade(), "must be upgrade");
      BOOLEAN locked = FALSE;
      INT32 rc = _context->tryLockFromUpgradeToExclusive(_lps->getSpaceType(), _lpid, locked);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d] from upgrade to exclusive:%d", rc);
         ossPanic();
      }

      if (locked)
      {
         _mode.setExclusive();
      }
      return locked;
   }

   void logicalPageBuffer::lockExclusiveFromUpgrade()
   {
      SDB_ASSERT(isValid(), "must be valid");
      SDB_ASSERT(_mode.isUpgrade(), "must be upgrade");
      INT32 rc = _context->lockFromUpgradeToExclusive(_lps->getSpaceType(), _lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to lock lpid[%d] from upgrade to exclusive:%d", rc);
         ossPanic();
      }
      _mode.setExclusive();
      return;
   }

   strictBuffer logicalPageBuffer::getReadableBodyBuffer()const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return _rpb.getReadableBodyBuffer();
   }

   strictBuffer logicalPageBuffer::getWritableBodyBuffer()
   {
      SDB_ASSERT(isWritable(), "can not be invalid");
      return _rpb.getWritableBodyBuffer();
   }

   BOOLEAN logicalPageBuffer::isWritable()const
   {
      return isValid() && _rpb.isWritingPrepared();
   }

   INT32 logicalPageBuffer::autoGetWritableBodyBuffer(strictBuffer &buffer)
   {
      INT32 rc = SDB_OK;
      buffer.reset();
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isWritable())
      {
         rc = prepareToWrite();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to auto prepare to write:%d", rc);
            goto error;
         }
      }

      buffer = getWritableBodyBuffer();
   done:
      return rc;
   error:
      goto done;
   }

   // void logicalPageBuffer::destroy()
   // {
   //    INT32 rc = SDB_OK;
   //    PAGE_ID lpid = INVALID_PAGE_ID;
   //    logicalPageSpace *lps = nullptr;
   //    requestContext *context = nullptr;

   //    if (!isValid())
   //    {
   //       SDB_ASSERT(FALSE, "invalid buffer");
   //       goto done;
   //    }
   //    else if (!_mode.isExclusive())
   //    {
   //       SDB_ASSERT(FALSE, "invalid locking mode");
   //       goto done;
   //    }

   //    lpid = _lpid;
   //    lps = _lps;
   //    context = _context;

   //    /// do not unlock when fini
   //    _mode.setNone();

   //    fini();
   //    rc = lps->releasePage(context, lpid);
   //    if (SDB_OK != rc)
   //    {
   //       PD_LOG(PDERROR, "failed to release lpid[%d], rc:%d", lpid, rc);
   //       SDB_ASSERT(FALSE, "failed to release lpid");
   //    }

   //    context->unlockLpid(lps->getSpaceType(), lpid);
   // done:
   //    return;
   // }
}//namespace vessel
}//namespace engine