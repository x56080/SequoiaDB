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

   Source File Name = lsmCollector.cpp

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
#include "oss.h"
#include "dpsDef.hpp"
#include "vessel/lsm/lsmCollector.h"
#include "vessel/lsm/lsmIndexKey.h"
#include "vessel/lsm/lsmDBDef.h"
#include "rocksdb/status.h"
#include <cstring>
#include <string>

namespace engine
{
namespace vessel
{
   using namespace ROCKSDB_NAMESPACE;
   /////////////////////////////////////////////////////////////////////////////
   // lsmIndexPropertiesCollector begin
   Status lsmIndexPropertiesCollector::AddUserKey(const Slice &key,
                                      const Slice &value,
                                      EntryType type,
                                      SequenceNumber seq,
                                      uint64_t file_size)
   {
      if (kEntryPut != type)
      {
         return Status::OK();
      }
      DPS_LSN_OFFSET lsn = _extractLsnFromKey(key);
      if (_minLsn > lsn)
      {
         _minLsn = lsn;
      }
      if (_maxLsn < lsn || DPS_INVALID_LSN_OFFSET == _maxLsn)
      {
         _maxLsn = lsn;
      }

      globalIndexID globalID;
      _extractIndexIdFromKey(key, globalID);
      if (globalID < _minGlobalID)
      {
         _minGlobalID = globalID;
      }
      if (_maxGlobalID < globalID || !_maxGlobalID.isValid())
      {
         _maxGlobalID = globalID;
      }
      return Status::OK();
   }

   Status lsmIndexPropertiesCollector::Finish(UserCollectedProperties *properties)
   {
      
      std::string temp;
      temp.assign((const char*)&_minLsn, sizeof(_minLsn));
      properties->emplace(LSM_COLLECTOR_FIELDNAME_MIN_LSN, temp);
      temp.assign((const char*)&_maxLsn, sizeof(_maxLsn));
      properties->emplace(LSM_COLLECTOR_FIELDNAME_MAX_LSN, temp);
      temp.assign((const char*)&_minGlobalID, sizeof(_minGlobalID));
      properties->emplace(LSM_COLLECTOR_FIELDNAME_MIN_GLOBAL_ID, temp);
      temp.assign((const char*)&_maxGlobalID, sizeof(_maxGlobalID));
      properties->emplace(LSM_COLLECTOR_FIELDNAME_MAX_GLOBAL_ID, temp);
      return Status::OK();
   }

   UserCollectedProperties lsmIndexPropertiesCollector::GetReadableProperties() const
   {
      return {};
   }

   DPS_LSN_OFFSET lsmIndexPropertiesCollector::_extractLsnFromKey(
       const Slice &key) const
   {
      SDB_ASSERT(LSM_IDX_MIN_FULL_KEY_SIZE <= key.size(), "invalid key length");
      const lsmIdxFixedKey *fixedKey =
          reinterpret_cast<const lsmIdxFixedKey *>(key.data());
      SDB_ASSERT(fixedKey->isValid(), "key must be valid");
      return fixedKey->lsn;
   }

   void lsmIndexPropertiesCollector::_extractIndexIdFromKey(const rocksdb::Slice &key, globalIndexID &globalID) const
   {
      SDB_ASSERT(LSM_IDX_MIN_FULL_KEY_SIZE <= key.size(), "invalid key length");
      const lsmIdxFixedKey *fixedKey =
          reinterpret_cast<const lsmIdxFixedKey *>(key.data());
      SDB_ASSERT(fixedKey->isValid(), "key must be valid");
      globalID = fixedKey->indexid;
   }

   /////////////////////////////////////////////////////////////////////////////
   // lsmCollectorFactory begin

   TablePropertiesCollector *
   lsmCollectorFactory::CreateTablePropertiesCollector(
           TablePropertiesCollectorFactory::Context context)
   {
      if (LSM_CF_HYBRID_INDEX == context.column_family_id &&
          0 == context.level_at_creation)
      {
         lsmIndexPropertiesCollector *p = new(std::nothrow) lsmIndexPropertiesCollector();
         if (nullptr == p)
         {
            PD_LOG(PDSEVERE, "memory allocation error");
            ossPanic();
         }
         return p;
      }
      else
      {
         lsmDummyCollector *p = new(std::nothrow) lsmDummyCollector;
         if (nullptr == p)
         {
            PD_LOG(PDSEVERE, "memory allocation error");
            ossPanic();   
         }
         return p;
      }
   }

   std::shared_ptr<lsmCollectorFactory> newLsmCollectorFactory()
   {
      return std::shared_ptr<lsmCollectorFactory>(new lsmCollectorFactory);
   }
} // namespace vessel
} // namespace engine