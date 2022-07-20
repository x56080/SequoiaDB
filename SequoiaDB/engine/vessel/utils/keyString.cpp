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
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   keyString::~keyString()
   {
      if (nullptr != _bufferOwned)
      {
         utilPoolAllocator allocator;
         allocator.free(_bufferOwned);
      }
   }

   keyString::keyString(const slice &s):
   _ref(s)
   {
      SDB_ASSERT(_ref.isValid(), "can not be invalid");
      _block.init(s);
      SDB_ASSERT(_block.isValid(), "can not be invalid");
   }

   keyString::keyString(CHAR *buffer,
                        UINT32 bufferSize,
                        UINT32 ksSize)
   {
      adopt(buffer, bufferSize, ksSize);
   }

   keyString::keyString(const keyString &o):
   _ref(o._ref),
   _block(o._block)
   {

   }

   keyString &keyString::operator=(const keyString &o)
   {
      reset();
      _ref = o._ref;
      _block = o._block;
      return *this;
   }

   keyString::keyString(keyString &&o)
   {
      if (o.isValid())
      {
         _bufferOwned = o._bufferOwned;
         _bufferSize = o._bufferSize;
         _ref = o._ref;
         _block = o._block;
         o._bufferOwned = nullptr;
      }
      o.reset();
   }

   keyString &keyString::operator=(keyString &&o)
   {
      reset();
      if (o.isValid())
      {
         _bufferOwned = o._bufferOwned;
         _bufferSize = o._bufferSize;
         _ref = o._ref;
         _block = o._block;
         o._bufferOwned = nullptr;
      }
      o.reset();
      return *this;
   }

   void keyString::reset()
   {
      _ref.reset();
      if (nullptr != _bufferOwned)
      {
         utilPoolAllocator allocator;
         allocator.free(_bufferOwned);
         _bufferOwned = nullptr;
      }
      _bufferSize = 0;
      _block.reset();
      return;
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
         _bufferOwned = (CHAR *)allocator.malloc(_ref.getSize());
         if (OSS_UNLIKELY(nullptr == _bufferOwned))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         _bufferSize = _ref.getSize();
         ossMemcpy(_bufferOwned, _ref.data(), _ref.getSize());
         _ref.reset(_ref.getSize(), _bufferOwned);
      }
   done:
      return rc;
   error:
      goto done;
   }

   void keyString::adopt(CHAR *buffer, UINT32 bufferSize, UINT32 ksSize)
   {
      SDB_ASSERT(nullptr != buffer && 0 < bufferSize, "can not be invalid");
      SDB_ASSERT(0 < ksSize && ksSize <= bufferSize, "invalid key string size");
      reset();
      _bufferOwned = buffer;
      _bufferSize = bufferSize;
      _ref.reset(ksSize, _bufferOwned);
      _block.init(_ref);
      SDB_ASSERT(_block.isValid(), "can not be invalid");
      return;
   }

   slice keyString::getKeySlice() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const CHAR* buf = _ref.getData() + _block.sizeBeforeKey;
      return slice(_block.keySize, buf);
   }

   slice keyString::getSliceFromKeyTo(UINT32 bytesAfterKey) const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      SDB_ASSERT(bytesAfterKey < (_ref.getSize() - _block.sizeBeforeKey),
                 "invalid bytes");
      const CHAR* buf = _ref.getData() + _block.sizeBeforeKey;
      return slice(_block.keySize, buf);
   }

   slice keyString::getSliceBeforeKey() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      return slice(_block.sizeBeforeKey, _ref.getData());
   }

   slice keyString::getSliceAfterKey() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const CHAR* buf = _ref.getData() + 
                        _block.sizeBeforeKey +
                        _block.keySize;
      return slice(_block.keySize, buf);
   }

   slice keyString::getTypeBits() const
   {
      SDB_ASSERT(isValid(), "can not be invalid");
      const CHAR* buf = _ref.getData() + 
                        _block.sizeBeforeKey +
                        _block.keySize +
                        _block.sizeAfterKey;
      return slice(_block.typeBitsSize, buf);
   }

} // namespace vessel
} // namespace engine