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

   Source File Name = indexEntryLocation.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_INDEX_ENTRY_LOCATION_H_
#define VESSEL_INDEX_ENTRY_LOCATION_H_

#include "ossMemPool.hpp"
#include "vessel/recordID.h"
#include "vessel/slice.h"

#include <memory>

namespace engine
{
namespace vessel
{
   enum class IDX_ENTRY_LOCATION_TYPE : INT32
   {
      HIT = 0x01,
   };

   class indexEntryLocation : public _utilPooledObject
   {
      public:
         indexEntryLocation() = default;
         virtual ~indexEntryLocation() = default;

      public:
         virtual IDX_ENTRY_LOCATION_TYPE getType()const = 0;
         virtual BOOLEAN isValid()const = 0;
   };
   using IDX_ENTRY_LOCATION_UPTR = std::unique_ptr<indexEntryLocation>;
} // namespace vessel

} // namespace engine


#endif//VESSEL_INDEX_ENTRY_LOCATION_H_