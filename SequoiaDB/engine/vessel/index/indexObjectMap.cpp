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

   Source File Name = indexObjectMap.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/indexObjectMap.h"
#include "ossLikely.hpp"
#include "pdTrace.hpp"
#include "vessel/buildingIndexContext.h"
#include "ixm_common.hpp"

namespace engine
{
namespace vessel
{
   void indexObjectMap::reset()
   {
      _objects.clear();
      _buildingMap.clear();
      return;
   }

   BOOLEAN indexObjectMap::isAllowedToCreateMore() const
   {
      return _objects.size() < MAX_INDEX_COUNT_PER_CL;
   }

   INT32 indexObjectMap::validateCreation(const indexProperties &properties) const
   {
      INT32 rc = SDB_OK;
      
      if (!isAllowedToCreateMore())
      {
         rc = SDB_DMS_MAX_INDEX;
         PD_LOG(PDERROR, "no more index allowed");
         goto error;
      }
      else if (OSS_UNLIKELY(!properties.isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (isIndexDuplicated(properties))
      {
         rc = SDB_IXM_EXIST;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   // INT32 indexObjectMap::insert(INDEX_OBJ_PTR &&obj)
   // {
   //    INT32 rc = SDB_OK;

   //    if (OSS_UNLIKELY(!obj ||
   //                     !obj->isValid()))
   //    {
   //       rc = SDB_INVALIDARG;
   //       goto error;
   //    }

   //    if (!_allocator.test(obj->getIndexSlot()))
   //    {
   //       PD_LOG(PDERROR, "index slot[%d] not free", obj->getIndexSlot());
   //       rc = SDB_VESSEL_DUPLICATED_KEY;
   //       goto error;
   //    }
   //    else if (0 != _objects.count(obj->getLogicalID()))
   //    {
   //       PD_LOG(PDERROR, "duplicated logical id[%d]", obj->getLogicalID());
   //       rc = SDB_VESSEL_DUPLICATED_KEY;
   //       goto error;
   //    }

   //    if (INDEX_STATUS_BUILDING == obj->getStatus())
   //    {
   //       _BUILDING_CTX_PTR ctx(SDB_OSS_NEW buildingIndexContext(obj.get()));
   //       if (OSS_UNLIKELY(!ctx))
   //       {
   //          PD_LOG(PDERROR, "failed to allocate mem.");
   //          rc = SDB_OOM;
   //          goto error;
   //       }
   //       _buildingMap[obj->getLogicalID()] = std::move(ctx);
   //    }

   //    /// do not goto error from here
   //    _allocator.clear(obj->getIndexSlot());
   //    setMaxIndexLid(obj->getLogicalID());
   //    _objects[obj->getLogicalID()] = std::move(obj);
   // done:
   //    return rc;
   // error:
   //    goto done;
   // }

   void indexObjectMap::destroy(UINT32 indexLid)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      _buildingMap.erase(indexLid);
      _objects.erase(indexLid);
      return;
   }

   indexObject *indexObjectMap::getIndexObj(UINT32 indexLid)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      indexObject *obj = nullptr;
      _OBJECT_PTR_MAP::iterator itr = _objects.find(indexLid);
      if (_objects.end() != itr)
      {
         obj = itr->second.get();
      }
      return obj;
   }

   const indexObject *indexObjectMap::getIndexObj(UINT32 indexLid)const
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      indexObject *obj = nullptr;
      _OBJECT_PTR_MAP::const_iterator itr = _objects.find(indexLid);
      if (_objects.end() != itr)
      {
         obj = itr->second.get();
      }
      return obj;
   }

   indexObject *indexObjectMap::getIndexObj(const strSlice &name)
   {
      SDB_ASSERT(!name.empty(), "can not be invalid");
      indexObject *obj = nullptr;
      for (auto itr = _objects.begin(); itr != _objects.end(); ++itr)
      {
         strSlice nameSlice = itr->second->getProperties().getNameSlice();
         if (nameSlice == name)
         {
            obj = itr->second.get();
            break;
         }
      }

      return obj;
   }

   indexObject *indexObjectMap::getIndexObjByInnerId(utilIdxInnerID innerId)
   {
      indexObject *obj = nullptr;
      for (auto itr = _objects.begin(); itr != _objects.end(); ++itr)
      {
         utilIdxInnerID id = itr->second->getProperties().getInnerID();
         if (id == innerId)
         {
            obj = itr->second.get();
            break;
         }
      }

      return obj;
   }

   buildingIndexContext *indexObjectMap::getBuildingCtx(UINT32 indexLid)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      buildingIndexContext *ctx = nullptr;
      _BUILDING_CTX_MAP::iterator itr = _buildingMap.find(indexLid);
      if (_buildingMap.end() != itr)
      {
         ctx = itr->second.get();
      }
      return ctx;
   }

   indexObject * indexObjectMap::abortCreating(UINT32 indexLid)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      indexObject *obj = getIndexObj(indexLid);
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "can not be invalid");
      _buildingMap.erase(indexLid);
      obj->setStatus(INDEX_STATUS_REMOVING);
      return obj;
   }

   void indexObjectMap::finishCreating(UINT32 indexLid)
   {
      SDB_ASSERT(INVALID_LOGICAL_INDEX_ID != indexLid, "can not be invalid");
      indexObject *obj = getIndexObj(indexLid);
      SDB_ASSERT(nullptr != obj && obj->isBuilding(), "can not be invalid");
      _buildingMap.erase(indexLid);
      obj->setStatus(INDEX_STATUS_NORMAL);
   }

   BOOLEAN indexObjectMap::isIndexDuplicated(const indexProperties &properties)const
   {
      SDB_ASSERT(properties.isValid(), "can not be invalid");
      BOOLEAN r = FALSE;

      for (auto itr = _objects.cbegin(); itr != _objects.cend(); ++itr)
      {
         const indexObject *obj = itr->second.get();
         if (!obj->isNormal() && !obj->isBuilding())
         {
            continue;
         }
         else if (properties.getName() == obj->getProperties().getName())
         {
            PD_LOG(PDINFO, "duplicated index name[%s]", properties.getName().c_str());
            r = TRUE;
            break;
         }
         else if (properties.getPattern().isCoveredBy(obj->getProperties().getPattern()))
         {
            PD_LOG(PDINFO, "duplicated index pattern[%s] of index[%s]",
                   obj->getProperties().getPattern().getPattern().toPoolString().c_str(),
                   obj->getProperties().getName().c_str());
            r = TRUE;
            break;
         }
      }

      return r;
   }

   ossPoolList<bson::BSONObj> indexObjectMap::getObjEntries()const
   {
      ossPoolList<bson::BSONObj> l;
      for (auto itr = _objects.cbegin(); itr != _objects.cend(); ++itr)
      {
         bson::BSONObj o = itr->second->toBson();
         l.push_back(o);
      }

      return std::move(l);
   }

   INT32 indexObjectMap::insert(std::unique_ptr<indexObject> &&obj)
   {
      INT32 rc = SDB_OK;
      if (OSS_UNLIKELY(!obj ||
                       !obj->isValid()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if (!isAllowedToCreateMore())
      {
         rc = SDB_DMS_MAX_INDEX;
         PD_LOG(PDERROR, "not allowed to create more");
         goto error;
      }
      else if (0 < _objects.count(obj->getLogicalID()))
      {
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      _objects[obj->getLogicalID()] = std::move(obj);
      
   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexObjectMap::insertBuildingObject(std::unique_ptr<indexObject> &&obj)
   {
      INT32 rc = SDB_OK;
      _BUILDING_CTX_PTR ctx;

      if (OSS_UNLIKELY(!obj ||
                       !obj->isValid() ||
                       !obj->isBuilding()))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }
      else if(!isAllowedToCreateMore())
      {
         rc = SDB_DMS_MAX_INDEX;
         PD_LOG(PDERROR, "not allowed to create more");
         goto error;
      }
      else if (0 < _objects.count(obj->getLogicalID()) ||
               0 < _buildingMap.count(obj->getLogicalID()))
      {
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      ctx.reset(SDB_OSS_NEW buildingIndexContext(obj.get()));
      if (OSS_UNLIKELY(!ctx))
      {
         PD_LOG(PDERROR, "failed to allocate mem.");
         rc = SDB_OOM;
         goto error;
      }

      _buildingMap[obj->getLogicalID()] = std::move(ctx);
      _objects[obj->getLogicalID()] = std::move(obj);

   done:
      return rc;
   error:
      goto done;
   }

   INT32 indexObjectMap::beginToTruncateAll(UINT64 lsn)
   {
      INT32 rc = SDB_OK;
      for (auto itr = _objects.cbegin(); itr != _objects.cend(); ++itr)
      {
         const indexObject *obj = itr->second.get();
         if (!obj->isNormal())
         {
            PD_LOG(PDERROR, "unnormal index[%s] found", obj->getProperties().getName().c_str());
            rc = SDB_INVALID_OPERATION;
            goto error;
         }
      }

      SDB_ASSERT(_buildingMap.empty(), "impossible");
      for (auto itr = _objects.begin(); itr != _objects.end(); ++itr)
      {
         itr->second->setStatus(INDEX_STATUS_TRUNCATING);
         itr->second->resetRebornLSN(lsn);
      }
   done:
      return rc;
   error:
      goto done;
   }

   void indexObjectMap::endToTruncateAll()
   {
      SDB_ASSERT(_buildingMap.empty(), "must be empty");
      for (auto itr = _objects.begin(); itr != _objects.end(); ++itr)
      {
         SDB_ASSERT(INDEX_STATUS_TRUNCATING == itr->second->getStatus(), "must be truncating");
         itr->second->setStatus(INDEX_STATUS_NORMAL);
      }
   }

   INT32 indexObjectMap::beginToRemoveAll(UINT64 lsn)
   {
      INT32 rc = SDB_OK;
      for (auto itr = _objects.cbegin(); itr != _objects.cend(); ++itr)
      {
         const indexObject *obj = itr->second.get();
         if (!obj->isNormal())
         {
            PD_LOG(PDERROR, "unnormal index[%s] found", obj->getProperties().getName().c_str());
            rc = SDB_INVALID_OPERATION;
            goto error;
         }
      }

      SDB_ASSERT(_buildingMap.empty(), "impossible");
      for (auto itr = _objects.begin(); itr != _objects.end(); ++itr)
      {
         itr->second->setStatus(INDEX_STATUS_REMOVING);
         itr->second->resetRebornLSN(lsn);
      }
   done:
      return rc;
   error:
      goto done;
   }

}//namespace vessel
}//namespace engine
