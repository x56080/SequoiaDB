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

   Source File Name = csProperties.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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