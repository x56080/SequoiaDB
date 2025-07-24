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

   Source File Name = rtnAlterJob.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/05/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnAlterJob.hpp"
#include "pd.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"
#include "msgDef.hpp"

using namespace bson ;

namespace engine
{
   _rtnAlterJob::_rtnAlterJob()
   :_type( RTN_ALTER_INVALID ),
    _name( NULL ),
    _v( 0 )
   {

   }

   _rtnAlterJob::~_rtnAlterJob()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB_INIT, "_rtnAlterJob::init" )
   INT32 _rtnAlterJob::init( const bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERJOB_INIT ) ;
      SDB_ASSERT( _obj.isEmpty(), "clear it first" ) ;
      clear() ;

      BSONObjBuilder builder ;
      BSONArrayBuilder arrBuilder ;
      string lower ;
      try
      {
         /// version
         BSONElement e = obj.getField( FIELD_NAME_VERSION ) ;
         if ( !e.isNumber() )
         {
            PD_LOG( PDERROR, "invalid type of field [%s], request obj: %s",
                    FIELD_NAME_VERSION, obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _v = e.Number() ;
         if ( SDB_ALTER_VERSION != _v )
         {
            PD_LOG( PDERROR, "invalid version:%d", _v ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         builder.append( FIELD_NAME_VERSION, _v ) ;

         /// type
         e = obj.getField( FIELD_NAME_ALTER_TYPE ) ;
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "invalid type of field [%s],request obj: %s",
                    FIELD_NAME_ALTER_TYPE, obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         _type = _getObjType( e.valuestrsafe() ) ;
         if ( RTN_ALTER_INVALID == _type )
         {
            PD_LOG( PDERROR, "invalid alter type:%s", e.valuestrsafe() ) ;
            goto error ;
         }

         lower.assign( e.valuestrsafe() );
         boost::algorithm::to_lower( lower ) ;
         builder.append( FIELD_NAME_ALTER_TYPE, lower ) ;

         /// name
         e = obj.getField( FIELD_NAME_NAME ) ;
         if ( String != e.type() )
         {
            PD_LOG( PDERROR, "invalid type of field [%s], request obj: %s",
                    FIELD_NAME_NAME, obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         builder.append( FIELD_NAME_NAME, e.valuestrsafe() ) ;

         /// alter list
         e = obj.getField( FIELD_NAME_ALTER ) ;
         if ( Array != e.type() &&
              Object != e.type() )
         {
            PD_LOG( PDERROR, "invalid type of field [%s],request obj: %s",
                    FIELD_NAME_ALTER, obj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         rc = _extractTasks( e, arrBuilder ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract task list:%d", rc ) ;
            goto error ;
         }

         builder.append( FIELD_NAME_ALTER, arrBuilder.arr() ) ;

         /// options is not necessary.
         e = obj.getField( FIELD_NAME_OPTIONS ) ;
         if ( Object == e.type() )
         {
            _extractOptions( e.embeddedObject() ) ;
         }

         _obj = builder.obj() ;
         _name = _obj.getStringField( FIELD_NAME_NAME ) ;
         _tasks = _obj.getObjectField( FIELD_NAME_ALTER ) ;
         _optionsObj = _obj.getObjectField( FIELD_NAME_OPTIONS ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected error heppened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNALTERJOB_INIT, rc ) ;
      return rc ;
   error:
      clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB_CLEAR, "_rtnAlterJob::clear" )
   void _rtnAlterJob::clear()
   {
      PD_TRACE_ENTRY( SDB__RTNALTERJOB_CLEAR ) ;
      _type = RTN_ALTER_INVALID ;
      _name = NULL ;
      _v = 0 ;
      _optionsObj = BSONObj() ;
      _tasks = BSONObj() ;
      _obj = BSONObj() ;
      _options.reset() ;
      PD_TRACE_EXIT( SDB__RTNALTERJOB_CLEAR ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__EXTRACTOPTIONS, "_rtnAlterJob::_extractOptions" )
   void _rtnAlterJob::_extractOptions( const bson::BSONObj &obj )
   {
      PD_TRACE_ENTRY( SDB__RTNALTERJOB__EXTRACTOPTIONS ) ;
      _options.ignoreException = obj.getBoolField( FIELD_NAME_IGNORE_EXCEPTION ) ;
      PD_TRACE_EXIT( SDB__RTNALTERJOB__EXTRACTOPTIONS ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__GETOBJTYPE, "_rtnAlterJob::_getObjType" )
   RTN_ALTER_TYPE _rtnAlterJob::_getObjType( const CHAR *name ) const
   {
      RTN_ALTER_TYPE type = RTN_ALTER_INVALID ;
      PD_TRACE_ENTRY( SDB__RTNALTERJOB__GETOBJTYPE ) ;
      string lower( name ) ;
      boost::algorithm::to_lower( lower ) ;
      if ( 0 == ossStrcmp( SDB_ALTER_CL, lower.c_str() ) )
      {
         type = RTN_ALTER_TYPE_CL ;
      }
      else
      {
         PD_LOG( PDERROR, "invalid type:%s", name ) ;
      }
      PD_TRACE_EXIT( SDB__RTNALTERJOB__GETOBJTYPE ) ;
      return type ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNALTERJOB__EXTRACTTASKS1, "_rtnAlterJob::_extractTasks" )
   INT32 _rtnAlterJob::_extractTasks( const BSONElement &tasks,
                                      BSONArrayBuilder &builder )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNALTERJOB__EXTRACTTASKS1 ) ;

      BSONObj obj = tasks.embeddedObject() ;
      if ( Object == tasks.type() )
      {
         const CHAR *taskName = NULL ;
         BSONObjBuilder taskBuilder ;
         BSONObj argObj ;
         _rtnAlterFuncObj fObj ;

         taskName = obj.getStringField( FIELD_NAME_NAME ) ;
         rc = _fl.getFuncObj( _type, taskName, fObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "invalid name:%s", taskName ) ;
            goto error ;
         }

         SDB_ASSERT( fObj.isValid(), "must be valid" ) ;

         argObj = obj.getObjectField( FIELD_NAME_ARGS ) ;
         rc = (*( fObj.verify ))( argObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "invalid args of func:%s", taskName ) ;
            goto error ;
         }

         taskBuilder.append( FIELD_NAME_NAME, fObj.name ) ;
         taskBuilder.append( FIELD_NAME_ARGS, argObj ) ;
         taskBuilder.append( FIELD_NAME_TASKTYPE, fObj.type ) ;
         builder << taskBuilder.obj() ;
      }
      else
      {
         BSONObjIterator i( obj ) ;
         while ( i.more() )
         {
            BSONElement e = i.next() ;
            if ( Object != e.type() )
            {
               PD_LOG( PDERROR, "type of task should be object" ) ;
               rc = SDB_INVALIDARG ;
               goto error ; 
            }

            rc = _extractTasks( e, builder ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to extract task:%d", rc ) ;
               goto error ;
            }
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNALTERJOB__EXTRACTTASKS1, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

