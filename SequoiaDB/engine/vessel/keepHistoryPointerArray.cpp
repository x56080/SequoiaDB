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

   Source File Name = keepHistoryPointerArray.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/keepHistoryPointerArray.h"
#include "utilMemListPool.hpp"

namespace engine
{
namespace vessel
{
   keepHistoryPointerArray::keepHistoryPointerArray()
   {

   }

   keepHistoryPointerArray::~keepHistoryPointerArray()
   {
      fini();
   }

   void keepHistoryPointerArray::fini()
   {
      if (NULL != _histroy)
      {
         SDB_THREAD_FREE(_histroy);
         _histroy = NULL;
      }
      if (NULL != _current)
      {
         SDB_THREAD_FREE(_current);
         _current = NULL;
      }
      _capacity = 0;
      _size = 0;
      return;
   }

   INT32 keepHistoryPointerArray::init(UINT32 initCapacity)
   {
      INT32 rc = SDB_OK;
      fini();
      if (0 < initCapacity)
      {
         rc = ensureCapacity(initCapacity);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

   done:
      return rc;
   error:
      fini();
      goto done;
   }

   INT32 keepHistoryPointerArray::ensureCapacity(UINT32 capacity)
   {
      INT32 rc = SDB_OK;
      ossValuePtr *tmp = NULL;
      ossValuePtr *history = NULL;
      UINT32 bufferSize = 0;
      UINT32 newCapacity = 0;
      if (capacity <= _capacity)
      {
         goto done;
      }

      newCapacity = (capacity << 1);
      bufferSize = (newCapacity << 3);
      tmp = (ossValuePtr *)SDB_THREAD_ALLOC(bufferSize);
      if (NULL == tmp)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }
      ossMemset(tmp, 0, bufferSize);

      if (0 != _size)
      {
         ossMemcpy(tmp, _current, (_size << 3));
      }

      history = _current;
      _current = tmp;
      _capacity = newCapacity;

      if (NULL != _histroy)
      {
         SDB_THREAD_FREE(_histroy);
      }
      _histroy = history;
   done:
      return rc;
   error:
      goto done;
   }
}//namespace vessel
}//namesapce engine