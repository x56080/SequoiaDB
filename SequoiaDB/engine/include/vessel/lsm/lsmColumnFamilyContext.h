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

   Source File Name = lsmColumnFamilyContext.h

   Descriptive Name = 

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/24/2021  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef VESSEL_LSM_COLUMN_FAMILY_CONTEXT_H_
#define VESSEL_LSM_COLUMN_FAMILY_CONTEXT_H_

#include "vessel/lsm/lsmLsnTracker.h"
#include "rocksdb/options.h"
#include "rocksdb/db.h"

namespace engine
{
namespace vessel
{
   class lsmColumnFamilyContext : public SDBObject
   {
      public:
         lsmColumnFamilyContext() = default;
         lsmColumnFamilyContext(const rocksdb::WriteOptions &wOpt);
         ~lsmColumnFamilyContext() = default;
         lsmColumnFamilyContext(const lsmColumnFamilyContext &) = delete;
         lsmColumnFamilyContext &operator= (const lsmColumnFamilyContext &) = delete;

      public:
         OSS_INLINE std::mutex &getFlushLock()
         {
            return _flushMutex;
         }

         OSS_INLINE const rocksdb::WriteOptions &getWriteOpt() const
         {
            return _wOpt;
         }

         OSS_INLINE rocksdb::ColumnFamilyHandle *getHandle() const
         {
            return _handle;
         }

      public:
         // If handle already exists, destroy it before set a new one.
         void setHandle(rocksdb::ColumnFamilyHandle *handle);
         // lock flush first
         DPS_LSN_OFFSET beginToFlush();

         // lock flush first
         void endToFlush(BOOLEAN flushDone);

         void setMinDirtyLsn(DPS_LSN_OFFSET lsn);

         DPS_LSN_OFFSET getMinDirtyLsn() const;

      private:
         rocksdb::ColumnFamilyHandle *_handle = nullptr;
         rocksdb::WriteOptions _wOpt;
         lsmLsnTracker _tracker;
         std::mutex _flushMutex;
   }; // class lsmColumnFamilyContext
 
} // namespace vessel
} // namespace engine

#endif // VESSEL_LSM_COLUMN_FAMILY_CONTEXT_H_