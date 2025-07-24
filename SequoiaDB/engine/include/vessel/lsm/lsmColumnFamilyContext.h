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