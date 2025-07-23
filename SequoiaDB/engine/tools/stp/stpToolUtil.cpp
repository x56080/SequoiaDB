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

   Source File Name = stpToolUtil.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpToolUtil.hpp"
#include "ossCmdRunner.hpp"
#include "ossIO.hpp"
#include "ossMem.hpp"
#include "ossPath.hpp"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "pmdDef.hpp"
#include "pmdOptions.hpp"
#include "stpToolCommon.hpp"
#include "utilCommon.hpp"
#include "utilNodeOpr.hpp"
#include "utilParam.hpp"

#include <boost/algorithm/string.hpp>

using namespace std ;
using namespace boost::algorithm ;

namespace engine
{

   // get SequoiaDB node with given service name
   static BOOLEAN _stpGetServiceNode( const CHAR *serviceName,
                                      utilNodeInfo &info )
   {
      UTIL_VEC_NODES nodes ;
      INT32 rc = utilListNodes( nodes, -1, serviceName ) ;
      if ( SDB_OK == rc && nodes.size() > 0 )
      {
         info = *nodes.begin() ;
         return TRUE ;
      }
      return FALSE ;
   }

   static void _stpBuildStartCommand( const CHAR *stpPathName,
                                      const string &configPath,
                                      const string &options,
                                      string &command )
   {
      SDB_ASSERT( NULL != stpPathName, "STP path is invalid" ) ;

      BOOLEAN addedConf = FALSE ;

      command = stpPathName ;

      // config path is specified, append to command line
      if ( !configPath.empty() )
      {
         command += " " ;
         command += SDBCM_OPTION_PREFIX STP_OPTION_CONFPATH ;
         command += " " ;
         command += configPath ;
         addedConf = TRUE ;
      }

      // additional options is specified, append to command line
      if ( !options.empty() )
      {
         command += " " ;
         command += options ;
      }

      // use --force to occupy spaces for rename process
      if ( !addedConf )
      {
         command += " " ;
         command += SDBCM_OPTION_PREFIX PMD_OPTION_FORCE ;
      }
   }

   INT32 stpStartNode( const CHAR *rootPath,
                       string configPath,
                       const string &options )
   {
      INT32 rc = SDB_OK ;
      INT32 tmpRC = SDB_OK ;

      CHAR stpPathName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      string serviceName ;
      utilNodeInfo info ;

      string command ;
      OSSHANDLE handle ;
      ossCmdRunner runner ;

      UINT32 exitCode = 0 ;

      // get STP executable path
      rc = utilBuildFullPath( rootPath, STP_NAME, OSS_MAX_PATHSIZE,
                              stpPathName ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Build engine path name failed: %d"OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      // check config path
      if ( configPath.empty() )
      {
         CHAR tmpPath[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
         rc = utilBuildFullPath( rootPath, STP_ROOT_PATH, OSS_MAX_PATHSIZE,
                                 tmpPath ) ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Failed to build config path: %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }
         configPath.assign( tmpPath ) ;
      }

      // get service name
      rc = utilGetServiceByConfigPath( configPath, STP_CFG_FILE_NAME,
                                       STP_OPTION_PORT, serviceName,
                                       STP_DEF_SERVICE_NAME ) ;
      if ( SDB_OK == rc && !serviceName.empty() &&
           _stpGetServiceNode( serviceName.c_str(), info ) )
      {
         ossPrintf( "Success: %s(%s) is already started (%d)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), serviceName.c_str(),
                    info._pid ) ;
         goto done ;
      }

      // build command
      _stpBuildStartCommand( stpPathName, configPath, options, command ) ;
      PD_LOG( PDDEBUG, "Start %s command: %s", utilDBTypeStr( SDB_TYPE_STP ),
              command.c_str() ) ;

      // run command
      tmpRC = runner.exec( command.c_str(), exitCode, TRUE, -1, TRUE,
                           &handle ) ;
      if ( SDB_OK != tmpRC )
      {
         rc = tmpRC ;
         ossPrintf( "Error: Start %s(%s) failed, rc: %d(%s)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), serviceName.c_str(), tmpRC,
                    getErrDesp( rc ) ) ;
         goto error ;
      }

      // get PID
      info._pid = runner.getPID() ;
      info._svcname = serviceName ;

      tmpRC = utilWaitNodeOK( info, info._svcname.c_str(), info._pid ) ;

      /// notify node to end pipe
      utilEndNodePipeDup( info._svcname.c_str(), info._pid ) ;
      runner.done() ;

      if ( SDB_OK == tmpRC )
      {
         ossPrintf( "Success: %s(%s) is successfully started (%d)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), serviceName.c_str(),
                    info._pid ) ;
      }
      else
      {
         rc = tmpRC ;

         /// read out
         if ( (OSSHANDLE)0 != handle )
         {
            string outString ;
            runner.read( outString ) ;
            utilStrTrim( outString ) ;
#if defined( _WINDOWS )
            // need to remove all '\r'
            erase_all( outString, "\r" ) ;
#endif // _WINDOWS
            if ( !outString.empty() )
            {
               ossPrintf( "%s: %u bytes out==>%s%s%s<=="OSS_NEWLINE,
                          info._svcname.c_str(),
                          (UINT32)( outString.length() +
                                    ossStrlen( OSS_NEWLINE ) * 2 ),
                          OSS_NEWLINE,
                          outString.c_str(),
                          OSS_NEWLINE ) ;
            }
         }

         if ( !ossIsProcessRunning( info._pid ) &&
              (OSSHANDLE)0 != handle &&
              SDB_OK == ossGetExitCodeProcess( handle, exitCode ) )
         {
            rc = exitCode ;
         }
         ossPrintf( "Error: Start %s(%s) failed, rc: %d(%s)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), serviceName.c_str(), rc,
                    getErrDesp( utilShellRC2RC( rc ) ) ) ;
      }

      // close handle
      ossCloseProcessHandle( handle ) ;

   done:
      return rc ;

   error:
      goto done ;
   }
}

