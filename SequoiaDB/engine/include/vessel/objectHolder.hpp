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

   Source File Name = objectHolder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_OBJECT_HOLDER_H_
#define VESSEL_OBJECT_HOLDER_H_

#include "ossRWMutex.hpp"
#include "vessel/fixedBitset.hpp"

namespace engine
{
namespace vessel
{
   template<class T>
   class objectHolder : public SDBObject
   {
      public:
         objectHolder() = default;
         ~objectHolder()
         {
            free();
         }
         objectHolder(const objectHolder &) = delete;
         objectHolder &operator=(const objectHolder &) = delete;

      public:
         BOOLEAN isFree()const
         {
            return nullptr == _obj;
         }
         ossRWMutex &mutex()
         {
            return _mutex;
         }
         ossRWMutex *getMutexPtr()
         {
            return &_mutex;
         }
         T *get()
         {
            return _obj;
         }

         template<class ...Args>
         T *ensure(Args &&... args)
         {
            if (isFree())
            {
               _obj = SDB_OSS_NEW T(std::forward<Args>...);
            }
            return _obj;
         }
         void free()
         {
            SAFE_OSS_DELETE(_obj);
         }
      private:
         ossRWMutex _mutex;
         T *_obj = nullptr;
   };//class objectHolder

   template<class T, UINT32 GroupSize=64>
   class objectHolderGroup : public SDBObject
   {
      public:
         objectHolderGroup() {_bs.setAll();}
         ~objectHolderGroup() = default;
         objectHolderGroup(const objectHolderGroup &) = delete;
         objectHolderGroup &operator=(const objectHolderGroup &) = delete;

      public:
         static constexpr UINT32 CAPACITY = GroupSize;

         INT32 findFirst()const
         {
            return _bs.findFirst();
         }
         objectHolder<T> &get(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            return _holders[pos];
         }
         objectHolder<T> *getPtr(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            return _holders + pos;
         }
         BOOLEAN test(UINT32 pos)const
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            return _bs.test(pos);
         }
         void clear(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            _bs.clear(pos);
         }
         void set(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            _bs.set(pos);
         }

         BOOLEAN none()const {return _bs.none();}
         BOOLEAN all()const {return _bs.all();}
         BOOLEAN any()const {return _bs.any();}
         UINT32 getFreeCnt()const {return _bs.getNonzeroBitCount();}

      private:
         fixedBitset<CAPACITY> _bs;
         objectHolder<T> _holders[CAPACITY];
   };//class collectionObjHolderGroup
}//namespace vessel
}//namesapce engine

#endif//VESSEL_OBJECT_HOLDER_H_