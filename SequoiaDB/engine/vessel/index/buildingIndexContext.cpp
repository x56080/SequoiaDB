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

   Source File Name = buildingIndexContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/buildingIndexContext.h"
#include "pdTrace.hpp"
#include "ossLatchGuard.hpp"

namespace engine
{
namespace vessel
{
   buildingIndexContext::~buildingIndexContext()
   {
      _mrl.clear();
   }

   void buildingIndexContext::fini()
   {
      _low.reset();
      _high.reset();
      _mrl.clear();
      _terminated = FALSE;
      return;
   }

   INT32 buildingIndexContext::insert(BOOLEAN enforce,
                                      const scanEntry &entry,
                                      PAGE_ID lpid,
                                      DPS_LSN_OFFSET lsn,
                                      const DPS_TRANS_ID &transID,
                                      const ossPoolList<bson::BSONObj> &keys,
                                      BOOLEAN &refused)
   {
      return merge(enforce, entry, lpid, lsn, transID, &keys, NULL, refused);
   }

   INT32 buildingIndexContext::update(BOOLEAN enforce,
                                       const scanEntry &entry,
                                       PAGE_ID lpid,
                                       DPS_LSN_OFFSET lsn,
                                       const DPS_TRANS_ID &transID,
                                       const ossPoolList<bson::BSONObj> &oldKeys,
                                       const ossPoolList<bson::BSONObj> &newKeys,
                                       BOOLEAN &refused)
   {
      return merge(enforce, entry, lpid, lsn, transID, &oldKeys, &newKeys, refused);
   }

   INT32 buildingIndexContext::remove(BOOLEAN enforce,
                                       const scanEntry &entry,
                                       PAGE_ID lpid,
                                       DPS_LSN_OFFSET lsn,
                                       const DPS_TRANS_ID &transID,
                                       const ossPoolList<bson::BSONObj> &keys,
                                       BOOLEAN &refused)
   {
      return merge(enforce, entry, lpid, lsn, transID, NULL, &keys, refused);
   }

   INT32 buildingIndexContext::merge(BOOLEAN enforce,
                                     const scanEntry &entry,
                                     PAGE_ID lpid,
                                     DPS_LSN_OFFSET lsn,
                                     const DPS_TRANS_ID &transID,
                                     const ossPoolList<bson::BSONObj> *inserting,
                                     const ossPoolList<bson::BSONObj> *discarded,
                                     BOOLEAN &refused)
   {
      INT32 rc = SDB_OK;
      INT32 buildingRes = 0;
      indexMergingRecord *mr = NULL;
      ossXLatchGuard guard(&_latch, FALSE);

      if (OSS_UNLIKELY(INVALID_PAGE_ID == lpid ||
                       DPS_INVALID_LSN_OFFSET == lsn ||
                       (NULL == inserting && NULL == discarded)))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      guard.lock();

      buildingRes = getEntryBuildingStatus(entry);
      if (buildingRes < 0)
      {
         if (!enforce)
         {
            refused = TRUE;
            goto done;
         }
      }
      else if (0 < buildingRes)
      {
         /// /// not builded yet, pretend to be pushed.
         goto done;
      }

      /// building or (enforce && builded)
      mr = SDB_OSS_NEW indexMergingRecord();
      if (OSS_UNLIKELY(NULL == mr))
      {
         PD_LOG(PDERROR, "failed to allocate mem");
         rc = SDB_OOM;
         goto error;
      }

      mr->entry = entry;
      mr->lpid = lpid;
      mr->lsn = lsn;
      mr->transID = transID;
      if (NULL != inserting)
      {
         ossPoolList<bson::BSONObj>::const_iterator itr = inserting->begin();
         for (; itr != inserting->end(); ++itr)
         {
            /// Can not use getOwned to save keys here.
            /// We do not know when key obj released by user thread.
            mr->inserting.push_back(itr->copy());
         }
      }
      if (NULL != discarded)
      {
         ossPoolList<bson::BSONObj>::const_iterator itr = discarded->begin();
         for (; itr != discarded->end(); ++itr)
         {
            /// Can not use getOwned to save keys here.
            /// We do not know when key obj released by user thread.
            mr->discarded.push_back(itr->copy());
         }
      }

      _mrl.rl.push_back(mr);
   done:
      return rc;
   error:
      goto done;
   }

   INT32 buildingIndexContext::getEntryBuildingStatus(const scanEntry &entry)const
   {
      if (entry < _low)
      {
         return -1;
      }
      else if(entry < _high)
      {
         return 0;
      }
      else
      {
         return 1;
      }
   }

   BOOLEAN buildingIndexContext::endToBuildCurrentRange(indexMergingRecordList &mrl)
   {
      SDB_ASSERT(_low != _high, "already ended");
      SDB_ASSERT(mrl.rl.empty(), "must be empty");
      ossXLatchGuard guard(&_latch);

      BOOLEAN r = _mrl.rl.empty();
      
      if (!r)
      {
         mrl.rl = std::move(_mrl.rl);
      }
      else
      {
         _low = _high;
      }

      return r;
   }

   void buildingIndexContext::updateBuildingHighBound(const scanEntry &entry)
   {
      ossXLatchGuard guard(&_latch);
      SDB_ASSERT(_high < entry, "must be over current high");
      _high = entry;
      return;
   }

   BOOLEAN buildingIndexContext::getNextBuildingBound(scanEntry &bound)const
   {
      if (_low != _high)
      {
         return FALSE;
      }
      bound = _low;
      return TRUE;
   }
} // namespace vessel

} // namespace engine
