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

   Source File Name = liteIOBuffer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/liteIOBuffer.h"
#include "pdTrace.hpp"
#include "vessel/ioBufferControlBlock.h"
#include "vessel/liteIOBufferPool.h"

namespace engine
{
namespace vessel
{
   liteIOBuffer::~liteIOBuffer()
   {
      if (isValid())
      {
         _pool->release(*this);
      }
   }

   liteIOBuffer::liteIOBuffer(liteIOBuffer &&o):
   _pool(o._pool),
   _bcb(std::move(o._bcb)),
   _mode(o._mode)
   {
      o._pool = nullptr;
      o._mode.setNone();
   }

   liteIOBuffer &liteIOBuffer::operator=(liteIOBuffer &&o)
   {
      reset();
      if (o.isValid())
      {
         _pool = o._pool;
         _bcb = std::move(o._bcb);
         _mode = o._mode;

         o._pool = nullptr;
         o._mode.setNone();
      }
      return *this;
   }

   void liteIOBuffer::reset()
   {
      if (isValid())
      {
         _pool->release(*this);
      }
   }

   void liteIOBuffer::init(liteIOBufferPool *pool,
                           SHARED_IO_BUFFER_CB &&bcb,
                           ossSharedLatchMode mode)
   {
      SDB_ASSERT(nullptr != pool, "can not be invalid");
      SDB_ASSERT(!mode.isNone(), "cana not be invalid");

      reset();
      _pool = pool;
      _bcb = bcb;
      _mode = mode;
   }

   BOOLEAN liteIOBuffer::isWritable()const
   {
      return isValid() &&
             _mode.isExclusive() &&
             _bcb->hasMemoryBlock();
   }

   void liteIOBuffer::commit(UINT64 lsn)
   {
      if (OSS_LIKELY(isValid()))
      {
         _pool->commit(lsn, *this);
      }
   }

   INT32 liteIOBuffer::makeWritable()
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!isValid()))
      {
         rc = SDB_VESSEL_RESOURCES_NOT_INIT;
         goto error;
      }
      else if (isWritable())
      {
         goto done;
      }

      rc = _pool->makeWritable(*this);
      if (SDB_OK != rc)
      {
         goto error;
      }
   done:
      return rc;
   error:
      goto done;
   }

   UINT32 liteIOBuffer::getBufferSize()const
   {
      return isValid() ? _pool->getBufferSize() : 0;
   }

/*
   strictBuffer liteIOBuffer::getReadableBuffer()const
   {
      strictBuffer b;
      if (isValid())
      {
         b.reset(getBufferSize(), _bcb->autoGetBufferPtr());
      }
      return b;
   }

   strictBuffer liteIOBuffer::getWritableBuffer()
   {
      strictBuffer b;
      if (isValid())
      {
         b.makeWritable(getBufferSize(), _bcb->autoGetBufferPtr());
      }
      return b;
   }
   */

   CHAR *liteIOBuffer::getBufferPtr()
   {
      return isValid() ? _bcb->autoGetBufferPtr() : nullptr;
   }

   const CHAR *liteIOBuffer::getBufferPtr()const
   {
      return isValid() ? _bcb->autoGetBufferPtr() : nullptr;
   }
} // namespace vessel

} // namespace engine

