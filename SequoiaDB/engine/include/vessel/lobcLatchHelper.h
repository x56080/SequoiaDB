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

   Source File Name = lobcLatchHelper.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LOBC_LATCH_HELPER_H_
#define VESSEL_LOBC_LATCH_HELPER_H_

#include "vessel/lobChunkKey.h"
#include "vessel/objectLatchMap.hpp"

namespace engine
{
namespace vessel
{
   class lobcLatchHelper : public SDBObject
   {
      public:
         INT32 lock(const globalLobChunkKey &key,
                    const ossSharedLatchMode &mode,
                    LOBC_LATCH_MAP::object &o)const;

         void unlock(const ossSharedLatchMode &mode,
                     LOBC_LATCH_MAP::object &o);
   };//class lobcLatchHelper
} // namespace vessel

} // namespace engine


#endif//VESSEL_LOBC_LATCH_HELPER_H_