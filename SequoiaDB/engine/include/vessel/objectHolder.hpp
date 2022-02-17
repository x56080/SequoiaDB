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
#include "vessel/collection.h"

namespace engine
{
namespace vessel
{
   template<class T>
   class objectHolder : public SDBObject
   {
      public:
         objectHolder(){}
         ~objectHolder()
         {
            releaseObj();
         }
         objectHolder(const objectHolder &) = delete;
         objectHolder &operator=(const objectHolder &) = delete;

      public:
         OSS_INLINE BOOLEAN isFree()const
         {
            return nullptr == _obj;
         }
         OSS_INLINE ossRWMutex &getLatch()
         {
            return _latch;
         }
         OSS_INLINE T *getObj()
         {
            return _obj;
         }

         T *ensureObj()
         {
            if (isFree())
            {
               _obj = SDB_OSS_NEW T();
            }
            return _obj;
         }
         void releaseObj()
         {
            SAFE_OSS_DELETE(_obj);
         }
      private:
         ossRWMutex _latch;
         T *_obj = nullptr;
   };//class objectHolder

   class collection;
   typedef class objectHolder<collection> collectionObjHolder;

   class collectionSpace;
   typedef class objectHolder<collectionSpace> collectionSpaceObjHolder;

   class collectionObjHolderGroup : public SDBObject
   {
      public:
         collectionObjHolderGroup(){}
         ~collectionObjHolderGroup(){}
         collectionObjHolderGroup(const collectionObjHolderGroup &) = delete;
         collectionObjHolderGroup &operator=(const collectionObjHolderGroup &) = delete;

      public:
         static constexpr UINT32 CAPACITY = 64;
      public:
         collectionObjHolder holders[CAPACITY];
   };//class collectionObjHolderGroup
}//namespace vessel
}//namesapce engine

#endif//VESSEL_OBJECT_HOLDER_H_