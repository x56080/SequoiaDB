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

   Source File Name = rtnAlterRunner.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnAlterRunner.hpp"
#include "pdTrace.hpp"
#include "pd.hpp"
#include "rtnTrace.hpp"
#include "pmdEDU.hpp"
#include "dpsLogWrapper.hpp"
#include "pmd.hpp"

using namespace bson ;

namespace engine
{
   _rtnAlterRunner::_rtnAlterRunner()
   {

   }

   _rtnAlterRunner::~_rtnAlterRunner()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERRUNNER_INIT, "_rtnAlterRunner::init" ) 
   INT32 _rtnAlterRunner::init( const BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERRUNNER_INIT ) ;
      clear() ;
      rc = _job.init( obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init alter job:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERRUNNER_INIT, rc ) ;
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERRUNNER_CLEAR, "_rtnAlterRunner::clear" )
   void _rtnAlterRunner::clear()
   {
      PD_TRACE_ENTRY( SDB__RTNALTERRUNNER_INIT ) ;
      _job.clear() ;
      PD_TRACE_EXIT( SDB__RTNALTERRUNNER_INIT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERRUNNER_RUN, "_rtnAlterRunner::run" )
   INT32 _rtnAlterRunner::run( _pmdEDUCB *cb, _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERRUNNER_RUN ) ;
      if ( _job.isEmpty() )
      {
         PD_LOG( PDERROR, "runner has not been initialized yet" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      {
      BSONObjIterator i( _job.getTasks() ) ;
      while ( i.more() )
      {
         BSONElement e = i.next() ;
         if ( Object != e.type() )
         {
            PD_LOG( PDERROR, "invalid alter:%s",
                    e.toString( FALSE, TRUE ).c_str() ) ;
            if ( _job.getOptions().ignoreException )
            {
               continue ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               goto error ; 
            }
         }

         rc = _run( e.embeddedObject(), cb,  dpsCB ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to run alter:%d", rc ) ;
            if ( _job.getOptions().ignoreException )
            {
               rc = SDB_OK ;
               continue ;
            }
            else
            {
               goto error ;
            }
         }
      }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERRUNNER_RUN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERRUNNER__RUN, "_rtnAlterRunner::_run" )
   INT32 _rtnAlterRunner::_run( const bson::BSONObj &rpc,
                              _pmdEDUCB *cb,
                              _dpsLogWrapper *dpsCB )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERRUNNER__RUN ) ;
      RTN_ALTER_FUNC func = NULL ;
      BSONElement args ;
      BSONElement name = rpc.getField( FIELD_NAME_NAME ) ;
      if ( String != name.type() )
      {
         PD_LOG( PDERROR, "invalid alter name:%s",
                 rpc.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _getFunc( name.valuestr(), func ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get func of alter[%s], rc:%d",
                 name.valuestr(), rc ) ;
         goto error ;
      }

      rc = (*func)( _job.getName(),
                    _job.getOptionObj(),
                    rpc.getObjectField( FIELD_NAME_ARGS ),
                    cb,
                    dpsCB ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "alter returned error:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERRUNNER__RUN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnAlterRunner::_getFunc( const CHAR *name,
                                  RTN_ALTER_FUNC &func )
   {
      INT32 rc = SDB_OK ;
      _rtnAlterFuncObj obj ;
      rc = _fl.getFuncObj( _job.getType(), name, obj ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get func:%s, rc:%d",
                 name, rc ) ;
         goto error ;
      }

      SDB_ASSERT( obj.isValid(), "must be valid" ) ;
      func = obj.func ;
   done:
      return rc ;
   error:
      goto done ;
   }
}


