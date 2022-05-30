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

   Source File Name = lsmColumnFamily.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/20/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_COLUMN_FAMILY_H_
#define VESSEL_LSM_COLUMN_FAMILY_H_

#include "oss.hpp"
#include "rocksdb/slice.h"
#include "rocksdb/write_batch.h"
#include "rocksdb/options.h"
#include "rocksdb/iterator.h"

namespace engine
{
namespace vessel
{
   class lsmDB;
   class lsmColumnFamily : public SDBObject
   {
      public:
         lsmColumnFamily() = default;
         lsmColumnFamily(lsmDB *db,
                         rocksdb::ColumnFamilyHandle *handle);
         ~lsmColumnFamily() = default;
         lsmColumnFamily(const lsmColumnFamily &cf) = default;
         lsmColumnFamily &operator=(const lsmColumnFamily &cf) = default;

      public:
         class writeBatch : public SDBObject
         {
            friend class lsmColumnFamily;
            public:
               writeBatch() = default;
               ~writeBatch();
               writeBatch(const writeBatch&) = delete;
               writeBatch &operator=(const writeBatch&) = delete;
            
            public:
               OSS_INLINE BOOLEAN isOpen()const
               {
                  return nullptr != _db &&
                         nullptr != _handle;
               }
               OSS_INLINE BOOLEAN isEmpty()const
               {
                  return 0 == _batch.Count();
               }

               void reset();
               INT32 put(const rocksdb::Slice &key,
                         const rocksdb::Slice &value);
               INT32 commit();

            private:
               rocksdb::DB *_db = nullptr;
               rocksdb::ColumnFamilyHandle *_handle = nullptr;
               rocksdb::WriteBatch _batch;
               rocksdb::WriteOptions _wOpt;
         }; // class writeBatch

      public:
         OSS_INLINE BOOLEAN isValid()const
         {
            return nullptr != _db &&
                   nullptr != _handle;
         }

      public:
         INT32 put(const rocksdb::Slice &key,
                   const rocksdb::Slice &value,
                   const rocksdb::WriteOptions &o);

         INT32 get(const rocksdb::Slice &key,
                   std::string &value,
                   BOOLEAN &notFound,
                   const rocksdb::ReadOptions &o);

         INT32 remove(const rocksdb::Slice &key,
                      const rocksdb::WriteOptions &o);

         INT32 truncate(const rocksdb::Slice &lowKey,
                        const rocksdb::Slice &upKey,
                        const rocksdb::WriteOptions &o);

         INT32 compact(const rocksdb::Slice *beginKey,
                       const rocksdb::Slice *endKey,
                       const rocksdb::CompactRangeOptions &o);
         
         rocksdb::Iterator *newIterator(const rocksdb::ReadOptions &o);

         void openBatch(writeBatch &batch);

      private:
         lsmDB *_db = nullptr;
         rocksdb::ColumnFamilyHandle *_handle = nullptr;
   };
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_COLUMN_FAMILY_H_