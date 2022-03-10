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

   Source File Name = lobChunkBufferPool.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOB_CHUNK_BUFFER_POOL_H_
#define VESSEL_LOB_CHUNK_BUFFER_POOL_H_

#include "ossMemPool.hpp"
#include "vessel/blockBasedMemPool.h"
#include "vessel/lobChunkBuffer.h"

#include <mutex> //c++11

namespace engine
{
namespace vessel
{
   class lobChunkBufferPool : public SDBObject
   {
      public:
         lobChunkBufferPool();
         ~lobChunkBufferPool();
         lobChunkBufferPool(const lobChunkBufferPool &) = delete;
         lobChunkBufferPool &operator=(const lobChunkBufferPool &) = delete;

      public:
         class options : public SDBObject
         {
            public:
               UINT32 bucketMutexCount = 0;
               UINT32 bucketCount = 0;
         };//class options
      public:
         typedef ossPoolList<lobChunkBufferPtr> BUFFER_BUCKET;

      private:
         typedef std::vector<BUFFER_BUCKET> _BUFFER_BUCKET_VEC;
         typedef std::vector<std::mutex> _BUCKET_MUTEX_VEC;
         typedef ossPoolList<lobChunkBufferPtr> _DIRTY_BUFFER_LIST;
      private:
         options _options;
         _BUCKET_MUTEX_VEC _mutexVec;
         _BUFFER_BUCKET_VEC _bucketVec;

         std::mutex _dlMutex;
         _DIRTY_BUFFER_LIST _dirtyList;
   };//class lobChunkBufferPool
} // namespace vessel

} // namespace engine

#endif//VESSEL_LOB_CHUNK_BUFFER_POOL_H_