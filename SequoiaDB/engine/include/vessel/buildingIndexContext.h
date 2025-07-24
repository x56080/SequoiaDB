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

   Source File Name = buildingIndexContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BUILDING_INDEX_CONTEXT_H_
#define VESSEL_BUILDING_INDEX_CONTEXT_H_

#include "vessel/indexMergingRecord.h"
#include "vessel/dmlIndexRequest.h"

#include <mutex>

namespace engine
{
namespace vessel
{
   class indexObject;
   class buildingIndexContext : public SDBObject
   {
      public:
         buildingIndexContext(indexObject *obj);
         ~buildingIndexContext() = default;
         buildingIndexContext(const buildingIndexContext &) = delete;
         buildingIndexContext &operator=(const buildingIndexContext &) = delete;

      public:
         OSS_INLINE const indexObject *getIndexObj()const {return _obj;}
         OSS_INLINE indexObject *getIndexObj() {return _obj;}
         OSS_INLINE BOOLEAN isScanning()const {return _low < _high;}
         OSS_INLINE UINT32 getLow()const {return _low;}
         OSS_INLINE UINT32 getHigh()const {return _high;}
      
      public:
         INT32 merge(dmlIndexRequest *ir,
                     UINT32 seq,
                     const recordID &rid,
                     const DPS_LSN_OFFSET &lsn,
                     const DPS_TRANS_ID &transID);

         BOOLEAN endToBuildCurrentRange(INDEX_MERGING_LIST &ml);
         
         UINT32 slideHigh(UINT32 size=1);

      private:
         /// res < 0: builded
         /// res == 0: building
         /// res > 0: not builded 
         INT32 _getBuildingStatus(UINT32 seq)const;

      private:
         indexObject *_obj = nullptr;
         std::mutex _mutex;
         /// scanning range is [_low, _high)
         UINT32 _low = 0;
         UINT32 _high = 0;

         INDEX_MERGING_LIST _ml;
   };//class buildingIndexContext
} // namespace vessel
  
} // namespace engine


#endif//VESSEL_BUILDING_INDEX_CONTEXT_H_