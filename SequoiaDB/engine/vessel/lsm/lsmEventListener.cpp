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

   Source File Name = lsmEventListener.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/21/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/
#include "pd.hpp"
#include "vessel/lsm/lsmDBDef.h"
#include "vessel/lsm/lsmEventListener.h"
#include "vessel/lsm/lsmCollector.h"
#include "rocksdb/types.h"

namespace engine
{
namespace vessel
{
   using namespace ROCKSDB_NAMESPACE;
   std::shared_ptr<lsmEventListener> newLsmEventListener(lsmDB *db)
   {
      return std::shared_ptr<lsmEventListener>(new lsmEventListener(db));
   }

   /////////////////////////////////////////////////////////////////////////////
   // lsmEventListener begin

   lsmEventListener::lsmEventListener(lsmDB *db)
   {
      SDB_ASSERT(nullptr != db, "can not be nullptr");
      this->_db = db;
   }

   void lsmEventListener::OnTableFileCreated(const TableFileCreationInfo &info)
   {
      if (LSM_CF_HYBRID_INDEX == info.table_properties.column_family_id &&
          TableFileCreationReason::kFlush == info.reason)
      {
         const UserCollectedProperties &ucp =
             info.table_properties.user_collected_properties;
         auto it = ucp.find(LSM_COLLECTOR_FIELDNAME_MAX_LSN);
         if (it != ucp.end())
         {
            const DPS_LSN_OFFSET maxLsn =
                *reinterpret_cast<const DPS_LSN_OFFSET *>(it->second.data());
            if (DPS_INVALID_LSN_OFFSET != maxLsn)
            {
               // _db->onFlush(maxLsn);
            }
         }
      }
   }

} // namespace vessel
} // namespace engine