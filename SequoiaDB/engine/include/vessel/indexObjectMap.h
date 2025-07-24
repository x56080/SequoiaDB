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

   Source File Name = indexObjectMap.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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

         INT32 beginToTruncateAll(UINT64 lsn);

         void endToTruncateAll();

         INT32 beginToRemoveAll(UINT64 lsn);

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
