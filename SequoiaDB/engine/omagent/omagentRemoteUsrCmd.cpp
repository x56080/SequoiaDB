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

   Source File Name = omagentRemoteUsrCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/03/2016  WJM Initial Draft

   Last Changed =

*******************************************************************************/
#include "omagentRemoteUsrCmd.hpp"
#include "omagentMgr.hpp"
#include "omagentDef.hpp"
#include "ossCmdRunner.hpp"
#include <boost/algorithm/string.hpp>

#define SPT_USER_CMD_ONCE_SLEEP_TIME            ( 2 )
using namespace bson ;

namespace engine
{
   /*
      _remoteCmdRun implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteCmdRun )

    _remoteCmdRun::_remoteCmdRun()
   {
   }

   _remoteCmdRun::~_remoteCmdRun()
   {
   }

   const CHAR* _remoteCmdRun::name()
   {
      return OMA_REMOTE_CMD_RUN ;
   }

   INT32 _remoteCmdRun::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string command ;
      string ev ;
      UINT32 timeout = 0 ;
      UINT32 useShell = 1 ;
      ossCmdRunner runner ;
      string strOut = "" ;
      UINT32 retCode = 0 ;
      BSONObjBuilder builder ;

      // get argument
      if ( FALSE == _valueObj.hasField( "command" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "command" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be string" ) ;
         goto error ;
      }
      command = _valueObj.getStringField( "command" ) ;

      if ( TRUE == _valueObj.hasField( "args" ) )
      {
         if( String != _valueObj.getField( "args" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "args must be string" ) ;
            goto error ;
         }
         else
         {
            ev = _valueObj.getStringField( "args" ) ;
         }
      }

      if ( TRUE == _optionObj.hasField( "timeout" ) )
      {
         if( NumberInt != _optionObj.getField( "timeout" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "timeout must be int" ) ;
            goto error ;
         }
         else
         {
            timeout = _optionObj.getIntField( "timeout" ) ;
         }
      }

      if ( TRUE == _optionObj.hasField( "useShell" ) )
      {
         if( NumberInt != _optionObj.getField( "useShell" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "useShell must be int" ) ;
            goto error ;
         }
         else
         {
            useShell = _optionObj.getIntField( "useShell" ) ;
         }
      }

      utilStrTrim( command ) ;
      if( !ev.empty() )
      {
         command += " " + ev ;
      }

      // run cmd
      rc = runner.exec( command.c_str(), retCode, FALSE,
                        0 == timeout ? -1 : (INT64)timeout,
                        FALSE, NULL, useShell ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << command << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      else
      {
         rc = runner.read( strOut ) ;
         if ( rc )
         {
            stringstream ss ;
            ss << "read run command[" << command << "] result failed" ;
            PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
            goto error ;
         }
         else if ( SDB_OK != retCode )
         {
            PD_LOG( PDERROR, strOut.c_str() ) ;
         }
      }
      builder.append( "retCode", retCode ) ;
      builder.append( "strOut", strOut.c_str() ) ;
      retObj = builder.obj() ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteCmdStart implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteCmdStart )

    _remoteCmdStart::_remoteCmdStart()
   {
   }

   _remoteCmdStart::~_remoteCmdStart()
   {
   }

   const CHAR* _remoteCmdStart::name()
   {
      return OMA_REMOTE_CMD_START ;
   }

   INT32 _remoteCmdStart::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      string command ;
      string ev ;
      ossCmdRunner runner ;
      string strOut = "" ;
      UINT32 retCode = 0 ;
      UINT32 useShell = 1 ;
      UINT32 timeout  = 100 ;
      OSSHANDLE processHandle = (OSSHANDLE)0 ;
#if defined( _LINUX )
      struct sigaction    ignore ;
      struct sigaction    savechild ;
      sigset_t            childmask ;
      sigset_t            savemask ;
      BOOLEAN             restoreSigMask         = FALSE ;
      BOOLEAN             restoreSIGCHLDHandling = FALSE ;
#endif // _LINUX

      if ( FALSE == _valueObj.hasField( "command" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "command" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "cmd must be string" ) ;
         goto error ;
      }
      command = _valueObj.getStringField( "command" ) ;

      utilStrTrim( command ) ;

      if ( TRUE == _valueObj.hasField( "args" ) )
      {
         if ( String != _valueObj.getField( "args" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "environment should be a string" );
            goto error ;
         }
         ev = _valueObj.getStringField( "args" ) ;
         if ( !ev.empty() )
         {
            command += " " ;
            command += ev ;
            utilStrTrim( command ) ;
         }
      }

      if ( TRUE == _optionObj.hasField( "timeout" ) )
      {
         if ( NumberInt != _optionObj.getField( "timeout" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "timeout should be a number" ) ;
            goto error ;
         }
         timeout = _optionObj.getIntField( "timeout" ) ;
      }

      // useShell, default : 1
      if ( TRUE == _optionObj.hasField( "useShell" ) )
      {
         if ( NumberInt != _optionObj.getField( "useShell" ).type() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "useShell should be a number" ) ;
            goto error ;
         }
         useShell = _optionObj.getIntField( "useShell" ) ;
      }

      rc = SDB_OK ;

#if defined( _LINUX )
      // we should block SIGCHLD and rembmer caller's mask
      sigemptyset ( &childmask ) ;
      sigaddset ( &childmask, SIGCHLD ) ;
      // set new mask, save old mask
      rc = pthread_sigmask( SIG_BLOCK, &childmask, &savemask ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to block sigchld, err=%d", rc ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      // once we changed signal mask, we have to restore it later
      restoreSigMask = TRUE ;

      // change sigchld action to default
      ignore.sa_handler = SIG_DFL ;
      sigemptyset ( &ignore.sa_mask ) ;
      ignore.sa_flags = 0 ;
      rc = sigaction ( SIGCHLD, &ignore, &savechild ) ;
      if ( rc < 0 )
      {
         PD_LOG ( PDERROR, "Failed to run sigaction, err = %d", rc ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      // once we change signal handler, we have to restore it later
      restoreSIGCHLDHandling = TRUE ;
#endif //_LINUX


      rc = runner.exec( command.c_str(), retCode, TRUE, -1, FALSE,
                        timeout > 0 ? &processHandle : NULL,
                        useShell ? TRUE : FALSE,
                        timeout > 0 ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << command << "] failed" ;
         PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
         goto error ;
      }
      else
      {
         OSSPID pid = runner.getPID() ;
         if ( 0 != processHandle )
         {
            UINT32 times = 0 ;
            while ( times < timeout )
            {
               if ( ossIsProcessRunning( pid ) )
               {
                  ossSleep( SPT_USER_CMD_ONCE_SLEEP_TIME ) ;
                  times += SPT_USER_CMD_ONCE_SLEEP_TIME ;
               }
               else
               {
                  /// get exitcode and out string
                  rc = ossGetExitCodeProcess( processHandle, retCode ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "get exit code from process[ " << runner.getPID()
                        << " ] failed" ;
                     PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
                     goto error ;
                  }
                  rc = runner.read( strOut ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "read run command[" << command
                        << "] result failed" ;
                     PD_LOG_MSG( PDERROR, ss.str().c_str() ) ;
                     goto error ;
                  }

                  if ( SDB_OK != retCode )
                  {
                     builder.append( "retCode", retCode ) ;
                     builder.append( "strOut", strOut ) ;
                     retObj = builder.obj() ;
                     goto done ;
                  }
                  break ;
               }
            }
         }
         builder.append( "retCode", retCode ) ;
         builder.append( "strOut", strOut ) ;
         builder.append( "pid", (UINT32) pid ) ;
         retObj = builder.obj() ;
      }

   done:
#if defined( _LINUX )
      if ( restoreSIGCHLDHandling )
      {
         sigaction ( SIGCHLD, &savechild, NULL ) ;
      }
      if ( restoreSigMask )
      {
         pthread_sigmask ( SIG_SETMASK, &savemask, NULL ) ;
      }
#endif // _LINUX
      {
         UINT32 tmpCode = SDB_OK ;
         ossGetExitCodeProcess( processHandle, tmpCode ) ;
      }
      if ( 0 != processHandle )
      {
         ossCloseProcessHandle( processHandle ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
      _remoteCmdRunJS implement
   */
   IMPLEMENT_OACMD_AUTO_REGISTER( _remoteCmdRunJS )

    _remoteCmdRunJS::_remoteCmdRunJS()
   {
   }

   _remoteCmdRunJS::~_remoteCmdRunJS()
   {
   }

   const CHAR* _remoteCmdRunJS::name()
   {
      return OMA_REMOTE_CMD_RUN_JS ;
   }

   INT32 _remoteCmdRunJS::init ( const CHAR *pInfomation )
   {
      INT32 rc = SDB_OK ;
      string errmsg ;
      BSONObjBuilder bob ;
      BSONObj rval ;
      BSONObj detail ;

      rc = _remoteExec::init( pInfomation ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get argument, rc: %d", rc ) ;

      // get js code
      if ( FALSE == _valueObj.hasField( "code" ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "code must be config" ) ;
         goto error ;
      }
      if ( String != _valueObj.getField( "code" ).type() )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG_MSG( PDERROR, "code must be string" ) ;
         goto error ;
      }
      _code = _valueObj.getStringField( "code" ) ;

      // get scope
      _jsScope = sdbGetOMAgentMgr()->getScopeBySession() ;
      if ( !_jsScope )
      {
         rc = SDB_OOM ;
         PD_LOG_MSG ( PDERROR, "Failed to get scope, rc = %d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteCmdRunJS::doit( BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      string errmsg ;
      const sptResultVal *pRval = NULL ;
      BSONObjBuilder builder ;
      BSONObj rval ;

      // run js code
      rc = _jsScope->eval( _code.c_str(), _code.size(),
                           "", 1, SPT_EVAL_FLAG_PRINT, &pRval ) ;
      if ( rc )
      {
         errmsg = _jsScope->getLastErrMsg() ;
         rc = _jsScope->getLastError() ;
         PD_LOG_MSG ( PDERROR, "%s", errmsg.c_str() ) ;
         goto error ;
      }

      // set result
      rval = pRval->toBSON() ;
      rc = final ( rval, retObj ) ;
      if ( rc )
      {
         PD_LOG_MSG ( PDERROR, "Failed to extract result for command[%s], "
                      "rc = %d", name(), rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _remoteCmdRunJS::final( BSONObj &rval, BSONObj &retObj )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      string strOut ;

      PD_LOG ( PDDEBUG, "Js return raw result for command[%s]: %s",
               name(), rval.toString(FALSE, TRUE).c_str() ) ;

      if ( FALSE == rval.hasField( "" ) )
      {
         strOut = "" ;
      }
      else
      {
         BSONElement ele = rval.getField( "" ) ;

         if ( String == ele.type() )
         {
            strOut = rval.getStringField( "" );
         }
         else
         {
            strOut = ele.toString( FALSE, TRUE ) ;
         }
      }
      bob.append( "strOut", strOut ) ;
      retObj = bob.obj() ;

      return rc ;
   }

}
