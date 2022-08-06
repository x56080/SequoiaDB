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

   Source File Name = indexEntryLocation.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

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