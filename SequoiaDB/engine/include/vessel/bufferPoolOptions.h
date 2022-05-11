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

   Source File Name = bufferPoolOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BUFFER_POOL_OPTIONS_H_
#define VESSEL_BUFFER_POOL_OPTIONS_H_

#include "ossUtil.hpp"

namespace engine
{
namespace vessel
{
   struct lobcBufferPoolOptions : public SDBObject
   {
      BOOLEAN isValid()const
      {
         return 0 < maxMemChunkCount &&
                ossIsPowerOf2(bucketCount) &&
                ossIsPowerOf2(bucketLatchCount) &&
                bucketLatchCount <= bucketCount;
      }

      UINT32 maxMemChunkCount = 128;
      UINT32 bucketCount = 8192;
      UINT32 bucketLatchCount = 512;

   };//struct lobcBufferPoolOptions

   struct liteBufferPoolOptions : public SDBObject
   {
      /// memory pool options
      UINT32 maxMemChunk = 128; /// default memory chunk is 32MB
      
      /// bucket options
      UINT32 buckets = 8192;
      UINT32 bucketLatches = 512;

      /// flush options
      FLOAT32 minFreeMemPct = 0.30f; /// valid range (0.00, 1.0)
      //UINT64 minFreeMemSize = (UINT64)1 << 30; /// can not be higher than max memory size
      UINT32 flushDirtyListMillis = 30000;
      UINT64 flushBatchSize = (UINT64)1 << 30;
   };//class bufferPoolOptions
} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_POOL_OPTIONS_H_