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

   Source File Name = seAdapterCtl.cpp

   Descriptive Name = Search Engine Adapter control

   When/how to use: this program may be used on binary and text-formatted
   versions of seadapter component. This file contains main function for sdbseactl,
   which is used to start/stop/list Search Engine Adapter .

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          25/04/2023  JRK Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "ossPath.hpp"
#include "ossProc.hpp"
#include "pmdDef.hpp"
#include "utilCommon.hpp"
#include "utilNodeOpr.hpp"
#include "pmdOptions.h"
#include "utilParam.hpp"
#include "ossVer.h"
#include "ossIO.hpp"
#include "ossCmdRunner.hpp"
#include "omagentDef.hpp"
#include "seAdptDef.hpp"

#include <string>
#include <boost/algorithm/string.hpp>

namespace po = boost::program_options ;

using namespace std ;
using namespace boost::algorithm ;
using namespace seadapter ;

namespace engine
{
   #define SEADPTCTL_OPTION_ALL            "all"

   // sdbseactl log path and file
   #define SEADPTCTL_LOG_FILE_NAME         "sdbseactl.log"
   #define SEADPTCTL_LOG_PATH              SDBCM_LOG_PATH
   #define SEADPTCTL_CFG_PATH              SDB_CM_ROOT_PATH SEADPT_EXE_FILE_NAME

   #define SEADPTCTL_MODE_NAME_START       "start"
   #define SEADPTCTL_MODE_NAME_STOP        "stop"
   #define SEADPTCTL_MODE_NAME_LIST        "list"

   /*
      Long format define
   */
   #define SEADPTCTL_LIST_LONG_FORMAT "%-13.12s %-12.11s %-11.10s %-9.8s %-13.12s %-11.10s %-20.19s"
   #define SEADPTCTL_LIST_TITLE "Name          SvcName      Role        PID       DataSvcName   Mode        StartTime"

   enum MODE_TYPE
   {
      MODE_TYPE_START  = 1,
      MODE_TYPE_STOP,
      MODE_TYPE_LIST,

      MODE_TYPE_MAX
   } ;

#if defined( _WINDOWS )
   // windows options
   #define COMMANDS_OPTIONS \
      ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_VERSION, ",v" ), "version" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_MODE, ",m"), po::value<string>(), \
        "mode of the ctl function (arg: [start|stop|list])" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_SVCNAME, ",p"), po::value<string>(), \
        "service name, separated by comma (',')" ) \
      ( PMD_COMMANDS_STRING( SEADPTCTL_OPTION_ALL, ",a"), \
        "start or stop all adapter nodes" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_LONG, ",l" ), \
        "show long style, when list node information" ) \
      ( PMD_OPTION_FORCE, "force stop when the node can't stop normally")

#else
   // linux options
   #define COMMANDS_OPTIONS \
      ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_VERSION, ",v" ), "version" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_MODE, ",m"), po::value<string>(), \
        "mode of the ctl function (arg: [start|stop|list])" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_SVCNAME, ",p"), po::value<string>(), \
        "service name, separated by comma (',')" ) \
      ( PMD_COMMANDS_STRING( SEADPTCTL_OPTION_ALL, ",a"), \
        "start or stop all adapter nodes" ) \
      ( PMD_COMMANDS_STRING( PMD_OPTION_LONG, ",l" ), \
        "show long style, when list node information" ) \
      ( PMD_OPTION_FORCE, "force stop when the node can't stop normally") \
      ( PMD_COMMANDS_STRING( PMD_OPTION_IGNOREULIMIT, ",i" ), \
        "skip checking ulimit, when start node" )

#endif

   #define COMMANDS_HIDE_OPTIONS \
      ( PMD_OPTION_HELPFULL, "help all configs" ) \
      ( PMD_OPTION_CURUSER,  "use current user to start or stop node" )

   void init( po::options_description &desc,
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

   void displayUsage()
   {
      ossPrintf( "Usage: sdbseactl [-m start|stop|list] [option]" OSS_NEWLINE ) ;
      ossPrintf( "Examples: " OSS_NEWLINE ) ;
      ossPrintf( "  sdbseactl -m start -a          "
                 "# start all adapter nodes." OSS_NEWLINE ) ;
      ossPrintf( "  sdbseactl -m stop -p <svcname> "
                 "# stop the node with the specified service name." OSS_NEWLINE ) ;
      ossPrintf( "  sdbseactl -m list -l           "
                 "# list all adapter nodes information use long style." OSS_NEWLINE ) ;
   }

   void displayArg( po::options_description &desc )
   {
      displayUsage() ;
      cout << desc << endl ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSEACTL_RESVARG, "resolveArgument" )
   INT32 resolveArgument( po::options_description &desc,
                          po::options_description &all,
                          po::variables_map &vm,
                          INT32 argc, CHAR **argv,
                          INT32 &mode,
                          vector< string > &serviceNameList,
                          BOOLEAN &isAll,
                          BOOLEAN &showLong,
                          BOOLEAN &isForce )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSEACTL_RESVARG ) ;

      rc = utilReadCommandLine( argc, argv, all, vm, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

      // not allow using without specified any parameter
      if ( 0 == vm.size() )
      {
         rc = SDB_INVALIDARG ;
         ossPrintf( "Sdbseactl does not allow using without"
                    " specified any parameter" OSS_NEWLINE ) ;
         goto error ;
      }

      // --help / -h
      if ( vm.count( PMD_OPTION_HELP ) )
      {
         displayArg( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }

      // --helpfull
      if ( vm.count( PMD_OPTION_HELPFULL ) )
      {
         displayArg( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }

      // --version / -v
      if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "SDB SeAdapterCtl Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto done ;
      }

      // --mode / -m
      if ( vm.count( PMD_OPTION_MODE ) )
      {
         string modeStr = vm[ PMD_OPTION_MODE ].as<string>() ;

         if ( 0 == ossStrcasecmp( modeStr.c_str(),
                                  SEADPTCTL_MODE_NAME_START ) )
         {
            mode = MODE_TYPE_START ;
         }
         else if ( 0 == ossStrcasecmp( modeStr.c_str(),
                                       SEADPTCTL_MODE_NAME_STOP ) )
         {
            mode = MODE_TYPE_STOP ;
         }
         else if ( 0 == ossStrcasecmp( modeStr.c_str(),
                                       SEADPTCTL_MODE_NAME_LIST ) )
         {
            mode = MODE_TYPE_LIST ;
         }
         else
         {
            mode = -1 ;
            rc = SDB_INVALIDARG ;
            ossPrintf( "Invalid value for parameter mode: %s" OSS_NEWLINE, modeStr.c_str() ) ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         ossPrintf( "Must specify --mode / -m parameter" OSS_NEWLINE ) ;
         goto error ;
      }

      // -p / --svcname
      if ( vm.count ( PMD_OPTION_SVCNAME ) )
      {
         string svcname = vm[ PMD_OPTION_SVCNAME ].as<string>() ;
         if( svcname.empty() )
         {
            ossPrintf( "Service name can't be empty" OSS_NEWLINE ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         // break service names using ','
         rc = utilSplitStr( svcname, serviceNameList, ", \t" ) ;
         if ( rc )
         {
            ossPrintf( "Parse svcname failed: %d" OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }

      // -a / --all
      if ( vm.count( SEADPTCTL_OPTION_ALL ) )
      {
         isAll = TRUE ;
      }

      // -l / --long
      if ( vm.count( PMD_OPTION_LONG ) )
      {
         showLong = TRUE ;
      }

      // --force
      if ( vm.count( PMD_OPTION_FORCE ) )
      {
         isForce = TRUE ;
      }

   done:
      PD_TRACE_EXITRC ( SDB_SDBSEACTL_RESVARG, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void buildCommand( const CHAR *execFileName,
                      const string &nodeCfgPath,
                      string &command )
   {
      command = execFileName ;

      // config path is specified, append to command line
      if ( !nodeCfgPath.empty() )
      {
         command += " " ;
         command += SDBCM_OPTION_PREFIX PMD_OPTION_CONFPATH ;
         command += " " ;
         command += nodeCfgPath ;
      }

   }

   void getNodeBySvcNames( UTIL_VEC_NODES &nodeInfoList,
                           vector< string > &serviceNameList )
   {
      BOOLEAN bFind = FALSE ;
      UTIL_VEC_NODES::iterator itrNode = nodeInfoList.begin() ;
      while ( itrNode != nodeInfoList.end() && serviceNameList.size() > 0 )
      {
         bFind = FALSE ;
         utilNodeInfo &nodeInfo = *itrNode ;
         for ( UINT32 i = 0 ; i < serviceNameList.size() ; ++i )
         {
            if ( nodeInfo._svcname == serviceNameList[ i ] )
            {
               bFind = TRUE ;
               break ;
            }
         }
         if ( !bFind )
         {
            itrNode = nodeInfoList.erase( itrNode ) ;
            continue ;
         }
         ++itrNode ;
      }
   }

   BOOLEAN checkNodeExistBySvcName ( const CHAR *serviceName,
                                     utilNodeInfo &nodeInfo )
   {
      UTIL_VEC_NODES nodeInfoList ;
      INT32 rc = utilListNodes( nodeInfoList, SDB_TYPE_SEADAPTER, serviceName ) ;
      if ( SDB_OK == rc && nodeInfoList.size() > 0 )
      {
         nodeInfo = *nodeInfoList.begin() ;
         return TRUE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSEACTL_STARTNODE, "startNode" )
   INT32 startNode( const CHAR *rootPath,
                    vector< string > &serviceNameList,
                    BOOLEAN isAll )
   {
      INT32 rc                                    = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSEACTL_STARTNODE ) ;
      INT32 tmpRC                                 = SDB_OK ;
      CHAR execFilePath[ OSS_MAX_PATHSIZE + 1 ]   = { 0 } ;
      CHAR cfgDirPath[ OSS_MAX_PATHSIZE + 1 ]     = { 0 } ;
      INT32 total                                 = 0 ;
      INT32 succeedNum                            = 0 ;
      INT32 failedNum                             = 0 ;
      utilNodeInfo nodeInfo ;
      string command ;
      vector< utilNodeInfo > nodeInfoList ;
      vector< OSSHANDLE > nodeHandleList ;
      vector< ossCmdRunner* > nodeCmdRunnerList ;
      vector< string > nodeCfgPathList ;

      // build sdbseadapter executable path, e.g: /opt/sequoiadb/bin/sdbseadapter
      rc = utilBuildFullPath( rootPath, SEADPT_EXE_FILE_NAME , OSS_MAX_PATHSIZE,
                              execFilePath ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Build sdbseadapter executable path name failed: %d" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      // build config directory path, e.g: /opt/sequoiadb/bin/../conf/sdbseadapter
      rc = utilBuildFullPath( rootPath, SEADPTCTL_CFG_PATH, OSS_MAX_PATHSIZE,
                              cfgDirPath ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to build sdbseadapter config directory path: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }

      // if start all node, get all subdirectory of config directory as serviceName
      if ( isAll )
      {
         serviceNameList.clear() ;
         rc = ossEnumSubDirs( cfgDirPath, serviceNameList, 1 ) ;
         if ( rc )
         {
            ossPrintf( "Error: Enum [%s] sub dirs failed: %d" OSS_NEWLINE,
                       cfgDirPath, rc ) ;
            goto error;
         }
      }

      // add nodeInfo, nodeCfgPath, nodeHandle, nodeCmdRunner for each node to be stared
      for ( UINT32 i = 0 ; i < serviceNameList.size() ; ++i )
      {
         nodeInfo._svcname = serviceNameList[ i ] ;
         nodeInfoList.push_back( nodeInfo ) ;
         nodeCfgPathList.push_back ( string( cfgDirPath ) +
                                     string( OSS_FILE_SEP ) +
                                     serviceNameList[ i ] ) ;
         nodeHandleList.push_back( (OSSHANDLE)0 ) ;
         nodeCmdRunnerList.push_back( SDB_OSS_NEW ossCmdRunner() ) ;
      }

      // start node
      for ( UINT32 i = 0 ; i < serviceNameList.size() ; ++i )
      {
         ++total ;
         string nodeSvcName         = serviceNameList[ i ] ;
         string nodeCfgPath         = nodeCfgPathList[ i ] ;
         utilNodeInfo &nodeInfo     = nodeInfoList[ i ] ;
         OSSHANDLE &nodeHandle      = nodeHandleList[ i ] ;
         ossCmdRunner *nodeRunner   = nodeCmdRunnerList[ i ] ;
         UINT32 exitCode = 0 ;

         // check exist
         if ( checkNodeExistBySvcName( nodeSvcName.c_str(), nodeInfo ) )
         {
            ossPrintf ( "Success: %s(%s) is already started (%d)" OSS_NEWLINE,
                        utilDBTypeStr( (SDB_TYPE) nodeInfo._type ),
                        nodeInfo._svcname.c_str(), nodeInfo._pid ) ;
            ++succeedNum ;
            continue ;
         }

         // build command
         buildCommand( execFilePath, nodeCfgPath.c_str(), command ) ;
         PD_LOG( PDDEBUG, "Start %s command: %s", SEADPT_PROCESS_NAME, command.c_str() ) ;

         // run command
         tmpRC = nodeRunner->exec( command.c_str(), exitCode,
                                   TRUE, -1, TRUE, &nodeHandle ) ;
         if ( SDB_OK != tmpRC )
         {
            rc = tmpRC ;
            ossPrintf( "Error: Start [%s] failed, rc: %d(%s)" OSS_NEWLINE,
                       nodeCfgPath.c_str(), tmpRC, getErrDesp( rc ) );
            ++failedNum ;
            continue ;
         }

         // get PID
         nodeInfo._pid = nodeRunner->getPID() ;
      }

      // wait node to be ok
      for ( UINT32 i = 0 ; i < serviceNameList.size() ; ++i )
      {
         string nodeSvcName         = serviceNameList[ i ] ;
         string nodeCfgPath         = nodeCfgPathList[ i ] ;
         utilNodeInfo &nodeInfo     = nodeInfoList[ i ] ;
         OSSHANDLE &nodeHandle      = nodeHandleList[ i ] ;
         ossCmdRunner *nodeRunner   = nodeCmdRunnerList[ i ] ;
         UINT32 exitCode = 0 ;

         // already start node
         if ( !nodeInfo._orgname.empty() )
         {
            continue ;
         }

         // failed to start node
         if ( nodeInfo._pid == OSS_INVALID_PID )
         {
            continue ;
         }

         tmpRC = utilWaitNodeOK( nodeInfo, nodeSvcName.c_str(),
                                 nodeInfo._pid, SDB_TYPE_SEADAPTER ) ;
         nodeRunner->done() ;

         if ( SDB_OK == tmpRC )
         {
            ossPrintf( "Success: %s(%s) is successfully started (%d)" OSS_NEWLINE,
                       SEADPT_PROCESS_NAME, nodeInfo._svcname.c_str(), nodeInfo._pid ) ;
            ++succeedNum ;
         }
         else
         {
            rc = tmpRC ;

            /// read out
            if ( (OSSHANDLE)0 != nodeHandle )
            {
               string outString ;
               nodeRunner->read( outString ) ;
               utilStrTrim( outString ) ;
#if defined( _WINDOWS )
               // need to remove all '\r'
               erase_all( outString, "\r" ) ;
#endif // _WINDOWS
               if ( !outString.empty() )
               {
                  ossPrintf( "%s: %u bytes out==>%s%s%s<==" OSS_NEWLINE,
                             nodeInfo._svcname.c_str(),
                             (UINT32)( outString.length() +
                             ossStrlen( OSS_NEWLINE ) * 2 ),
                             OSS_NEWLINE,
                             outString.c_str(),
                             OSS_NEWLINE ) ;
               }
            }

            if ( !ossIsProcessRunning( nodeInfo._pid ) &&
                 (OSSHANDLE)0 != nodeHandle &&
                 SDB_OK == ossGetExitCodeProcess( nodeHandle, exitCode ) )
            {
               rc = exitCode ;
            }
            ossPrintf( "Error: Start [%s] failed, rc: %d(%s)" OSS_NEWLINE,
                       nodeCfgPath.c_str(),  rc, getErrDesp( utilShellRC2RC( rc ) ) ) ;
            ++failedNum ;
         }

         // close handle
         ossCloseProcessHandle( nodeHandle ) ;
      }

      // print start total info
      if ( 0 == total )
      {
         ossPrintf( "No node configs need to be started" OSS_NEWLINE ) ;
         rc = SDB_INVALIDARG ;
      }
      else
      {
         ossPrintf( "Total: %d; Succeed: %d; Failed: %d" OSS_NEWLINE,
                    total, succeedNum, failedNum ) ;
      }

   done:
      {
         vector< ossCmdRunner* >::iterator it = nodeCmdRunnerList.begin() ;
         while ( it != nodeCmdRunnerList.end() )
         {
            SDB_OSS_DEL *it ;
            ++it ;
         }
      }
      return rc ;
      PD_TRACE_EXITRC( SDB_SDBSEACTL_STARTNODE, rc ) ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSEACTL_STOPNODE, "stopNode" )
   INT32 stopNode( vector< string > serviceNameList,
                   BOOLEAN &isAll, BOOLEAN &isForce )
   {
      INT32 rc         = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSEACTL_STOPNODE ) ;
      INT32 succeedNum = 0 ;
      INT32 failedNum  = 0 ;
      INT32 total      = 0 ;
      UTIL_VEC_NODES nodeInfoList ;
      UTIL_VEC_NODES::iterator itrNode ;

      // get all running adapter nodes
      rc = utilListNodes( nodeInfoList, SDB_TYPE_SEADAPTER ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // if no stop all adapter node, stop the specified node
      if ( !isAll )
      {
         getNodeBySvcNames( nodeInfoList, serviceNameList ) ;
      }

      // stop adapter node
      itrNode = nodeInfoList.begin() ;
      while ( itrNode != nodeInfoList.end() )
      {
         ++total ;
         utilNodeInfo &nodeInfo = *itrNode ;

         rc = utilAsyncStopNode( nodeInfo ) ;
         if ( SDB_OK != rc )
         {
            ossPrintf ( "Terminating process %d: %s(%s)" OSS_NEWLINE,
                        nodeInfo._pid,
                        utilDBTypeStr( (SDB_TYPE)nodeInfo._type ),
                        nodeInfo._svcname.c_str() ) ;
            if ( SDB_CLS_NODE_NOT_EXIST == rc )
            {
               rc = SDB_OK ;
               ossPrintf( "DONE" OSS_NEWLINE ) ;
               ++succeedNum ;
            }
            else
            {
               ossPrintf( "FAILED" OSS_NEWLINE ) ;
               ++failedNum ;
            }

            itrNode = nodeInfoList.erase( itrNode ) ;
            continue ;
         }
         else
         {
            ++itrNode ;
         }
      }

      // wait node stop
      itrNode = nodeInfoList.begin() ;
      while ( itrNode != nodeInfoList.end() )
      {
         utilNodeInfo &infoInfo = *itrNode ;

         ossPrintf ( "Terminating process %d: %s(%s)" OSS_NEWLINE,
                     infoInfo._pid,
                     utilDBTypeStr( (SDB_TYPE) infoInfo._type ),
                     infoInfo._svcname.c_str() ) ;

         rc = utilStopNode( infoInfo, UTIL_STOP_NODE_TIMEOUT, isForce, TRUE ) ;
         if ( SDB_OK == rc )
         {
            PD_LOG( PDEVENT, "Successful to stop seadapter node %d: %s(%s)",
                    infoInfo._pid,
                    utilDBTypeStr( (SDB_TYPE) infoInfo._type ),
                    infoInfo._svcname.c_str() ) ;
            ossPrintf( "DONE" OSS_NEWLINE ) ;
            ++succeedNum ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to stop seadapter node %d: %s(%s), rc: %d",
                    infoInfo._pid,
                    utilDBTypeStr( (SDB_TYPE) infoInfo._type ),
                    infoInfo._svcname.c_str(), rc ) ;
            ossPrintf( "FAILED" OSS_NEWLINE ) ;
            ++failedNum ;
         }
         ++ itrNode ;
      }

      // print the result info
      ossPrintf ( "Total: %d; Success: %d; Failed: %d" OSS_NEWLINE,
                  total, succeedNum, failedNum ) ;

      if ( total == succeedNum )
      {
         rc = SDB_OK ;
      }
      else if ( 0 == succeedNum )
      {
         rc = STOPFAIL ;
         goto error ;
      }
      else
      {
         rc = STOPPART ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_SDBSEACTL_STOPNODE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void printfAll( utilNodeInfo &node, BOOLEAN showLong )
   {
      CHAR tmpPID[ 11 ] = { '-', 0 } ;
      const CHAR* type = utilDBTypeStr( (SDB_TYPE)node._type ) ;

      if ( node._pid != OSS_INVALID_PID )
      {
         ossSnprintf( tmpPID, sizeof( tmpPID ) - 1, "%d", node._pid ) ;
      }

      if ( !showLong )
      {
         // style
         // Type(SvcName) (PID) Role
         // sdbseadapter(11827) (15896) A
         ossPrintf( "%s(%s) (%s) %s" OSS_NEWLINE,
                    type,
                    node._svcname.c_str(),
                    tmpPID,
                    utilDBRoleShortStr( (SDB_ROLE)node._role ) ) ;
      }
      else
      {
         struct tm otm ;
         time_t tt = node._startTime ;
         CHAR startTime[ 21 ] = { 0 } ;

         ossLocalTime( tt, otm ) ;
         ossSnprintf( startTime, sizeof( startTime ) - 1,
                      "%04d-%02d-%02d-%02d.%02d.%02d",
                      otm.tm_year+1900,
                      otm.tm_mon+1,
                      otm.tm_mday,
                      otm.tm_hour,
                      otm.tm_min,
                      otm.tm_sec ) ;

         // Long style
         // Name          SvcName       Role        PID    DataSvcName   Mode        StartTime
         // sdbseadapter  11827         seadapter   15896  11820         1001        2023-04-23-11:01:01
         ossPrintf( SEADPTCTL_LIST_LONG_FORMAT OSS_NEWLINE,
                    type,
                    node._svcname.c_str(),
                    utilDBRoleStr( (SDB_ROLE)node._role ),
                    tmpPID,
                    node._dataSvcname.empty() ? "-" : node._dataSvcname.c_str(),
                    node._mode.empty() ? "-" : node._mode.c_str(),
                    startTime ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSEACTL_LISTNODE, "listNode" )
   INT32 listNode( vector< string > &serviceNameList, BOOLEAN showLong )
   {
      INT32 rc      = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSEACTL_LISTNODE ) ;
      INT32 total   = 0 ;
      UTIL_VEC_NODES nodeInfoList ;

      // get all running adapter nodes
      rc = utilListNodes( nodeInfoList, SDB_TYPE_SEADAPTER ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // by default, list all node informations,
      // if use -p / --svcname list the specified node informations
      getNodeBySvcNames( nodeInfoList, serviceNameList ) ;

      if ( showLong )
      {
         ossPrintf( "%s" OSS_NEWLINE, SEADPTCTL_LIST_TITLE ) ;
      }

      for ( UINT32 i = 0 ; i < nodeInfoList.size() ; ++i )
      {
         total++ ;
         printfAll( nodeInfoList[ i ], showLong ) ;
      }

      ossPrintf ( "Total: %d" OSS_NEWLINE, total ) ;
   done :
      PD_TRACE_EXITRC( SDB_SDBSEACTL_LISTNODE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 buildDialogFilePath( CHAR *currentPath, CHAR *dialogFile )
   {
      INT32 rc = SDB_OK ;

      // build dialog path, e.g: /opt/sequoiadb/bin/../conf/log
      rc = utilBuildFullPath( currentPath, SEADPTCTL_LOG_PATH, OSS_MAX_PATHSIZE,
                              dialogFile ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to build dialog path: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }

      // make sure the dialog path exist
      rc = ossMkdir( dialogFile ) ;
      if ( SDB_OK != rc && SDB_FE != rc )
      {
         ossPrintf( "Create dialog directory [%s] failed, rc: %d" OSS_NEWLINE,
                    dialogFile, rc ) ;
         goto error ;
      }

      // build dialog file, e.g: /opt/sequoiadb/bin/../conf/log/sdbseactl.log
      rc = utilCatPath( dialogFile, OSS_MAX_PATHSIZE, SEADPTCTL_LOG_FILE_NAME ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to build dialog file: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_SDBSEACTL_MAIN, "mainEntry" )
   INT32 mainEntry( INT32 argc, CHAR **argv )
   {
      INT32 rc                                 = SDB_OK ;
      PD_TRACE_ENTRY ( SDB_SDBSEACTL_MAIN ) ;
      // cmd base option define
      po::options_description desc( "Command options" ) ;
      // cmd base and hide optin define
      po::options_description all( "Command options" ) ;
      // dir and file path define
      po::variables_map vm ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ]    = { 0 } ;
      CHAR dialogFile[ OSS_MAX_PATHSIZE + 1 ]  = { 0 } ;
      CHAR verText[ OSS_MAX_PATHSIZE + 1 ]     = { 0 } ;
      BOOLEAN isAll                            = FALSE ;
      BOOLEAN isForce                          = FALSE ;
      BOOLEAN showLong                         = FALSE ;
      INT32 mode                               = -1 ;
      vector< string > serviceNameList ;

      /// initial cmd option define
      init( desc, all ) ;

      /// validate and resolve cmd arguments
      rc = resolveArgument( desc, all, vm, argc, argv,
                            mode, serviceNameList, isAll, showLong, isForce ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            ossPrintf( "Error: Invalid argument: %d" OSS_NEWLINE, rc ) ;
            displayArg ( desc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_OK ;
            goto done ;
         }
      }

#if defined ( _LINUX )
      /// check ulimit
      if ( MODE_TYPE_START == mode && !vm.count( PMD_OPTION_IGNOREULIMIT ) )
      {
         rc = utilSetAndCheckUlimit() ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Error: start sdbseadapter will set ulimit by file"
                       "[conf/limits.conf], if you want to set ulimit by "
                       "current terminal, please use parameter '-i'." OSS_NEWLINE ) ;
            goto error ;
         }
      }
#endif

      /// change user
      if ( MODE_TYPE_START == mode || MODE_TYPE_STOP == mode )
      {
         if ( !vm.count( PMD_OPTION_CURUSER ) )
         {
            UTIL_CHECK_AND_CHG_USER() ;
         }
      }

      /// get program root path
      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( rc )
      {
         ossPrintf( "Error: Get module self path failed: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }

      /// build dialogFile path
      rc = buildDialogFilePath( rootPath, dialogFile ) ;
      if ( rc )
      {
         ossPrintf( "Error: Build dialog File Path failed: %d" OSS_NEWLINE, rc ) ;
         goto error ;
      }

      /// enable pd log
      sdbEnablePD( dialogFile ) ;
      setPDLevel( PDINFO ) ;

      /// print version info to the dialog file
      ossSprintVersion( "Version", verText, OSS_MAX_PATHSIZE, FALSE ) ;
      PD_LOG( PDEVENT, "Start program [%s]...", verText ) ;
#if defined(_LINUX)
      if ( MODE_TYPE_START == mode && vm.count( PMD_OPTION_IGNOREULIMIT ) )
      {
         PD_LOG( PDWARNING, "Start programme with setting ulimit based on "
                 "current terminal" ) ;
      }
#endif

      /// choose mode
      switch ( mode )
      {
         case MODE_TYPE_START :
            rc = startNode( rootPath, serviceNameList, isAll ) ;
            break ;

         case MODE_TYPE_STOP :
            rc = stopNode( serviceNameList, isAll, isForce ) ;
            break ;

         case MODE_TYPE_LIST :
            rc = listNode( serviceNameList, showLong ) ;
            break ;

         default :
            rc = SDB_INVALIDARG ;
            break ;
      }

   done:
      PD_LOG( PDEVENT, "Stop program(%d).", rc ) ;
      PD_TRACE_EXITRC( SDB_SDBSEACTL_MAIN, rc ) ;
      return ( rc >= 0 ) ? rc : utilRC2ShellRC( rc ) ;
   error:
      goto done ;
   }

}

INT32 main( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}
