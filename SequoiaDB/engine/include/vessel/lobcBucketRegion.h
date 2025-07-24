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

   Source File Name = lobcBucketRegion.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LOBC_BUCKET_REGION_H_
#define VESSEL_LOBC_BUCKET_REGION_H_

#include "vessel/pageIdentifier.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   struct lobcBucketRegionBlock
   {
      static constexpr UINT32 BUCKET_COUNT_SQUARE = 4;
      static constexpr UINT32 BUCKET_COUNT = 16;
      PAGE_ID buckets[BUCKET_COUNT];
   };//struct lobcBucketRegionBlock
   constexpr UINT32 LOBC_BUCKET_REGION_BLOCK_SIZE = sizeof(lobcBucketRegionBlock);

   class lobcBucketRegion : public SDBObject
   {
      public:
         lobcBucketRegion(){}
         explicit lobcBucketRegion(UINT32 regionId,
                                   lobcBucketRegionBlock *block);
         lobcBucketRegion(const lobcBucketRegion &o):
         _regionId(o._regionId),
         _block(o._block){}
         ~lobcBucketRegion();

         lobcBucketRegion &operator=(const lobcBucketRegion &o)
         {
            _regionId = o._regionId;
            _block = o._block;
            return *this;
         }

      public:
         struct bucketDesc : public SDBObject
         {
            bucketDesc(){}
            explicit bucketDesc(PAGE_ID page, UINT32 position):
            pid(page),
            pos(position){}

            PAGE_ID pid = INVALID_PAGE_ID;
            UINT32 pos = 0;
         };//struct bucket

         class resizingStrategy : public SDBObject
         {
            friend class lobcBucketRegion;
            public:
               OSS_INLINE UINT32 getTargetPos()const {return _targetPos;}
               OSS_INLINE void reset()
               {
                  _range = 0;
                  _targetPos = 0;
               }
               OSS_INLINE BOOLEAN isValid()const
               {
                  return 0 < _range;
               }

               BOOLEAN targetOwned(UINT32 hash)const
               {
                  SDB_ASSERT(isValid(), "can not be invalid");
                  return _targetPos <= (hash & (_range - 1));
               } 

            private:
               UINT32 _range = 0;
               UINT32 _targetPos = 0;
         };// class resizingStrategy

      public:
         static bucketDesc searchBucket(UINT32 beginPos,
                                        const lobcBucketRegionBlock *rblock);

      public:
         OSS_INLINE BOOLEAN isValid()const {return nullptr != _block;}
         OSS_INLINE UINT32 getRegionId()const {return _regionId;}
         OSS_INLINE const lobcBucketRegionBlock *getBlock()const {return _block;}

         /// find the first valid bucket pid from begin pos left to pos zero.
         bucketDesc searchBucket(UINT32 beginPos)const;

         PAGE_ID getBucket(UINT32 pos)const;
         void setBucketPid(UINT32 pos, PAGE_ID pid);

         resizingStrategy getResizingStrategy(UINT32 pos)const;

      private:
         UINT32 _regionId = 0;
         lobcBucketRegionBlock *_block = nullptr;
   };//class lobcBucketRegion
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBC_BUCKET_REGION_H_