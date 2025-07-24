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

   Source File Name = slice.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_SLICE_H_
#define VESSEL_SLICE_H_

#include "oss.hpp"
#include "ossTypes.hpp"
#include "ossUtil.h"

namespace engine
{
namespace vessel
{
   class slice : public SDBObject
   {
      public:
         slice() = default;
         explicit slice(UINT32 size, const void *data):
                        _size(size),
                        _data((const CHAR *)data){}
         slice(const slice &r) = default;

         ~slice() = default;

         slice &operator=(const slice &r) = default;


      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _data && 0 < _size;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE UINT32 size()const
         {
            return _size;
         }
         OSS_INLINE const CHAR *getData()const
         {
            return _data;
         }
         OSS_INLINE const CHAR *data()const
         {
            return _data;
         }
         
         OSS_INLINE void reset()
         {
            _size = 0;
            _data = nullptr;
            return;
         }

         OSS_INLINE void reset(UINT32 size, const void *data)
         {
            _size = size;
            _data = (const CHAR *)data;
            return;
         }

         OSS_INLINE slice getSlice(UINT32 offset, UINT32 size)const
         {
            slice s;
            if ((offset + size) <= _size)
            {
               s.reset(size, _data + offset);
            }
            return s;
         }

         OSS_INLINE slice getSliceFromOffsetToEnd(UINT32 offset)const
         {
            slice s;
            if (offset < _size)
            {
               s.reset(_size - offset, _data + offset);
            }
            return s;
         }

         OSS_INLINE INT32 compare(const slice &o)const
         {
            UINT32 n = OSS_MIN(_size, o._size);
            INT32 res = ossMemcmp(_data, o._data, n);
            if (0 == res)
            {
               if (_size < o._size)
               {
                  res = -1;
               }
               else if (_size > o._size)
               {
                  res = 1;
               }
            }

            return res;
         }

         OSS_INLINE INT32 compare(const slice &prefix,
                                  const slice &suffix)const
         {
            UINT32 n = OSS_MIN(_size, prefix._size);
            INT32 res = ossMemcmp(_data, prefix._data, n);
            if (0 == res)
            {
               if (n < prefix.size())
               {
                  return -1;
               }
               else
               {
                  UINT32 remain = _size - n;
                  UINT32 m = OSS_MIN(remain, suffix._size);
                  res = ossMemcmp(_data + n, suffix._data, m);
                  if (0 == res)
                  {
                     if (remain < suffix._size)
                     {
                        return -1;
                     }
                     else if (remain == suffix._size)
                     {
                        return 0;
                     }
                     else
                     {
                        return 1;
                     }
                  }
                  else
                  {
                     return res;
                  }
               }
            }
            else
            {
               return res;
            }
         }

         OSS_INLINE BOOLEAN equal(const slice &o)const
         {
            return _size == o._size &&
                   0 == ossMemcmp(_data, o._data, _size);
         }

         OSS_INLINE slice commonPrefix(const slice &r)const
         {
            UINT32 len = 0;
            UINT32 minLen = _size < r._size ? _size : r._size;
            while (len < minLen && _data[len] == r._data[len])
            {
               len++;
            }
            return slice(len, _data);
         }

         template<class T>
         const T *castTo(UINT32 offset=0)const
         {
            return (sizeof(T) + offset ) <= _size ?
                   reinterpret_cast<const T *>(_data) : nullptr;
         }

      private:
         UINT32 _size = 0;
         const CHAR *_data = nullptr;
   };//class slice

   extern INT32 compareSlicePairs(const slice &lprefix, const slice &lsuffix,
                                  const slice &rprefix, const slice rsuffix);

} // namespace vessel
} // namespace engine

#endif // VESSEL_SLICE_H_
