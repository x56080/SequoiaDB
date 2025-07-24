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

   Source File Name = ossLatchGuard.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/08/2020  WY  Initial Draft

   Last Changed =

*******************************************************************************/
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
      ossRWMutexGuard(ossRWMutexGuard &&o):
      _mutex(o._mutex),
      _mode(o._mode),
      _locked(o._locked)
      {
         o._mutex = nullptr;
         o._mode = SHARED;
         o._locked = FALSE;
      }
      ossRWMutexGuard &operator=(ossRWMutexGuard &&o)
      {
         autoUnlock();
         _mutex = o._mutex;
         _mode = o._mode;
         _locked = o._locked;
         o._mutex = nullptr;
         o._mode = SHARED;
         o._locked = FALSE;
         return *this;
      }

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