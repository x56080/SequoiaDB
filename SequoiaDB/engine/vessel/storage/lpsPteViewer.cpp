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

   Source File Name = lpsPteViewer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#include "vessel/lpsPteViewer.h"
#include "pdTrace.hpp"

namespace engine
{
namespace vessel
{
   lpsPteViewer::lpsPteViewer(ossSharedLatch *locker,
                              ossSharedLatchMode mode,
                              UINT32 psn):
   _locker(locker),
   _mode(mode),
   _psn(psn)
   {
      SDB_ASSERT(nullptr != _locker && !_mode.isNone(), "can not be invalid");
   }

   lpsPteViewer::~lpsPteViewer()
   {
      reset();
   }

   lpsPteViewer::lpsPteViewer(lpsPteViewer &&o)noexcept:
   _locker(o._locker),
   _mode(o._mode),
   _psn(o._psn)
   {
      o._reset();
   }

   lpsPteViewer &lpsPteViewer::operator=(lpsPteViewer &&o)noexcept
   {
      reset();
      if (o.isValid())
      {
         _locker = o._locker;
         _mode = o._mode;
         _psn = o._psn;
         o._reset();
      }
      return *this;
   }

   void lpsPteViewer::reset()
   {
      if (isValid())
      {
         _locker->unlockWith(_mode);
         _reset();
      }
      return;
   }

   void lpsPteViewer::_reset()
   {
      _locker = nullptr;
      _mode.setNone();
      _psn = 0;
   }

   void lpsPteViewer::_transferToExclusiveLock()
   {
      SDB_ASSERT(nullptr != _locker, "can not be invalid");
      SDB_ASSERT(_mode.isUpgrade(), "invalid locking level");
      _locker->unlockUpgradeAndLock();
      _mode.setExclusive();
   }

   void lpsPteViewer::_transferToUpgradeLock()
   {
      SDB_ASSERT(nullptr != _locker, "can not be invalid");
      SDB_ASSERT(_mode.isExclusive(), "invalid locking level");
      _locker->unlockAndLockUpgrade();
      _mode.setUpgrade();
   }
} // namespace vessel

} // namespace engine
