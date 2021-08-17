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

   Source File Name = unstableIndexContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/unstableIndexContext.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   unstableIndexContext::unstableIndexContext()
   {}
   
   unstableIndexContext::~unstableIndexContext()
   {}

   void unstableIndexContext::fini()
   {
      _indexSlot = -1;
      _obj.fini();
      _status = INDEX_STATUS_INVALID;
      _rebuildingLow.reset();
      _rebuildingHigh.reset();
      _keys.clear();
   }

   INT32 unstableIndexContext::init(INT32 indexSlot,
                                    const indexObject &obj,
                                    INDEX_STATUS status)
   {
      INT32 rc = SDB_OK;
      fini();

      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       !obj.isValid() ||
                       INDEX_STATUS_INVALID ==status ||
                       INDEX_STATUS_NORMAL == status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _indexSlot = indexSlot;
      _obj.shallowCopy(obj);
      _obj.getOwned();
      _status = status;
   done:
      return rc;
   error:
      fini();
      goto done;
   }

   BOOLEAN unstableIndexContext::insertKeys(const scanEntry &entry,
                                           const bson::BSONObjSet &keys)
   {
      return upsertKeyOperation(entry, &keys, NULL);
   }

   BOOLEAN unstableIndexContext::updateKeys(const scanEntry &entry,
                                            const bson::BSONObjSet &oldKeys,
                                            const bson::BSONObjSet &newKeys)
   {
      return upsertKeyOperation(entry, &newKeys, &oldKeys);
   }

   BOOLEAN unstableIndexContext::deleteKeys(const scanEntry &entry,
                                            const bson::BSONObjSet &keys)
   {
      return upsertKeyOperation(entry, NULL, &keys);
   }

   BOOLEAN unstableIndexContext::endToBuildCurrentRangeOrPopKeys(ossPoolList<keyOperation> &keys)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(!keys.empty(), "must be empty");
      
      ossXLatchGuard guard(&_latch);

      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      SDB_ASSERT(_rebuildingLow <= _rebuildingHigh, "impossible");
      if (!_keys.empty())
      {
         for (_KEY_MAP::const_iterator itr = _keys.begin();
              itr != _keys.end(); ++itr)
         {
            keys.push_back(itr->second);
         }
         _keys.clear();
      }
      else
      {
         _rebuildingLow = _rebuildingHigh;
         r = TRUE;
      }
   
      return r;
   }

   void unstableIndexContext::updateRebuildingHighBound(const scanEntry &highBound)
   {
      ossXLatchGuard guard(&_latch);
      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      SDB_ASSERT(_rebuildingHigh < highBound, "must be over current high bound");
      _rebuildingHigh = highBound;
      return;
   }

   BOOLEAN unstableIndexContext::getNextRebuildingRangeBound(scanEntry &bound)
   {
      BOOLEAN r = FALSE;
      ossXLatchGuard guard(&_latch);
      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      if (INDEX_STATUS_BUILDING == getStatus() &&
          _rebuildingLow == _rebuildingHigh)
      {
         bound = _rebuildingLow;
         r = TRUE;
      }
      return r;
   }

   BOOLEAN unstableIndexContext::upsertKeyOperation(const scanEntry &entry,
                                                    const bson::BSONObjSet *inserting,
                                                    const bson::BSONObjSet *discarded)
   {
      BOOLEAN r = FALSE;
      BOOLEAN rebuilded = FALSE;
      SDB_ASSERT(!(NULL == inserting && NULL == discarded), "impossible");

      ossXLatchGuard guard(&_latch);

      if (INDEX_STATUS_BUILDING != getStatus())
      {
         r = TRUE;
         goto done;
      }

      if (isRebuilding(entry, rebuilded))
      {
         keyOperation op;
         op.entry = entry;

         bson::BSONArrayBuilder builder;

         if (NULL != inserting)
         {
            for (bson::BSONObjSet::const_iterator itr = inserting->begin();
               itr != inserting->end(); ++itr)
            {
               builder.append(*itr);
            }
         }
         op.inserting = builder.arr();
         
         if (NULL != discarded)
         {
            for (bson::BSONObjSet::const_iterator itr = discarded->begin();
               itr != discarded->end(); ++itr)
            {
               builder.append(*itr);
            }
         }
         op.discarded = builder.arr();

         _keys[entry] = op;
         r = TRUE;
      }
      else if (!rebuilded)
      {
         r = TRUE;
      }
   done:
      return r;
   }

   BOOLEAN unstableIndexContext::isRebuilding(const scanEntry &entry,
                                               BOOLEAN &rebuilded)const
   {
      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      BOOLEAN r = _rebuildingLow <= entry && entry < _rebuildingHigh;
      rebuilded = entry < _rebuildingLow;
      return r;
   }

   BOOLEAN unstableIndexContext::testIfDuplicatedIfBuilding(const strSlice &name,
                                                            const indexKeyPattern &pattern)const
   {
      return INDEX_STATUS_BUILDING == getStatus() &&
              (_obj.getIndexName() == name ||
              pattern.isCoveredBy(_obj.getPattern()));
   }

   ///////////////unstableIndexContextMap
   unstableIndexContextMap::unstableIndexContextMap()
   {}

   unstableIndexContextMap::~unstableIndexContextMap()
   {
      UNSTABLE_INDEX_MAP::const_iterator itr = _map.begin();
      for (; itr != _map.end(); ++itr)
      {
         if (NULL != itr->second)
         {
            SDB_OSS_DEL itr->second;
         }
      }
      _map.clear();
   }

   INT32 unstableIndexContextMap::insert(INT32 indexSlot,
                                         const indexObject &obj,
                                         INDEX_STATUS status,
                                         unstableIndexContext **context)
   {
      INT32 rc = SDB_OK;
      unstableIndexContext *uic = NULL;
      if (OSS_UNLIKELY(!isValidIndexSlot(indexSlot) ||
                       !obj.isValid() ||
                       INDEX_STATUS_INVALID == status ||
                       INDEX_STATUS_NORMAL == status))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      uic = SDB_OSS_NEW unstableIndexContext();
      if (NULL == uic)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      rc = uic->init(indexSlot, obj, status);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init context:%d", rc);
         goto error;
      }

      if (!_map.insert(std::make_pair(indexSlot, uic)).second)
      {
         PD_LOG(PDERROR, "duplicated index slot:%d", indexSlot);
         rc = SDB_VESSEL_DUPLICATED_KEY;
         goto error;
      }

      if (NULL != context)
      {
         *context = uic;
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(uic);
      goto done;
   }

   void unstableIndexContextMap::erase(INT32 indexSlot)
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      UNSTABLE_INDEX_MAP::const_iterator itr = _map.find(indexSlot);
      if (_map.end() != itr)
      {
         SDB_OSS_DEL itr->second;
         _map.erase(itr);
      }
      return;
   }

   unstableIndexContext *unstableIndexContextMap::find(INT32 indexSlot)const
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      unstableIndexContext *context = NULL;
      UNSTABLE_INDEX_MAP::const_iterator itr = _map.find(indexSlot);
      if (_map.end() != itr)
      {
         context = itr->second;
      }
      return context;
   }

   unstableIndexContext *unstableIndexContextMap::findBuidingContext(INT32 indexSlot)const
   {
      SDB_ASSERT(isValidIndexSlot(indexSlot), "can not be invalid");
      unstableIndexContext *context = find(indexSlot);
      if (NULL != context && INDEX_STATUS_BUILDING != context->getStatus())
      {
         context = NULL;
      }
      return context;
   }
}//namespace vessel
}//namespace engine