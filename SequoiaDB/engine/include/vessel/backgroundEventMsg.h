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

   Source File Name = backgroundEventMsg.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_BACKGROUND_EVENT_MSG_H_
#define VESSEL_BACKGROUND_EVENT_MSG_H_

#include "vessel/vesselIdDef.h"
#include "vessel/vesselFileDef.h"
#include "vessel/backgroundEvent.h"

namespace engine
{
namespace vessel
{ 
#pragma pack(4)
   struct lpsCheckpointApplying : public SDBObject
   {
      lpsCheckpointApplying(){}
      ~lpsCheckpointApplying(){}
   
      SPACE_ID _sid = INVALID_SPACE_ID;
      SPACE_TYPE _type = INVALID_SPACE_TYPE;
   };//class lpsCheckpointApplying
   
   struct lpsFlushingSegments
   {
      lpsFlushingSegments(){}
      ~lpsFlushingSegments(){}

      SPACE_ID _sid = INVALID_SPACE_ID;
      SPACE_TYPE _type = INVALID_SPACE_TYPE;
      UINT8 _count = 0;
      UINT32 _segmentId = 0;
   };//class lpsFlushingSegments

#pragma pack()
} // namespace vessel

} // namespace engine


#endif//VESSEL_BACKGROUND_EVENT_MSG_H_