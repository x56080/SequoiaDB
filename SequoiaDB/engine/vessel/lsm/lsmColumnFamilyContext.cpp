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

   Source File Name = lsmColumnFamilyContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/24/2022  LYC  Initial Draft

   Last Changed =

*******************************************************************************/
#include "vessel/lsm/lsmColumnFamilyContext.h"

namespace engine
{
namespace vessel
{
   lsmColumnFamilyContext::lsmColumnFamilyContext(const rocksdb::WriteOptions &opt)
   {
      _wOpt = opt;
   }

   void lsmColumnFamilyContext::setHandle(rocksdb::ColumnFamilyHandle *handle)
   {
      SDB_ASSERT(nullptr != handle, "can not be null");
      _handle = handle;
   }

   DPS_LSN_OFFSET lsmColumnFamilyContext::beginToFlush()
   {
      SDB_ASSERT(nullptr != _handle, "can not be null");
      return _tracker.beginToFlush();
   }

   void lsmColumnFamilyContext::endToFlush(BOOLEAN flushDone)
   {
      SDB_ASSERT(nullptr != _handle, "can not be null");
      _tracker.endToFlush(flushDone);
   }

   void lsmColumnFamilyContext::setMinDirtyLsn(DPS_LSN_OFFSET lsn)
   {
      SDB_ASSERT(nullptr != _handle, "can not be null");
      _tracker.setMinWriteLsn(lsn);
   }

   DPS_LSN_OFFSET lsmColumnFamilyContext::getMinDirtyLsn() const
   {
      SDB_ASSERT(nullptr != _handle, "can not be null");
      return _tracker.getMinDirtyLsn();
   }

} // namespace vessel
} // namespace engine
