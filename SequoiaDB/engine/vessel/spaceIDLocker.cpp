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

   Source File Name = spaceIDLocker.cpp

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

#include "vessel/spaceIDLocker.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   spaceIDLocker::spaceIDLocker():
   _size(0),
   _mutexVec(NULL)
   {}

   spaceIDLocker::~spaceIDLocker()
   {
      fini();
   }

   INT32 spaceIDLocker::init(UINT32 count)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(NULL != _mutexVec))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (OSS_UNLIKELY(0 == count))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _mutexVec = SDB_OSS_NEW ossSpinSLatch[count];
      if (NULL == _mutexVec)
      {
         rc = SDB_OOM;
         goto error;
      }

      _size = count;
   done:
      return rc;
   error:
      SDB_OSS_DEL []_mutexVec;
      _mutexVec = NULL;
      _size = 0;
      goto done;
   }

   INT32 spaceIDLocker::fini()
   {
      if (NULL != _mutexVec)
      {
         SDB_OSS_DEL []_mutexVec;
         _mutexVec = NULL;
         _size = 0;
      }
   done:
      return SDB_OK;
   }

   void spaceIDLocker::lock(SPACE_ID sid, OSS_LATCH_MODE mode)
   {
      SDB_ASSERT(NULL != _mutexVec && 0 < _size, "impossible");
      if (SHARED == mode)
      {
         _mutexVec[sid%_size].get_shared();
      }
      else
      {
         _mutexVec[sid%_size].get();
      }
      return;
   }

    BOOLEAN spaceIDLocker::lock(SPACE_ID sid, OSS_LATCH_MODE mode, INT32 millis)
    {
       return FALSE;
    }

    void spaceIDLocker::unlock(SPACE_ID sid, OSS_LATCH_MODE mode)
    {
       SDB_ASSERT(NULL != _mutexVec && 0 < _size, "impossible");
       if (SHARED == mode)
       {
          _mutexVec[sid%_size].release_shared();
       }
       else
       {
          _mutexVec[sid%_size].release();
       }
       return;
    }

    BOOLEAN spaceIDLocker::tryLock(SPACE_ID sid, OSS_LATCH_MODE mode)
    {
       SDB_ASSERT(NULL != _mutexVec && 0 < _size, "impossible");
       BOOLEAN r = FALSE;
       if (SHARED == mode)
       {
          r = _mutexVec[sid%_size].try_get_shared();
       }
       else
       {
          r = _mutexVec[sid%_size].try_get();
       }
       return r;
    }
}//namespace vessel
}//namespace engine