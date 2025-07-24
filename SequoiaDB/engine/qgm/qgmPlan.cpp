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

   Source File Name = qgmPlan.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains declare for QGM operators

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/09/2013  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "qgmPlan.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "qgmTrace.hpp"

namespace engine
{
   _qgmPlan::_qgmPlan( QGM_PLAN_TYPE type, const qgmField &alias )
   :_type( type ),
   _eduCB( NULL ),
   _executed( FALSE ),
   _initialized( FALSE ),
   _merge( FALSE ),
   _param( NULL )
   {
      SDB_ASSERT( QGM_PLAN_TYPE_MAX != _type, "impossible" ) ;
      _alias = alias ;
   }

   _qgmPlan::~_qgmPlan()
   {
      QGM_PINPUT::iterator itr = _input.begin() ;
      for ( ; itr != _input.end(); itr++ )
      {
         SAFE_OSS_DELETE( *itr ) ;
      }
      _input.clear() ;
   }

   void _qgmPlan::close()
   {
      QGM_PINPUT::iterator itr = _input.begin() ;
      for ( ; itr != _input.end(); itr++ )
      {
         (*itr)->close() ;
      }
   }

   INT32 _qgmPlan::addChild( _qgmPlan *child )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != child, "impossible" ) ;
      if ( NULL == child )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         _input.push_back( child ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLAN_EXECUTE, "_qgmPlan::execute" )
   INT32 _qgmPlan::execute( _pmdEDUCB *eduCB )
   {
      PD_TRACE_ENTRY( SDB__QGMPLAN_EXECUTE ) ;
      INT32 rc = SDB_OK ;

      if ( !_initialized )
      {
         PD_LOG( PDERROR, "node is not initialized, type:%s",
                 toString().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( NULL == eduCB )
      {
         SDB_ASSERT( NULL != eduCB, "can not be NULL" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _execute( eduCB ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _eduCB = eduCB ;
      _executed = TRUE ;
   done:
      PD_TRACE_EXITRC( SDB__QGMPLAN_EXECUTE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION( SDB__QGMPLAN_FETCHNEXT, "_qgmPlan::fetchNext" )
   INT32 _qgmPlan::fetchNext( qgmFetchOut &next )
   {
      PD_TRACE_ENTRY( SDB__QGMPLAN_FETCHNEXT ) ;
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != _eduCB, "impossible" ) ;
      SDB_ASSERT( _executed, "impossible" ) ;

      if ( !_executed || NULL == _eduCB )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _fetchNext( next ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDDEBUG, "failed to fecth next:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__QGMPLAN_FETCHNEXT, rc ) ;
      return rc ;
   error:
      close() ;
      goto done ;
   }

   

}
