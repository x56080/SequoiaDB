/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
         _r.dec () ;

         if ( _event.wait ( timeout ) == SDB_TIMEOUT )
         {
            millisec -= timeout ;
            if ( millisec < 0 )
            {
               rc = SDB_TIMEOUT ;
               goto done ;
            }
         }

         _r.inc () ;
      }

   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_LOCK_R, rc );
      return rc ;
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

         if ( _event.wait ( timeout ) == SDB_TIMEOUT )
         {
            millisec -= timeout ;
            if ( millisec < 0 )
            {
               rc = SDB_TIMEOUT ;
               goto done ;
            }
         }
      }

      /*while ( !(_type&RW_SHARDWRITE) && _w.fetch() > 1 )
      {
         _w.dec () ;

         if ( _event.wait ( millisec ) == SDB_TIMEOUT )
         {
            rc = SDB_TIMEOUT ;
            goto done ;
         }

         _w.inc () ;
      }*/

      while ( _r.fetch () )
      {
         if ( _event.wait ( timeout ) == SDB_TIMEOUT )
         {
            millisec -= timeout ;
            if ( millisec < 0 )
            {
               _w.dec () ;
               _event.signalAll () ;
               rc = SDB_TIMEOUT ;
               goto done ;
            }
         }
      }

   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_LOCK_W, rc );
      return rc ;
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
         _event.signalAll () ;
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
      _event.signalAll () ;
   done :
      PD_TRACE_EXITRC ( SDB__OSSRWM_RLS_W, rc );
      return rc ;
   }

   _ossScopedRWLock::_ossScopedRWLock ( ossRWMutex * pMutex,
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
