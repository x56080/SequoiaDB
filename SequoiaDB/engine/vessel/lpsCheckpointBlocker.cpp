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

   Source File Name = lpsCheckpointBlocker.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpsCheckpointBlocker.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   lpsCheckpointBlocker::~lpsCheckpointBlocker()
   {
      fini();
   }

   void lpsCheckpointBlocker::fini()
   {
      if (0 < _count)
      {
         SDB_ASSERT(FALSE, "unblocking missed");
         _mutex->release_r();
      }
      _mutex = NULL;
      _sid = INVALID_SPACE_ID;
      _type = INVALID_SPACE_TYPE;
      _count = 0;
      return;
   }

   INT32 lpsCheckpointBlocker::block(SPACE_ID sid, 
                                     SPACE_TYPE type,
                                     ossRWMutex *mutex)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       INVALID_SPACE_TYPE == type ||
                       NULL == mutex))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isBlocking())
      {
         if (!isSameBlocker(sid, type, mutex))
         {
            PD_LOG(PDERROR, "only one checkpoint can be blocked at one time");
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }

         ++_count;
      }
      else
      {
         mutex->lock_r();
         _mutex = mutex;
         _sid = sid;
         _type = type;
         _count = 1;
      }
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpsCheckpointBlocker::tryToBlock(SPACE_ID sid, 
                                          SPACE_TYPE type,
                                          ossRWMutex *mutex,
                                          BOOLEAN &blocked)
   {
      INT32 rc = SDB_OK;
      blocked = FALSE;
      if (OSS_UNLIKELY(INVALID_SPACE_ID == sid ||
                       INVALID_SPACE_TYPE == type ||
                       NULL == mutex))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isBlocking())
      {
         if (!isSameBlocker(sid, type, mutex))
         {
            PD_LOG(PDERROR, "only one checkpoint can be blocked at one time");
            rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
            goto error;
         }

         ++_count;
         blocked = TRUE;
      }
      else
      {
         blocked = mutex->try_lock_r();
         if (blocked)
         {
            _mutex = mutex;
            _sid = sid;
            _type = type;
            _count = 1;
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lpsCheckpointBlocker::unblock()
   {
      if (isBlocking())
      {
         _mutex->release_r();
         if (0 == --_count)
         {
            fini();
         }
      }
      return;
   }

   BOOLEAN lpsCheckpointBlocker::isSameBlocker(SPACE_ID sid, 
                                               SPACE_TYPE type,
                                               ossRWMutex *mutex)const
   {
      SDB_ASSERT(isBlocking(), "must be blocking");
      return sid == _sid &&
             type == _type &&
             mutex == _mutex;
   }
}//namesapce vessel
}//namesapce engine