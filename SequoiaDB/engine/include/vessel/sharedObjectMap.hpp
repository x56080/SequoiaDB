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

   Source File Name = sharedObjectMap.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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
               _i(o._i){}
               object &operator=(const object &o)
               {
                  _i = o._i;
                  return *this;
               }

            public:
               BOOLEAN isValid()const
               {
                  return NULL != _i;
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
            SDB_ASSERT(!o.isValid(), "impossible");
            UINT32 hash = 0;
            _bucket *bucket = NULL;
            ossSpinXLatch *latch = NULL;
            _item *itemFound = NULL;
            _item *itemCreated = NULL;
            UINT32 bucketNo = 0;

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

         void release(object &o)
         {
            SDB_ASSERT(isOpen(), "must be open");
            SDB_ASSERT(o.isValid(), "can not be valid");
            if (isOpen() && o.isValid())
            {
               SDB_ASSERT(0 < o._i->_shared, "impossible");
               UINT32 hash = o._i->getKey().hash();
               _bucket *bucket = NULL;
               ossSpinXLatch *latch = NULL;
               UINT32 bucketNo = 0;
               bucket = getBucket(hash, bucketNo);
               latch = getBucketLatch(bucketNo);

               ossXLatchGuard guard(latch);
               --o._i->_shared;
               if (0 == o._i->_shared)
               {
                  remove(bucket, o._i);
                  guard.unlock();
                  SDB_OSS_DEL o._i;
               }
               guard.unlock();
               o._i = NULL;
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
