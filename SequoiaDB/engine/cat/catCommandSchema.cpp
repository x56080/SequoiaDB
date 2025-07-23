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

   Source File Name = catCommandSchema.cpp

   Descriptive Name = Catalogue schema commands.

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains catalog command class.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          02/07/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "catCommandSchema.hpp"
#include "catLevelLock.hpp"
#include "catCommon.hpp"
#include "catContextData.hpp"
#include "rtnAlterTask.hpp"
#include "rtn.hpp"
#include "catTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _catCMDCreateSchema implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDCreateSchema )

   _catCMDCreateSchema::_catCMDCreateSchema()
   : _catWriteCMDBase(),
     _schema()
   {
   }

   _catCMDCreateSchema::~_catCMDCreateSchema()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTSCHEMA_INIT, "_catCMDCreateSchema::init" )
   INT32 _catCMDCreateSchema::init( const CHAR *pQuery,
                                    const CHAR *pSelector,
                                    const CHAR *pOrderBy,
                                    const CHAR *pHint,
                                    INT32 flags,
                                    INT64 numToSkip,
                                    INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCMDCRTSCHEMA_INIT ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;

         rc = _schema.parse( boQuery, TRUE, FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse schema, rc: %d", rc ) ;

         rc = _schema.adjustOID( FALSE ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to adjust schema, rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse schema, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTSCHEMA_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDCRTSCHEMA_DOIT, "_catCMDCreateSchema::doit" )
   INT32 _catCMDCreateSchema::doit( _pmdEDUCB *cb,
                                    rtnContextBuf &ctxBuf,
                                    INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCMDCRTSCHEMA_DOIT ) ;

      INT16 w = sdbGetCatalogueCB()->majoritySize() ;

      pdLogRCShield shield ;
      shield.addRC( SDB_IXM_DUP_KEY ) ;

      rc = catAddSchema( _schema, cb, w ) ;
      if ( SDB_IXM_DUP_KEY == rc )
      {
         // rewrite return code
         rc = SDB_SCHEMA_EXIST ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to add schema [%s], rc: %d",
                   _schema.getName(), rc ) ;

      PD_LOG( PDDEBUG, "Create info schema [%s] with define [%s] succeed",
              _schema.getName(), _schema.getDefine().toString().c_str() ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTSCHEMA_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDDropSchema implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDDropSchema )

   _catCMDDropSchema::_catCMDDropSchema()
   : _catWriteCMDBase(),
     _schemaName( NULL )
   {
   }

   _catCMDDropSchema::~_catCMDDropSchema()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPSCHEMA_INIT, "_catCMDDropSchema::init" )
   INT32 _catCMDDropSchema::init( const CHAR *pQuery,
                                  const CHAR *pSelector,
                                  const CHAR *pOrderBy,
                                  const CHAR *pHint,
                                  INT32 flags,
                                  INT64 numToSkip,
                                  INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCMDDROPSCHEMA_INIT ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;

         BSONElement ele = boQuery.getField( FIELD_NAME_NAME ) ;
         PD_CHECK( String == ele.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to get field [%s], it is not a string",
                   FIELD_NAME_NAME ) ;
         _schemaName = ele.valuestrsafe() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse command, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDDROPSCHEMA_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDDROPSCHEMA_DOIT, "_catCMDDropSchema::doit" )
   INT32 _catCMDDropSchema::doit( _pmdEDUCB *cb,
                                  rtnContextBuf &ctxBuf,
                                  INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCMDCRTSCHEMA_DOIT ) ;

      INT16 w = sdbGetCatalogueCB()->majoritySize() ;
      utilSchema schema ;

      rc = catGetSchema( _schemaName, schema, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get schema [%s], rc: %d",
                   _schemaName, rc ) ;

      PD_LOG_MSG_CHECK( NULL == schema.getCollection(),
                        SDB_OPERATION_INCOMPATIBLE, error, PDERROR,
                        "Failed to drop schema [%s], it is bind to "
                        "collection [%s]", _schemaName,
                        schema.getCollection() ) ;

      rc = catRemoveSchema( _schemaName, cb, w ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to remove schema [%s], rc: %d",
                   _schemaName, rc ) ;

      PD_LOG( PDDEBUG, "Remove info schema [%s] succeed", _schemaName ) ;

   done:
      PD_TRACE_EXITRC( SDB_CATCMDCRTSCHEMA_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _catCMDAlterSchema implement
    */
   CAT_IMPLEMENT_CMD_AUTO_REGISTER( _catCMDAlterSchema )

   _catCMDAlterSchema::_catCMDAlterSchema()
   : _action()
   {
   }

   _catCMDAlterSchema::~_catCMDAlterSchema()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDALTERSCHEMA_INIT, "_catCMDAlterSchema::init" )
   INT32 _catCMDAlterSchema::init( const CHAR *pQuery,
                                   const CHAR *pSelector,
                                   const CHAR *pOrderBy,
                                   const CHAR *pHint,
                                   INT32 flags,
                                   INT64 numToSkip,
                                   INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCMDALTERSCHEMA_INIT ) ;

      try
      {
         BSONObj boQuery( pQuery ) ;

         rc = _action.parse( boQuery ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse alter action, "
                      "rc: %d", rc ) ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse command, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDALTERSCHEMA_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_CATCMDALTERSCHEMA_DOIT, "_catCMDAlterSchema::doit" )
   INT32 _catCMDAlterSchema::doit( _pmdEDUCB *cb,
                                   rtnContextBuf &ctxBuf,
                                   INT64 &contextID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB_CATCMDALTERSCHEMA_DOIT ) ;

      INT16 w = sdbGetCatalogueCB()->majoritySize() ;
      utilSchema schema ;
      const CHAR *schemaName = _action.getSchemaName() ;

      rc = catGetSchema( schemaName, schema, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get schema [%s], rc: %d",
                   schemaName, rc ) ;

      if ( NULL != schema.getCollection() )
      {
         SDB_RTNCB *rtnCB = pmdGetKRCB()->getRTNCB() ;
         catCtxAlterCL::sharePtr context ;
         const CHAR *collectionName = schema.getCollection() ;

         BSONObj boAlterCommand ;

         rc = _rtnCLAlterSchemaTask::buildAlterCommand( collectionName,
                                                        _action.getActionObject(),
                                                        boAlterCommand ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build alter command for "
                      "collection [%s], rc: %d", collectionName, rc ) ;

         rc = rtnCB->contextNew( RTN_CONTEXT_CAT_ALTER_CL,
                                 context,
                                 contextID,
                                 cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create alter collection context, "
                      "rc: %d", rc ) ;

         rc = context->open( boAlterCommand, ctxBuf, cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to open alter collection context, "
                      "rc: %d", rc ) ;
      }
      else
      {
         rc = _action.checkSchema( schema ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check alter action on "
                      "schema [%s], rc: %d", schemaName, rc ) ;

         rc = _action.applySchema( schema ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to apply alter action on "
                      "schema [%s], rc: %d", schemaName, rc ) ;

         rc = catUpdateSchema( schema, _action.getAlterMask(), cb, w ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to update schema [%s], rc: %d",
                      schemaName, rc ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_CATCMDALTERSCHEMA_DOIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
