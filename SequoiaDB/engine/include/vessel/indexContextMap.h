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

   Source File Name = indexContextMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_INDEX_CONTEXT_MAP_H_
#define VESSEL_INDEX_CONTEXT_MAP_H_

#include "vessel/unstableIndexContext.h"
#include "vessel/indexContext.h"

namespace engine
{
namespace vessel
{
   class indexContextMap : public SDBObject
   {
      public:
         indexContextMap(){}
         ~indexContextMap();
         indexContextMap(const indexContextMap &) = delete;
         indexContextMap &operator=(const indexContextMap &) = delete;

      public:
         OSS_INLINE UINT32 getNextIndexId()const
         {
            return _nextIndexId;
         }
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _contexts.empty();
         }
         OSS_INLINE BOOLEAN isAllowedToCreateMore()const
         {
            return INVALID_LOGICAL_INDEX_ID != _nextIndexId &&
                   0 != _freeIndexSlots;
         }

      public:
         void fini();

         INT32 findFreeIndexSlot()const;         

         INT32 insert(INT32 indexSlot,
                      PAGE_ID lpid,
                      const indexObject &obj,
                      INDEX_STATUS status);

         void erase(INT32 indexSlot);

         void setBuildingContextAsNormal(INT32 indexSlot);
                     
         indexContext *find(INT32 indexSlot,
                            INDEX_STATUS filter=INDEX_STATUS_INVALID)const;

      private:
         BOOLEAN isIndexSlotFree(INT32 indexSlot);
         void unfreeIndexSlot(INT32 indexSlot);
         void freeIndexSlot(INT32 indexSlot);

         INT32 insert(indexContext *ic);
         

      private:
         typedef ossPoolMap<INT32, indexContext*> _CONTEXT_MAP;

      public:
         typedef _CONTEXT_MAP::const_iterator CONST_ITERATOR;
         CONST_ITERATOR begin()const {return _contexts.begin();}
         CONST_ITERATOR end()const {return _contexts.end();}

         typedef _CONTEXT_MAP::iterator ITERATOR;
         ITERATOR begin() {return _contexts.begin();}
         ITERATOR end() {return _contexts.end();}
      private:
         UINT32 _nextIndexId = 0;
         UINT64 _freeIndexSlots = OSS_UINT64_MAX;

         _CONTEXT_MAP _contexts;
   };//indexContextMap
}//namespace vessel
}//nemespace engine

#endif//VESSEL_INDEX_CONTEXT_MAP_H_
