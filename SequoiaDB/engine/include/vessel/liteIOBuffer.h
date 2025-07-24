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

   Source File Name = liteIOBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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