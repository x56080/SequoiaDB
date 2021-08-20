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
      _buildingLow.reset();
      _buildingHigh.reset();
      for (ossPoolList<mergingKey *>::iterator itr = _mergingKeys.begin();
           itr != _mergingKeys.end(); ++itr)
      {
         SDB_OSS_DEL *itr;
      }
      _mergingKeys.clear();
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

   INT32 unstableIndexContext::insertKeys(const scanEntry &entry,
                                          const ossPoolList<bson::BSONObj> &keys,
                                          BOOLEAN &refused)
   {
      return upsertBuildingKeys(entry, &keys, NULL, refused);
   }

   INT32 unstableIndexContext::updateKeys(const scanEntry &entry,
                                          const ossPoolList<bson::BSONObj> &oldKeys,
                                          const ossPoolList<bson::BSONObj> &newKeys,
                                          BOOLEAN &refused)
   {
      return upsertBuildingKeys(entry, &newKeys, &oldKeys, refused);
   }

   INT32 unstableIndexContext::deleteKeys(const scanEntry &entry,
                                          const ossPoolList<bson::BSONObj> &keys,
                                          BOOLEAN &refused)
   {
      return upsertBuildingKeys(entry, NULL, &keys, refused);
   }

   BOOLEAN unstableIndexContext::endToBuildCurrentRangeOrPopKeys(ossPoolList<mergingKey *> &keys)
   {
      BOOLEAN r = FALSE;
      SDB_ASSERT(keys.empty(), "must be empty");
      
      ossXLatchGuard guard(&_latch);

      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      SDB_ASSERT(_buildingLow <= _buildingHigh, "impossible");
      if (!_mergingKeys.empty())
      {
         for (ossPoolList<mergingKey *>::const_iterator itr = _mergingKeys.begin();
              itr != _mergingKeys.end(); ++itr)
         {
            keys.push_back(*itr);
         }
         _mergingKeys.clear();
      }
      else
      {
         _buildingLow = _buildingHigh;
         r = TRUE;
      }
   
      return r;
   }

   void unstableIndexContext::updateRebuildingHighBound(const scanEntry &highBound)
   {
      ossXLatchGuard guard(&_latch);
      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      SDB_ASSERT(_buildingHigh < highBound, "must be over current high bound");
      _buildingHigh = highBound;
      return;
   }

   BOOLEAN unstableIndexContext::getNextRebuildingRangeBound(scanEntry &bound)
   {
      BOOLEAN r = FALSE;
      ossXLatchGuard guard(&_latch);
      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be rebuilding");
      if (INDEX_STATUS_BUILDING == getStatus() &&
          _buildingLow == _buildingHigh)
      {
         bound = _buildingLow;
         r = TRUE;
      }
      return r;
   }

   INT32 unstableIndexContext::upsertBuildingKeys(const scanEntry &entry,
                                                  const ossPoolList<bson::BSONObj> *inserting,
                                                  const ossPoolList<bson::BSONObj> *discarded,
                                                  BOOLEAN &refused)
   {
      INT32 rc = SDB_OK;
      SDB_ASSERT(isBuilding(), "must be building");
      SDB_ASSERT(!(NULL == inserting && NULL == discarded), "impossible");

      mergingKey *mk = NULL;
      ossXLatchGuard guard(&_latch);
      INT32 buildingStatus = getEntryBuildingStatus(entry);
      if (buildingStatus < 0)
      {
         refused = TRUE;
         goto done;
      }
      else if (0 < buildingStatus)
      {
         refused = FALSE;
         goto done;
      }

      mk = SDB_OSS_NEW mergingKey();
      if (NULL == mk)
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      mk->entry = entry;
      if (NULL != inserting)
      {
         for (ossPoolList<bson::BSONObj>::const_iterator itr = inserting->begin();
               itr != inserting->end(); ++itr)
         {
            /// Can not use getOwned to save key here.
            /// We do not know when key obj released by user thread.
            mk->inserting.push_back(itr->copy());
         }
      }
      if (NULL != discarded)
      {
         for (ossPoolList<bson::BSONObj>::const_iterator itr = discarded->begin();
               itr != discarded->end(); ++itr)
         {
            mk->discarded.push_back(itr->copy());
         }
      }
   done:
      return rc;
   error:
      SAFE_OSS_DELETE(mk);
      goto done;
   }

   INT32 unstableIndexContext::getEntryBuildingStatus(const scanEntry &entry)const
   {
      SDB_ASSERT(INDEX_STATUS_BUILDING == getStatus(), "must be building");
      if (entry < _buildingLow)
      {
         return -1;
      }
      else if(entry < _buildingHigh)
      {
         return 0;
      }
      else
      {
         return 1;
      }
   }

   BOOLEAN unstableIndexContext::testDuplicatedIfBuilding(const strSlice &name,
                                                          const indexKeyPattern &pattern)const
   {
      return INDEX_STATUS_BUILDING == getStatus() &&
              (_obj.getIndexName() == name ||
              pattern.isCoveredBy(_obj.getPattern()));
   }

   void unstableIndexContext::terminateBuilding(BOOLEAN remove)
   {
      SDB_ASSERT(isBuilding(), "must be building");
      if (remove)
      {
         _status = INDEX_STATUS_REMOVING;
      }
      else
      {
         _status = INDEX_STATUS_TRUNCATING;
      }
   }
}//namespace vessel
}//namespace engine