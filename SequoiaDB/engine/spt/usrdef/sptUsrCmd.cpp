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

   Source File Name = sptUsrCmd.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          31/03/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "sptUsrCmd.hpp"
#include "ossCmdRunner.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"

using namespace bson ;

namespace engine
{
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, exec )
   JS_CONSTRUCT_FUNC_DEFINE( _sptUsrCmd, construct )
   JS_DESTRUCT_FUNC_DEFINE( _sptUsrCmd, destruct )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, toString )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, getLastRet )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, getLastOut )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, start )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, getCommand )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, getInfo )
   JS_MEMBER_FUNC_DEFINE( _sptUsrCmd, memberHelp )
   JS_STATIC_FUNC_DEFINE( _sptUsrCmd, staticHelp )

   JS_BEGIN_MAPPING( _sptUsrCmd, "Cmd" )
      JS_ADD_CONSTRUCT_FUNC( construct )
      JS_ADD_DESTRUCT_FUNC( destruct )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_getLastRet", getLastRet, 0 )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_getLastOut", getLastOut, 0 )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_run", exec, 0 )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_start", start, 0 )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_getCommand", getCommand, 0 )
      JS_ADD_MEMBER_FUNC_WITHATTR( "_getInfo", getInfo, 0 )
      JS_ADD_MEMBER_FUNC( "toString", toString )
      JS_ADD_MEMBER_FUNC( "help", memberHelp )
      JS_ADD_STATIC_FUNC( "help", staticHelp )
   JS_MAPPING_END()

   #define SPT_USER_CMD_ONCE_SLEEP_TIME            ( 2 )

   _sptUsrCmd::_sptUsrCmd()
   {
      _retCode    = 0 ;
   }

   _sptUsrCmd::~_sptUsrCmd()
   {
   }

   INT32 _sptUsrCmd::construct( const _sptArguments &arg,
                                _sptReturnVal &rval,
                                bson::BSONObj &detail )
   {
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::destruct()
   {
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::toString( const _sptArguments & arg,
                               _sptReturnVal & rval,
                               BSONObj & detail )
   {
      rval.getReturnVal().setValue( "CommandRunner" ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::getInfo( const _sptArguments & arg,
                              _sptReturnVal & rval,
                              BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj remoteInfo ;
      BSONObjBuilder builder ;
      if ( 0 < arg.argc() )
      {
         rc = arg.getBsonobj( 0, remoteInfo ) ;
         if ( SDB_OK != rc )
         {
            detail = BSON( SPT_ERR << "remoteInfo must be obj" ) ;
            goto error ;
         }
      }
      builder.append( "type", "Cmd" ) ;
      builder.appendElements( remoteInfo ) ;

      rval.getReturnVal().setValue( builder.obj() ) ;
   done:
      return rc ;
   error:
      goto done ;

   }

   INT32 _sptUsrCmd::getLastRet( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      rval.getReturnVal().setValue( _retCode ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::getLastOut( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      rval.getReturnVal().setValue( _strOut ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::getCommand( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      rval.getReturnVal().setValue( _command ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::exec( const _sptArguments &arg,
                           _sptReturnVal &rval,
                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string ev ;
      UINT32 timeout = 0 ;
      UINT32 useShell = TRUE ;
      ossCmdRunner runner ;

      _command.clear() ;

      rc = arg.getString( 0, _command ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "cmd must be config" ) ;
         goto error ;
      }
      utilStrTrim( _command ) ;

      rc = arg.getString( 1, ev ) ;
      if ( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "environment should be a string" ) ;
         goto error ;
      }
      else if ( SDB_OK == rc && !ev.empty() )
      {
         _command += " " ;
         _command += ev ;
      }

      rc = arg.getNative( 2, (void*)&timeout, SPT_NATIVE_INT32 ) ;
      if ( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "timeout should be a number" ) ;
         goto error ;
      }
      rc = SDB_OK ;

      // useShell, default : 1
      rc = arg.getNative( 3, (void*)&useShell, SPT_NATIVE_INT32 ) ;
      if ( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "useShell should be a number" ) ;
         goto error ;
      }
      rc = SDB_OK ;

      _strOut = "" ;
      _retCode = 0 ;
      rc = runner.exec( _command.c_str(), _retCode, FALSE,
                        0 == timeout ? -1 : (INT64)timeout,
                        FALSE, NULL, useShell ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << _command << "] failed" ;
         detail = BSON( SPT_ERR << ss.str() ) ;
         goto error ;
      }
      else
      {
         rc = runner.read( _strOut ) ;
         if ( rc )
         {
            stringstream ss ;
            ss << "read run command[" << _command << "] result failed" ;
            detail = BSON( SPT_ERR << ss.str() ) ;
            goto error ;
         }
         else if ( SDB_OK != _retCode )
         {
            if ( _strOut.empty() )
            {
               stringstream ss ;
               ss << "Run command(\"" << _command << "\") return code is "
                  << _retCode ;
               detail = BSON( SPT_ERR << ss.str() ) ;
            }
            else
            {
               detail = BSON( SPT_ERR << _strOut ) ;
            }
            rc = _retCode ;
            goto error ;
         }

         rval.getReturnVal().setValue( _strOut ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrCmd::start( const _sptArguments & arg,
                            _sptReturnVal & rval,
                            BSONObj & detail )
   {
      INT32 rc = SDB_OK ;
      string ev ;
      ossCmdRunner runner ;
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

      _command.clear() ;

      rc = arg.getString( 0, _command ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "cmd must be config" ) ;
         goto error ;
      }
      utilStrTrim( _command ) ;

      rc = arg.getString( 1, ev ) ;
      if ( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "environment should be a string" ) ;
         goto error ;
      }
      else if ( SDB_OK == rc && !ev.empty() )
      {
         _command += " " ;
         _command += ev ;
         utilStrTrim( _command ) ;
      }

      // useShell, default : 1
      rc = arg.getNative( 2, (void*)&useShell, SPT_NATIVE_INT32 ) ;
      if ( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "useShell should be a number" ) ;
         goto error ;
      }
      rc = SDB_OK ;

      // timeout, default : 100
      rc = arg.getNative( 3, (void*)&timeout, SPT_NATIVE_INT32 ) ;
      if ( SDB_OK != rc && SDB_OUT_OF_BOUND != rc )
      {
         rc = SDB_INVALIDARG ;
         detail = BSON( SPT_ERR << "timeout should be a number" ) ;
         goto error ;
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

      _strOut = "" ;
      _retCode = 0 ;
      rc = runner.exec( _command.c_str(), _retCode, TRUE, -1, FALSE,
                        timeout > 0 ? &processHandle : NULL,
                        useShell ? TRUE : FALSE,
                        timeout > 0 ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << _command << "] failed" ;
         detail = BSON( SPT_ERR << ss.str() ) ;
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
                  rc = ossGetExitCodeProcess( processHandle, _retCode ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "get exit code from process[ " << runner.getPID()
                        << " ] failed" ;
                     detail = BSON( SPT_ERR << ss.str() ) ;
                     goto error ;
                  }
                  rc = runner.read( _strOut ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "read run command[" << _command
                        << "] result failed" ;
                     detail = BSON( SPT_ERR << ss.str() ) ;
                     goto error ;
                  }

                  if ( SDB_OK != _retCode )
                  {
                     detail = BSON( SPT_ERR << _strOut ) ;
                     rc = _retCode ;
                     goto error ;
                  }
                  break ;
               }
            }
         }
         rval.getReturnVal().setValue( (INT32)pid ) ;
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

   INT32 _sptUsrCmd::memberHelp( const _sptArguments & arg,
                                 _sptReturnVal & rval,
                                 BSONObj & detail )
   {
      stringstream ss ;
      ss << "Cmd member functions:" << endl
         << "   run( cmd, [args], [timeout], [useShell] )  " << endl
         << "        timeout(ms), default 0: never timeout," << endl
         << "        useShell 0/1, default 1" << endl
         << "   start( cmd, [args], [useShell], [timeout] )  " << endl
         << "          useShell 0/1, default 1" << endl
         << "          timeout(ms), default 100" << endl
         << "   getCommand()" << endl
         << "   getLastRet()" << endl
         << "   getLastOut()" << endl
         << "Remote Cmd functions:" << endl
         << "   runJS( code )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::staticHelp( const _sptArguments & arg,
                           _sptReturnVal & rval,
                           BSONObj & detail )
   {
      stringstream ss ;
      ss << "Methods to access:" << endl
         << " var cmd = new Cmd()" << endl
         << " var cmd = remoteObj.getCmd()" << endl
         << "Cmd member functions:" << endl
         << "   run( cmd, [args], [timeout], [useShell] )  " << endl
         << "        timeout(ms), default 0: never timeout," << endl
         << "        useShell 0/1, default 1" << endl
         << "   start( cmd, [args], [useShell], [timeout] )  " << endl
         << "          useShell 0/1, default 1" << endl
         << "          timeout(ms), default 100" << endl
         << "   getCommand()" << endl
         << "   getLastRet()" << endl
         << "   getLastOut()" << endl
         << "Remote Cmd member functions:" << endl
         << "   runJS( code )" << endl ;
      rval.getReturnVal().setValue( ss.str() ) ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmd::_setRVal( _ossCmdRunner *runner,
                               _sptReturnVal &rval,
                               BOOLEAN setToRVal,
                               BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      string outStr ;

      rc = runner->read( outStr ) ;
      if ( rc )
      {
         detail = BSON( SPT_ERR << "read run result failed" ) ;
         goto error ;
      }

      if ( setToRVal )
      {
         rval.getReturnVal().setValue( outStr ) ;
      }
      else
      {
         detail = BSON( SPT_ERR << outStr ) ;
      }

   done:
      return rc  ;
   error:
      goto done ;
   }
}

