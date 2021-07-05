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

   Source File Name = mainDataSpace.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_MAIN_DATA_SPACE_H_
#define VESSEL_MAIN_DATA_SPACE_H_

#include "vessel/logicalPageSpace.h"
#include "vessel/collectionSpaceGlobalPage.h"
#include "vessel/vesselOptions.h"

namespace engine
{
namespace vessel
{
   class mainDataSpace : public logicalPageSpace
   {
      public:
         mainDataSpace();
         virtual ~mainDataSpace();

      public:
         virtual SPACE_TYPE getSpaceType()const
         {
            return SPACE_TYPE_MAIN_DATA;
         }

      private:
         virtual UINT32 getReservedImpCount()const
         {
            return 1;
         }
         virtual UINT32 getFreeBoundOfLpidPool()const 
         {
            return PAGE_COUNT_IN_EXTENT;
         }
         virtual UINT32 getFreeBoundOfPpidPool()const
         {
            return PAGE_COUNT_IN_EXTENT;
         }
         virtual UINT32 getIdMapFileHeadFlagsWhenCreating()const
         {
            return 0;
         }
      private:
         virtual INT32 openFiles(requestContext *context,
                                 SPACE_ID sid,
                                 const std::string &dir,
                                 idMapFile **out);

         virtual INT32 mapDataStorageSegmentsWhenStarup(requestContext *context,
                                                        inMemBitMap &bitmap);
   };//class mainDataSpace
}//namespace vessel
}//namespace engine

#endif//VESSEL_MAIN_DATA_SPACE_H_