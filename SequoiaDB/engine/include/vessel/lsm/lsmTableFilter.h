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

   Source File Name = lsmTableFilter.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/27/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_TABLE_FILTER_H_
#define VESSEL_LSM_TABLE_FILTER_H_

#include "pd.hpp"
#include "rocksdb/table_properties.h"

namespace engine
{
namespace vessel
{
   struct lsmTableFilter
   {
      explicit lsmTableFilter(const rocksdb::Slice &s):
      filter(s)
      {
         SDB_ASSERT(!filter.empty(), "can not be empty");
      }
      bool operator()(const rocksdb::TableProperties &t) const;

      rocksdb::Slice filter;
   }; // struct lsmTableFilter

} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_TABLE_FILTER_H_
