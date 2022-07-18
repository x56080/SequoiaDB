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
#include "pd.hpp"
#include "ossUtil.h"
#include "utilMemListPool.hpp"

namespace engine
{
namespace vessel
{

   keyStringOwned::~keyStringOwned()
   {
      reset();
   }

   keyStringOwned::keyStringOwned(keyStringOwned &&k)
   {
      _data = k._data;
      _buf = k._buf;
      _bufSize = k._bufSize;
      k.reset();
   }

   keyStringOwned &keyStringOwned::operator=(keyStringOwned &&k)
   {
      reset();
      _data = k._data;
      _buf = k._buf;
      _bufSize = k._bufSize;
      k.reset();
      return *this;
   }

   void keyStringOwned::reset()
   {
      keyString::reset();
      if (nullptr != _buf)
      {
         SDB_THREAD_FREE(_buf);
         _buf = nullptr;
      }
      _bufSize = 0;
   }

   INT32 keyStringOwned::own(const keyString &k)
   {
      INT32 rc = SDB_OK;

      if (!k.isValid())
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (_bufSize < k.getSize())
      {
         reset();
         _buf = (CHAR *)SDB_THREAD_ALLOC(k.getSize());
         if (nullptr == _buf)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "out of memory");
            goto error;
         }
         _bufSize = k.getSize();
      }

      ossMemcpy(_buf, k.getData(), k.getSize());
      _data = slice(k.getSize(), _buf);
      
   done:
      return rc;
   error:
      reset();
      goto done;
   }

   INT32 keyStringOwned::adopt(CHAR *buf,
                               UINT32 bufSize,
                               UINT32 keyStringSize)
   {
      INT32 rc = SDB_OK;
      
      if (nullptr == buf ||
          0 == bufSize ||
          0 == keyStringSize ||
          keyStringSize > bufSize)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      reset();
      _buf = buf;
      _bufSize = bufSize;
      _data = slice(keyStringSize, _buf);

   done:
      return rc;
   error:
      reset();
      goto done;
   }
} // namespace vessel
} // namespace engine