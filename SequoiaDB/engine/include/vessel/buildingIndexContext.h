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

   Source File Name = buildingIndexContext.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BUILDING_INDEX_CONTEXT_H_
#define VESSEL_BUILDING_INDEX_CONTEXT_H_

#include "vessel/unstableIndexContext.h"
#include "vessel/indexMergingRecord.h"
#include "vessel/dmlContext.h"
#include "vessel/dmlIndexRequest.h"

namespace engine
{
namespace vessel
{
   class buildingIndexContext : public unstableIndexContext
   {
      public:
         buildingIndexContext(){}
         virtual ~buildingIndexContext();

      public:
         INT32 merge(dmlContext *context,
                     dmlIndexRequest *ir);

         void fini();
         
         /// res < 0: builded
         /// res == 0: building
         /// res > 0: not builded 
         INT32 getEntryBuildingStatus(const scanEntry &entry)const;

         BOOLEAN endToBuildCurrentRange(indexMergingRecordList &mrl);

         void updateBuildingHighBound(const scanEntry &entry);

         BOOLEAN getNextBuildingBound(scanEntry &bound)const;

         void terminate()
         {
            _terminated = TRUE;
         }

         OSS_INLINE BOOLEAN isTerminated()const
         {
            return _terminated;
         }

      private:
         ossSpinXLatch _latch;
         /// scanning range is [_low, _high)
         scanEntry _low;
         scanEntry _high;

         indexMergingRecordList _mrl;

         BOOLEAN _terminated = FALSE;
   };//class buildingIndexContext
} // namespace vessel
  
} // namespace engine


#endif//VESSEL_BUILDING_INDEX_CONTEXT_H_