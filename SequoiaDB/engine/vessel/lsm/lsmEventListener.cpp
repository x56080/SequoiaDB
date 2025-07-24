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
#include "vessel/lsm/lsmTableProperties.h"

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
         auto it = ucp.find(LSM_TABLE_PROPERTIES_MAX_LSN);
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