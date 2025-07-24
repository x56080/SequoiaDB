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

   Source File Name = memoryBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_MEMORY_BLOCK_H_
#define VESSEL_MEMORY_BLOCK_H_

#include "core.hpp"
#include "oss.hpp"
#include "vessel/slice.h"

namespace engine
{
namespace vessel
{
   class memoryBlock : public SDBObject
   {
      public:
         memoryBlock(){}
         memoryBlock(memoryBlock &&o);
         ~memoryBlock();
         memoryBlock(const memoryBlock &) = delete;
         memoryBlock &operator=(const memoryBlock &) = delete;
         memoryBlock &operator=(memoryBlock &&o);

      public:
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _size;
         }
         OSS_INLINE UINT32 getFreeCapacity()const
         {
            return _capacity - _size;
         }
         OSS_INLINE UINT32 getCapacity()const
         {
            return _capacity;
         }

         void release();
         INT32 reserve(UINT32 size);

         /// 'v' will be filled into buffer
         /// if current size is lower than 'size'.
         INT32 resize(UINT32 size, CHAR v = 0);

         /// reset block by buffer
         INT32 copy(UINT32 size, const void *buffer);

         INT32 append(UINT32 size, const void *buffer);

         void swap(memoryBlock &mb);

         slice getReadableSlice()const
         {
            return isEmpty() ? slice() : slice(_size, _buffer);
         }

         CHAR *getBuffer()
         {
            return _buffer;
         }
         const CHAR *getBuffer()const
         {
            return _buffer;
         }
      private:
         CHAR *_buffer = NULL;
         UINT32 _size = 0;
         UINT32 _capacity = 0;
   };//class memoryBlock
}//namespace vessel
}//namespace engine

#endif//VESSEL_MEMORY_BLOCK_H_