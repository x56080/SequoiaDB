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

   Source File Name = lsmCollector.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/21/2022  ZHY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_COLLECTOR_H_
#define VESSEL_LSM_COLLECTOR_H_

#include "dpsDef.hpp"
#include "oss.hpp"
#include "rocksdb/table_properties.h"
#include "vessel/keyStringCoder.h"
#include <memory>
namespace engine
{
namespace vessel
{
   class lsmDummyCollector : public rocksdb::TablePropertiesCollector
   {
      virtual const CHAR *Name() const override
      {
         return "sdb.lsmDummyCollector";
      }

      virtual rocksdb::Status Finish(
          rocksdb::UserCollectedProperties *properties) override
      {
         return rocksdb::Status::OK();
      }

      virtual rocksdb::UserCollectedProperties GetReadableProperties()
          const override
      {
         return {};
      }

      virtual rocksdb::Status AddUserKey(const rocksdb::Slice &key,
                                         const rocksdb::Slice &value,
                                         rocksdb::EntryType type,
                                         rocksdb::SequenceNumber seq,
                                         uint64_t file_size) override
      {
         return rocksdb::Status::OK();
      } 
   };

   class lsmIndexPropertiesCollector : public rocksdb::TablePropertiesCollector
   {
   public:
      virtual const CHAR *Name() const override
      {
         return "sdb.lsmIndexPropertiesCollector";
      }

      virtual rocksdb::Status AddUserKey(const rocksdb::Slice &key,
                                         const rocksdb::Slice &value,
                                         rocksdb::EntryType type,
                                         rocksdb::SequenceNumber seq,
                                         uint64_t file_size) override;

      virtual rocksdb::Status Finish(
          rocksdb::UserCollectedProperties *properties) override;

      virtual rocksdb::UserCollectedProperties GetReadableProperties()
          const override;

   private:
      DPS_LSN_OFFSET _extractLsnFromKey(const rocksdb::Slice &key) const;
      void _extractIndexIdFromKey(const rocksdb::Slice &key, globalIndexID &globalID) const;

   private:
      DPS_LSN_OFFSET _minLsn = DPS_INVALID_LSN_OFFSET;
      DPS_LSN_OFFSET _maxLsn = DPS_INVALID_LSN_OFFSET;
      CHAR _minIndexId[keyStringCoder::INDEX_ID_ENCODEING_SIZE] = {};
      CHAR _maxIndexId[keyStringCoder::INDEX_ID_ENCODEING_SIZE] = {};
      BOOLEAN _indexIdInited = FALSE;
   };

   class lsmCollectorFactory : public rocksdb::TablePropertiesCollectorFactory
   {
   public:
      virtual rocksdb::TablePropertiesCollector *CreateTablePropertiesCollector(
          TablePropertiesCollectorFactory::Context context) override;

      const char *Name() const override
      {
         return "sdb.lsmCollectorFactory";
      }
   };

   extern std::shared_ptr<lsmCollectorFactory> newLsmCollectorFactory();
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_COLLECTOR_H_