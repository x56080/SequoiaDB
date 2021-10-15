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

   Source File Name = strictPointer.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_STRICT_POINTER_H_
#define VESSEL_STRICT_POINTER_H_

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
namespace vessel
{
   class strictPointer : public SDBObject
   {
      public:
         strictPointer(){}
         ~strictPointer(){}
         OSS_INLINE strictPointer(const strictPointer &o):
         _size(o._size),
         _readablePtr(o._readablePtr),
         _writablePtr(o._writablePtr){}

         OSS_INLINE strictPointer &operator=(const strictPointer &o)
         {
            _size = o._size;
            _readablePtr = o._readablePtr;
            _writablePtr = o._writablePtr;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return 0 < _size;
         }
         OSS_INLINE BOOLEAN isReadable()const
         {
            return NULL != _readablePtr;
         }
         OSS_INLINE BOOLEAN isWritable()const
         {
            return NULL != _writablePtr;
         }

         OSS_INLINE void reset()
         {
            _size = 0;
            _readablePtr = NULL;
            _writablePtr = NULL;
            return;
         }

         OSS_INLINE UINT32 getSize()const
         {
            return _size;
         }
         OSS_INLINE const CHAR *getReadablePtr()const
         {
            return _readablePtr;
         }
         OSS_INLINE CHAR *getWriablePtr()
         {
            return _writablePtr;
         }

         const CHAR *getReadablePtr(UINT32 offset, UINT32 size)const;
         const CHAR *getReadablePtrWithoutSize(UINT32 offset)const;
         CHAR *getWritablePtr(UINT32 offset, UINT32 size);
         CHAR *getWritablePtrWithoutSize(UINT32 offset);
         INT32 write(UINT32 offset, UINT32 size, const void *data);
         INT32 read(UINT32 offset, UINT32 size, void *data)const;
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
         void setReadable(UINT32 size, const CHAR *ptr);
         void setWritable(UINT32 size, CHAR *ptr);

      private:
         OSS_INLINE BOOLEAN isValidAccessing(UINT32 offset, UINT32 size)const
         {
            return (offset + size) <= _size;
         }

      private:
         UINT32 _size = 0;
         const CHAR *_readablePtr = NULL;
         CHAR *_writablePtr = NULL;
   };//strictPointer
} // namespace vessel

} // namespace engine


#endif//VESSEL_STRICT_POINTER_H_