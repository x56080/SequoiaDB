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

   Source File Name = lsmColumnFamilyContext.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/24/2022  LYC  Initial Draft

   Last Changed =

******************************************************************************/

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
