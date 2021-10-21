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

   Source File Name = memoryBlock.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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