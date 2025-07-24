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

   Source File Name = ossSpinLatch.h

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_SPIN_LATCH_H_
#define OSS_SPIN_LATCH_H_

#include "core.hpp"
#include "oss.hpp"
#include <atomic> /// c++11
#include <thread> /// c++11

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
      OSS_INLINE ossSpinLatch()
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