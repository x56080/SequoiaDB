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

   Source File Name = sharedObjectMap.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SHARED_OBJECT_MAP_HPP_
#define VESSEL_SHARED_OBJECT_MAP_HPP_

#include "utilPooledObject.hpp"
#include "ossLatch.hpp"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   template <typename KEY, typename VALUE>
   class sharedObjectMap : public SDBObject
   {
      public:
         sharedObjectMap(){}
         ~sharedObjectMap()
         {
            fini();
         }

         sharedObjectMap(const sharedObjectMap &) = delete;
         sharedObjectMap &operator=(const sharedObjectMap &) = delete;

      private:
         class _item : public utilPooledObject
         {
            friend class sharedObjectMap;
            public:
               _item(){}
               ~_item(){}
               _item(const _item &) = delete;
               _item &operator=(const _item &) = delete;

            public:
               const KEY &getKey()const
               {
                  return _key;
               }
               VALUE &getValue()
               {
                  return _value;
               }
            private:
               _item *_pre = NULL;
               _item *_next = NULL;
               UINT32 _shared = 0;
               KEY _key;
               VALUE _value;
         };//class _item

         class _bucket : public SDBObject
         {
            public:
               _bucket(){}
               ~_bucket(){}
               _bucket(const _bucket &) = delete;
               _bucket &operator=(const _bucket &) = delete;

            public:
               UINT32 _size = 0;
               _item *_head = NULL;
         };//class _bucket

      public:
         class object : public SDBObject
         {
            public:
               friend class sharedObjectMap;
               object(){}
               ~object(){}
               object(const object &o):
               _i(o._i),
               _bucket(o._bucket){}
               object &operator=(const object &o)
               {
                  _i = o._i;
                  _bucket = o._bucket;
                  return *this;
               }

            public:
               BOOLEAN isValid()const
               {
                  return NULL != _i && 0 <= _bucket;
               }
               const KEY &getKey()const
               {
                  return _i->getKey();
               }
               VALUE &getValue()
               {
                  return _i->getValue();
               }

            private:
               _item *_i = NULL;
               INT32 _bucket = -1;
         };//class object

      public:
         BOOLEAN isOpen()const
         {
            return 0 < _bucketCount;
         }
         INT32 init(UINT32 bucketCount,
                    UINT32 latchCount)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(!isOpen(), "do not reinit");

            if (!ossIsPowerOf2(bucketCount) ||
                !ossIsPowerOf2(latchCount))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            else if (bucketCount < latchCount)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            _bucketCount = bucketCount;
            _buckets = SDB_OSS_NEW _bucket[bucketCount];
            if (NULL == _buckets)
            {
               rc = SDB_OOM;
               goto error;
            }
            
            _latchCount = latchCount;
            _latches = SDB_OSS_NEW ossSpinXLatch[latchCount];
            if (NULL == _latches)
            {
               rc = SDB_OOM;
               goto error;
            }
         done:
            return rc;
         error:
            fini();
            goto done;
         }

         void fini()
         {
            if (NULL != _latches)
            {
               SDB_OSS_DEL []_latches;
               _latches = NULL;
            }
            if (NULL != _buckets)
            {
               SDB_OSS_DEL []_buckets;
               _buckets = NULL;
            }
            _bucketCount = 0;
            _latchCount = 0;
            return;
         }

         INT32 ensure(const KEY &k, object &o)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(k.isValid() && !o.isValid(), "impossible");
            UINT32 hash = 0;
            _bucket *bucket = NULL;
            ossSpinXLatch *latch = NULL;
            _item *itemFound = NULL;
            _item *itemCreated = NULL;
            UINT32 bucketNo = 0;
            o = object();

            if (OSS_UNLIKELY(!isOpen()))
            {
               rc = SDB_VESSEL_RESOURCES_NOT_INIT;
               goto error;
            }

            hash = k.hash();
            bucket = getBucket(hash, bucketNo);
            latch = getBucketLatch(bucketNo);
            latch->get();
            itemFound = find(bucket, k);
            if (NULL != itemFound)
            {
               ++(itemFound->_shared);
               o._i = itemFound;
               o._bucket = (INT32)bucketNo;
            }
            else
            {
               itemCreated = SDB_OSS_NEW _item();
               if (OSS_UNLIKELY(NULL == itemCreated))
               {
                  rc = SDB_OOM;
                  goto error;
               }
               itemCreated->_key = k;
               insert(bucket, itemCreated);
               ++itemCreated->_shared;
               o._i = itemCreated;
               o._bucket = (INT32)bucketNo;
            }     
         done:
            if (NULL != latch)
            {
               latch->release();
            }
            return rc;
         error:
            goto done;
         }

         /// User should always validate o outside.
         object get(const KEY &k)
         {
            SDB_ASSERT(isOpen(), "can not be invalid");
            SDB_ASSERT(k.isValid(), "can not be invalid");
            object o;
            UINT32 bucketNo = 0;
            UINT32 hash = k.hash();
            _bucket *bucket = getBucket(hash, bucketNo);
            ossSpinXLatch *latch = getBucketLatch(bucketNo);

            ossXLatchGuard guard(latch);

            _item *itemFound = find(bucket, k);
            if (NULL != itemFound)
            {
               ++(itemFound->_shared);
               o._i = itemFound;
               o._bucket = (INT32)bucketNo;
            }   
         
            return o;
         }
            
         void release(object &o)
         {
            SDB_ASSERT(isOpen(), "must be open");
            SDB_ASSERT(o.isValid(), "can not be valid");
            if (isOpen() && o.isValid())
            {
               SDB_ASSERT(0 < o._i->_shared, "impossible");
               SDB_ASSERT(o._bucket < (INT32)_bucketCount, "out of bound");
               ossSpinXLatch *latch = NULL;
               UINT32 bucketNo = (UINT32)o._bucket;
               _bucket *bucket = _buckets + bucketNo;
               latch = getBucketLatch(bucketNo);

               ossXLatchGuard guard(latch);
               --o._i->_shared;
               if (0 == o._i->_shared)
               {
                  remove(bucket, o._i);
                  guard.unlock();
                  SDB_OSS_DEL o._i;
               }

               o = object();
            }
            return;
         }

         BOOLEAN test(const KEY &key)
         {
            SDB_ASSERT(isOpen(), "must be open");
            SDB_ASSERT(key.isValid(), "must be valid");
            _bucket *bucket = NULL;
            ossSpinXLatch *latch = NULL;
            UINT32 bucketNo = 0;
            UINT32 hash = key.hash();
            bucket = getBucket(hash, bucketNo);
            latch = getBucketLatch(bucketNo);
            ossXLatchGuard guard(latch);
            return NULL != find(bucket, key);
         }

      private:
         _bucket *getBucket(UINT32 hash, UINT32 &bucketNo)
         {
            bucketNo = (hash & (_bucketCount - 1));
            return _buckets + bucketNo;
         }
         ossSpinXLatch *getBucketLatch(UINT32 bucketNo)
         {
            UINT32 i = (bucketNo & (_latchCount - 1));
            return _latches + i;
         }

         _item *find(_bucket *bucket, const KEY &k)
         {
            _item *out = NULL;
            _item *i = bucket->_head;
            while (NULL != i)
            {
               if (i->_key == k)
               {
                  out = i;
                  break;
               }
               i = i->_next;
            }
            return out;
         }

         void insert(_bucket *bucket, _item *i)
         {
            i->_pre = NULL;
            i->_next = bucket->_head;
            if (NULL != bucket->_head)
            {
               bucket->_head->_pre = i;
            }
            bucket->_head = i;
            ++bucket->_size;
         }

         void remove(_bucket *bucket, _item *i)
         {
            /// not head
            if (NULL != i->_pre)
            {
               i->_pre->_next = i->_next;
            }
            else
            {
               bucket->_head = i->_next;
            }

            /// not tail
            if (NULL != i->_next)
            {
               i->_next->_pre = i->_pre;
            }

            i->_pre = NULL;
            i->_next = NULL;
            --bucket->_size;
            return;
         }

      private:
         UINT32 _bucketCount = 0;
         UINT32 _latchCount = 0;
         _bucket *_buckets = NULL;
         ossSpinXLatch *_latches = NULL;

   };//class sharedObjectMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_SHARED_OBJECT_MAP_HPP_
