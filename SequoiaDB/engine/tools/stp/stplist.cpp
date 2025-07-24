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

   Source File Name = stplist.cpp

   Descriptive Name = stplist main

   When/how to use: this program may be used on binary and text-formatted
   versions of PMD component. This file contains main function for stplist.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          03/10/2021  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "ossUtil.hpp"
#include "ossProc.hpp"
#include "ossMem.hpp"
#include "ossPath.hpp"
#include "msgDef.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "pmdTrace.hpp"
#include "pmdDef.hpp"
#include "pmd.hpp"
#include "pmdOptionsMgr.hpp"
#include "utilNodeOpr.hpp"
#include "utilCommon.hpp"
#include "ossVer.h"
#include "utilParam.hpp"
#include "utilStr.hpp"
#include "pmdDaemon.hpp"
#include "../bson/bson.h"
#include <string>
#include <iostream>
#include <vector>
#include "stpOptions.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   // options define
   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_MODE, ",m" ), po::value<string>(),"mode type: run/local, default: run" ) \
       ( PMD_COMMANDS_STRING( PMD_OPTION_LONG, ",l" ), "show long style" ) \
       ( PMD_OPTION_VERSION, "version" ) \
       ( PMD_OPTION_DETAIL, "show details" ) \
       ( PMD_OPTION_EXPAND, "show expanded details" )

   // long format define
   #define PMD_LIST_LONG_FORMAT "%-10.9s %-13.12s %-9.8s %-4.3s %-20.19s"
   #define PMD_LIST_TITLE "Name       SvcName       PID       PRY  StartTime"

   static void _printfConf( const CHAR *rootPath, BOOLEAN isLocal )
   {
      INT32 rc = SDB_OK ;

      BSONObj objData ;
      stpOptions option ;

      rc = option.initFromRootPath( rootPath ) ;
      if ( SDB_OK != rc )
      {
         // not OK, do nothing
         return ;
      }

      rc = option.toBSON( objData,
                          isLocal ?
                                PMD_CFG_MASK_SKIP_UNFIELD :
                                PMD_CFG_MASK_SKIP_HIDEDFT ) ;
      if ( SDB_OK != rc )
      {
         // not OK, do nothing
         return ;
      }

      BSONObjIterator it = objData.begin() ;
      while( it.more() )
      {
         BSONElement e = it.next() ;
         if( e.type() == String )
         {
            ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, e.fieldName(),
                       e.valuestr() ) ;
         }
         else if( e.type() == NumberInt )
         {
            ossPrintf( "   %-18.18s: %d" OSS_NEWLINE, e.fieldName(),
                       e.numberInt() ) ;
         }
         else if( e.type() == NumberLong )
         {
            ossPrintf( "   %-18.18s: %lld" OSS_NEWLINE, e.fieldName(),
                       e.numberLong() ) ;
         }
         else if( e.type() == NumberDouble )
         {
            ossPrintf( "   %-18.18s: %f" OSS_NEWLINE, e.fieldName(),
                       e.numberDouble() ) ;
         }
         else if( e.type() == Bool )
         {
            ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, e.fieldName(),
                       (e.boolean() ? "TRUE" : "FALSE" ) ) ;
         }
         else
         {
            ossPrintf( "   %-18.18s: %s" OSS_NEWLINE, e.fieldName(), "-" ) ;
         }
      }
   }

   //printf detail or expand
   static void _printfAll( const CHAR *rooPath,
                           utilNodeInfo &node,
                           BOOLEAN detail,
                           BOOLEAN expand,
                           BOOLEAN showLong )
   {
      CHAR tmpPID[ 11 ] = { '-', 0 } ;

      if ( node._pid != OSS_INVALID_PID )
      {
         ossSnprintf( tmpPID, sizeof( tmpPID ) - 1, "%d", node._pid ) ;
      }

      if ( !showLong )
      {
         ossPrintf( "%s(%s) (%s) %s" OSS_NEWLINE,
                    utilDBTypeStr( (SDB_TYPE)node._type ),
                    node._svcname.c_str(), tmpPID,
                    utilDBRoleShortStr( (SDB_ROLE)node._role ) ) ;
      }
      else
      {
         struct tm otm ;
         time_t tt = node._startTime ;

         CHAR tmpGID[ 11 ] = { '-', 0 } ;
         CHAR tmpNID[ 11 ] = { '-', 0 } ;
         CHAR tmpPRY[ 11 ] = { '-', 0 } ;
         CHAR tmpTime[ 21 ] = { 0 } ;
         string roleStr = utilDBRoleStr( (SDB_ROLE)node._role ) ;
         // name       svcname       role        pid    gid    nid    gname           StartTime            dbpath
         // sequoaidb  11810         standalone  15896  1001   1001   db1             2014-02-02-11:01:01  /opt/sequoiadb/database/coord/11810
         // sdbcm      11790         -           10076  -      -      -               2014-02-02-11:01:01  -

#if defined (_WINDOWS)
         localtime_s( &otm, &tt ) ;
#else
         localtime_r( &tt, &otm ) ;
#endif
         ossSnprintf( tmpTime, sizeof( tmpTime ) - 1,
                      "%04d-%02d-%02d-%02d.%02d.%02d",
                      otm.tm_year+1900,
                      otm.tm_mon+1,
                      otm.tm_mday,
                      otm.tm_hour,
                      otm.tm_min,
                      otm.tm_sec ) ;

         if ( 0 != node._groupID )
         {
            ossSnprintf( tmpGID, sizeof( tmpGID ) - 1, "%d", node._groupID ) ;
         }
         if ( 0 != node._nodeID )
         {
            ossSnprintf( tmpNID, sizeof( tmpNID ) - 1, "%d", node._nodeID ) ;
         }

         if ( -1 != node._primary )
         {
            ossStrcpy( tmpPRY, ( 1 == node._primary ) ? "Y" : "N" ) ;
         }

         ossPrintf( PMD_LIST_LONG_FORMAT OSS_NEWLINE,
                    utilDBTypeStr( (SDB_TYPE)node._type ),
                    node._svcname.c_str(),
                    tmpPID,
                    tmpPRY,
                    tmpTime ) ;
      }

      if( detail )
      {
         _printfConf( rooPath, TRUE ) ;
      }
      else if( expand )
      {
         _printfConf( rooPath, FALSE ) ;
      }
   }

   static INT32 _addLocalNode( const CHAR *rootPath,
                               UTIL_VEC_NODES &listNodes )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN isConfFileValid = FALSE ;
      string serviceName ;
      CHAR stpConfPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      // build 'conf' file path
      rc = utilBuildFullPath( rootPath, STP_ROOT_PATH, OSS_MAX_PATHSIZE,
                              stpConfPath ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error:Get STP config path failed: %d" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

      rc = utilGetServiceByConfigPath( stpConfPath,
                                       STP_CFG_FILE_NAME,
                                       STP_OPTION_PORT,
                                       STP_DEF_SERVICE_NAME,
                                       serviceName,
                                       TRUE,
                                       &isConfFileValid ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error:Get STP config file failed: %d" OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }
      if ( isConfFileValid )
      {
         utilNodeInfo node ;
         node._orgname = "" ;
         node._pid = OSS_INVALID_PID ;
         node._role = SDB_ROLE_STP ;
         node._type = SDB_TYPE_STP ;
         node._svcname = serviceName ;
         listNodes.push_back( node ) ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // initialize options
   static void init( po::options_description &desc )
   {
      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END
   }

   void displayArg( po::options_description &desc )
   {
      std::cout << desc << std::endl ;
   }

   static INT32 resolveArgument( po::options_description &desc,
                                 INT32 argc,
                                 CHAR **argv,
                                 INT32 &modeFilter,
                                 BOOLEAN &detail,
                                 BOOLEAN &expand,
                                 BOOLEAN &showLong )
   {
      INT32 rc = SDB_OK ;

      po::variables_map vm ;

      rc = utilReadCommandLine( argc, argv,  desc, vm, FALSE ) ;
      if ( rc )
      {
         std::cout << "Read command line failed: " << rc << endl ;
         goto error ;
      }

      if ( vm.count ( PMD_OPTION_HELP ) )
      {
         displayArg ( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto error ;
      }
      else if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "STP List Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto error ;
      }

      if ( vm.count( PMD_OPTION_MODE ) )
      {
         string modeTemp = vm[PMD_OPTION_MODE].as<string>() ;
         if( 0 == ossStrcasecmp( modeTemp.c_str(),
                                 SDB_RUN_MODE_TYPE_LOCAL_STR ) )
         {
            modeFilter = RUN_MODE_LOCAL ;
         }
         else if( 0 == ossStrcasecmp( modeTemp.c_str(),
                                      SDB_RUN_MODE_TYPE_RUN_STR ) )
         {
            modeFilter = RUN_MODE_RUN ;
         }
         else
         {
            std::cout << "mode invalid" << endl ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      if( vm.count( PMD_OPTION_DETAIL ) )
      {
         detail = TRUE ;
      }
      if ( vm.count( PMD_OPTION_EXPAND ) )
      {
         expand = TRUE ;
         detail = FALSE ;
      }
      if ( vm.count( PMD_OPTION_LONG ) )
      {
         showLong = TRUE ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 mainEntry( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;

      INT32 total = 0 ;
      UTIL_VEC_NODES listNodes ;
      BOOLEAN detail       = FALSE ;
      BOOLEAN expand       = FALSE ;
      BOOLEAN showLong     = FALSE ;
      INT32 modeFilter     = RUN_MODE_RUN ;
      CHAR rootPath[ OSS_MAX_PATHSIZE + 1 ] = { 0 } ;

      po::options_description desc ( "Command options" ) ;
      init ( desc ) ;

      // validate arguments
      rc = resolveArgument( desc, argc, argv, modeFilter, detail, expand,
                            showLong ) ;
      if( SDB_OK != rc )
      {
         if( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            std::cout << "Invalid argument" << endl ;
            displayArg ( desc ) ;
         }
         goto done ;
      }

      // get program's running path
      rc = ossGetEWD( rootPath, OSS_MAX_PATHSIZE ) ;
      if ( SDB_OK != rc )
      {
        ossPrintf( "Error:Get module self path failed: %d" OSS_NEWLINE, rc ) ;
        goto error ;
      }

      utilListNodes( listNodes, SDB_TYPE_STP, NULL, OSS_INVALID_PID, -1 ) ;

      // if local mode and no STP node is found, should get STP from conf file
      if ( RUN_MODE_LOCAL == modeFilter && listNodes.empty() )
      {
         rc = _addLocalNode( rootPath, listNodes ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      if ( showLong )
      {
         // print title
         ossPrintf( "%s" OSS_NEWLINE, PMD_LIST_TITLE ) ;
      }
      // print
      for ( UINT32 i = 0 ; i < listNodes.size() ; ++i )
      {
         ++total ;
         _printfAll( rootPath, listNodes[ i ], detail, expand, showLong ) ;
      }

      ossPrintf ( "Total: %d" OSS_NEWLINE, total ) ;

   done :
      if ( SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc )
      {
         return 0 ;
      }
      return total > 0 ? 0 : ( rc ? SDB_SRC_INVALIDARG : 1 ) ;
   error :
      goto done ;
   }
}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}


