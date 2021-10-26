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

   Source File Name = slice.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_SLICE_H_
#define VESSEL_SLICE_H_

#include "oss.hpp"
#include "ossTypes.hpp"
#include "pd.hpp"

namespace engine
{
namespace vessel
{
   class slice : public SDBObject
   {
      public:
         OSS_INLINE slice(){}
         OSS_INLINE explicit slice(UINT32 size, const void *data):
                             _size(size),
                             _rptr((const CHAR *)data),
                             _wptr(NULL)
         {
            SDB_ASSERT(NULL != _rptr, "can not be null");
         }
         OSS_INLINE slice(const slice &r):
                    _size(r._size),
                    _rptr(r._rptr),
                    _wptr(r._wptr){}

         OSS_INLINE ~slice(){}

         OSS_INLINE slice &operator=(const slice &r)
         {
            _size = r._size;
            _rptr = r._rptr;
            _wptr = r._wptr;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _rptr;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return 0 == _size;
         }
         OSS_INLINE BOOLEAN isWritale()const
         {
            return NULL != _wptr;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE const CHAR *data()const
         {
            return _rptr;
         }
         OSS_INLINE const CHAR *getRPtr()const
         {
            return _rptr;
         }
         OSS_INLINE CHAR *getWPtr()
         {
            return _wptr;
         }

         OSS_INLINE void reset()
         {
            _size = 0;
            _rptr = NULL;
            _wptr = NULL;
            return;
         }

         OSS_INLINE void reset(UINT32 size, const void *data)
         {
            SDB_ASSERT(NULL != data, "can not be null");
            _size = size;
            _rptr = (const CHAR *)data;
            _wptr = NULL;
            return;
         }

         OSS_INLINE void makeWritable(UINT32 size,
                                      void *data)
         {
            SDB_ASSERT(NULL != data, "can not be null");
            _size = size;
            _wptr = (CHAR *)data;
            _rptr = _wptr;
            return;
         }

      public:
         OSS_INLINE const CHAR *getReadablePtr(UINT32 offset, UINT32 size)const
         {
            SDB_ASSERT(isValid(), "can not be invalid");
            return isValidAccessing(offset, size) ?
                   (_rptr + offset) : NULL;
         }
         OSS_INLINE const CHAR *getReadablePtrWithoutSize(UINT32 offset)const
         {
            SDB_ASSERT(isValid(), "can not be invalid");
            return isValidAccessing(offset, 1) ?
                   (_rptr + offset) : NULL;
         }

         OSS_INLINE CHAR *getWritablePtr(UINT32 offset, UINT32 size)
         {
            SDB_ASSERT(isWritale(), "can not be invalid");
            return (isWritale() && isValidAccessing(offset, size)) ?
                   (_wptr + offset) : NULL;
         }
         OSS_INLINE CHAR *getWritablePtrWithoutSize(UINT32 offset)
         {
            SDB_ASSERT(isWritale(), "can not be invalid");
            return (isWritale() && isValidAccessing(offset, 1)) ?
                   (_wptr + offset) : NULL;
         }

      public:
         template<class T>
         const T *getReadableObjPtr(UINT32 offset)const
         {
            return (const T *)getReadablePtr(offset, sizeof(T));
         }

         template<class T>
         T *getWritableObjPtr(UINT32 offset)
         {
            return (T *)getWritablePtr(offset, sizeof(T));
         }

      public:
         INT32 write(UINT32 offset, UINT32 size, const void *data);
         INT32 read(UINT32 offset, UINT32 size, void *data)const;
         slice getReadableSlice(UINT32 offset, UINT32 size)const;
         slice getReadableSlice()const;
         slice getWritableSlice(UINT32 offset, UINT32 size);

      private:
         OSS_INLINE BOOLEAN isValidAccessing(UINT32 offset, UINT32 size)const
         {
            return (offset + size) <= _size;
         }

      private:
         UINT32 _size = 0;
         const CHAR *_rptr = NULL;
         CHAR *_wptr = NULL;
   };//class slice

} // namespace vessel
} // namespace engine

#endif // VESSEL_SLICE_H_
