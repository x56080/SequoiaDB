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
#include "vessel/lsm/lsmDBDef.h"
#include "vessel/lsm/lsmIndexEntryValue.h"
#include "vessel/keyString.h"
#include "vessel/sliceTransfer.h"
#include "vessel/lsm/lsmTableProperties.h"
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
      SDB_ASSERT(!key.empty() && !value.empty(), "invalid key and value");

#if defined(_DEBUG) 
      keyString ks(toSlice(key));
      SDB_ASSERT(ks.isValid() &&
                 keyStringCoder::INDEX_ID_ENCODEING_SIZE == ks.getKeyHeadSize(),
                 "invalid key");
#endif

      if (keyStringCoder::INDEX_ID_ENCODEING_SIZE <= key.size())
      {
         if (!_indexIdInited)
         {
            ossMemcpy(_minIndexId, key.data(), sizeof(_minIndexId));
            ossMemcpy(_maxIndexId, key.data(), sizeof(_maxIndexId));
            _indexIdInited = TRUE;
         }
         else
         {
            if (0 < ossMemcmp(_minIndexId, key.data(), sizeof(_minIndexId)))
            {
               ossMemcpy(_minIndexId, key.data(), sizeof(_minIndexId));
            }

            if (0 > ossMemcmp(_maxIndexId, key.data(), sizeof(_maxIndexId)))
            {
               ossMemcpy(_maxIndexId, key.data(), sizeof(_maxIndexId));
            }
         }
      }

      lsmIndexEntryValueRef ref(value);
      if (ref.isValid())
      {
         const lsmIndexEntryValue *val = ref.getValuePtr();
         if (DPS_INVALID_LSN_OFFSET == _minLsn || val->lsn < _minLsn)
         {
            _minLsn = val->lsn;
         }

         if (DPS_INVALID_LSN_OFFSET == _maxLsn || val->lsn > _maxLsn)
         {
            _maxLsn = val->lsn;
         }
      }

      return Status::OK();
   }

   Status lsmIndexPropertiesCollector::Finish(UserCollectedProperties *properties)
   {
      std::string temp;
      temp.reserve(12);
      if (DPS_INVALID_LSN_OFFSET != _minLsn)
      {
         temp.assign((const char*)&_minLsn, sizeof(_minLsn));
         properties->emplace(LSM_TABLE_PROPERTIES_MIN_LSN, temp);
         temp.assign((const char*)&_maxLsn, sizeof(_maxLsn));
         properties->emplace(LSM_TABLE_PROPERTIES_MAX_LSN, temp);
      }

      if (_indexIdInited)
      {
         temp.assign(_minIndexId, sizeof(_minIndexId));
         properties->emplace(LSM_TABLE_PROPERTIES_MIN_IDX_ID, temp);
         temp.assign(_maxIndexId, sizeof(_maxIndexId));
         properties->emplace(LSM_TABLE_PROPERTIES_MAX_IDX_ID, temp);
      }

      return Status::OK();
   }

   UserCollectedProperties lsmIndexPropertiesCollector::GetReadableProperties() const
   {
      return {};
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