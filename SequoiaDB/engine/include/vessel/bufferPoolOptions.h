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

   Source File Name = bufferPoolOptions.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BUFFER_POOL_OPTIONS_H_
#define VESSEL_BUFFER_POOL_OPTIONS_H_

#include "ossUtil.hpp"

namespace engine
{
namespace vessel
{
   struct bufferPoolOptions : public SDBObject
   {
      void correctIfNecessary()
      {
         if (0 == maxMemSize)
         {
            maxMemSize = (UINT64)4 << 30;
         }

         if (0 == buckets)
         {
            buckets = 8192;
         }
         else if (!ossIsPowerOf2(buckets))
         {
            ossAlignX(buckets, 2);
         }

         if (0 == bucketLatches)
         {
            bucketLatches = 512;
         }
         else if (!ossIsPowerOf2(bucketLatches))
         {
            ossAlignX(bucketLatches, 2);
         }

         if (buckets < bucketLatches)
         {
            bucketLatches = buckets;
         }

         if (flushDirtyListThreshold <= 0.0f)
         {
            flushDirtyListThreshold = 0.7f;
         }
         else if (1.0f <= flushDirtyListThreshold)
         {
            flushDirtyListThreshold = 0.7f;
         }

         if (0 == flushDirtyListMillis)
         {
            flushDirtyListMillis = 30000;
         }

         if (0 == flushBatchSize)
         {
            flushBatchSize = (UINT64)256 << 20;
         }

         return;
      }

      /// memory pool options
      UINT64 maxMemSize = (UINT64)4 << 30;
      
      /// bucket options
      UINT32 buckets = 8192;
      UINT32 bucketLatches = 512;

      /// flush options
      FLOAT32 flushDirtyListThreshold = 0.70f; /// valid range (0.00, 1.0)
      UINT32 flushDirtyListMillis = 30000;
      UINT64 flushBatchSize = (UINT64)256 << 20;
   };//class bufferPoolOptions

   typedef class bufferPoolOptions liteBufferPoolOptions;
   typedef class bufferPoolOptions lobcBufferPoolOptions;
} // namespace vessel

} // namespace engine


#endif//VESSEL_BUFFER_POOL_OPTIONS_H_