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

   Source File Name = pmdTransExecutor.cpp

   Descriptive Name = Operating System Services Types Header

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declare for data types used in
   SequoiaDB.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/08/2018  XJH Initial Draft

   Last Changed =

*******************************************************************************/
#include "pmdTransExecutor.hpp"
#include "pmdEDU.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"

namespace engine
{

   /*
      _pmdTransExecutor implement
   */
   _pmdTransExecutor::_pmdTransExecutor( _pmdEDUCB *cb, monMonitorManager *monMgr )
      : _dpsTransExecutor( monMgr )
   {
      SDB_ASSERT( cb, "CB can't be NULL" ) ;
      _pEDUCB = cb ;
   }

   _pmdTransExecutor::~_pmdTransExecutor()
   {
      _pEDUCB = NULL ;
   }

   EDUID _pmdTransExecutor::getEDUID() const
   {
      if ( _pEDUCB )
      {
         return _pEDUCB->getID() ;
      }
      return PMD_INVALID_EDUID ;
   }

   UINT32 _pmdTransExecutor::getTID() const
   {
      if ( _pEDUCB )
      {
         return _pEDUCB->getTID() ;
      }
      return PMD_INVALID_EDUID ;
   }

   void _pmdTransExecutor::wakeup( INT32 wakeupRC )
   {
      SDB_ASSERT( _pEDUCB, "EDU CB can't be NULL" ) ;
      if ( _pEDUCB )
      {
         UINT32 tmpDummy = 0 ;
         UINT32 tmpRC = (UINT32)wakeupRC ;
         _pEDUCB->postEvent( pmdEDUEvent( PMD_EDU_EVENT_LOCKWAKEUP,
                                          PMD_EDU_MEM_NONE,
                                          NULL,
                                          ossPack32To64( tmpDummy, tmpRC ) ) ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__PMDTRANSEXECUTOR_WAIT, "_pmdTransExecutor::wait" )
   INT32 _pmdTransExecutor::wait( INT64 timeout )
   {
      INT32 rc = SDB_SYS ;
      PD_TRACE_ENTRY ( SDB__PMDTRANSEXECUTOR_WAIT );
      pmdEDUEvent data ;

      if ( _pEDUCB )
      {
         if ( _pEDUCB->waitEvent( PMD_EDU_EVENT_LOCKWAKEUP, data, timeout ) )
         {
            UINT32 tmpDummy = 0, tmpRC = 0 ;
            ossUnpack32From64( data._userData, tmpDummy, tmpRC ) ;
            rc = (INT32)tmpRC ;
         }
         else if ( _pEDUCB->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
         }
         else
         {
            rc = SDB_TIMEOUT ;
         }
      }

      PD_TRACE_EXITRC ( SDB__PMDTRANSEXECUTOR_WAIT, rc );
      return rc ;
   }

   IExecutor* _pmdTransExecutor::getExecutor()
   {
      return _pEDUCB ;
   }

   BOOLEAN _pmdTransExecutor::isInterrupted ()
   {
      if ( NULL != _pEDUCB && _pEDUCB->isInterrupted() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

}

