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

   Source File Name = objectHolder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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