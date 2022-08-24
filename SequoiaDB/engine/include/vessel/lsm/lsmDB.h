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
#include "vessel/lsm/lsmDBOptions.h"
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
                    const lsmDBOptions *o = nullptr);
         void close();

         /* 
            restore() should be called after open(), any read 
            and write operations are not allowed during restore() execution.
            If there's a record's lsn in a sst file that is greater than 
            the dps max lsn, it means the record is invalid. Deleting the 
            invalid record will cause valid records with the same key to be 
            deleted, all sst files are invalid in this case. 
            Therefore, if there are invalid records in sst files,
            all sst files will be deleted during restore() execution.
         */
         INT32 restore(DPS_LSN_OFFSET checkpointLsn, DPS_LSN_OFFSET dpsMaxLsn);

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

         void openBatch(LSM_CF_ID id, lsmWriteBatch &batch);

         INT32 write(lsmWriteBatch &batch);
         
         rocksdb::Iterator *newIterator(LSM_CF_ID id,
                                        const rocksdb::ReadOptions &opt);

      public:
         // manual call to ensure the record corresponding to
         // the current min dirty lsn is flushed.
         INT32 flush(LSM_CF_ID id = LSM_CF_INVALID);

         void setMinDirtyLsn(LSM_CF_ID id,
                             DPS_LSN_OFFSET lsn);

         /// return global min dirty lsn if id not specified.
         DPS_LSN_OFFSET getMinDirtyLsn(LSM_CF_ID id = LSM_CF_INVALID) const;

         // void setJournal(IDataJournal *journal);

      public:
      //    void onFlush(DPS_LSN_OFFSET maxLsn);

      public:
         INT32 loadSSTs(LSM_CF_ID id,
                        INT32 level,
                        BOOLEAN dirIncluded,
                        BOOLEAN creationAsc,
                        ossPoolVector<std::string> &ssts);

         INT32 removeSST(const std::string &name);

      private:
         rocksdb::ColumnFamilyDescriptor _getDescriptor(LSM_CF_ID id,
                                          const rocksdb::Options &opt) const;
         rocksdb::WriteOptions _getDefaultWriteOptions(LSM_CF_ID id) const;

         INT32 _flushDB();

         INT32 _flushCF(LSM_CF_ID id);

         lsmColumnFamilyContext *_getColumnFamilyCtx(LSM_CF_ID id);

         const lsmColumnFamilyContext *_getColumnFamilyCtx(LSM_CF_ID id) const;

         rocksdb::ColumnFamilyHandle *_getHandle(LSM_CF_ID id) const;

         const rocksdb::WriteOptions &_getWriteOpt(LSM_CF_ID id) const;

      private:
         INT32 _restoreHybridIndexCF(DPS_LSN_OFFSET checkpointLsn,
                                     DPS_LSN_OFFSET dpsMaxLsn);

         INT32 _extractLsnFromProperties(
            const rocksdb::TablePropertiesCollection &tpc,
            DPS_LSN_OFFSET &minLsn, DPS_LSN_OFFSET &maxLsn) const;

         /* 
            Determine if the restore operation is permitted by comparing lsn.
            If max lsn is greater than dps max lsn, it means that there is 
            at least one invalid record in sst files. We need to delete all 
            sst files to restore lsmDB.
            If min lsn is less than checkpoint lsn, it means that deleting all 
            sst files will cause the record lost. The restore operation is not
            permitted. 
         */ 
         INT32 _checkToRestore(DPS_LSN_OFFSET checkpointLsn,
                               DPS_LSN_OFFSET dpsMaxLsn,
                               DPS_LSN_OFFSET minLsn,
                               DPS_LSN_OFFSET maxLsn,
                               BOOLEAN &hasInvalid) const;

         INT32 _removeCFData(LSM_CF_ID id,
                             const rocksdb::Slice &lowKey,
                             const rocksdb::Slice &upKey);

      private:
         lsmDBOptions _o;
         rocksdb::DB *_db = nullptr;
         // IDataJournal *_journal = nullptr;
         std::vector<lsmColumnFamilyContext *> _contexts;
   }; // class lsmDB


} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_DB_H_