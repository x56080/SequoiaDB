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

   Source File Name = ossLatchGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

******************************************************************************/

#ifndef SDB_OSS_LATCH_GUARD_H_
#define SDB_OSS_LATCH_GUARD_H_

#include "ossLatch.hpp"
#include "ossRWMutex.hpp"

class ossSpinSLatchGuard : public SDBObject
{
   public:
      ossSpinSLatchGuard() = delete;
      ossSpinSLatchGuard(ossSpinSLatch *latch, OSS_LATCH_MODE mode):
      _latch(latch),
      _locked(FALSE),
      _mode(mode)
      {
         lock();
      }
      ossSpinSLatchGuard(ossSpinSLatch *latch,
                         OSS_LATCH_MODE mode,
                         BOOLEAN immediate):
      _latch(latch),
      _locked(FALSE),
      _mode(mode)
      {
         if (immediate)
         {
            lock();
         }
      }  

      ~ossSpinSLatchGuard()
      {
         unlock();
      }
      
      ossSpinSLatchGuard(const ossSpinSLatchGuard &) = delete;
      ossSpinSLatchGuard &operator=(const ossSpinSLatchGuard &) = delete;

   public:
      void lock()
      {
         if (NULL != _latch && !_locked)
         {
            if (SHARED == _mode)
            {
               _latch->get_shared();
            }
            else
            {
               _latch->get();
            }
            _locked = TRUE;
         }
         return;
      }
      void unlock()
      {
         if (NULL != _latch && _locked)
         {
            if (SHARED == _mode)
            {
               _latch->release_shared();
            }
            else
            {
               _latch->release();
            }
            _locked = FALSE;
         }
         return;
      }

      BOOLEAN isLocked()const
      {
         return _locked;
      }
      OSS_LATCH_MODE getMode()const
      {
         return _mode;
      }

   private:
      ossSpinSLatch *_latch;
      BOOLEAN _locked = FALSE;
      OSS_LATCH_MODE _mode = SHARED;
};//class ossSpinSLatchGuard

class ossSpinXLatchGuard : public SDBObject
{
   public:
      ossSpinXLatchGuard() = delete;
      ossSpinXLatchGuard(ossSpinXLatch *latch):
      _latch(latch),
      _locked(FALSE)
      {
         lock();
      }
      ossSpinXLatchGuard(ossSpinXLatch *latch,
                         BOOLEAN immediate):
      _latch(latch),
      _locked(FALSE)
      {
         if (immediate)
         {
            lock();
         }
      }  

      ~ossSpinXLatchGuard()
      {
         unlock();
      }
      
      ossSpinXLatchGuard(const ossSpinXLatchGuard &) = delete;
      ossSpinXLatchGuard &operator=(const ossSpinXLatchGuard &) = delete;

   public:
      void lock()
      {
         if (NULL != _latch && !_locked)
         {
            _latch->get();
            _locked = TRUE;
         }
         return;
      }
      void unlock()
      {
         if (NULL != _latch && _locked)
         {
            _latch->release();
            _locked = FALSE;
         }
         return;
      }
      BOOLEAN isLocked()const
      {
         return _locked;
      }

   private:
      ossSpinXLatch *_latch;
      BOOLEAN _locked = FALSE;
};//class ossSpinXLatchGuard

class ossRWMutexGuard : public SDBObject
{
   public:
      ossRWMutexGuard() = delete;
      ossRWMutexGuard(ossRWMutexBase *mutex,
                      OSS_LATCH_MODE mode,
                      BOOLEAN immediate):
      _mutex(mutex),
      _mode(mode)
      {
         if (immediate)
         {
            lock();
         }
      }

      ~ossRWMutexGuard()
      {
         unlock();
      }

      ossRWMutexGuard(const ossRWMutexGuard &) = delete;
      ossRWMutexGuard &operator=(const ossRWMutexGuard &) = delete;

   public:
      void lock()
      {
         if (NULL != _mutext && !_locked)
         {
            if (SHARED == _mode)
            {
               _mutex->lock_r();
            }
            else
            {
               _mutex->lock_w();
            }
            _locked = TRUE;
         }
         return;
      }
      void unlock()
      {
         if (_locked)
         {
            if (SHARED == _mode)
            {
               _mutex->release_r();
            }
            else
            {
               _mutex->release_w();
            }
            _locked = FALSE;
         }
         return;
      }

   private:
      ossRWMutexBase *_mutex = NULL;
      OSS_LATCH_MODE _mode = SHARED;
      BOOLEAN _locked = FALSE;
};//class ossRWMutexGuard

#endif//SDB_OSS_LATCH_GUARD_H_