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

   Source File Name = collectionObjHolder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_COLLECTION_OBJ_HOLDER_H_
#define VESSEL_COLLECTION_OBJ_HOLDER_H_

#include "ossRWMutex.hpp"
#include "vessel/collection.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   class collectionObjHolder : public SDBObject
   {
      public:
         collectionObjHolder(){}
         ~collectionObjHolder()
         {
            releaseObj();
         }
         collectionObjHolder(const collectionObjHolder &) = delete;
         collectionObjHolder &operator=(const collectionObjHolder &) = delete;

      public:
         OSS_INLINE BOOLEAN isFree()const
         {
            return NULL == _obj;
         }
         OSS_INLINE ossRWMutex &getLatch()
         {
            return _latch;
         }
         OSS_INLINE collection *getObj()
         {
            return _obj;
         }
         collection *ensureObj()
         {
            if (isFree())
            {
               _obj = SDB_OSS_NEW collection();
            }
            return _obj;
         }
         void releaseObj()
         {
            SAFE_OSS_DELETE(_obj);
         }
      private:
         ossRWMutex _latch;
         collection *_obj = NULL;
   };//class collectionObjHolder

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
         void clear(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            UINT64 v = (UINT64)1 << pos;
            OSS_BIT_CLEAR(bits, v);
         }

         void set(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            UINT64 v = (UINT64)1 << pos;
            OSS_BIT_SET(bits, v);
         }

         INT32 findFirst()const
         {
            return ossGetLowestBit1From64Bits(bits);
         }

         collectionObjHolder *get(UINT32 pos)
         {
            SDB_ASSERT(pos < CAPACITY, "out of bound");
            return holders + pos;
         }

      public:
         UINT64 bits = ~0;
         collectionObjHolder holders[CAPACITY];
   };//class collectionObjHolderGroup
}//namespace vessel
}//namesapce engine

#endif//VESSEL_COLLECTION_OBJ_HOLDER_H_