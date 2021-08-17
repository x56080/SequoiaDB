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
         struct keyOperation : public SDBObject
         {
            keyOperation(){}
            ~keyOperation(){}

            keyOperation(const keyOperation &o):
            entry(o.entry),
            inserting(o.inserting),
            discarded(o.discarded)
            {}

            keyOperation &operator=(const keyOperation &o)
            {
               entry = o.entry;
               inserting = o.inserting;
               discarded = o.discarded;
               return *this;
            }

            scanEntry entry;
            bson::BSONArray inserting;
            bson::BSONArray discarded;
         };//struct keyOperation

         typedef ossPoolMap<scanEntry, keyOperation> _KEY_MAP;

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

         BOOLEAN testIfDuplicatedIfBuilding(const strSlice &name,
                                            const indexKeyPattern &pattern)const;

         BOOLEAN insertKeys(const scanEntry &entry,
                            const bson::BSONObjSet &keys);

         BOOLEAN updateKeys(const scanEntry &entry,
                            const bson::BSONObjSet &oldKeys,
                            const bson::BSONObjSet &newKeys);

         BOOLEAN deleteKeys(const scanEntry &entry,
                            const bson::BSONObjSet &keys);

         /// return false when keys not empty
         BOOLEAN endToBuildCurrentRangeOrPopKeys(ossPoolList<keyOperation> &keys);

         void updateRebuildingHighBound(const scanEntry &highBound);

         BOOLEAN getNextRebuildingRangeBound(scanEntry &bound);

         void terminateBuilding();

      private:
         BOOLEAN upsertKeyOperation(const scanEntry &entry,
                                    const bson::BSONObjSet *inserting,
                                    const bson::BSONObjSet *discarded);

         BOOLEAN isRebuilding(const scanEntry &entry,
                              BOOLEAN &rebuilded)const;

      private:
         INT32 _indexSlot = -1;
         indexObject _obj;
         INDEX_STATUS _status = INDEX_STATUS_INVALID;
         ossSpinXLatch _latch;

         /// scanning range is [_rebuildingLow, _rebuildingHigh)
         scanEntry _rebuildingLow;
         scanEntry _rebuildingHigh;

         _KEY_MAP _keys;
         
   };//class unstableIndexContext

   typedef ossPoolMap<INT32, unstableIndexContext*> UNSTABLE_INDEX_MAP;

   class unstableIndexContextMap : public SDBObject
   {
      public:
         unstableIndexContextMap();
         ~unstableIndexContextMap();
         unstableIndexContextMap(const unstableIndexContextMap &) = delete;
         unstableIndexContextMap &operator=(const unstableIndexContextMap &) = delete;

      public:
         INT32 insert(INT32 indexSlot,
                      const indexObject &obj,
                      INDEX_STATUS status,
                      unstableIndexContext **context=NULL);


         void erase(INT32 indexSlot);

         /// 
         unstableIndexContext *find(INT32 indexSlot)const;

         OSS_INLINE UINT64 getBitmap()const
         {
            return _bitmap;
         }

      private:
         typedef ossPoolMap<INT32, unstableIndexContext*> UNSTABLE_INDEX_MAP;

      private:
         UINT64 _bitmap = 0;
         UNSTABLE_INDEX_MAP _map;
   };//class unstableIndexContextMap
}//namespace vessel
}//namespace engine

#endif//VESSEL_UNSTABLE_INDEX_CONTEXT_H_
