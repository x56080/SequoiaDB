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