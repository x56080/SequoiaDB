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

   Source File Name = lcPageTagHolder.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef VESSEL_LC_PAGE_TAG_HOLDER_H_
#define VESSEL_LC_PAGE_TAG_HOLDER_H_

#include "vessel/liteCachePageTag.h"
#include "ossLikely.hpp"

namespace engine
{
namespace vessel
{
   class lcPageTagHolder
   {
      public:
         OSS_INLINE lcPageTagHolder(){}
         OSS_INLINE explicit lcPageTagHolder(liteCachePageTag *tag,
                                             const ossSharedLatchMode &mode):
                             _tag(tag), _mode(mode){}
         OSS_INLINE ~lcPageTagHolder()
         {
            /// WARNING: holder's destructor will not release lock automaticly.
            _tag = NULL;
         }

         lcPageTagHolder(const lcPageTagHolder &r) = delete;
         OSS_INLINE lcPageTagHolder &operator=(const lcPageTagHolder &r)
         {
            _tag = r._tag;
            _mode = r._mode;
            return *this;
         }

      public:
         OSS_INLINE BOOLEAN valid()const
         {
            return NULL != _tag;
         }

         OSS_INLINE void reset(liteCachePageTag *tag)
         {
            _tag = tag;
            _mode.setNone();
         }

         OSS_INLINE void autoUnlock()
         {
            if (NULL != _tag && !_mode.isNone())
            {
               _tag->getAccessingLatch().unlockWith(_mode);
               _mode.setNone();
            }
            return;
         }

         OSS_INLINE BOOLEAN isLocked()const
         {
            return !_mode.isNone();
         }

         OSS_INLINE void lock()
         {
            const ossSharedLatchMode M;
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().lock();
            _mode.setExclusive();
         }

         OSS_INLINE void unlock()
         {
            const ossSharedLatchMode M(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().unlock();
            _mode.setNone();
         }

         OSS_INLINE BOOLEAN tryLock()
         {
            const ossSharedLatchMode M;
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            if (_tag->getAccessingLatch().tryLock())
            {
               _mode.setExclusive();
               return TRUE;
            }
            return FALSE;
         }

         OSS_INLINE void lockShared()
         {
            const ossSharedLatchMode M;
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().lockShared();
            _mode.setShared();
         }

         OSS_INLINE void lockUpgrade()
         {
            const ossSharedLatchMode M;
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().lockUpgrade();
            _mode.setUpgrade();
         }

         OSS_INLINE void unlockUpgradeAndLock()
         {
            const ossSharedLatchMode M(OSS_SHARED_LATCH_MODE_ENUM_UPGRADE);
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().unlockUpgradeAndLock();
            _mode.setExclusive();
         }

         OSS_INLINE void unlockAndlockUpgrade()
         {
            const ossSharedLatchMode M(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().unlockAndLockUpgrade();
            _mode.setUpgrade();
         }

         OSS_INLINE void lockWithMode(const ossSharedLatchMode &mode)
         {
            SDB_ASSERT(isValidAndLockingTypeIs(ossSharedLatchMode()), "impossible");
            _tag->getAccessingLatch().lockWith(mode);
            _mode = mode;
         }

         OSS_INLINE void unlockAndLockShared()
         {
            const ossSharedLatchMode M(OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE);
            SDB_ASSERT(isValidAndLockingTypeIs(M), "impossible");
            _tag->getAccessingLatch().unlockAndLockShared();
            _mode.setShared();
         }

         OSS_INLINE const ossSharedLatchMode &getLockMode()const
         {
            return _mode;
         }

         OSS_INLINE liteCachePageTag *tag()
         {
            return _tag;
         }

         OSS_INLINE const liteCachePageTag *tag() const
         {
            return _tag;
         }
      private:
         OSS_INLINE BOOLEAN isValidAndLockingTypeIs(const ossSharedLatchMode &mode)const
         {
            return NULL != _tag && mode == _mode;
         }
      private:
         liteCachePageTag *_tag = NULL;
         ossSharedLatchMode _mode;
   }; /// end of class lcPageTagHolder
} /// end of namespace vessel
} /// end of namespace engine

#endif /// end of VESSEL_LC_PAGE_TAG_HOLDER_H_