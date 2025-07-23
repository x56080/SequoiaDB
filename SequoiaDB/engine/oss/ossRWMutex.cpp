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

   Source File Name = ossRWMutex.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossRWMutex.hpp"
#include "pdTrace.hpp"
#include "ossTrace.hpp"
namespace engine
{

#define OSS_RW_MAX_TIMEOUT    (0x7FFFFFFF)
#define OSS_RW_READ_TIMEOUT   (203)
#define OSS_RW_WIRTE_TIMEOUT  (107)

   _ossRWMutex::_ossRWMutex ( UINT32 type )
      :_r(0), _w(0), _type(type)
   {

   }

   _ossRWMutex::~_ossRWMutex ()
   {
   }

   UINT32 _ossRWMutex::_makeTimeout( INT32 & millisec, UINT32 timeout )
   {
      if ( millisec < 0 )
      {
         millisec = OSS_RW_MAX_TIMEOUT ;
      }
      else if ( 0 == millisec )
      {
         millisec = 1 ;
      }

      return (UINT32)millisec < timeout ? (UINT32)millisec : timeout ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSRWM_LOCK_R, "_ossRWMutex::lock_r" )
   INT32 _ossRWMutex::lock_r ( INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OSSRWM_LOCK_R );

      UINT32 timeout = _makeTimeout( millisec, OSS_RW_READ_TIMEOUT ) ;

      _r.inc () ;

      while ( _w.fetch() )
      {
         boost::chrono::milliseconds timeoutObj = boost::chrono::milliseconds(timeout) ;
         boost::mutex::scoped_lock lock ( _mutex ) ;

         /// double check
         if ( !_w.fetch() )
         {
            break ;
         }

         _r.dec () ;

         /// notify all
         _cond.notify_all () ;

         /// wait
         do
         {
            if ( boost::cv_status::timeout == _cond.wait_for( lock, timeoutObj ) )
            {
               millisec -= timeout ;
               if ( millisec < 0 )
               {
                  rc = SDB_TIMEOUT ;
                  goto done ;
               }
            }
         } while( _w.fetch() ) ;

         _r.inc () ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_LOCK_R, rc );
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSRWM_TRY_LOCK_R, "_ossRWMutex::try_lock_r" )
   BOOLEAN _ossRWMutex::try_lock_r ()
   {
      INT32 ret = FALSE ;
      PD_TRACE_ENTRY ( SDB__OSSRWM_TRY_LOCK_R );

      _r.inc () ;

      if ( _w.fetch() )
      {
         boost::mutex::scoped_lock lock ( _mutex ) ;

         /// double check
         if ( !_w.fetch() )
         {
            ret = TRUE ;
         }
         else
         {
            _r.dec () ;
            /// notify all
            _cond.notify_all () ;
         }
      }
      else
      {
         ret = TRUE ;
      }

      PD_TRACE_EXIT ( SDB__OSSRWM_TRY_LOCK_R );
      return ret ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSRWM_LOCK_W, "_ossRWMutex::lock_w" )
   INT32 _ossRWMutex::lock_w ( INT32 millisec )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OSSRWM_LOCK_W );

      UINT32 timeout = _makeTimeout( millisec, OSS_RW_WIRTE_TIMEOUT ) ;

      while ( !_w.compareAndSwap( 0, 1 ) )
      {
         if ( _type & RW_SHARDWRITE )
         {
            _w.inc () ;
            break ;
         }

         boost::chrono::milliseconds timeoutObj = boost::chrono::milliseconds(timeout) ;
         boost::mutex::scoped_lock lock ( _mutex ) ;

         /// wait
         if ( boost::cv_status::timeout == _cond.wait_for( lock, timeoutObj ) )
         {
            millisec -= timeout ;
            if ( millisec < 0 )
            {
               rc = SDB_TIMEOUT ;
               goto done ;
            }
         }
      }

      while ( _r.fetch () )
      {
         boost::chrono::milliseconds timeoutObj = boost::chrono::milliseconds(timeout) ;
         boost::mutex::scoped_lock lock ( _mutex ) ;

         /// double check
         if ( !_r.fetch() )
         {
            break ;
         }

         /// wait
         if ( boost::cv_status::timeout == _cond.wait_for( lock, timeoutObj ) )
         {
            millisec -= timeout ;
            if ( millisec < 0 )
            {
               _w.dec () ;

               /// notify all
               _cond.notify_all () ;

               rc = SDB_TIMEOUT ;
               goto done ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_LOCK_W, rc );
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSRWM_TRY_LOCK_W, "_ossRWMutex::try_lock_w" )
   BOOLEAN _ossRWMutex::try_lock_w ()
   {
      INT32 ret = FALSE ;
      PD_TRACE_ENTRY ( SDB__OSSRWM_TRY_LOCK_W );

      if ( ! _w.compareAndSwap( 0, 1 ) )
      {
         if ( _type & RW_SHARDWRITE )
         {
            _w.inc () ;
         }
         else
         {
            goto done ;
         }
      }

      if ( _r.fetch () )
      {
         boost::mutex::scoped_lock lock ( _mutex ) ;

         /// double check
         if ( _r.fetch() )
         {
            _w.dec () ;

            /// notify all
            _cond.notify_all () ;

            goto done ;
         }
      }

      ret = TRUE ;

   done:
      PD_TRACE_EXIT ( SDB__OSSRWM_TRY_LOCK_W );
      return ret ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSRWM_RLS_R, "_ossRWMutex::release_r" )
   INT32 _ossRWMutex::release_r ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OSSRWM_RLS_R );

      if ( _r.compare ( 0 ) )
      {
         rc = SDB_SYS ;
         goto done ;
      }

      _r.dec () ;

      if ( _w.fetch () )
      {
         boost::mutex::scoped_lock lock ( _mutex ) ;
         /// notify all
         _cond.notify_all () ;
      }
   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_RLS_R, rc );
      return rc ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB__OSSRWM_RLS_W, "_ossRWMutex::release_w" )
   INT32 _ossRWMutex::release_w ()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__OSSRWM_RLS_W );
      if ( _w.compare ( 0 ) )
      {
         rc = SDB_SYS ;
         goto done ;
      }

      _w.dec () ;

      {
         boost::mutex::scoped_lock lock ( _mutex ) ;
         /// notify all
         _cond.notify_all () ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_RLS_W, rc );
      return rc ;
   }

   _ossScopedRWLock::_ossScopedRWLock ( ossRWMutexBase * pMutex,
                                        OSS_LATCH_MODE mode )
   {
      _pMutex = pMutex ;
      _mode = mode ;

      if ( _pMutex )
      {
         if ( SHARED == _mode )
         {
            _pMutex->lock_r () ;
         }
         else
         {
            _pMutex->lock_w () ;
         }
      }
   }

   _ossScopedRWLock::~_ossScopedRWLock ()
   {
      if ( _pMutex )
      {
         if ( SHARED == _mode )
         {
            _pMutex->release_r () ;
         }
         else
         {
            _pMutex->release_w () ;
         }
         _pMutex = NULL ;
      }
   }

}
