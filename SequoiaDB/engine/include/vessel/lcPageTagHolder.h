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
         OSS_INLINE ~lcPageTagHolder()
         {
            /// WARNING: holder's destructor will not release lock automaticly.
            _tag = NULL;
            _mode = ossSharedLatch::NONE;
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
            _mode = ossSharedLatch::NONE;
         }

         OSS_INLINE void autoUnlock()
         {
            if (NULL != _tag && ossSharedLatch::NONE != _mode)
            {
               _tag->getAccessingLatch().unlockWith(_mode);
               _mode = ossSharedLatch::NONE;
            }
            return;
         }

         OSS_INLINE BOOLEAN isLocked()const
         {
            return _mode != ossSharedLatch::NONE;
         }

         OSS_INLINE void lock()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::NONE), "impossible");
            _tag->getAccessingLatch().lock();
            _mode = ossSharedLatch::EXCLUSIVE;
         }

         OSS_INLINE void unlock()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::EXCLUSIVE), "impossible");
            _tag->getAccessingLatch().unlock();
            _mode = ossSharedLatch::NONE;
         }

         OSS_INLINE BOOLEAN tryLock()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::NONE), "impossible");
            if (_tag->getAccessingLatch().tryLock())
            {
               _mode = ossSharedLatch::EXCLUSIVE;
               return TRUE;
            }
            return FALSE;
         }

         OSS_INLINE void lockShared()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::NONE), "impossible");
            _tag->getAccessingLatch().lockShared();
            _mode = ossSharedLatch::SHARED;
         }

         OSS_INLINE void lockUpgrade()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::NONE), "impossible");
            _tag->getAccessingLatch().lockUpgrade();
            _mode = ossSharedLatch::UPGRADE;
         }

         OSS_INLINE void unlockUpgradeAndLock()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::UPGRADE), "impossible");
            _tag->getAccessingLatch().unlockUpgradeAndLock();
            _mode = ossSharedLatch::EXCLUSIVE;
         }

         OSS_INLINE void unlockAndlockUpgrade()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::EXCLUSIVE), "impossible");
            _tag->getAccessingLatch().unlockAndLockUpgrade();
            _mode = ossSharedLatch::UPGRADE;
         }

         OSS_INLINE void lockWithMode(ossSharedLatch::mode mode)
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::NONE), "impossible");
            _tag->getAccessingLatch().lockWith(mode);
            _mode = mode;
         }

         OSS_INLINE void unlockAndLockShared()
         {
            SDB_ASSERT(isValidAndLocking(ossSharedLatch::EXCLUSIVE), "impossible");
            _tag->getAccessingLatch().unlockAndLockShared();
            _mode = ossSharedLatch::SHARED;
         }

         OSS_INLINE ossSharedLatch::mode getLockMode()const
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
         OSS_INLINE BOOLEAN isValidAndLocking(ossSharedLatch::mode mode)const
         {
            return NULL != _tag && mode == _mode;
         }
      private:
         liteCachePageTag *_tag = NULL;
         ossSharedLatch::mode _mode = ossSharedLatch::NONE;
   }; /// end of class lcPageTagHolder
} /// end of namespace vessel
} /// end of namespace engine

#endif /// end of VESSEL_LC_PAGE_TAG_HOLDER_H_