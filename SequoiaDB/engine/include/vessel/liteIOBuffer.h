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

   Source File Name = liteIOBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LITE_IO_BUFFER_H_
#define VESSEL_LITE_IO_BUFFER_H_

#include "ossSharedLatch.hpp"
#include "vessel/ioBufferControlBlock.h"

namespace engine
{
namespace vessel
{
   class liteIOBufferPool;

   class liteIOBuffer : public SDBObject
   {
      friend class liteIOBufferPool;
      public:
         liteIOBuffer() = default;
         ~liteIOBuffer();
         liteIOBuffer(const liteIOBuffer &) = delete;
         liteIOBuffer &operator=(const liteIOBuffer &) = delete;
         liteIOBuffer(liteIOBuffer &&);
         liteIOBuffer &operator=(liteIOBuffer &&);

      public:
         OSS_INLINE BOOLEAN isValid()const{return nullptr != _pool;}
         OSS_INLINE ossSharedLatchMode getMode()const {return _mode;}
         
      public:
         BOOLEAN isWritable()const;
         void commit(UINT64 lsn);
         void reset();
         INT32 makeWritable();

         UINT32 getBufferSize()const;
         CHAR *getBufferPtr();
         const CHAR *getBufferPtr()const;

      private:
         void init(liteIOBufferPool *pool,
                   SHARED_IO_BUFFER_CB &&bcb,
                   ossSharedLatchMode mode);
      private:
         liteIOBufferPool *_pool = nullptr;
         SHARED_IO_BUFFER_CB _bcb;
         ossSharedLatchMode _mode;
   };//class liteIOBuffer
} // namespace vessel

} // namespace engine


#endif//VESSEL_LITE_IO_BUFFER_H_