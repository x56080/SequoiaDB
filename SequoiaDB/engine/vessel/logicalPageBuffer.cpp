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

   Source File Name = logicalPageBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/logicalPageBuffer.h"
#include "vessel/requestContext.h"
#include "vessel/logicalPageSpace.h"

namespace engine
{
namespace vessel
{
   logicalPageBuffer::~logicalPageBuffer()
   {
      fini();
   }

   void logicalPageBuffer::fini()
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
      _cowTrigger = copyOnWriteTrigger();
      return;
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
                                          getCowTrigger().getPsv());
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

   void logicalPageBuffer::destroy()
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isValid(), "can not be invalid");
      if (!isValid())
      {
         goto done;
      }

      _rpb.fini();
      rc = _lps->releasePage(_context, _lpid);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to release lpid[%d], rc:%d", _lpid, rc);
      }

      fini();
   done:
      return;
   }
}//namespace vessel
}//namespace engine