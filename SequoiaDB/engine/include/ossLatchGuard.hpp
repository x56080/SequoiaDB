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

class ossSLatchGuard : public SDBObject
{
   public:
      ossSLatchGuard() = delete;
      ossSLatchGuard(ossSLatch *latch, OSS_LATCH_MODE mode):
      _latch(latch),
      _locked(FALSE),
      _mode(mode)
      {
         lock();
      }
      ossSLatchGuard(ossSLatch *latch,
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

      ~ossSLatchGuard()
      {
         unlock();
      }
      
      ossSLatchGuard(const ossSLatchGuard &) = delete;
      ossSLatchGuard &operator=(const ossSLatchGuard &) = delete;

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
      ossSLatch *_latch;
      BOOLEAN _locked = FALSE;
      OSS_LATCH_MODE _mode = SHARED;
};//class ossSLatchGuard

class ossXLatchGuard : public SDBObject
{
   public:
      ossXLatchGuard() = delete;
      ossXLatchGuard(ossXLatch *latch):
      _latch(latch),
      _locked(FALSE)
      {
         lock();
      }
      ossXLatchGuard(ossXLatch *latch,
                         BOOLEAN immediate):
      _latch(latch),
      _locked(FALSE)
      {
         if (immediate)
         {
            lock();
         }
      }  

      ~ossXLatchGuard()
      {
         unlock();
      }
      
      ossXLatchGuard(const ossXLatchGuard &) = delete;
      ossXLatchGuard &operator=(const ossXLatchGuard &) = delete;

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
      ossXLatch *_latch;
      BOOLEAN _locked = FALSE;
};//class ossXLatchGuard

class ossRWMutexGuard : public SDBObject
{
   public:
      ossRWMutexGuard() = delete;
      ossRWMutexGuard(engine::ossRWMutexBase *mutex,
                      OSS_LATCH_MODE mode):
      _mutex(mutex),
      _mode(mode),
      _locked(FALSE)
      {
         autoLock();
      }

      ossRWMutexGuard(engine::ossRWMutexBase *mutex,
                      OSS_LATCH_MODE mode,
                      BOOLEAN immediate):
      _mutex(mutex),
      _mode(mode)
      {
         if (immediate)
         {
            autoLock();
         }
      }
      

      ~ossRWMutexGuard()
      {
         autoUnlock();
      }

      ossRWMutexGuard(const ossRWMutexGuard &) = delete;
      ossRWMutexGuard &operator=(const ossRWMutexGuard &) = delete;

   public:
      void autoLock()
      {
         if (NULL != _mutex && !_locked)
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
      void autoUnlock()
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
      engine::ossRWMutexBase *_mutex = NULL;
      OSS_LATCH_MODE _mode = SHARED;
      BOOLEAN _locked = FALSE;
};//class ossRWMutexGuard

#endif//SDB_OSS_LATCH_GUARD_H_