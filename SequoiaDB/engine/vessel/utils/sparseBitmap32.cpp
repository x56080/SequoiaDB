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

   Source File Name = sparseBitmap32.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/sparseBitmap32.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/fixedBitset.hpp"

namespace engine
{
namespace vessel
{
   class arrayContainer : public sparseBitmap32::container
   {
      public:
         arrayContainer() = default;
         virtual ~arrayContainer() = default;

      public:
         virtual INT32 set(UINT32 value, BOOLEAN &before) override;
         virtual void reset(UINT32 value) override;
         virtual void reset() override;
         virtual BOOLEAN test(UINT32 value)const override;
         virtual UINT32 getTotalNum()const override;
         virtual BOOLEAN betterToTransform()const override;
         virtual INT32 findTheFirstOrNext(INT32 start,
                                          UINT32 &valueconst)const override;
      public:
         BOOLEAN isFull()const {return _MAX_CAPACITY == _values.size();}
         const ossPoolVector<UINT32> &get()const {return _values;}

      private:
         INT32 _reserve();

      private:
         static constexpr UINT32 _MAX_CAPACITY = 1024;
         ossPoolVector<UINT32> _values;
   };

   INT32 arrayContainer::set(UINT32 value, BOOLEAN &before)
   {
      INT32 rc = SDB_OK;
      if (isFull())
      {
         rc = SDB_VESSEL_OUT_OF_RESOURCE;
         goto error;
      }
      else
      {
         auto itr = std::lower_bound(_values.begin(), _values.end(), value);
         if (_values.end() == itr)
         {
            rc = _reserve();
            if (SDB_OK != rc)
            {
               goto error;
            }

            before = FALSE;
            _values.emplace_back(value);
         }
         else if (value == *itr)
         {
            before = TRUE;
         }
         else
         {
            rc = _reserve();
            if (SDB_OK != rc)
            {
               goto error;
            }
            
            before = FALSE;
            _values.insert(itr, value);
         }
      }
   done:
      return rc;
   error:
      goto done;
   }

   void arrayContainer::reset(UINT32 value)
   {
      auto itr = std::lower_bound(_values.begin(), _values.end(), value);
      if (_values.end() != itr && value == *itr)
      {
         _values.erase(itr);
      }
      return;
   }

   void arrayContainer::reset()
   {
      _values.clear();
      _values.shrink_to_fit();
   }

   BOOLEAN arrayContainer::test(UINT32 value)const
   {
      auto itr = std::lower_bound(_values.begin(), _values.end(), value);
      return (_values.end() != itr && value == *itr);
   }

   UINT32 arrayContainer::getTotalNum()const
   {
      return _values.size();
   }

   BOOLEAN arrayContainer::betterToTransform()const
   {
      return _MAX_CAPACITY <= _values.size();
   }

   INT32 arrayContainer::findTheFirstOrNext(INT32 start, UINT32 &value)const
   {
      INT32 pos = (0 <= start) ? start + 1 : 0;
      if ((UINT32)pos < _values.size())
      {
         value = _values[pos];
      }
      else
      {
         pos = -1;
      }

      return pos;
   }


   INT32 arrayContainer::_reserve()
   {
      INT32 rc = SDB_OK;
      try
      {
         _values.reserve(1);
      }
      catch(const std::exception& e)
      {
         PD_LOG(PDERROR, "failed to reserve space from vector:%s", e.what());
         rc = SDB_VESSEL_INTERNAL_ERR;
         goto error;
      }
      
   done:
      return rc;
   error:
      goto done;
   }
//////////////////////////// bitsetContainer

   class bitsetContainer : public sparseBitmap32::container
   {
      public:
         bitsetContainer() = default;
         virtual ~bitsetContainer() = default;

      public:
         static constexpr UINT32 CAPACITY = 65536;

      public:
         virtual INT32 set(UINT32 value, BOOLEAN &before) override;
         virtual void reset(UINT32 value) override;
         virtual void reset() override;
         virtual BOOLEAN test(UINT32 value)const override;
         virtual UINT32 getTotalNum()const override {return _count;}
         virtual BOOLEAN betterToTransform()const override {return FALSE;}
         virtual INT32 findTheFirstOrNext(INT32 start,
                                          UINT32 &value)const override;
      private:
         UINT32 _count = 0;
         fixedBitset<CAPACITY> _bs;
   };

   INT32 bitsetContainer::set(UINT32 value, BOOLEAN &before)
   {
      SDB_ASSERT(value < CAPACITY, "out of bound");
      _bs.set(value, &before);
      if (!before)
      {
         ++_count;
      }
      return SDB_OK;
   }

   void bitsetContainer::reset(UINT32 value)
   {
      SDB_ASSERT(value < CAPACITY, "out of bound");
      if (_bs.test(value))
      {
         _bs.clear(value);
         --_count;
      }
      return;
   }

   void bitsetContainer::reset()
   {
      if (0 < _count)
      {
         _bs.clearAll();
         _count = 0;
      }
      return;
   }

   BOOLEAN bitsetContainer::test(UINT32 value)const
   {
      SDB_ASSERT(value < CAPACITY, "out of bound");
      return _bs.test(value);
   }

   INT32 bitsetContainer::findTheFirstOrNext(INT32 start,
                                             UINT32 &value)const
   {
      static constexpr INT32 MAX_POS = (INT32)CAPACITY - 1;
      INT32 pos = -1;
      if (start < MAX_POS)
      {
         pos = _bs.findNext(start);
         if (0 <= pos)
         {
            value = (UINT32)pos;
         }
      }

      return pos;
   }

///////////////////////////
   sparseBitmap32::sparseBitmap32(sparseBitmap32 &&o)noexcept:
   _cmap(std::move(o._cmap))
   {

   }

   sparseBitmap32 &sparseBitmap32::operator=(sparseBitmap32 &&o)noexcept
   {
      _cmap = std::move(o._cmap);
      return *this;
   }

   sparseBitmap32::CONTAINER_UPTR sparseBitmap32::_transform(const container *c)const
   {
      SDB_ASSERT(nullptr != c, "can not be invalid");
      const arrayContainer *ac = static_cast<const arrayContainer *>(c);
      const ossPoolVector<UINT32> &v = ac->get();
      CONTAINER_UPTR uptr;
      bitsetContainer *bs = SDB_OSS_NEW bitsetContainer();
      if (OSS_LIKELY(nullptr != bs))
      {
         uptr.reset(bs);
         BOOLEAN old = FALSE;
         for (UINT32 i = 0; i < v.size(); ++i)
         {
            bs->set(v[i], old);
         }
      }

      return std::move(uptr);
   }

   INT32 sparseBitmap32::set(UINT32 v, BOOLEAN *before)
   {
      INT32 rc = SDB_OK;
      BOOLEAN beforValue = FALSE;
      UINT32 bucket = _getBucket(v);
      UINT32 value = _getValueInContainer(v);
      container *c = nullptr;
      BOOLEAN created = FALSE;
      auto itr = _cmap.find(bucket);
      if (_cmap.end() == itr)
      {
         CONTAINER_UPTR uptr(SDB_OSS_NEW arrayContainer());
         if (OSS_UNLIKELY(!uptr))
         {
            PD_LOG(PDERROR, "failed to allocate mem.");
            rc = SDB_OOM;
            goto error;
         }

         c = uptr.get();
         _cmap.emplace(bucket, std::move(uptr));
         created = TRUE;
      }
      else
      {
         c = itr->second.get();
      }

      if (c->betterToTransform())
      {
         CONTAINER_UPTR newContainer = _transform(c);
         if (OSS_UNLIKELY(!newContainer))
         {
            PD_LOG(PDERROR, "failed to transform container");
            rc = SDB_VESSEL_INTERNAL_ERR;
            goto error;
         }

         c = newContainer.get();
         itr->second.reset(newContainer.release());
      }

      rc = c->set(value, beforValue);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to set container:%d", rc);
         if (created)
         {
            _cmap.erase(bucket);
         }
         goto error;
      }

      if (nullptr != before)
      {
         *before = beforValue;
      }

   done:
      return rc;
   error:   
      goto done;
   }

   void sparseBitmap32::reset(UINT32 v)
   {
      UINT32 bucket = _getBucket(v);
      UINT32 value = _getValueInContainer(v);
      auto itr = _cmap.find(bucket);
      if (_cmap.end() != itr)
      {
         itr->second->reset(value);
         if (0 == itr->second->getTotalNum())
         {
            _cmap.erase(itr);
         }
      }
      return;
   }

   void sparseBitmap32::reset()
   {
      _cmap.clear();
   }

   BOOLEAN sparseBitmap32::test(UINT32 v)const
   {
      BOOLEAN r = FALSE;
      UINT32 bucket = _getBucket(v);
      UINT32 value = _getValueInContainer(v);
      auto itr = _cmap.find(bucket);
      if (_cmap.end() != itr)
      {
         r = itr->second->test(value);
      }
      return r;
   }

   UINT32 sparseBitmap32::getTotalNum()const
   {
      UINT32 n = 0;
      for (auto itr = _cmap.cbegin(); itr != _cmap.cend(); ++itr)
      {
         n += itr->second->getTotalNum();
      }
      return n;
   }

   BOOLEAN sparseBitmap32::next(iterator &i)const
   {
      auto itr = _cmap.lower_bound(i._bucket);
      if (_cmap.cend() == itr)
      {
         i.reset();
         goto done;
      }
      else if (itr->first != i._bucket)
      {
         i._swichBucket(itr->first);
      }

      do
      {
         UINT32 value = 0;
         INT32 pos = itr->second->findTheFirstOrNext(i._pos, value);
         if (0 <= pos)
         {
            i._pos = pos;
            i._value = _combine(i._bucket, value);
            break;
         }
         else
         {
            ++itr;
            if (_cmap.cend() == itr)
            {
               i.reset();
               break;
            }
            else
            {
               i._swichBucket(itr->first);
               continue;
            }
         }
      } while (TRUE);
      

   done:
      return i.isValid();
   }
} // namespace vessel

} // namespace engine
