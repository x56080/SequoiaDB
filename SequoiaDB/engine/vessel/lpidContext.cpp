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

   Source File Name = lpidContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpidContext.h"
#include "vessel/requestContext.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lpidContext::~lpidContext()
   {
      if (NULL != _slots && _slots != _statcBuf)
      {
         SDB_OSS_DEL []_slots;
      }
   }

   INT32 lpidContext::lock(FILE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(FILE_TYPE_DD != type &&
                       FILE_TYPE_IDX_D != type))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_size == _capacity)
      {
         rc = extendBuf(_capacity + LPID_CONTEXT_STATIC_BUF_COUNT);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      ///TODO: lock
      _slots[_size].type = type;
      _slots[_size].lpid = lpid;
      _slots[_size].mode = mode;
      ++_size;
   done:
      return rc;
   error:
      goto done;
   }

   INT32 lpidContext::unlock(FILE_TYPE type, PAGE_ID lpid)
   {
      INT32 rc = SDB_OK;
      UINT32 moveCount = 0;
      _lpidLockSlot *slot = NULL;
      INT32 i = ((INT32)_size - 1);
      for (; i >= 0; --i)
      {
         if (type != _slots[i].type || lpid != _slots[i].lpid)
         {
            ++moveCount;
            continue;
         }
         slot = &(_slots[i]);
         break;
      }

      if (NULL == slot)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      ///TODO: unlock.

      for (UINT32 j = 0; j < moveCount; ++j)
      {
         _slots[i] = _slots[i+1];
         ++i;
      }
      --_size;
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lpidContext::testLockMode(FILE_TYPE type, PAGE_ID lpid, OSS_LATCH_MODE mode)const
   {
      BOOLEAN r = FALSE;
      for (INT32 i = ((INT32)_size - 1); i >= 0; --i)
      {
         const _lpidLockSlot &slot = _slots[i];
         if (type != slot.type || lpid != slot.lpid)
         {
            continue;
         }
         else if (mode != slot.mode)
         {
            goto done;
         }
         else
         {
            r = TRUE;
            goto done;
         }
      }
      
   done:
      return r;
   }

   BOOLEAN lpidContext::testLocked(FILE_TYPE type, PAGE_ID lpid)const
   {
      BOOLEAN r = FALSE;
      for (INT32 i = ((INT32)_size - 1); i >= 0; --i)
      {
         const _lpidLockSlot &slot = _slots[i];
         if (type != slot.type || lpid != slot.lpid)
         {
            continue;
         }
         r = TRUE;
         goto done;
      }
      
   done:
      return r;
   }

   INT32 lpidContext::extendBuf(UINT32 capacity)
   {
      INT32 rc = SDB_OK;
      _lpidLockSlot *tmp = NULL;
      if (capacity <= _capacity)
      {
         goto done;
      }

      tmp = SDB_OSS_NEW _lpidLockSlot[capacity];
      if (NULL == tmp)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to allocate mem");
         goto error;
      }

      for (UINT32 i = 0; i < _size; ++i)
      {
         tmp[i] = _slots[i];
      }

      if (_slots != _statcBuf)
      {
         SDB_OSS_DEL []_slots;
      }

      _slots = tmp;
      _capacity = capacity;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namespace engine