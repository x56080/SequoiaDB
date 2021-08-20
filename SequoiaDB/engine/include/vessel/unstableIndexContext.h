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

   Source File Name = unstableIndexContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_UNSTABLE_INDEX_CONTEXT_H_
#define VESSEL_UNSTABLE_INDEX_CONTEXT_H_

#include "vessel/indexDef.h"
#include "ossLatch.hpp"
#include "../bson/bson.hpp"
#include "ossMemPool.hpp"
#include "vessel/scanEntry.h"
#include "vessel/indexObject.h"
#include "utilPooledObject.hpp"

namespace engine
{
namespace vessel
{
   class unstableIndexContext : public SDBObject
   {
      public:
         unstableIndexContext();
         ~unstableIndexContext();
         unstableIndexContext(const unstableIndexContext &) = delete;
         unstableIndexContext &operator=(const unstableIndexContext &) = delete;

      public:
         struct mergingKey : public _utilPooledObject
         {
            mergingKey(){}
            ~mergingKey(){}

            mergingKey(const mergingKey &o) = delete;

            mergingKey &operator=(const mergingKey &o) = delete;

            scanEntry entry;
            ossPoolList<bson::BSONObj> inserting;
            ossPoolList<bson::BSONObj> discarded;
         };//struct mergingKey

      public:
         INT32 init(INT32 indexSlot,
                    const indexObject &obj,
                    INDEX_STATUS status);

         void fini();

         INT32 getIndexSlot()const
         {
            return _indexSlot;
         }

         UINT32 getIndexId()const
         {
            return _obj.getIndexID();
         }

         INDEX_STATUS getStatus()const
         {
            return _status;
         }

         const indexObject &getIndexObj()const
         {
            return _obj;
         }

         const strSlice &getName()const
         {
            return _obj.getIndexName();
         }

         BOOLEAN isBuilding()const
         {
            return INDEX_STATUS_BUILDING == _status;
         }

         BOOLEAN isRemoving()const
         {
            return INDEX_STATUS_REMOVING == _status;
         }

         BOOLEAN isTruncating()const
         {
            return INDEX_STATUS_TRUNCATING == _status;
         }

         BOOLEAN testDuplicatedIfBuilding(const strSlice &name,
                                          const indexKeyPattern &pattern)const;

         /// must be building
         INT32 insertKeys(const scanEntry &entry,
                          const ossPoolList<bson::BSONObj> &keys,
                          BOOLEAN &refused);

         /// must be building
         INT32 updateKeys(const scanEntry &entry,
                          const ossPoolList<bson::BSONObj> &oldKeys,
                          const ossPoolList<bson::BSONObj> &newKeys,
                          BOOLEAN &refuse);

         /// must be building
         INT32 deleteKeys(const scanEntry &entry,
                          const ossPoolList<bson::BSONObj> &keys,
                          BOOLEAN &refuse);

         /// return false when keys not empty
         BOOLEAN endToBuildCurrentRangeOrPopKeys(ossPoolList<mergingKey*> &keys);

         void updateRebuildingHighBound(const scanEntry &highBound);

         BOOLEAN getNextRebuildingRangeBound(scanEntry &bound);

         void terminateBuilding(BOOLEAN remove);

      private:
         INT32 upsertBuildingKeys(const scanEntry &entry,
                                  const ossPoolList<bson::BSONObj> *inserting,
                                  const ossPoolList<bson::BSONObj> *discarded,
                                  BOOLEAN &refused);

         INT32 getEntryBuildingStatus(const scanEntry &entry)const;

      private:
         INT32 _indexSlot = -1;
         indexObject _obj;
         INDEX_STATUS _status = INDEX_STATUS_INVALID;
         ossSpinXLatch _latch;

         /// scanning range is [_rebuildingLow, _rebuildingHigh)
         scanEntry _buildingLow;
         scanEntry _buildingHigh;

         ossPoolList<mergingKey *> _mergingKeys;
         
   };//class unstableIndexContext
}//namespace vessel
}//namespace engine

#endif//VESSEL_UNSTABLE_INDEX_CONTEXT_H_
