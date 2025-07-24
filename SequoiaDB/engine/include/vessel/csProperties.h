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

   Source File Name = csProperties.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_CS_PROPERTIES_H_
#define VESSEL_CS_PROPERTIES_H_

#include "vessel/objectIdentifier.h"
#include "vessel/objectBaseDef.h"

namespace engine
{
namespace vessel
{
   struct csProperties : public SDBObject
   {
      void reset()
      {
         csid.reset();
         status = CS_STATUS_INVALID;
         type = CS_TYPE_INVALID;
         flags = 0;
         name.clear();
         maxCLLogicalID = DMS_INVALID_LOGICCLID;
      }

      collectionSpaceId csid;
      CS_STATUS status = CS_STATUS_INVALID;
      CS_TYPE type = CS_TYPE_INVALID;
      UINT32 flags = 0;
      std::string name;
      UINT32 maxCLLogicalID = DMS_INVALID_LOGICCLID;
   };
} // namespace vessel

} // namespace engine


#endif//VESSEL_CS_PROPERTIES_H_