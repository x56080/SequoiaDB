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

   Source File Name = sptUsrCmdCommon.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/24/2017  WJM  Initial Draft

   Last Changed =

*******************************************************************************/
#include "ossCmdRunner.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "pd.hpp"
#include "sptUsrCmdCommon.hpp"
#include <string>

namespace engine
{
   #define SPT_USER_CMD_ONCE_SLEEP_TIME            ( 2 )

   _sptUsrCmdCommon::_sptUsrCmdCommon()
   {
      _retCode    = 0 ;
   }

   _sptUsrCmdCommon::~_sptUsrCmdCommon()
   {
   }

   INT32 _sptUsrCmdCommon::exec( const string& command, const string& env,
                                const INT32& timeout, const INT32& useShell,
                                std::string &err, std::string &retStr )
   {
      INT32 rc = SDB_OK ;
      ossCmdRunner runner ;

      _command.clear() ;

      _command = command ;
      utilStrTrim( _command ) ;

      if( !env.empty() )
      {
         _command += " " ;
         _command += env ;
      }

      _strOut = "" ;
      _retCode = 0 ;
      rc = runner.exec( _command.c_str(), _retCode, FALSE,
                        0 == timeout ? -1 : (INT64)timeout,
                        FALSE, NULL, useShell ? TRUE : FALSE ) ;
      if ( SDB_OK != rc )
      {
         stringstream ss ;
         ss << "run[" << _command << "] failed" ;
         err = ss.str() ;
         goto error ;
      }
      else
      {
         rc = runner.read( _strOut ) ;
         if ( rc )
         {
            stringstream ss ;
            ss << "read run command[" << _command << "] result failed" ;
            err = ss.str() ;
            goto error ;
         }
         else if ( SDB_OK != _retCode )
         {
            err = _strOut ;
            rc = _retCode ;
            goto error ;
         }
         retStr = _strOut ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sptUsrCmdCommon::start( const string& command, const string& env,
                                 const INT32& useShell, const INT32& timeout,
                                 string &err, INT32 &pid, string &retStr )
   {
      INT32 rc = SDB_OK ;
      ossCmdRunner runner ;
      OSSHANDLE processHandle = (OSSHANDLE)0 ;

      _command.clear() ;
      _command = command ;
      utilStrTrim( _command ) ;

      if ( !env.empty() )
      {
         _command += " " ;
         _command += env ;
         utilStrTrim( _command ) ;
      }

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
         err = ss.str() ;
         goto error ;
      }
      else
      {
         OSSPID childPid = runner.getPID() ;
         if ( 0 != processHandle )
         {
            INT64 times = 0 ;
            while ( times < timeout )
            {
               if ( ossIsProcessRunning( childPid ) )
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
                     ss << "get exit code from process[ " << childPid
                        << " ] failed" ;
                     err = ss.str() ;
                     goto error ;
                  }
                  rc = runner.read( _strOut ) ;
                  if ( rc )
                  {
                     stringstream ss ;
                     ss << "read run command[" << _command
                        << "] result failed" ;
                     err = ss.str() ;
                     goto error ;
                  }

                  if ( SDB_OK != _retCode )
                  {
                     err = _strOut ;
                     rc = _retCode ;
                     goto error ;
                  }
                  break ;
               }
            }
         }
         retStr = _strOut ;
         pid = childPid ;
      }

   done:
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

   INT32 _sptUsrCmdCommon::getLastRet( string &err, UINT32 &lastRet )
   {
      lastRet = _retCode ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmdCommon::getLastOut( string &err, string &lastOut )
   {
      lastOut = _strOut ;
      return SDB_OK ;
   }

   INT32 _sptUsrCmdCommon::getCommand( string &err, string &command )
   {
      command = _command ;
      return SDB_OK ;
   }
}
