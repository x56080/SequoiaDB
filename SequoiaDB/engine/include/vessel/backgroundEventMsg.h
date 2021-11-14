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

   Source File Name = backgroundEventMsg.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_BACKGROUND_EVENT_MSG_H_
#define VESSEL_BACKGROUND_EVENT_MSG_H_

#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"

namespace engine
{
namespace vessel
{ 
#pragma pack(4)
   class lpsCheckpointApplying : public SDBObject
   {
      public:
         lpsCheckpointApplying(){}
         ~lpsCheckpointApplying(){}
         lpsCheckpointApplying(const lpsCheckpointApplying &) = delete;
         lpsCheckpointApplying &operator=(const lpsCheckpointApplying &) = delete;

      
      public:
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
   };//class lpsCheckpointApplying

   class lpsFlushingSegments
   {
      public:
         lpsFlushingSegments(){}
         ~lpsFlushingSegments(){}
         lpsFlushingSegments(const lpsFlushingSegments &) = delete;
         lpsFlushingSegments &operator=(const lpsFlushingSegments &) = delete;
      
      public:
         SPACE_ID _sid = INVALID_SPACE_ID;
         SPACE_TYPE _type = INVALID_SPACE_TYPE;
         UINT8 _count = 0;
         UINT32 _segmentId = 0;
   };//class lpsFlushingSegments
#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_BACKGROUND_EVENT_MSG_H_