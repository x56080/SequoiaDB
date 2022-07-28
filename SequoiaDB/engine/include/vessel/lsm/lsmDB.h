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

   Source File Name = lsmDB.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_DB_H_
#define VESSEL_LSM_DB_H_

#include "dpsDef.hpp"
#include "interface/IDataJournal.h"
#include "vessel/lsm/lsmDBDef.h"
#include "vessel/lsm/lsmColumnFamily.h"
#include "vessel/lsm/lsmWriteBatch.h"
#include "rocksdb/db.h"

namespace engine
{
namespace vessel
{
   class lsmColumnFamilyContext;
   class lsmDB : public SDBObject
   {
      public:
         lsmDB() = default;
         ~lsmDB();
         lsmDB(const lsmDB &) = delete;
         lsmDB &operator= (const lsmDB &) = delete;

      public:
         INT32 open(const CHAR* dbPath,
                    const rocksdb::Options *o = nullptr);
         void close();

      public:
         BOOLEAN isOpen()const;
         lsmColumnFamily getHitColumnFamily();
         lsmColumnFamily getLobmColumnFamily();
         lsmColumnFamily getIdxMetaColumnFamily();
      
      public:
         INT32 put(LSM_CF_ID id,
                   const rocksdb::Slice &key,
                   const rocksdb::Slice &value);
         
         INT32 get(LSM_CF_ID id,
                   const rocksdb::Slice &key,
                   std::string &value,
                   BOOLEAN &notFound);

         INT32 remove(LSM_CF_ID id,
                      const rocksdb::Slice &key);
         
         INT32 truncate(LSM_CF_ID id,
                        const rocksdb::Slice &lowKey,
                        const rocksdb::Slice &upKey);

         INT32 compact(LSM_CF_ID id,
                       const rocksdb::Slice *lowKey = nullptr,
                       const rocksdb::Slice *upKey = nullptr);

         void openBatch(LSM_CF_ID id, lsmWriteBatch &batch);

         INT32 write(lsmWriteBatch &batch);
         
         rocksdb::Iterator *newIterator(LSM_CF_ID id,
                                        const rocksdb::ReadOptions &opt);

         INT32 flush(LSM_CF_ID id = LSM_CF_INVALID);

      public:
         void setMinDirtyLsn(LSM_CF_ID id,
                             DPS_LSN_OFFSET lsn);

         /// return global min dirty lsn if id not specified.
         DPS_LSN_OFFSET getMinDirtyLsn(LSM_CF_ID id = LSM_CF_INVALID) const;

         // void setJournal(IDataJournal *journal);

      public:
      //    void onFlush(DPS_LSN_OFFSET maxLsn);

      private:
         rocksdb::ColumnFamilyDescriptor _getDescriptor(LSM_CF_ID id,
                                          const rocksdb::Options &opt) const;
         rocksdb::WriteOptions _getDefaultWriteOptions(LSM_CF_ID id) const;

         INT32 _flushDB();

         INT32 _flushCF(LSM_CF_ID id);

      private:
         rocksdb::DB *_db = nullptr;
         // IDataJournal *_journal = nullptr;
         std::vector<lsmColumnFamilyContext *> _contexts;
   }; // class lsmDB


} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_DB_H_