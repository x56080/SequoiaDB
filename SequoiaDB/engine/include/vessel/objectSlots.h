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

   Source File Name = objectSlots.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_SLOTS_H_
#define VESSEL_OBJECT_SLOTS_H_

#include "ossUtil.hpp"
#include "ossLikely.hpp"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   static const UINT32 OBJECT_SLOT_Y_BITWISE = 10;

   template <class T>
   class objectSlots : public SDBObject
   {
      public:
         objectSlots(){}
         ~objectSlots()
         {
            fini();
         }
         objectSlots(const objectSlots &) = delete;
         objectSlots &operator=(const objectSlots &) = delete;

      public:
         OSS_INLINE UINT32 size()const
         {
            return _size;
         }
         OSS_INLINE BOOLEAN isInitialized()const
         {
            return 0 < _size;
         }

      private:
         template <class U>
         class _objectSlot : public SDBObject
         {
            public:
            _objectSlot(){}
            ~_objectSlot()
            {
               SAFE_OSS_DELETE(obj);
            }
            OSS_INLINE BOOLEAN isFree()const
            {
               return NULL == obj;
            }
            OSS_INLINE U *allocate()
            {
               SDB_ASSERT(NULL == obj, "must be null");
               obj = SDB_OSS_NEW U();
               return obj;
            }
            OSS_INLINE void release()
            {
               SAFE_OSS_DELETE(obj);
            }
            OSS_INLINE U *getObj()
            {
               return obj;
            }
            private:
               U *obj = NULL;
         };//class _objectSlot

      public:
         INT32 init(UINT32 size, BOOLEAN delayInitY=FALSE)
         {
            INT32 rc = SDB_OK;
            UINT32 xSize = 0;
            UINT32 matrixSize = 0;
            
            if (0 == size ||
                !ossIsPowerOf2(size))
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            fini();
            _size = size;
            xSize = getXSize(size);
            matrixSize = xSize * sizeof(_objectSlot<T>*);
            _matrix = (_objectSlot<T>**)SDB_OSS_MALLOC(matrixSize);
            if (NULL == _matrix)
            {
               rc = SDB_OOM;
               goto error;
            }
            ossMemset(_matrix, 0, matrixSize);
            
            if (!delayInitY)
            {
               for (UINT32 i = 0; i < xSize; ++i)
               {
                  rc = ensureYSpaceAtX(i);
                  if (SDB_OK != rc)
                  {
                     goto error;
                  }
               }
            }
         done:
            return rc;
         error:
            fini();
            goto done;
         }

         void fini()
         {
            if (NULL != _matrix)
            {
               UINT32 size = getXSize(_size);
               for (UINT32 i = 0; i < size; ++i)
               {
                  if (NULL != _matrix[i])
                  {
                     SDB_OSS_DEL [](_matrix[i]);
                     _matrix[i] = NULL;
                  }
               }
               SDB_OSS_FREE(_matrix);
               _matrix = NULL;
            }
            _size = 0;
            return;
         }

         /// return error if obj already exists.
         INT32 allocateNewObj(UINT32 slot, T **obj)
         {
            INT32 rc = SDB_OK;
            UINT32 x = 0;
            UINT32 y = 0;
            _objectSlot<T> *objSlot = NULL;

            if (!isInitialized())
            {
               rc = SDB_VESSEL_RESOURCES_NOT_INIT;
               goto done;
            }
            else if (_size <= slot || NULL == obj)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            x = getX(slot);
            y = getY(slot);
            rc = ensureYSpaceAtX(x);
            if (SDB_OK != rc)
            {
               goto error;
            }

            objSlot = getSlot(x, y);
            if (!objSlot->isFree())
            {
               rc = SDB_INVALIDARG;
               goto error;
            }

            if (NULL == objSlot->allocate())
            {
               rc = SDB_OOM;
               goto error;
            }
            
            *obj = objSlot->getObj();
         done:
            return rc;
         error:
            if (NULL != obj)
            {
               *obj = NULL;
            }
            goto done;
         }

         void releaseObject(UINT32 slot)
         {
            _objectSlot<T> *objSlot = NULL;
            UINT32 x = getX(slot);
            UINT32 y = getY(slot);
            if (_size <= slot || !isInitialized())
            {
               goto done;
            }
            else if (NULL == _matrix[x])
            {
               goto done;
            }

            objSlot = getSlot(x, y);
            objSlot->release();
         done:
            return;
         }

         T *getObject(UINT32 slot)
         {
            T *obj = NULL;
            _objectSlot<T> *objSlot = NULL;
            UINT32 x = getX(slot);
            UINT32 y = getY(slot);
            if(_size <= slot || !isInitialized())
            {
               goto done;
            }
            else if (NULL == _matrix[x])
            {
               goto done;
            }
            
            objSlot = getSlot(x, y);
            if (objSlot->isFree())
            {
               goto done;
            }

            obj = objSlot->getObj();
         done:
            return obj;
         }

      private:
         OSS_INLINE _objectSlot<T> *getSlot(UINT32 x, UINT32 y)
         {
            return &(_matrix[x][y]);
         }

         INT32 ensureYSpaceAtX(UINT32 x)
         {
            INT32 rc = SDB_OK;
            SDB_ASSERT(isInitialized(), "can not be invalid");
            UINT32 realY = getYSize();
            UINT32 size = getXSize(_size);
            if (size <= x)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            else if (NULL != _matrix[x])
            {
               goto done;
            }
            else if ((size - 1) == x)
            {
               UINT32 mod = (_size & (realY - 1));
               if (0 != mod)
               {
                  realY = mod;
               }
            }

            _matrix[x] = SDB_OSS_NEW _objectSlot<T>[realY];
            if (NULL == _matrix[x])
            {
               rc = SDB_OOM;
               goto error;
            }
         done:
            return rc;
         error:
            goto done;
         }

      private:
         OSS_INLINE UINT32 getXSize(UINT32 size)const
         {
            UINT32 s = (size >> OBJECT_SLOT_Y_BITWISE);
            if (0 != (size & (getYSize() - 1)))
            {
               ++s;
            }
            return s;
         }
         OSS_INLINE UINT32 getX(UINT32 slot)const
         {
            return slot >> OBJECT_SLOT_Y_BITWISE;
         }
         constexpr UINT32 getYSize()const
         {
            return (UINT32)1 << OBJECT_SLOT_Y_BITWISE;
         }
         OSS_INLINE UINT32 getY(UINT32 slot)const
         {
            return slot & (getYSize() - 1);
         }
      private:
         UINT32 _size = 0;
         _objectSlot<T> **_matrix = NULL;
   };//class objectSlots

}//namespace vessel
}//namespace engine

#endif//VESSEL_OBJECT_SLOTS_H_