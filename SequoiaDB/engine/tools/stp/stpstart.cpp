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
#include "stpToolUtil.hpp"

#include <string>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options ;

using namespace std ;
using namespace boost::algorithm ;

namespace engine
{

   #define STPSTART_LOG_FILE_NAME         "stpstart.log"
   #define STPSTART_OPTION_OPTIONS        "options"
   #define STPSTART_OPTION_IGNOREULIMIT   PMD_OPTION_IGNOREULIMIT

#if defined (_WINDOWS)
   // windows options
   #define COMMANDS_OPTIONS \
      ( PMD_COMMANDS_STRING( STP_OPTION_HELP, ",h" ), \
            "help" ) \
      ( STP_OPTION_VERSION, \
            "version" ) \
      ( PMD_COMMANDS_STRING( STP_OPTION_CONFPATH, ",c" ), \
            po::value<string>(), \
            "configuration file path of STP\n" \
            "e.g. \"E:\\Sequoiadb\\conf\\stp\\\"" ) \
      ( STPSTART_OPTION_OPTIONS, po::value<string>(), \
            "options" )

   #define COMMANDS_HIDE_OPTIONS \
      ( STP_OPTION_HELPFULL, "help all configs" ) \
      ( STP_OPTION_CURUSER, "use current user" )

#else
   // linux options
   #define COMMANDS_OPTIONS \
      ( PMD_COMMANDS_STRING( STP_OPTION_HELP, ",h" ), \
            "help" ) \
      ( STP_OPTION_VERSION, \
            "version" ) \
      ( PMD_COMMANDS_STRING( STP_OPTION_CONFPATH, ",c" ), \
            po::value<string>(), \
            "configuration file path of STP\n" \
            "e.g. \"/opt/sequoiadb/conf/stp\"") \
      ( STPSTART_OPTION_OPTIONS, \
            po::value<string>(), \
            "options" )

   #define COMMANDS_HIDE_OPTIONS \
      ( STP_OPTION_HELPFULL, "help all configs" ) \
      ( STP_OPTION_CURUSER, "use current user" ) \
      ( PMD_COMMANDS_STRING( STPSTART_OPTION_IGNOREULIMIT, ",i" ), \
            "skip checking ulimit" )

#endif

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

   static INT32 resolveArgument( po::options_description &desc,
                                 po::options_description &all,
                                 po::variables_map &vm,
                                 INT32 argc,
                                 CHAR **argv,
                                 string &configPath,
                                 string &options )
   {
      INT32 rc = SDB_OK ;

      rc = utilReadCommandLine( argc, argv, all, vm, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( vm.count( STP_OPTION_HELP ) )
      {
         displayArg( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      if ( vm.count( STP_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      else if ( vm.count( STP_OPTION_VERSION ) )
      {
         ossPrintVersion( "STP Start Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      if ( vm.count( STP_OPTION_CONFPATH ) )
      {
         configPath = vm[ STP_OPTION_CONFPATH ].as<string>() ;
      }

      if ( vm.count( STPSTART_OPTION_OPTIONS ) )
      {
         options = vm[ STPSTART_OPTION_OPTIONS ].as<string>() ;

         // can't include '-c/--confpath'
         if ( ossStrstr( options.c_str(), "-c" ) ||
              ossStrstr( options.c_str(),
                         SDBCM_OPTION_PREFIX STP_OPTION_CONFPATH ) )
         {
            cout << "options invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   static INT32 mainEntry( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;

      po::options_description desc( "Command options" ) ;
      po::options_description all( "Command options" ) ;
      po::variables_map vm ;

      string configPath ;
      string options ;

      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      init( desc, all ) ;

      /// 1.validate arguments
      rc = resolveArgument( desc, all, vm, argc, argv, configPath, options ) ;
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
      if ( !vm.count( STPSTART_OPTION_IGNOREULIMIT ) )
      {
         rc = utilSetAndCheckUlimit() ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Error: start stp will set ulimit by file"
                       "[conf/limits.conf], if you want to set ulimit by "
                       "current terminal, please use parameter '-i'."
                       OSS_NEWLINE ) ;
            goto error ;
         }
      }
#endif

      /// change user
      if ( !vm.count( STP_OPTION_CURUSER ) )
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

      rc = stpStartNode( rootPath, configPath, options ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to start STP node, rc: %d", rc ) ;

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
