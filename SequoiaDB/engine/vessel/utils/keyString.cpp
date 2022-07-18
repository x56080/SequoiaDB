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

   Source File Name = keyString.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/16/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/keyString.h"
#include "pdTrace.hpp"
#include "utilAllocator.hpp"
#include "utilSharedPtrMaker.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
///////////keyString::holder
   keyString::holder::holder(CHAR *buffer, UINT32 bufferSize):
   _buffer(buffer),
   _bufferSize(bufferSize)
   {
      SDB_ASSERT(nullptr != _buffer, "can not be invalid");
      SDB_ASSERT(0 < _bufferSize, "can not be invalid");
   }

   keyString::holder::~holder()
   {
      if (nullptr != _buffer)
      {
         utilPoolAllocator allocator;
         allocator.free(_buffer);
      }
   }   

///////////keyString::holder end

   keyString::keyString(const slice &s):
   _ref(s)
   {
      SDB_ASSERT(_ref.isValid(), "can not be invalid");
   }

   keyString::keyString(KEY_STRING_HOLER &&holder, UINT32 size):
   _holder(std::move(holder))
   {
      SDB_ASSERT(_holder, "can not be invalid");
      SDB_ASSERT(0 < size && size <= holder->getBufferSize(), "can not be invalid");
      _ref = slice(size, _holder->getBuffer());
   }

   keyString::keyString(const keyString &o)
   {
      _holder = o._holder;
      if (_holder)
      {
         _ref = slice(o._ref.getSize(), _holder->getBuffer());
      }
   }

   keyString &keyString::operator=(const keyString &o)
   {
      _holder = o._holder;
      if (_holder)
      {
         _ref = slice(o._ref.getSize(), _holder->getBuffer());
      }
      else
      {
         _ref.reset();
      }
      return *this;
   }

   keyString::keyString(keyString &&o)
   {
      _holder = std::move(o._holder);
      if (_holder)
      {
         _ref.reset(o._ref.getSize(), _holder->getBuffer());
      }
      o.reset();
   }

   keyString &keyString::operator=(keyString &&o)
   {
      _holder = std::move(o._holder);
      if (_holder)
      {
         _ref.reset(o._ref.getSize(), _holder->getBuffer());
      }
      else
      {
         _ref.reset();
      }
      o.reset();
      return *this;
   }

   INT32 keyString::getOwned()
   {
      INT32 rc = SDB_OK;

      if (!isValid())
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (!isOwned())
      {
         utilPoolAllocator allocator;
         CHAR *buffer = (CHAR *)allocator.malloc(_ref.getSize());
         if (OSS_UNLIKELY(nullptr == buffer))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }
         else
         {
            _holder = makeSharedPtrFromPool<keyString::holder>(buffer, _ref.getSize());
            if (OSS_UNLIKELY(!_holder))
            {
               allocator.free(buffer);
               PD_LOG(PDERROR, "failed to allocate mem.");
               rc = SDB_OOM;
               goto error;
            }
         }

         ossMemcpy(buffer, _ref.data(), _ref.getSize());
         _ref.reset(_ref.getSize(), buffer);
      }
   done:
      return rc;
   error:
      goto done;
   }

   keyString::KEY_STRING_HOLER keyString::makeHolder(CHAR *buffer, UINT32 bufSize)
   {
      SDB_ASSERT(nullptr != buffer && 0 < bufSize, "can not be invalid");
      return makeSharedPtrFromPool<keyString::holder>(buffer, bufSize);
   }
} // namespace vessel
} // namespace engine