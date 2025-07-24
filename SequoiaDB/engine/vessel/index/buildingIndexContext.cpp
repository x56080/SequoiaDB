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

   Source File Name = buildingIndexContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/buildingIndexContext.h"
#include "pdTrace.hpp"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   buildingIndexContext::buildingIndexContext(indexObject *obj):
   _obj(obj)
   {
      SDB_ASSERT(nullptr != obj && obj->isValid(), "can not be invalid");
   }

   INT32 buildingIndexContext::merge(dmlIndexRequest *ir,
                                     UINT32 seq,
                                     const recordID &rid,
                                     const DPS_LSN_OFFSET &lsn,
                                     const DPS_TRANS_ID &transID)
   {
      INT32 rc = SDB_OK;
      INT32 buildingRes = 0;
      INDEX_MERGING_RECORD record;
      std::unique_lock<std::mutex> lock(_mutex, std::defer_lock);

      if (OSS_UNLIKELY(nullptr == ir ||
                       !ir->isValid() ||
                       !rid.isValid() ||
                       DPS_INVALID_LSN_OFFSET == lsn))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      SDB_ASSERT(ir->getObject() == _obj, "must be same");
      SDB_ASSERT(!ir->isExecuted(), "already been executed");

      lock.lock();

      buildingRes = _getBuildingStatus(seq);
      if (0 < buildingRes)
      {
         /// not builded yet, pretend to be executed.
         ir->setExecuted();
         goto done;
      }
      else if (buildingRes < 0 && !_obj->getProperties().isUnique())
      {
         /// always insert non-uniuqe index entries by dml thread.
         goto done;
      }
       

      /// building or (unique && builded)
      record.reset(SDB_OSS_NEW indexMergingRecord());
      if (OSS_UNLIKELY(!record))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      ir->setExecuted();
      record->rid = rid;
      record->lsn = lsn;
      record->transID = transID;
      
      for (ossPoolList<bson::BSONObj>::const_iterator itr = ir->getKeysToInsert().begin();
           itr != ir->getKeysToInsert().end(); ++itr)
      {
         record->inserting.push_back(itr->getOwned());
      }
      
      for (ossPoolList<bson::BSONObj>::const_iterator itr = ir->getKeysToRemove().begin();
           itr != ir->getKeysToRemove().end(); ++itr)
      {
         record->discarded.push_back(itr->getOwned());
      }

      _ml.push_back(std::move(record));
   done:
      return rc;
   error:
      goto done;
   }

   INT32 buildingIndexContext::_getBuildingStatus(UINT32 seq)const
   {
      if (seq < _low)
      {
         ///already builded
         return -1;
      }
      else if(seq < _high)
      {
         /// scanning
         return 0;
      }
      else
      {
         /// not scanned yet
         return 1;
      }
   }

   BOOLEAN buildingIndexContext::endToBuildCurrentRange(INDEX_MERGING_LIST &ml)
   {
      std::unique_lock<std::mutex> lock(_mutex);
      SDB_ASSERT(isScanning(), "not scanning");
      BOOLEAN r = _ml.empty();
      
      if (!r)
      {
         ml.splice(ml.end(), _ml);
      }
      else
      {
         _low = _high;
      }

      return r;
   }

   UINT32 buildingIndexContext::slideHigh(UINT32 size)
   {
      SDB_ASSERT(0 < size, "can not be invalid");
      std::unique_lock<std::mutex> lock(_mutex);
      _high += size;
      return _high;
   }
} // namespace vessel

} // namespace engine
