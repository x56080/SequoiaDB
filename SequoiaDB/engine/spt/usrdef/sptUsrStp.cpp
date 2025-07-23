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

   Source File Name = sptUsrStp.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrStp.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "ossProc.hpp"
#include "ossIO.hpp"
#include "msgDef.h"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "pmdDaemon.hpp"
#include "pmdDef.hpp"
#include "utilNodeOpr.hpp"
#include "stpToolCommon.hpp"
#include "sptUsrOmaAssit.hpp"
#include "../bson/bsonobj.h"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      Function Define
    */
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrStp, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrStp, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrStp, toString )
   JS_MEMBER_FUNC_DEFINE( _sptUsrStp, close )
   JS_MEMBER_FUNC_DEFINE( _sptUsrStp, start )
   JS_MEMBER_FUNC_DEFINE( _sptUsrStp, runCommand )

   /*
      Function Map
    */
   JS_BEGIN_MAPPING( _sptUsrStp, "Stp" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "toString", toString )
      JS_ADD_MEMBER_FUNC( "close", close )
      JS_ADD_MEMBER_FUNC( "start", start )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_runCommand", runCommand, 0 )
   JS_MAPPING_END()

   /*
      _sptUsrTp implement
    */
   _sptUsrStp::_sptUsrStp()
   {
      _hostName = "localhost" ;
      _serviceName = STP_DEF_SERVICE_NAME ;
   }

   _sptUsrStp::_sptUsrStp( const string &hostName,
                           const string &serviceName,
                           const string &omaServiceName )
   {
      _hostName = hostName ;
      _serviceName = serviceName ;
      _omaServiceName = omaServiceName ;
   }

   _sptUsrStp::~_sptUsrStp()
   {
      _assit.disconnect() ;
   }

   INT32 _sptUsrStp::construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      if ( arg.argc() >= 1 )
      {
         rc = arg.getString( 0, _hostName ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "host name must be string" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get host name, rc: %d", rc ) ;
      }
      if ( arg.argc() >= 2 )
      {
         rc = arg.getString( 1, _serviceName ) ;
         if ( SDB_OK != rc )
         {
            UINT16 port = 0 ;
            rc = arg.getNative( 1, (void*)&port, SPT_NATIVE_INT16 ) ;
            if ( SDB_OK != rc )
            {
               detail = BSON( SPT_ERR << "service name must be string or "
                              "integer" ) ;
            }
            else if ( port <= 0 || port >= 65535 )
            {
               detail = BSON( SPT_ERR << "service name must in range "
                              "( 0, 65535 )" ) ;
               rc = SDB_INVALIDARG ;
            }
            else
            {
               _serviceName = boost::lexical_cast< string >( port ) ;
            }
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to get service name, rc: %d", rc ) ;
      }

      rc = _assit.connect( _hostName.c_str(), _serviceName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to connect to STP node" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to connect %s:%s, rc: %d",
                   _hostName.c_str(), _serviceName.c_str(), rc ) ;

      rval.addSelfProperty( "_host" )->setValue( _hostName ) ;
      rval.addSelfProperty( "_svcname" )->setValue( _serviceName ) ;

   done:
      return rc ;

   error:
      _assit.disconnect() ;
      goto done ;
   }

   INT32 _sptUsrStp::toString( const _sptArguments &arg,
                               _sptReturnVal &rval,
                               BSONObj &detail )
   {
      string name = _hostName ;
      name += ":" ;
      name += _serviceName ;

      rval.getReturnVal().setValue( name ) ;

      return SDB_OK ;
   }

   INT32 _sptUsrStp::destruct()
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrStp::close( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrStp::start( const _sptArguments &arg,
                            _sptReturnVal &rval,
                            BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      BSONObj dummy ;

      if ( arg.argc() != 0 )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "Wrong arguments" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to check argument, should have no "
                      "arguments, rc: %d", rc ) ;
      }

      rc = _runOmaCommand( CMD_NAME_STP_START, dummy, FALSE, rval,
                           detail ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run start STP node command in "
                   "remote sdbcm, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptUsrStp::runCommand( const _sptArguments &arg,
                                 _sptReturnVal &rval,
                                 BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      string command ;
      BSONObj optionObject ;
      INT32 needRecv = 1 ;

      // get command
      rc = arg.getString( 0, command ) ;
      if ( SDB_OUT_OF_BOUND == rc )
      {
         detail = BSON( SPT_ERR << "command must be config" ) ;
         PD_LOG( PDERROR, "Command must be config, rc: %d", rc ) ;
         goto error ;
      }
      else if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "command must be string" ) ;
         PD_LOG( PDERROR, "Command must be string, rc: %d", rc ) ;
         goto error ;
      }
      else if ( command.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "command can't be empty" ) ;
         PD_LOG( PDERROR, "Command can't be empty, rc: %d", rc ) ;
         goto error ;
      }

      // get option
      if ( arg.argc() >= 2 )
      {
         rc = arg.getBsonobj( 1, optionObject ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "option must be BSONObj" ) ;
            PD_LOG( PDERROR, "Failed to get option , rc: %d", rc ) ;
            goto error ;
         }
      }

      // get matchObj
      if ( arg.argc() >= 3 )
      {
         rc = arg.getNative( 2, &needRecv, SPT_NATIVE_INT32 ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "needRecv must be bool" ) ;
            goto error ;
         }
      }

      rc = _runCommand( command.c_str(), optionObject,
                        needRecv > 0 ? TRUE : FALSE, rval, detail ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in STP node, "
                   "rc: %d", command.c_str(), rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptUsrStp::_runCommand( const CHAR *command,
                                  const bson::BSONObj &argument,
                                  BOOLEAN needResult,
                                  _sptReturnVal &rval,
                                  BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      INT32 retCode = SDB_OK ;
      CHAR *retBuffer = NULL ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      if ( !_assit.isConnected() )
      {
         rc = _assit.connect( _hostName.c_str(), _serviceName.c_str() ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "Failed to connect to STP node" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to connect to STP node %s:%s, "
                      "rc: %d", _hostName.c_str(), _serviceName.c_str(), rc ) ;
      }

      rc = _assit.runCommand( command, argument.objdata(),
                              &retBuffer, retCode, TRUE ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to run command in STP node" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in STP node, "
                   "rc: %d", command, rc ) ;

      try
      {
         BSONObj retObject ;
         retObject.init( retBuffer ) ;

         if ( SDB_OK != retCode )
         {
            rc = retCode ;
            detail = BSON( SPT_ERR <<
                           retObject.getStringField( OP_ERR_DETAIL ) ) ;
            PD_LOG( PDERROR, "Failed to run command in STP node, rc: %d",
                    rc ) ;
            goto error ;
         }

         if ( needResult )
         {
            rval.getReturnVal().setValue( retObject ) ;
         }
      }
      catch ( exception &e )
      {
         detail = BSON( SPT_ERR << "Failed to build return object" ) ;
         PD_LOG( PDERROR, "Failed to build return object, occurred error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptUsrStp::_runOmaCommand( const CHAR *command,
                                     const bson::BSONObj &argument,
                                     BOOLEAN needResult,
                                     _sptReturnVal &rval,
                                     BSONObj &detail )
   {
      INT32 rc = SDB_OK ;

      sptUsrOmaAssit omaAssit ;
      INT32 retCode = SDB_OK ;
      CHAR *retBuffer = NULL ;

      SDB_ASSERT( NULL != command, "command is invalid" ) ;

      if ( _omaServiceName.empty() )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "STP node is not bind to OMA" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to stop STP node without OMA" ) ;
      }

      rc = omaAssit.connect( _hostName.c_str(), _omaServiceName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to connect to OMA" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to connect to OMA %s:%s, rc: %d",
                   _hostName.c_str(), _omaServiceName.c_str(), rc ) ;

      rc = omaAssit.runCommand( command, argument.objdata(),
                                &retBuffer, retCode, TRUE ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to stop STP node" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to stop STP node, rc: %d", rc ) ;

      try
      {
         BSONObj retObject ;
         retObject.init( retBuffer ) ;

         if ( SDB_OK != retCode )
         {
            rc = retCode ;
            detail = BSON( SPT_ERR <<
                           retObject.getStringField( OP_ERR_DETAIL ) ) ;
            PD_LOG( PDERROR, "Failed to run command in remote sdbcm, "
                             "rc: %d", rc ) ;
            goto error ;
         }

         if ( needResult )
         {
            rval.getReturnVal().setValue( retObject ) ;
         }
      }
      catch ( exception &e )
      {
         detail = BSON( SPT_ERR << "Failed to build return object" ) ;
         PD_LOG( PDERROR, "Failed to build return object, occurred error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      omaAssit.disconnect() ;
      return rc ;

   error:
      goto done ;
   }

}
