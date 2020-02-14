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

   Source File Name = stpstart.cpp

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

#include "core.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "ossPath.hpp"
#include "ossProc.hpp"
#include "pmdDef.hpp"
#include "utilCommon.hpp"
#include "utilNodeOpr.hpp"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "ossVer.h"
#include "stpToolCommon.hpp"
#include "ossIO.hpp"
#include "ossCmdRunner.hpp"

#include <string>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options ;

using namespace std ;
using namespace boost::algorithm ;

namespace engine
{

   #define STPSTART_LOG_FILE_NAME   "stpstart.log"
   #define STPSTART_OPTION_OPTIONS  "options"

#if defined (_WINDOWS)
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_OPTION_FORCE, "force" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_CONFPATH, ",c"), po::value<string>(), "configuration file path" ) \
       ( STPSTART_OPTION_OPTIONS, po::value<string>(), "options" )
#else
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h"), "help" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_OPTION_FORCE, "force" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_CONFPATH, ",c"), po::value<string>(), "configuration file path" ) \
       ( STPSTART_OPTION_OPTIONS, po::value<string>(), "options" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_IGNOREULIMIT, ",i"), "skip checking ulimit" )
#endif

   #define COMMANDS_HIDE_OPTIONS \
      ( PMD_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER, "use current user" )

   static void init( po::options_description &desc,
                     po::options_description &all )
   {
      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN( all )
         COMMANDS_OPTIONS
         COMMANDS_HIDE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END
   }

   static void displayArg( po::options_description &desc )
   {
      cout << desc << endl ;
   }

   static BOOLEAN serviceExists( const CHAR *serviceName, utilNodeInfo &info )
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

   static INT32 resolveArgument( po::options_description &desc,
                                 po::options_description &all,
                                 po::variables_map &vm,
                                 INT32 argc,
                                 CHAR **argv,
                                 BOOLEAN &force,
                                 string &configPath,
                                 string &options )
   {
      INT32 rc = SDB_OK ;

      rc = utilReadCommandLine( argc, argv, all, vm, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( vm.count( PMD_OPTION_HELP ) )
      {
         displayArg( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( PMD_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      else if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "SDBTP Start Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      if ( vm.count( PMD_OPTION_FORCE ) )
      {
         force = TRUE ;
      }
      else
      {
         force = FALSE ;
      }

      if ( vm.count( PMD_OPTION_CONFPATH ) )
      {
         configPath = vm[PMD_OPTION_CONFPATH].as<string>() ;
      }

      if ( vm.count( STPSTART_OPTION_OPTIONS ) )
      {
         options = vm[ STPSTART_OPTION_OPTIONS ].as<string>() ;

         // can't include '-c/--confpath'
         if ( ossStrstr( options.c_str(), "-c" ) ||
              ossStrstr( options.c_str(),
                         SDBCM_OPTION_PREFIX PMD_OPTION_CONFPATH ) )
         {
            cout << "options invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if ( configPath.empty() )
      {
         force = TRUE ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   static void buildListArgs( const CHAR *sdbtpPathName,
                              BOOLEAN force,
                              const string &configPath,
                              const string &options,
                              string &cmd )
   {
      BOOLEAN addedConf = FALSE ;

      cmd = sdbtpPathName ;

      if ( !configPath.empty() )
      {
         cmd += " " ;
         cmd += SDBCM_OPTION_PREFIX PMD_OPTION_CONFPATH ;
         cmd += " " ;
         cmd += configPath ;
         addedConf = TRUE ;
      }

      if ( !options.empty() )
      {
         cmd += " " ;
         cmd += options ;
      }

      if ( force || !addedConf )
      {
         cmd += " " ;
         cmd += SDBCM_OPTION_PREFIX PMD_OPTION_FORCE ;
      }
   }

   static INT32 mainEntry( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;
      INT32 tmpRC = SDB_OK ;

      po::options_description desc( "Command options" ) ;
      po::options_description all( "Command options" ) ;
      po::variables_map vm ;

      BOOLEAN force = FALSE ;
      string configPath ;
      string options ;

      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR stpPathName[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      string svcname ;
      utilNodeInfo info ;

      string runCmd ;
      OSSHANDLE handle ;
      ossCmdRunner runner ;

      UINT32 exitCode = 0 ;

      init( desc, all ) ;

      /// 1.validate arguments
      rc = resolveArgument( desc, all, vm, argc, argv, force, configPath,
                            options ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            ossPrintf( "Error: Invalid argument: %d"OSS_NEWLINE, rc ) ;
            displayArg ( desc ) ;
         }
         else
         {
            rc = SDB_OK ;
         }
         goto done ;
      }

#if defined ( _LINUX )
      /// check ulimit
      if ( !vm.count( PMD_OPTION_IGNOREULIMIT ) )
      {
         rc = utilSetAndCheckUlimit() ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Error: start sdbtp will set ulimit by file"
                       "[conf/limits.conf], if you want to set ulimit by "
                       "current terminal, please use parameter '-i'."
                       OSS_NEWLINE ) ;
            goto error ;
         }
      }
#endif

      /// change user
      if ( !vm.count( PMD_OPTION_CURUSER ) )
      {
         UTIL_CHECK_AND_CHG_USER() ;
      }

      /// make path
      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Get module self path failed:  %d"OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      /// binary path
      rc = utilBuildFullPath( rootPath, STP_NAME, OSS_MAX_PATHSIZE,
                              stpPathName ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Build engine path name failed: %d"OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      /// config path
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

      /// dialog path and file
      rc = utilBuildFullPath( rootPath, STP_LOG_PATH, OSS_MAX_PATHSIZE,
                              dialogFile ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to build dialog path: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

      // make sure the dir exist
      rc = ossMkdir( dialogFile ) ;
      if ( SDB_OK != rc && SDB_FE != rc )
      {
         ossPrintf( "Create dialog directory [%s] failed, rc: %d"OSS_NEWLINE,
                    dialogFile, rc ) ;
         // not go to error, continue
         rc = SDB_OK ;
      }

      rc = utilCatPath( dialogFile, OSS_MAX_PATHSIZE, STPSTART_LOG_FILE_NAME ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to build dialog file: %d"OSS_NEWLINE, rc ) ;
         // not go to error, continue
         rc = SDB_OK ;
      }

      // enable pd log
      sdbEnablePD( dialogFile ) ;
      setPDLevel( PDINFO ) ;

      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start program [%s]...", verText ) ;

      // first check
      rc = utilGetServiceByConfigPath( configPath, STP_CFG_FILE_NAME,
                                       PMD_OPTION_PORT, svcname,
                                       STP_DEF_SERVICE_NAME ) ;
      if ( SDB_OK == rc && !svcname.empty() &&
           serviceExists( svcname.c_str(), info ) )
      {
         ossPrintf( "Success: %s(%s) is already started (%d)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), svcname.c_str(),
                    info._pid ) ;
         goto done ;
      }

      // start node
      buildListArgs( stpPathName, force, configPath, options, runCmd ) ;

      tmpRC = runner.exec( runCmd.c_str(), exitCode, TRUE, -1, TRUE, &handle ) ;
      if ( SDB_OK != tmpRC )
      {
         rc = tmpRC ;
         ossPrintf( "Error: Start %s(%s) failed, rc: %d(%s)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), svcname.c_str(), tmpRC,
                    getErrDesp( rc ) ) ;
         goto error ;
      }

      info._pid = runner.getPID() ;
      info._svcname = svcname ;

      tmpRC = utilWaitNodeOK( info, info._svcname.c_str(), info._pid ) ;

      /// notify node to end pipe
      utilEndNodePipeDup( info._svcname.c_str(), info._pid ) ;
      runner.done() ;

      if ( SDB_OK == tmpRC )
      {
         ossPrintf( "Success: %s(%s) is successfully started (%d)"OSS_NEWLINE,
                    utilDBTypeStr( SDB_TYPE_STP ), svcname.c_str(),
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
                          (UINT32)(outString.length() + ossStrlen( OSS_NEWLINE ) * 2 ),
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
                    utilDBTypeStr( SDB_TYPE_STP ), svcname.c_str(), rc,
                    getErrDesp( utilShellRC2RC( rc ) ) ) ;
      }

      // close handle
      ossCloseProcessHandle( handle ) ;

   done:
      PD_LOG( PDEVENT, "Stop program." ) ;

      return SDB_OK == rc ? 0 : utilRC2ShellRC( rc ) ;

   error:
      goto done ;
   }

}

INT32 main( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}
