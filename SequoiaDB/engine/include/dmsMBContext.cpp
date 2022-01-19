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

   Source File Name = dmsMBContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsMBContext.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
   void _dmsMBContext::_reset(UINT16 mbID,
                              monSpinSLatch *latch)
   {
      
   }

   _dmsMBContext::~_dmsMBContext()
   {
      SDB_ASSERT(!isMBLock(), "unlocking missed");
   }

   BOOLEAN _dmsMBContext::isMBLock()const
   {
      return 0 < _lockRefCnt;
   }

   BOOLEAN _dmsMBContext::isMBLock(OSS_LATCH_MODE mode)const
   {
      return isMBLock() && _mode == mode;
   }

   INT32 _dmsMBContext::mbLock(OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      rc = _lockMB(mode, FALSE);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMBContext::mbTryLock(OSS_LATCH_MODE mode, BOOLEAN &locked)
   {
      INT32 rc = SDB_OK;
      locked = FALSE;

      rc = _lockMB(mode, TRUE);
      if (SDB_TIMEOUT == rc)
      {
         rc = SDB_OK;
      }
      else if (SDB_OK != rc)
      {
         goto error;
      }
      else
      {
         locked = TRUE;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 _dmsMBContext::_lockMB(OSS_LATCH_MODE mode, BOOLEAN tryLock)
   {
      INT32 rc = SDB_OK;
      BOOLEAN locked = FALSE;

      if (isMBLock())
      {
         if (mode <= _mode)
         {
            if (OSS_UNLIKELY(OSS_UINT16_MAX == _lockRefCnt))
            {
               SDB_ASSERT(FALSE, "out of ref count range");
               rc = SDB_OSS_UP_TO_LIMIT;
               goto error;
            }
            ++_lockRefCnt;
            goto done;
         }
         else
         {
            SDB_ASSERT(FALSE, "wrong type locking");
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      rc = _checkBeforeLock(mode);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (tryLock)
      {
         if (SHARED == mode)
         {
            locked = _mbLatch->try_get_shared();
         }
         else
         {
            locked = _mbLatch->try_get();
         }
      }
      else
      {
         ossLatch(_mbLatch, mode);
         locked = TRUE;
      }

      if (locked)
      {
         rc = _checkAfterLock(mode);
         if (SDB_OK != rc)
         {
            goto error;
         }

         _mode = mode;
         ++_lockRefCnt;
      }
      else
      {
         rc = SDB_TIMEOUT;
         goto error;
      }
      
   done:
      return rc;
   error:
      if (locked)
      {
         ossUnlatch(_mbLatch, mode);
      }
      goto done;
   }

   void _dmsMBContext::mbUnlock()
   {
      SDB_ASSERT(isMBLock(), "must be locked");
      if (0 == --_lockRefCnt)
      {
         ossUnlatch(_mbLatch, _mode);
         _mode = SHARED;
      }
   }
} // namespace engine
