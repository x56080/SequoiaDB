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

   Source File Name = lsmTableFilter.cpp

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/27/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/

#include "vessel/lsm/lsmTableFilter.h"
#include "vessel/lsm/lsmCollector.h"

namespace engine
{
namespace vessel
{
   bool lsmTableFilter::operator()(const rocksdb::TableProperties &t) const
   {
      if (!filter.empty())
      {
         return TRUE;
      }

      auto itr = t.user_collected_properties.find(LSM_COLLECTOR_FIELDNAME_MIN_GLOBAL_ID);
      if (t.user_collected_properties.end() != itr)
      {
         if (0 > filter.compare(rocksdb::Slice(itr->second)))
         {
            return FALSE;
         }

         itr = t.user_collected_properties.find(LSM_COLLECTOR_FIELDNAME_MAX_GLOBAL_ID);
         if (t.user_collected_properties.end() != itr)
         {
            if (0 < filter.compare(rocksdb::Slice(itr->second)))
            {
               return FALSE;
            }
         }
      }

      return TRUE;
   }
} // namespace vessel
} // namespace engine
