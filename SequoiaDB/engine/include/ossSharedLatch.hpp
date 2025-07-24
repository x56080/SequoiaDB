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

   Source File Name = ossSharedLatch.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SHARED_LATCH_HPP_
#define OSS_SHARED_LATCH_HPP_

#include "ossLatch.hpp"

enum OSS_SHARED_LATCH_MODE_ENUM
{
   OSS_SHARED_LATCH_MODE_ENUM_NONE = 0,
   OSS_SHARED_LATCH_MODE_ENUM_SHARED = 1,
   OSS_SHARED_LATCH_MODE_ENUM_UPGRADE = 2,
   OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE = 3,
};//enum OSS_SHARED_LATCH_MODE

class ossSharedLatchMode : public SDBObject
{
   public:
      OSS_INLINE ossSharedLatchMode(){}
      OSS_INLINE ~ossSharedLatchMode(){}
      OSS_INLINE ossSharedLatchMode(const ossSharedLatchMode &o):
      _m(o._m){}
      OSS_INLINE ossSharedLatchMode &operator=(const ossSharedLatchMode &o)
      {
         _m = o._m;
         return *this;
      }
      OSS_INLINE ossSharedLatchMode &operator=(OSS_SHARED_LATCH_MODE_ENUM m)
      {
         _m = m;
         return *this;
      }
      OSS_INLINE explicit ossSharedLatchMode(OSS_SHARED_LATCH_MODE_ENUM m):
      _m(m){}

      OSS_INLINE BOOLEAN operator==(const ossSharedLatchMode &o)const
      {
         return o._m == _m;
      }
   public:
      OSS_INLINE OSS_SHARED_LATCH_MODE_ENUM getModeEnum()const
      {
         return _m;
      }
      OSS_INLINE BOOLEAN isExclusive()const
      {
         return OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE == _m;
      }
      OSS_INLINE void setExclusive()
      {
         _m = OSS_SHARED_LATCH_MODE_ENUM_EXCLUSIVE;
      }
      OSS_INLINE BOOLEAN isUpgrade()const
      {
         return OSS_SHARED_LATCH_MODE_ENUM_UPGRADE == _m;
      }
      OSS_INLINE void setUpgrade()
      {
         _m = OSS_SHARED_LATCH_MODE_ENUM_UPGRADE;
      }
      OSS_INLINE BOOLEAN isShared()const
      {
         return OSS_SHARED_LATCH_MODE_ENUM_SHARED == _m;
      }
      OSS_INLINE void setShared()
      {
         _m = OSS_SHARED_LATCH_MODE_ENUM_SHARED;
      }
      OSS_INLINE BOOLEAN isNone()const
      {
         return OSS_SHARED_LATCH_MODE_ENUM_NONE == _m;
      }
      OSS_INLINE void setNone()
      {
         _m = OSS_SHARED_LATCH_MODE_ENUM_NONE;
      }

      OSS_INLINE BOOLEAN isExclusiveOrUpgrade()const
      {
         return isExclusive() || isUpgrade();
      }
   private:
      OSS_SHARED_LATCH_MODE_ENUM _m = OSS_SHARED_LATCH_MODE_ENUM_NONE;
};//class ossSharedLatchMode

class ossSharedLatch : public SDBObject
{
   public:
      ossSharedLatch(){}
      ~ossSharedLatch(){}
      ossSharedLatch(const ossSharedLatch &) = delete;
      ossSharedLatch &operator=(const ossSharedLatch &) = delete;
   public:
      void lockShared()
      {
         _mutex.lock_shared();
      }
      BOOLEAN tryLockShared()
      {
         return _mutex.try_lock_shared();
      }
      void unlockShared()
      {
         _mutex.unlock_shared();
      }
      

      void lockUpgrade()
      {
         _mutex.lock_upgrade();
      }
      BOOLEAN tryLockUpgrade()
      {
         return _mutex.try_lock_upgrade();
      }
      void unlockUpgrade()
      {
         _mutex.unlock_upgrade();
      }

      void lock()
      {
         _mutex.lock();
      }
      BOOLEAN tryLock()
      {
         return _mutex.try_lock();
      }
      void unlock()
      {
         _mutex.unlock();
      }

      void lockWith(const ossSharedLatchMode &m)
      {
         if (m.isShared())
         {
            lockShared();
         }
         else if (m.isUpgrade())
         {
            lockUpgrade();
         }
         else if (m.isExclusive())
         {
            lock();
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid mode");
         }
         return;
      }

      BOOLEAN tryLockWith(const ossSharedLatchMode &m)
      {
         BOOLEAN r = FALSE;
         if (m.isShared())
         {
            r = tryLockShared();
         }
         else if (m.isUpgrade())
         {
            r = tryLockUpgrade();
         }
         else if (m.isExclusive())
         {
            r = tryLock();
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid mode");
         }
         return r;
      }

      BOOLEAN tryLockWith(const ossSharedLatchMode &m,
                          UINT32 millis)
      {
         BOOLEAN r = FALSE;
         if (m.isShared())
         {
            r = _mutex.try_lock_shared_for(boost::chrono::milliseconds(millis));
         
         }
         else if (m.isUpgrade())
         {
            r = _mutex.try_lock_upgrade_for(boost::chrono::milliseconds(millis));
         }
         else if (m.isExclusive())
         {
            r = _mutex.try_lock_for(boost::chrono::milliseconds(millis));
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid mode");
         }
         return r;
      }

      void unlockWith(const ossSharedLatchMode &m)
      {
         if (m.isShared())
         {
            unlockShared();
         }
         else if (m.isUpgrade())
         {
            unlockUpgrade();
         }
         else if (m.isExclusive())
         {
            unlock();
         }
         else
         {
            SDB_ASSERT(FALSE, "invalid mode");
         }
         return;
      }

      void unlockUpgradeAndLock()
      {
         _mutex.unlock_upgrade_and_lock();
      }

      /// will not release upgrade if return false.
      BOOLEAN tryUnlockUpgradeAndLock()
      {
         return _mutex.try_unlock_upgrade_and_lock();
      }
      void unlockUpgradeAndLockShared()
      {
         _mutex.unlock_upgrade_and_lock_shared();
      }
      void unlockAndLockUpgrade()
      {
         _mutex.unlock_and_lock_upgrade();
      }
      void unlockAndLockShared()
      {
         _mutex.unlock_and_lock_shared();
      }

      BOOLEAN tryUnlockSharedAndLock()
      {
         return _mutex.try_unlock_shared_and_lock();
      }

      BOOLEAN tryUnlockSharedAndLock(UINT32 millis)
      {
         return _mutex.try_unlock_shared_and_lock_for(boost::chrono::milliseconds(millis));
      }

   private:
      boost::shared_mutex _mutex;
};//class ossSharedLatch

#endif//OSS_SHARED_LATCH_HPP_