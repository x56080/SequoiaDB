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

   Source File Name = lpsPteViewer.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
