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

   Source File Name = ossSpinLatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef OSS_SPIN_LATCH_H_
#define OSS_SPIN_LATCH_H_

#include "core.hpp"
#include "oss.hpp"
#include <atomic> /// c++11

///In my test: 5 threads increase global int var 100,000,000 times each(lock every time)
/// 0: no lock (incorrect result)                     |  < 1s  |
/// 1: atomic int(c++ 11)                             |  10 s  | 
/// 2: ossSpinLatch (c++11)                           |  13 s  |
/// 3. pthread_mutex_t exclusive                      |  44 s  |
/// 4. boost::shared_lock exclusive                   |  6m 50s |
/// 5. boost::shared_lock shared (incorrect result)   |  1m 49s |
/// 6. pthread_rwlock_t exclusive                     |  1m 45s |          

///WARNING: If you do not clearly
///know which to use, just pick pthread_mutex_t.
class ossSpinLatch : public SDBObject
{
   public:
      OSS_INLINE ossSpinLatch():
      {}

      OSS_INLINE ~ossSpinLatch(){}

      ossSpinLatch(const ossSpinLatch &) = delete;
      ossSpinLatch &operator=(const ossSpinLatch &) = delete;

   public:
      OSS_INLINE void lock()
      {
         lockWithYield();
      }

      OSS_INLINE void lockWithYield()
      {
         /// "test_and_set" set the flag as true and return value before set.
         /// The first thread which set flag from false to true wins.
         while (_flag.test_and_set(std::memory_order_acquire))
         {
            std::this_thread::yield();
         }
         return;
      }

      OSS_INLINE void lockWithoutYield()
      {
         while (_flag.test_and_set(std::memory_order_acquire));
         return;
      }

      BOOLEAN tryLock()
      {
         return !_flag.test_and_set(std::memory_order_acquire);
      }

      OSS_INLINE void unlock()
      {
         _flag.clear(std::memory_order_release);
         return;
      }

   private:
      std::atomic_flag _flag = ATOMIC_FLAG_INIT;
};//class ossSpinLatch

class ossSpinGuard : public SDBObject
{
   public:
      OSS_INLINE ossSpinGuard(ossSpinLatch *latch):
      _latch(latch),
      _locked(FALSE)
      {
         if (NULL != _latch)
         {
            _latch->lock();
            _locked = TRUE;
         }
      }

      OSS_INLINE ~ossSpinGuard()
      {
         if (_locked)
         {
            _latch->unlock();
            _locked = FALSE;
         }
      }

      ossSpinGuard(const ossSpinGuard &) = delete;
      ossSpinGuard &operator=(const ossSpinGuard &) = delete;
   private:
      ossSpinLatch *_latch = NULL;
      BOOLEAN _locked = FALSE;
};//class ossSpinGuard





#endif//SDB_OSS_SPIN_LATCH_H_