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

   Source File Name = sparseBitmap32.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SPARSE_BITMAP_H_
#define VESSEL_SPARSE_BITMAP_H_

#include "ossMemPool.hpp"
#include <memory>

namespace engine
{
namespace vessel
{
   class sparseBitmap32 : public SDBObject
   {
      public:
         sparseBitmap32() = default;
         ~sparseBitmap32() = default;
         sparseBitmap32(const sparseBitmap32 &) = delete;
         sparseBitmap32 &operator=(const sparseBitmap32 &) = delete;
         sparseBitmap32(sparseBitmap32 &&)noexcept;
         sparseBitmap32 &operator=(sparseBitmap32 &&)noexcept;

      public:
         INT32 set(UINT32 v, BOOLEAN *before=nullptr);
         void reset(UINT32 v);
         void reset();
         BOOLEAN test(UINT32 v)const;
         UINT32 getTotalNum()const;
         BOOLEAN isEmpty()const {return 0 == getTotalNum();}

      private:
         static constexpr UINT32 _CONTAINER_VALUE_MASK = 0xFFFF;

      public:
         class container : public SDBObject
         {
            public:
               container() = default;
               virtual ~container() = default;
               container(const container &) = delete;
               container &operator=(const container &) = delete;

            public:
               virtual INT32 set(UINT32 value, BOOLEAN &before) = 0;
               virtual void reset(UINT32 value) = 0;
               virtual void reset() = 0;
               virtual BOOLEAN test(UINT32 value)const = 0;
               virtual UINT32 getTotalNum()const = 0;
               virtual BOOLEAN betterToTransform()const = 0;

               /// return the first valid pos if start < 0,
               /// else return the next valid pos of start.
               virtual INT32 findTheFirstOrNext(INT32 start,
                                                UINT32 &value)const = 0;
         };//class container
         using CONTAINER_UPTR = std::unique_ptr<container>;
         using CONTAINER_MAP = ossPoolMap<UINT32, CONTAINER_UPTR>;

      public:
         class iterator : public SDBObject
         {
            friend class sparseBitmap32;
            public:
               iterator() = default;
               ~iterator() = default;

            public:
               OSS_INLINE BOOLEAN isValid()const {return 0 <= _pos;}
               OSS_INLINE UINT32 get() const {return _value;}
               OSS_INLINE void reset()
               {
                  _bucket = 0;
                  _pos = -1;
                  _value = 0;
               }

            private:
               void _swichBucket(UINT32 bucket)
               {
                  _bucket = bucket;
                  _pos = -1;
                  _value = 0;
               }

            private:
               UINT32 _bucket = 0;
               INT32 _pos = -1;
               UINT32 _value = 0;
         };//class iterator
 
         /// do not modify bitmap when scanning it.
         BOOLEAN next(iterator &i)const;

      private:
         INT32 _ensureContainer(UINT32 bucket, container **c);
         CONTAINER_UPTR _transform(const container *c)const;

      private:
         OSS_INLINE UINT32 _getValueInContainer(UINT32 v)const
         {
            return v & _CONTAINER_VALUE_MASK;
         }
         OSS_INLINE UINT32 _getBucket(UINT32 v)const
         {
            return v >> 16;
         }
         OSS_INLINE UINT32 _combine(UINT32 bucket, UINT32 value)const
         {
            UINT32 v = bucket;
            v <<= 16;
            v |= (_getValueInContainer(value));
            return v;
         }

      private:
         CONTAINER_MAP _cmap;
   };//sparseBitmap32
} // namespace vessel

} // namespace engine


#endif//VESSEL_SPARSE_BITMAP_H_