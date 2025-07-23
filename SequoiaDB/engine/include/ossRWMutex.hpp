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

   Source File Name = ossRWMutex.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef OSS_RWMUTEX_HPP_
#define OSS_RWMUTEX_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "ossAtomic.hpp"
#include "ossEvent.hpp"
#include "ossLatch.hpp"

namespace engine
{

#define RW_EXCLUSIVEWRITE        0x0000
#define RW_SHARDWRITE            0x0001

   class _ossRWMutexBase : public SDBObject
   {
      public:
         virtual ~_ossRWMutexBase(){} ;
         virtual INT32 lock_r ( INT32 millisec = -1 ) = 0 ;
         virtual INT32 lock_w ( INT32 millisec = -1 ) = 0 ;
         virtual INT32 release_r () = 0 ;
         virtual INT32 release_w () = 0 ;
   };
   typedef _ossRWMutexBase ossRWMutexBase ;

   class _ossRWMutex : public ossRWMutexBase
   {
      public:
         _ossRWMutex ( UINT32 type = RW_EXCLUSIVEWRITE ) ;
         ~_ossRWMutex () ;

      public:
         INT32 lock_r ( INT32 millisec = -1 ) ;
         INT32 lock_w ( INT32 millisec = -1 ) ;
         INT32 release_r () ;
         INT32 release_w () ;
         BOOLEAN try_lock_r() ;
         BOOLEAN try_lock_w() ;

      protected:
         UINT32   _makeTimeout( INT32 &millisec, UINT32 timeout ) ;

      private:
         ossAtomic32        _r ;
         ossAtomic32        _w ;
         UINT32             _type ;
         mutable boost::mutex       _mutex ;
         boost::condition_variable  _cond ;

   };

   typedef _ossRWMutex ossRWMutex ;

   class _ossScopedRWLock
   {
      public:
         _ossScopedRWLock ( ossRWMutexBase *pMutex, OSS_LATCH_MODE mode ) ;
         ~_ossScopedRWLock () ;

      private:
         ossRWMutexBase *_pMutex ;
         OSS_LATCH_MODE _mode ;
   };

   typedef _ossScopedRWLock ossScopedRWLock ;
}

#endif //OSS_RWMUTEX_HPP_

