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

   Source File Name = lpidLatchContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpidLatchContext.h"
#include "vessel/requestContext.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lpidLatchContext::~lpidLatchContext()
   {
      SDB_ASSERT(0 == _size, "unlock missed");
      if (NULL != _slots && _slots != _staticBuf)
      {
         SDB_OSS_DEL []_slots;
      }
   }

   void lpidLatchContext::reset()
   {
      if (NULL != _slots && _slots != _staticBuf)
      {
         SDB_OSS_DEL []_slots;
      }
      _capacity = STATIC_BUF_SIZE;
      _size = 0;
      _slots = _staticBuf;
      return;
   }

   INT32 lpidLatchContext::push(const LOGICAL_ID_LATCH_MAP::object &obj,
                                ossSharedLatch::mode mode)
   {
      INT32 rc = SDB_OK;
      UINT32 pos = 0;
      if (OSS_UNLIKELY(!obj.isValid() || ossSharedLatch::NONE == mode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = ensureBuf(_size + 1);
      if (SDB_OK != rc)
      {
         goto error;
      }

      _slots[_size].obj = obj;
      _slots[_size].mode = mode;
      ++_size;
   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN lpidLatchContext::pop(const logicalIdLatchKey &key,
                                 LOGICAL_ID_LATCH_MAP::object &obj,
                                 ossSharedLatch::mode &mode)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      SDB_ASSERT(!obj.isValid(), "can not be valid");
      UINT32 pos = 0;
      _lpidLatchSlot *slot = NULL;

      slot = find(key, pos);
      if (NULL != slot)
      {
         obj = slot->obj;
         mode = slot->mode;
         remove(pos);
         r = TRUE;
      }
   done:
      return r;
   }

   BOOLEAN lpidLatchContext::find(const logicalIdLatchKey &key,
                                  LOGICAL_ID_LATCH_MAP::object &obj,
                                  ossSharedLatch::mode &mode)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      _lpidLatchSlot *slot = NULL;
      UINT32 pos = 0;
      slot = find(key, pos);
      if (NULL != slot)
      {
         obj = slot->obj;
         mode = slot->mode;
         r = TRUE;
      }
   done:
      return r;
   }

   INT32 lpidLatchContext::findAndUpdate(const logicalIdLatchKey &key,
                                         ossSharedLatch::mode oldMode,
                                         ossSharedLatch::mode newMode,
                                         LOGICAL_ID_LATCH_MAP::object &obj)
   {
      INT32 rc = SDB_OK;
      UINT32 pos = 0;
      _lpidLatchSlot *slot = NULL;
      if (OSS_UNLIKELY(!key.isValid() || ossSharedLatch::NONE == newMode))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      slot = find(key, pos);
      if (NULL == slot)
      {
         rc = SDB_VESSEL_KEY_NOT_FOUND;
         goto error;
      }

      if (oldMode == slot->mode)
      {
         slot->mode = newMode;
      }
      else
      {
         rc = SDB_VESSEL_OPERATOION_NOT_PERMITTED;
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   void lpidLatchContext::remove(UINT32 pos)
   {
      SDB_ASSERT(pos < _size, "out of bound");
      
      for (UINT32 i = pos; (i + 1) < _size; ++i)
      {
         _slots[i] = _slots[i + 1];
      }

      if (0 < _size)
      {
         _slots[_size - 1] = _lpidLatchSlot();
         --_size;
      }
      return;
   }

   BOOLEAN lpidLatchContext::test(const logicalIdLatchKey &key,
                                  ossSharedLatch::mode &mode)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(key.isValid(), "can not be invalid");
      UINT32 pos = 0;
      const _lpidLatchSlot *slot = find(key, pos);
      if (NULL != slot)
      {
         mode = slot->mode;
         r = TRUE;
      }
   done:
      return r;
   }

   INT32 lpidLatchContext::ensureBuf(UINT32 size)
   {
      INT32 rc = SDB_OK;
      _lpidLatchSlot *tmp = NULL;

      if (size <= _capacity)
      {
         goto done;
      }

      tmp = SDB_OSS_NEW _lpidLatchSlot[size + STATIC_BUF_SIZE];
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

      if (_staticBuf != _slots)
      {
         SDB_OSS_DEL []_slots;
      }

      _slots = tmp;
      _capacity = size + STATIC_BUF_SIZE;
   done:
      return rc;
   error:

      goto done;
   }

   lpidLatchContext::_lpidLatchSlot *lpidLatchContext::find(const logicalIdLatchKey &key,
                                                            UINT32 &pos)
   {
      _lpidLatchSlot *out = NULL;
      SDB_ASSERT(key.isValid(), "can not be invalid");

      for (INT32 i = ((INT32)_size - 1); i >= 0; --i)
      {
         out = _slots + i;
         if (out->obj.getKey() == key)
         {
            pos = i;
            break;
         }
         out = NULL;
      }
   done:
      return out;
   }
   
}//namespace vessel
}//namespace engine