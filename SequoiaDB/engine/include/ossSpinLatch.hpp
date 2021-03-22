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

class ossSpinLatch : public SDBObject
{
   public:
      OSS_INLINE ossSpinLatch():
      _flag(ATOMIC_FLAG_INIT)
      {}

      OSS_INLINE ~ossSpinLatch(){}

      ossSpinLatch(const ossSpinLatch &) = delete;
      ossSpinLatch &operator=(const ossSpinLatch &) = delete;

   public:
      OSS_INLINE void lock()
      {
         while (!_flag.test_and_set(std::memory_order_acquire));
      }
      OSS_INLINE void unlock()
      {
         _flag.clear(std::memory_order_release);
      }

   private:
      std::atomic_flag _flag;
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
      ossSpinLatch *_latch;
      BOOLEAN _locked;
};//class ossSpinGuard





#endif//SDB_OSS_SPIN_LATCH_H_