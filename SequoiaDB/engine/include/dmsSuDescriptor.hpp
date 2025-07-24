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

      OSS_INLINE BOOLEAN isValid() const
      {
         return engine < DMS_ENGINE_INVALID && !name.empty() && logicalID != DMS_INVALID_LOGICCSID;
      }

      DMS_ENGINE_TYPE engine = DMS_ENGINE_INVALID;
      std::string name;
      utilCSUniqueID csUID = UTIL_UNIQUEID_NULL;
      UINT32 logicalID = DMS_INVALID_LOGICCSID;
   };
   using DMS_SU_DESCRIPTOR = std::shared_ptr< const dmsSuDescriptor >;
} // namespace engine

#endif