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

#include "vessel/indexObject.h"
#include "vessel/fixedBitset.hpp"
#include "vessel/buildingIndexContext.h"
#include "ossMemPool.hpp"

namespace engine
{
namespace vessel
{
   class indexObjectMap : public SDBObject
   {
      public:
         indexObjectMap() = default;
         ~indexObjectMap() = default;
         indexObjectMap(const indexObjectMap &) = delete;
         indexObjectMap &operator=(const indexObjectMap &) = delete;

      public:
         OSS_INLINE BOOLEAN isEmpty()const
         {
            return _objects.empty();
         }

         INT32 init(UINT32 maxLogicalId, const ossPoolList<bson::BSONObj> &objs);

         void reset();    

         void destroy(UINT32 indexLid);

         BOOLEAN isAllowedToCreateMore() const;
                     
         indexObject *getIndexObj(UINT32 indexLid);

         const indexObject *getIndexObj(UINT32 indexLid)const;

         indexObject *getIndexObj(const strSlice &name);

         indexObject *getIndexObjByInnerId(utilIdxInnerID innerId);

         buildingIndexContext *getBuildingCtx(UINT32 indexLid);

         BOOLEAN isIndexDuplicated(const indexProperties &properties)const;

         INT32 validateCreation(const indexProperties &properties) const;

         indexObject *abortCreating(UINT32 indexLid);

         void finishCreating(UINT32 indexLid);

         ossPoolList<bson::BSONObj> getObjEntries()const;

         INT32 insert(std::unique_ptr<indexObject> &&obj);

         INT32 insertBuildingObject(std::unique_ptr<indexObject> &&obj);

      private:
         using _UNIQUE_OBJ_PTR = std::unique_ptr<indexObject>;
         using _OBJECT_PTR_MAP = std::map<UINT32, _UNIQUE_OBJ_PTR>;

         using _BUILDING_CTX_PTR = std::unique_ptr<buildingIndexContext>;
         using _BUILDING_CTX_MAP = ossPoolMap<UINT32, _BUILDING_CTX_PTR>;

      public:
         const _OBJECT_PTR_MAP &getObjectMap()const {return _objects;}

         using ITERATOR = _OBJECT_PTR_MAP::iterator;
         using CONST_ITERATOR = _OBJECT_PTR_MAP::const_iterator;

         ITERATOR begin() {return _objects.begin();}
         CONST_ITERATOR cbegin()const {return _objects.cbegin();}

         ITERATOR end() {return _objects.end();}
         CONST_ITERATOR cend()const {return _objects.cend();}

      private:
         _OBJECT_PTR_MAP _objects;
         _BUILDING_CTX_MAP _buildingMap;
   };//class indexObjectMap
}//namespace vessel
}//nemespace engine

#endif//VESSEL_INDEX_OBJECT_MAP_H_
