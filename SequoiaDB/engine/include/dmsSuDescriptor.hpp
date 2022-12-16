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

   Source File Name = dmsSuDescriptor.hpp

   Descriptive Name = Data Management Service SU Descriptor

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/14/2022  ZHY Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef DMS_SU_DESCRIPTOR_HPP__
#define DMS_SU_DESCRIPTOR_HPP__

#include "dms.hpp"

namespace engine
{
   struct dmsSuDescriptor
   {
      dmsSuDescriptor( DMS_ENGINE_TYPE engine,
                       std::string name,
                       utilCSUniqueID csUID,
                       UINT32 logicalID )
      : engine( engine ), name( name ), csUID( csUID ), logicalID( logicalID )
      {
      }

      DMS_ENGINE_TYPE engine = DMS_ENGINE_INVALID;
      std::string name;
      utilCSUniqueID csUID = UTIL_UNIQUEID_NULL;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
   };
   using DMS_SU_DESCRIPTOR = std::shared_ptr< const dmsSuDescriptor >;
} // namespace engine

#endif