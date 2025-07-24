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

   Source File Name = strictBuffer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_AUTO_BUFFER_H_
#define VESSEL_AUTO_BUFFER_H_

#include "vessel/slice.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   ///WARNING: strictBuffer does not own the memory!
   class strictBuffer : public SDBObject
   {
      public:
         strictBuffer(){}
         explicit strictBuffer(UINT32 size, const void *data):
         _size(size), _rptr((const CHAR *)data), _wptr(NULL){}
         ~strictBuffer(){}
         strictBuffer(const strictBuffer &o):
         _size(o._size),
         _rptr(o._rptr),
         _wptr(o._wptr){}
         strictBuffer &operator=(const strictBuffer &o)
         {
            _size = o._size;
            _rptr = o._rptr;
            _wptr = o._wptr;
            return *this;
         }
      
      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return NULL != _rptr;
         }
         OSS_INLINE BOOLEAN isWritable()const
         {
            return NULL != _wptr;
         }
         OSS_INLINE UINT32 getSize()const
         {
            return _size;
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
         }
         OSS_INLINE void reset(UINT32 size, const void *buf)
         {
            SDB_ASSERT(NULL != buf && 0 < size, "can not be invalid");
            _size = size;
            _rptr = (const CHAR *)buf;
            _wptr = NULL;
            return;
         }
         OSS_INLINE void makeWritable(UINT32 size, void *buf)
         {
            SDB_ASSERT(NULL != buf && 0 < size, "can not be invalid");
            _size = size;
            _rptr = (const CHAR *)buf;
            _wptr = (CHAR *)buf;
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
            SDB_ASSERT(isWritable(), "can not be invalid");
            return (isWritable() && isValidAccessing(offset, size)) ?
                   (_wptr + offset) : NULL;
         }
         OSS_INLINE CHAR *getWritablePtrWithoutSize(UINT32 offset)
         {
            SDB_ASSERT(isWritable(), "can not be invalid");
            return (isWritable() && isValidAccessing(offset, 1)) ?
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
         void setBuffer(CHAR v = 0x00);
         void setBuffer(UINT32 offset, UINT32 size, CHAR v = 0x00);
         INT32 write(UINT32 offset, UINT32 size, const void *data);
         INT32 read(UINT32 offset, UINT32 size, void *data)const;
         slice getSlice(UINT32 offset, UINT32 size)const;
         slice getSlice()const;
         strictBuffer getReadableBuffer()const;
         strictBuffer getReadableBuffer(UINT32 size, UINT32 offset)const;
         strictBuffer getWritableBuffer(UINT32 size, UINT32 offset);

      private:
         OSS_INLINE BOOLEAN isValidAccessing(UINT32 offset, UINT32 size)const
         {
            return (offset + size) <= _size;
         }

      private:
         UINT32 _size = 0;
         const CHAR *_rptr = NULL;
         CHAR *_wptr = NULL;
   };//class strictBuffer
} // namespace vessel

} // namespace engine



#endif//VESSEL_AUTO_BUFFER_H_