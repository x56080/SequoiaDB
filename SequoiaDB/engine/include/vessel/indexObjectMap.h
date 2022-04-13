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

   Source File Name = indexObjectMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_OBJECT_MAP_H_
#define VESSEL_INDEX_OBJECT_MAP_H_

#include "vessel/unstableIndexContext.h"
#include "vessel/indexObject.h"

namespace engine
{
namespace vessel
{
   class indexObjectMap : public SDBObject
   {
      public:
         indexObjectMap(){}
         ~indexObjectMap();
         indexObjectMap(const indexObjectMap &) = delete;
         indexObjectMap &operator=(const indexObjectMap &) = delete;

      public:
         OSS_INLINE UINT32 getMaxIndexLid()const
         {
            return _maxIndexLid;
         }
         UINT32 getNextIndexLid();

         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _objects.empty();
         }
         BOOLEAN isAllowedToCreateMore()const;

      public:
         void fini();

         INT32 findFreeIndexSlot()const;         

         INT32 insert(INT32 indexSlot,
                      UINT32 indexLid,
                      PAGE_ID lpid,
                      const indexDescription &desc,
                      INDEX_STATUS status,
                      PAGE_ID btreeRoot = INVALID_PAGE_ID);

         void erase(INT32 indexSlot);
                     
         /// when filter is invalid, return index object found with any status.
         indexObject *find(const indexIdentifier &indexId,
                           INDEX_STATUS filter=INDEX_STATUS_INVALID)const;

         void setMaxIndexLid(UINT32 indexLid);

      private:
         BOOLEAN isIndexSlotFree(INT32 indexSlot);
         void unfreeIndexSlot(INT32 indexSlot);
         void freeIndexSlot(INT32 indexSlot);

         INT32 insert(indexObject *ic);
         

      private:
         typedef ossPoolMap<INT32, indexObject*> _OBJECT_MAP;

      public:
         typedef _OBJECT_MAP::const_iterator CONST_ITERATOR;
         CONST_ITERATOR begin()const {return _objects.begin();}
         CONST_ITERATOR end()const {return _objects.end();}

         typedef _OBJECT_MAP::iterator ITERATOR;
         ITERATOR begin() {return _objects.begin();}
         ITERATOR end() {return _objects.end();}
      private:
         UINT32 _maxIndexLid = INVALID_LOGICAL_INDEX_ID;
         UINT64 _freeIndexSlots = OSS_UINT64_MAX;

         _OBJECT_MAP _objects;
          
   };//class indexObjectMap
}//namespace vessel
}//nemespace engine

#endif//VESSEL_INDEX_OBJECT_MAP_H_
