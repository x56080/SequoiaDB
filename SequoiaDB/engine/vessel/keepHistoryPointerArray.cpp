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

   Source File Name = keepHistoryPointerArray.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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