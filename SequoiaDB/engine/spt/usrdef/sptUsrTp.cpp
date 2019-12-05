/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sptUsrTp.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          18/08/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "sptUsrTp.hpp"
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
#include "tpToolCommon.hpp"
#include "sptUsrOmaAssit.hpp"
#include "../bson/bsonobj.h"

using namespace std ;
using namespace bson ;

namespace engine
{
   #define SDB_SDBCM_WAIT_TIMEOUT      ( 15 )   /// seconds

   /*
      Function Define
   */
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrTp, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrTp, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrTp, toString )
   JS_MEMBER_FUNC_DEFINE( _sptUsrTp, close )
   JS_MEMBER_FUNC_DEFINE( _sptUsrTp, start )
   JS_MEMBER_FUNC_DEFINE( _sptUsrTp, stop )
   JS_MEMBER_FUNC_DEFINE( _sptUsrTp, runCommand )

   /*
      Function Map
   */
   JS_BEGIN_MAPPING( _sptUsrTp, "SdbTP" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC( "toString", toString )
      JS_ADD_MEMBER_FUNC( "close", close )
      JS_ADD_MEMBER_FUNC( "start", start )
      JS_ADD_MEMBER_FUNC( "stop", stop )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_runCommand", runCommand, 0 )
   JS_MAPPING_END()

   /*
      _sptUsrTp implement
    */
   _sptUsrTp::_sptUsrTp()
   {
      _hostName = "localhost" ;
      _serviceName = TP_DEF_SVCNAME ;
   }

   _sptUsrTp::_sptUsrTp( const string &hostName,
                         const string &serviceName,
                         const string &omaServiceName )
   {
      _hostName = hostName ;
      _serviceName = serviceName ;
      _omaServiceName = omaServiceName ;
   }

   _sptUsrTp::~_sptUsrTp()
   {
      _assit.disconnect() ;
   }

   INT32 _sptUsrTp::construct( const _sptArguments &arg,
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
         detail = BSON( SPT_ERR << "Failed to connect to TP node" ) ;
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

   INT32 _sptUsrTp::toString( const _sptArguments &arg,
                              _sptReturnVal &rval,
                              BSONObj &detail )
   {
      string name = _hostName ;
      name += ":" ;
      name += _serviceName ;

      rval.getReturnVal().setValue( name ) ;

      return SDB_OK ;
   }

   INT32 _sptUsrTp::destruct()
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrTp::close( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           BSONObj &detail )
   {
      return _assit.disconnect() ;
   }

   INT32 _sptUsrTp::start( const _sptArguments &arg,
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

      rc = _runOmaCommand( CMD_NAME_TP_START, dummy, FALSE, rval,
                           detail ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run start TP node command in "
                   "remote sdbcm, rc: %d", rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptUsrTp::stop( const _sptArguments &arg,
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

      rc = _runOmaCommand( CMD_NAME_TP_STOP, dummy, FALSE, rval,
                           detail ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to run stop TP node command in "
                   "remote sdbcm, rc: %d", rc ) ;

   done:
      _assit.disconnect() ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptUsrTp::runCommand( const _sptArguments &arg,
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
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in TP node, "
                   "rc: %d", command.c_str(), rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sptUsrTp::_runCommand( const CHAR *command,
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
            detail = BSON( SPT_ERR << "Failed to connect to TP node" ) ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to connect to TP node %s:%s, "
                      "rc: %d", _hostName.c_str(), _serviceName.c_str(), rc ) ;
      }

      rc = _assit.runCommand( command, argument.objdata(),
                              &retBuffer, retCode, needResult ) ;
      if ( SDB_OK != rc )
      {
         detail = BSON( SPT_ERR << "Failed to run command in TP node" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to run command [%s] in TP node, "
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
            PD_LOG( PDERROR, "Failed to run command in TP node, rc: %d",
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

   INT32 _sptUsrTp::_runOmaCommand( const CHAR *command,
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
         detail = BSON( SPT_ERR << "TP node is not bind to OMA" ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to stop TP node without OMA" ) ;
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
         detail = BSON( SPT_ERR << "Failed to stop TP node" ) ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to stop TP node, rc: %d", rc ) ;

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
