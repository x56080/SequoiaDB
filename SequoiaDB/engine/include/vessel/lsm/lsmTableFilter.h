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
