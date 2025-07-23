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

   Source File Name = stpstop.cpp

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
#include "ossProc.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "pmdDef.hpp"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "utilCommon.hpp"
#include "utilNodeOpr.hpp"
#include "stpToolCommon.hpp"
#include "utilStr.hpp"
#include "ossVer.h"
#include "ossIO.hpp"

namespace po = boost::program_options ;

using namespace std ;

namespace engine
{
   #define STPSTOP_LOG_FILE_NAME    "stpstop.log"
   #define STPSTOP_OPTION_FORCE     PMD_OPTION_FORCE

   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( STP_OPTION_HELP, ",h" ), "help" )\
       ( STP_OPTION_VERSION, "version" ) \
       ( STPSTOP_OPTION_FORCE, "force stop when the node can't stop normally" )

   #define COMMANDS_HIDE_OPTIONS \
      ( STP_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER, "use current user" )

   // initialize options
   void init( po::options_description &desc,
              po::options_description &all )
   {
      PMD_ADD_PARAM_OPTIONS_BEGIN ( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN ( all )
         COMMANDS_OPTIONS
         COMMANDS_HIDE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END
   }

   void displayArg( po::options_description &desc )
   {
      cout << desc << endl ;
   }

   INT32 resolveArgument( po::options_description &desc,
                          po::options_description &all,
                          po::variables_map &vm,
                          INT32 argc,
                          CHAR **argv,
                          BOOLEAN &force )
   {
      INT32 rc = SDB_OK ;

      rc = utilReadCommandLine2( argc, argv, all, vm, FALSE ) ;
      if ( SDB_OK != rc )
      {
         cout << "Read command line failed: " << rc << endl ;
         goto error ;
      }

      if ( vm.count( STP_OPTION_HELP ) )
      {
         displayArg( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      else if ( vm.count( STP_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      else if ( vm.count( STP_OPTION_VERSION ) )
      {
         ossPrintVersion( "STP Stop Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      if ( vm.count( STPSTOP_OPTION_FORCE ) )
      {
         force = TRUE ;
      }
      else
      {
         force = FALSE ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_STPSTOP_MAIN, "mainEntry" )
   INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;

      INT32 success = 0 ;
      INT32 total = 0 ;

      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ] = {0} ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      UTIL_VEC_NODES listNodes ;
      UTIL_VEC_NODES::iterator itrNode ;

      BOOLEAN force = FALSE ;
      po::options_description desc ( "Command options" ) ;
      po::options_description all ( "Command options" ) ;
      po::variables_map vm ;

      init( desc, all ) ;

      // validate arguments
      rc = resolveArgument( desc, all, vm, argc, argv, force ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            cout << "Invalid argument" << endl ;
            displayArg( desc ) ;
         }
         else
         {
            rc = SDB_OK ;
         }
         goto done ;
      }

      if ( !vm.count( PMD_OPTION_CURUSER ) )
      {
         UTIL_CHECK_AND_CHG_USER() ;
      }

      // make path
      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Get module self path failed:  %d"OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      // dialog path and file
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

      rc = utilCatPath( dialogFile, OSS_MAX_PATHSIZE, STPSTOP_LOG_FILE_NAME ) ;
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
      PD_LOG( PDEVENT, "Start program[%s]...", verText ) ;

      // list all nodes
      utilListNodes( listNodes, SDB_TYPE_STP, NULL, OSS_INVALID_PID, -1 ) ;

      itrNode = listNodes.begin() ;
      while ( itrNode != listNodes.end() )
      {
         utilNodeInfo &info = *itrNode ;

         rc = utilAsyncStopNode( info ) ;
         if ( SDB_OK != rc )
         {
            ossPrintf ( "Terminating process %d: %s(%s)"OSS_NEWLINE,
                        info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                        info._svcname.c_str() ) ;
            if ( SDB_CLS_NODE_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
               ossPrintf( "DONE"OSS_NEWLINE ) ;
            }
            else
            {
               ossPrintf( "FAILED"OSS_NEWLINE ) ;
            }

            itrNode = listNodes.erase( itrNode ) ;
         }
         else
         {
            ++itrNode ;
         }
      }

      /// The second time for wait

      itrNode = listNodes.begin() ;
      while ( itrNode != listNodes.end() )
      {
         utilNodeInfo &info = *itrNode ;

         ossPrintf ( "Terminating process %d: %s(%s)"OSS_NEWLINE,
                     info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                     info._svcname.c_str() ) ;

         rc = utilStopNode( info, UTIL_STOP_NODE_TIMEOUT, force, TRUE ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Successful to stop stp node %d: %s(%s)",
                    info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                    info._svcname.c_str() ) ;
            ossPrintf( "DONE"OSS_NEWLINE ) ;
            ++ success ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to stop stp node %d: %s(%s), rc: %d",
                    info._pid, utilDBTypeStr( (SDB_TYPE)info._type ),
                    info._svcname.c_str(), rc ) ;
            ossPrintf( "FAILED"OSS_NEWLINE ) ;
         }
         ++ itrNode ;
         ++ total ;
      }

      if ( total == success )
      {
         ossPrintf( "Successful to stop stp"OSS_NEWLINE ) ;
         rc = SDB_OK ;
      }
      else
      {
         ossPrintf( "Failed to stop stp, rc: %d"OSS_NEWLINE, rc ) ;
         rc = success == 0 ? STOPFAIL : STOPPART ;
      }

   done:
      PD_LOG( PDEVENT, "Stop program." ) ;
      return ( rc >= 0 ) ? rc : utilRC2ShellRC( rc ) ;

   error:
      goto done ;
   }

}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}
