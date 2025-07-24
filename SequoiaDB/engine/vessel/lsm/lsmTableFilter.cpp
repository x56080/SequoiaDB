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
#include "vessel/lsm/lsmTableProperties.h"

namespace engine
{
namespace vessel
{
   bool lsmTableFilter::operator()(const rocksdb::TableProperties &t) const
   {
      if (filter.empty())
      {
         return TRUE;
      }

      auto itr = t.user_collected_properties.find(LSM_TABLE_PROPERTIES_MIN_IDX_ID);
      if (t.user_collected_properties.end() != itr)
      {
         if (0 > filter.compare(rocksdb::Slice(itr->second)))
         {
            return FALSE;
         }

         itr = t.user_collected_properties.find(LSM_TABLE_PROPERTIES_MAX_IDX_ID);
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
