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
#include "pd.hpp"
#include "pmdDef.hpp"
#include "pmd.hpp"
#include "ossVer.h"
#include "pmdDaemon.hpp"
#include "stpToolCommon.hpp"
#include "stpToolUtil.hpp"
#include "stpClient.hpp"
#include "stpNode.hpp"
#include "stpOptions.hpp"
#include "../bson/bson.hpp"

namespace po = boost::program_options ;

using namespace std ;
using namespace bson ;

namespace engine
{

   #define STPQ_MAX_SHORT_STR_LEN   ( 32 )
   #define STPQ_MAX_LONG_STR_LEN    ( 256 )

   #define STPQ_OPTION_HOSTNAME           "hostname"
   #define STPQ_OPTION_PORT               "port"
   #define STPQ_OPTION_GETCONF            "conf"
   #define STPQ_OPTION_GETCONFFULL        "conffull"
   #define STPQ_OPTION_GETTIME            "time"
   #define STPQ_OPTION_GETTIMEUS          "timeus"
   #define STPQ_OPTION_GETMETA            "meta"
   #define STPQ_OPTION_GETSERVERS         "servers"
   #define STPQ_OPTION_GETSYNCCLIENTS     "syncclients"
   #define STPQ_OPTION_GETSYNCSTATUS      "syncstatus"
   #define STPQ_OPTION_GETSYNCHISTORY     "synchistory"
   #define STPQ_OPTION_COUNT              "count"
   #define STPQ_OPTION_DELAY              "delay"

   #define STPQ_DFT_COUNT     ( 1 )
   #define STPQ_DFT_DELAY_SEC ( 10 )

   #define STP_NANOOFFSET_TO_MICROOFFSET( x ) ( (INT64)( x ) / 1000LL )

   #define COMMANDS_OPTIONS \
       ( PMD_COMMANDS_STRING( STP_OPTION_HELP, ",h" ), \
             "help" ) \
       ( STP_OPTION_VERSION, \
             "version" ) \
       ( PMD_COMMANDS_STRING( STPQ_OPTION_HOSTNAME, ",s" ), \
             po::value<string>(), \
             "host name of STP, default: localhost" ) \
       ( PMD_COMMANDS_STRING( STPQ_OPTION_PORT, ",p" ), \
             po::value<string>(), \
             "port of STP, default: 9622" ) \
       ( STPQ_OPTION_GETTIME, \
             "get logical time in nanoseconds, default behavior" ) \
       ( STPQ_OPTION_GETTIMEUS, \
             "get logical time in microseconds" ) \
       ( STPQ_OPTION_GETCONF, \
             "get configuration of STP" ) \
       ( STPQ_OPTION_GETCONFFULL, \
             "get full configuration of STP" ) \
       ( STPQ_OPTION_GETMETA, \
             "get meta" ) \
       ( STPQ_OPTION_GETSERVERS, \
             "list servers" ) \
       ( STPQ_OPTION_GETSYNCCLIENTS, \
             "list synchronize clients" ) \
       ( STPQ_OPTION_GETSYNCSTATUS, \
             "get synchronize status of current source and history " \
             "synchronize records of current source" ) \
       ( STPQ_OPTION_GETSYNCHISTORY, \
             "list history sources to synchronize with" ) \
       ( PMD_COMMANDS_STRING( STPQ_OPTION_COUNT, ",n" ), \
             po::value<INT32>(), \
             "count of iterations" ) \
       ( PMD_COMMANDS_STRING( STPQ_OPTION_DELAY, ",d" ), \
             po::value<INT32>(), \
             "delay seconds between each iteration, default is 10 seconds. " \
             "specified --delay without --count will print results in " \
             "non-stop mode" )

   #define COMMANDS_HIDE_OPTIONS \
      ( STP_OPTION_HELPFULL, "help all options" )

   // task of STPQ
   enum STPQ_TASK_TYPE
   {
      STPQ_TASK_NONE,
      STPQ_TASK_GETTIME,
      STPQ_TASK_GETTIMEUS,
      STPQ_TASK_GETCONF,
      STPQ_TASK_GETCONFFULL,
      STPQ_TASK_GETMETA,
      STPQ_TASK_GETSERVERS,
      STPQ_TASK_GETSYNCCLIENTS,
      STPQ_TASK_GETSYNCSTATUS,
      STPQ_TASK_GETSYNCHISTORY
   } ;

   // get command for task
   static const CHAR *_stpqGetTaskCommand( STPQ_TASK_TYPE taskType )
   {
      switch ( taskType )
      {
         case STPQ_TASK_GETTIME :
         {
            return CMD_NAME_STP_GET_TIME ;
         }
         case STPQ_TASK_GETTIMEUS :
         {
            return CMD_NAME_STP_GET_TIME_US ;
         }
         case STPQ_TASK_GETCONF :
         case STPQ_TASK_GETCONFFULL :
         {
            return CMD_NAME_STP_GET_CONFIG ;
         }
         case STPQ_TASK_GETMETA :
         {
            return CMD_NAME_STP_GET_META ;
         }
         case STPQ_TASK_GETSERVERS :
         {
            return CMD_NAME_STP_GET_SERVERS ;
         }
         case STPQ_TASK_GETSYNCCLIENTS :
         {
            return CMD_NAME_STP_GET_SYNC_CLIENTS ;
         }
         case STPQ_TASK_GETSYNCSTATUS :
         {
            return CMD_NAME_STP_GET_SYNC_STATUS ;
         }
         case STPQ_TASK_GETSYNCHISTORY :
         {
            return CMD_NAME_STP_GET_SYNC_HISTORY ;
         }
         default :
         {
            break ;
         }
      }
      return "unknown" ;
   }

   // initialize arguments
   static void _stpqInitArgument( po::options_description &desc,
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

   // display arguments as help messages
   static void _stpqDisplayArgument( po::options_description &desc )
   {
      cout << desc << endl ;
   }

   // resolve arguments
   static INT32 _stpqResolveArgument( po::options_description &desc,
                                      po::options_description &all,
                                      po::variables_map &vm,
                                      INT32 argc,
                                      CHAR **argv,
                                      string &hostName,
                                      string &serviceName,
                                      list<STPQ_TASK_TYPE> &taskList,
                                      UINT32 &count,
                                      UINT32 &delay )
   {
      INT32 rc = SDB_OK ;

      taskList.clear() ;

      // read arguments from command line
      rc = utilReadCommandLine( argc, argv, all, vm, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // check if help or version
      if ( vm.count( STP_OPTION_HELP ) )
      {
         _stpqDisplayArgument( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto error ;
      }
      else if ( vm.count( STP_OPTION_HELPFULL ) )
      {
         _stpqDisplayArgument( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
         goto done ;
      }
      else if ( vm.count( STP_OPTION_VERSION ) )
      {
         ossPrintVersion( "STP Query Version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
         goto error ;
      }

      // get host name
      if ( vm.count( STPQ_OPTION_HOSTNAME ) )
      {
         hostName = vm[ STPQ_OPTION_HOSTNAME ].as<string>() ;
      }
      else
      {
         hostName.assign( OSS_LOCALHOST ) ;
      }

      // get port
      if ( vm.count( STPQ_OPTION_PORT ) )
      {
         serviceName = vm[ STPQ_OPTION_PORT ].as<string>() ;
      }
      else
      {
         serviceName = STP_DEF_SERVICE_NAME ;
      }

      // get count
      if ( vm.count( STPQ_OPTION_COUNT ) )
      {
         INT32 tmpCount = vm[ STPQ_OPTION_COUNT ].as<INT32>() ;
         if ( tmpCount <= 0 )
         {
            // if <= 0, set to default value
            count = STPQ_DFT_COUNT ;
         }
         else
         {
            count = (UINT32)tmpCount ;
         }
      }
      else
      {
         count = STPQ_DFT_COUNT ;
      }

      // get delay
      if ( vm.count( STPQ_OPTION_DELAY ) )
      {
         INT32 tmpDelay = vm[ STPQ_OPTION_DELAY ].as<INT32>() ;
         if ( tmpDelay <= 0 )
         {
            // if <= 0, set to default value
            delay = STPQ_DFT_DELAY_SEC ;
         }
         else
         {
            delay = (UINT32)tmpDelay ;
         }

         // set count to zero, means not stop
         if ( !vm.count( STPQ_OPTION_COUNT ) )
         {
            count = 0 ;
         }
      }
      else
      {
         delay = STPQ_DFT_DELAY_SEC ;
      }

      // get tasks
      // add task into list which will keep the order when execute
      try
      {
         if ( vm.count( STPQ_OPTION_GETTIME ) )
         {
            taskList.push_back( STPQ_TASK_GETTIME ) ;
         }
         if ( vm.count( STPQ_OPTION_GETTIMEUS ) )
         {
            taskList.push_back( STPQ_TASK_GETTIMEUS ) ;
         }
         if ( vm.count( STPQ_OPTION_GETCONF ) &&
              !vm.count( STPQ_OPTION_GETCONFFULL ) )
         {
            taskList.push_back( STPQ_TASK_GETCONF ) ;
         }
         if ( vm.count( STPQ_OPTION_GETCONFFULL ) )
         {
            taskList.push_back( STPQ_TASK_GETCONFFULL ) ;
         }
         if ( vm.count( STPQ_OPTION_GETMETA ) )
         {
            taskList.push_back( STPQ_TASK_GETMETA ) ;
         }
         if ( vm.count( STPQ_OPTION_GETSERVERS ) )
         {
            taskList.push_back( STPQ_TASK_GETSERVERS ) ;
         }
         if ( vm.count( STPQ_OPTION_GETSYNCCLIENTS ) )
         {
            taskList.push_back( STPQ_TASK_GETSYNCCLIENTS ) ;
         }
         if ( vm.count( STPQ_OPTION_GETSYNCSTATUS ) )
         {
            taskList.push_back( STPQ_TASK_GETSYNCSTATUS ) ;
         }
         if ( vm.count( STPQ_OPTION_GETSYNCHISTORY ) )
         {
            taskList.push_back( STPQ_TASK_GETSYNCHISTORY ) ;
         }

         // if task is empty, add get time instead
         if ( taskList.empty() )
         {
            taskList.push_back( STPQ_TASK_GETTIME ) ;
         }
      }
      catch ( exception &e )
      {
         ossPrintf( "Error: Failed to generate task list, error: %s",
                    e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // get enough length for formatted output with multiple lines
   static UINT32 _stpqGetFormatLen( UINT32 curLen,
                                    const CHAR *fieldName,
                                    const CHAR *strOutput )
   {
      SDB_ASSERT( NULL != fieldName, "field name is invalid" ) ;
      SDB_ASSERT( NULL != strOutput, "output is invalid" ) ;
      // get maximum length of output field
      return OSS_MAX( curLen,
                      OSS_MAX( ossStrlen( fieldName ),
                               ossStrlen( strOutput ) ) ) ;
   }

   // get enough length for formatted output with multiple lines
   static UINT32 _stpqGetFormatLen( UINT32 curLen,
                                    const CHAR *fieldName,
                                    UINT64 uintOutput )
   {
      SDB_ASSERT( NULL != fieldName, "field name is invalid" ) ;
      CHAR tmpOutput[ STPQ_MAX_SHORT_STR_LEN + 1 ] = { 0 } ;
      ossSnprintf( tmpOutput, STPQ_MAX_SHORT_STR_LEN, "%llu", uintOutput ) ;
      // get maximum length of output field
      return OSS_MAX( curLen,
                      OSS_MAX( ossStrlen( fieldName ),
                               ossStrlen( tmpOutput ) ) ) ;
   }

   // helper function to run specified command to STP and get back result
   static INT32 _stpqRunCommand( stpClient &client,
                                 const CHAR *command,
                                 const BSONObj &argument,
                                 BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      INT32 returnCode = SDB_OK ;

      rc = client.runCommand( command, argument, result, returnCode ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( SDB_OK != returnCode )
      {
         rc = returnCode ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output time
   static INT32 _stpqOutputTime( const stpLogicalTimeNS &currentTime )
   {
      ossPrintf( "Time:"OSS_NEWLINE
                 "   %-9s : ( %llu second, %llu nanosec )"OSS_NEWLINE
                 "   %-9s : %u"OSS_NEWLINE,
                 STP_FIELD_NAME_TIMESTAMP,
                 currentTime.getTime().getSecond(),
                 currentTime.getTime().getNanoSecond(),
                 STP_FIELD_NAME_TIME_ERROR,
                 currentTime.getTimeError() ) ;

      return SDB_OK ;
   }

   // get time from STP
   static INT32 _stpqGetTime( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      stpLogicalTimeNS currentTime ;
      BSONObj argument, result ;

      // run get time command
      rc = _stpqRunCommand( client, _stpqGetTaskCommand( STPQ_TASK_GETTIME ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETTIME ), rc ) ;
         goto error ;
      }

      // parse time from BSON
      rc = currentTime.fromBSON( result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to parse BSONObj for logical time, "
                    "rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

      // print result
      rc = _stpqOutputTime( currentTime ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output time, rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output time in microsecond
   static INT32 _stpqOutputTimeUS( const stpLogicalTimeUS &currentTime )
   {
      ossPrintf( "TimeUS:"OSS_NEWLINE
                 "   %-9s : %llu microsec"OSS_NEWLINE
                 "   %-9s : %u"OSS_NEWLINE,
                 STP_FIELD_NAME_TIMESTAMP,
                 currentTime.getTime(),
                 STP_FIELD_NAME_TIME_ERROR,
                 currentTime.getTimeError() ) ;

      return SDB_OK ;
   }

   // get time in microsecond from STP
   static INT32 _stpqGetTimeUS( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      stpLogicalTimeUS currentTime ;
      BSONObj argument, result ;

      // run get time command
      rc = _stpqRunCommand( client, _stpqGetTaskCommand( STPQ_TASK_GETTIMEUS ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETTIMEUS ), rc ) ;
         goto error ;
      }

      // parse time from BSON
      rc = currentTime.fromBSON( result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to parse BSONObj for logical time, "
                    "rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

      // print result
      rc = _stpqOutputTimeUS( currentTime ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output time in microsecond, "
                    "rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output full configurations
   static INT32 _stpqOutputFullConfig( const stpOptions &options )
   {
      // print result
      ossPrintf( "Config:"OSS_NEWLINE
                 "   %-24s : %s"OSS_NEWLINE  // port
                 "   %-24s : %s"OSS_NEWLINE  // serverlist
                 "   %-24s : %s"OSS_NEWLINE  // role
                 "   %-24s : %u"OSS_NEWLINE  // weight
                 "   %-24s : %u"OSS_NEWLINE  // syncinterval
                 "   %-24s : %u"OSS_NEWLINE  // maxtimeerror
                 "   %-24s : %u"OSS_NEWLINE  // maxsynchist
                 "   %-24s : %u"OSS_NEWLINE  // maxsyncports
                 "   %-24s : %u"OSS_NEWLINE  // defclientsperport
                 "   %-24s : %s"OSS_NEWLINE  // preopenports
                 "   %-24s : %s"OSS_NEWLINE  // allowsyncwithsysport
                 "   %-24s : %d"OSS_NEWLINE  // diaglevel
                 "   %-24s : %u"OSS_NEWLINE  // sharingbreak
                 "   %-24s : %u"OSS_NEWLINE  // startshifttime
                 "   %-24s : %s"OSS_NEWLINE, // testmode
                 STP_OPTION_PORT, options.getServiceName(),
                 STP_OPTION_SERVERLIST, options.getServerListString(),
                 STP_OPTION_ROLE, options.getRoleString(),
                 STP_OPTION_WEIGHT, options.getWeight(),
                 STP_OPTION_SYNCINTERVAL, options.getSyncInterval(),
                 STP_OPTION_MAXTIMEERROR, options.getMaxTimeErrorUS(),
                 STP_OPTION_MAXSYNCHIST, options.getMaxSyncHist(),
                 STP_OPTION_MAXSYNCPORTS, options.getMaxSyncPorts(),
                 STP_OPTION_DEFCLIENTSPERPORT, options.getDefClientsPerPort(),
                 STP_OPTION_PREOPENPORTS,
                 options.isPreOpenPorts() ? "TRUE" : "FALSE",
                 STP_OPTION_SYNCWITHSYSPORT,
                 options.isSyncWithSysPort() ? "TRUE" : "FALSE",
                 STP_OPTION_DIAGLEVEL, options.getDiagLevel(),
                 STP_OPTION_SHARINGBRK, options.getSharingBreakTime(),
                 STP_OPTION_STARTSHIFTTIME, options.getStartShiftTime(),
                 STP_OPTION_TESTMODE,
                 options.isTestMode() ? "TRUE" : "FALSE" ) ;

      return SDB_OK ;
   }

   // output configurations
   static INT32 _stpqOutputConfig( const stpOptions &options )
   {
      ossPrintf( "Config:"OSS_NEWLINE
                 "   %-14s : %s"OSS_NEWLINE  // port
                 "   %-14s : %s"OSS_NEWLINE  // serverlist
                 "   %-14s : %s"OSS_NEWLINE  // role
                 "   %-14s : %u"OSS_NEWLINE  // syncinterval
                 "   %-14s : %u"OSS_NEWLINE  // maxtimeerror
                 "   %-14s : %d"OSS_NEWLINE, // diaglevel
                 STP_OPTION_PORT, options.getServiceName(),
                 STP_OPTION_SERVERLIST, options.getServerListString(),
                 STP_OPTION_ROLE, options.getRoleString(),
                 STP_OPTION_SYNCINTERVAL, options.getSyncInterval(),
                 STP_OPTION_MAXTIMEERROR, options.getMaxTimeErrorUS(),
                 STP_OPTION_DIAGLEVEL, options.getDiagLevel() ) ;

      return SDB_OK ;
   }

   // get config from STP
   static INT32 _stpqGetConf( stpClient &client, BOOLEAN fullConfig )
   {
      INT32 rc = SDB_OK ;

      BSONObj argument, result, errorResult ;
      stpOptions options ;

      // run get config command
      rc = _stpqRunCommand( client, _stpqGetTaskCommand( STPQ_TASK_GETCONF ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETCONF ), rc ) ;
         goto error ;
      }

      // parse config from BSON
      rc = options.update( result, FALSE, errorResult ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to parse BSON for config, rc: %d",
                    rc ) ;
         goto error ;
      }

      if ( fullConfig )
      {
         // print result
         rc = _stpqOutputFullConfig( options ) ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Error: Failed to output full configurations, "
                       "rc: %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }
      else
      {
         // print result
         rc = _stpqOutputConfig( options ) ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Error: Failed to output configurations, "
                       "rc: %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }
      }



   done:
      return rc ;

   error:
      goto done ;
   }

   // output meta
   static INT32 _stpqOutputMeta( const string &shmKey,
                                 const stpMetaData &meta,
                                 DPS_LSN metaLSN )
   {
      ossPrintf( "Meta:"OSS_NEWLINE
                 "   %-12s : %s"OSS_NEWLINE  // shm key
                 "   %-12s : %u"OSS_NEWLINE  // version
                 "   %-12s : %u"OSS_NEWLINE  // synchronize interval
                 "   %-12s : ( %llu second, %llu nanosec )"OSS_NEWLINE  // sync hardware time
                 "   %-12s : ( %llu second, %llu nanosec )"OSS_NEWLINE  // base hardware time
                 "   %-12s : ( %llu second, %llu nanosec )"OSS_NEWLINE  // base real time
                 "   %-12s : %lld"OSS_NEWLINE   // offset
                 "   %-12s : %llu"OSS_NEWLINE   // slew rate
                 "   %-12s : %u"OSS_NEWLINE     // time error
                 "   %-12s : ( offset %llu, version %u )"OSS_NEWLINE,   // LSN
                 STP_FIELD_NAME_META_SHMKEY, shmKey.c_str(),
                 STP_FIELD_NAME_VERSION, meta.getVersion(),
                 STP_FIELD_NAME_SYNC_INTERVAL, meta.getSyncInterval(),
                 STP_FIELD_NAME_SYNC_HW_TIME,
                 meta.getSyncHardwareTime().getSecond(),
                 meta.getSyncHardwareTime().getNanoSecond(),
                 STP_FIELD_NAME_BASE_HW_TIME,
                 meta.getBaseHardwareTime().getSecond(),
                 meta.getBaseHardwareTime().getNanoSecond(),
                 STP_FIELD_NAME_BASE_REAL_TIME,
                 meta.getBaseRealTime().getSecond(),
                 meta.getBaseRealTime().getNanoSecond(),
                 STP_FIELD_NAME_OFFSET, meta.getOffset(),
                 STP_FIELD_NAME_SLEW_RATE, meta.getSlewRate(),
                 STP_FIELD_NAME_TIME_ERROR, meta.getTimeError(),
                 STP_FIELD_NAME_META_LSN, metaLSN.offset, metaLSN.version ) ;

      return SDB_OK ;
   }

   // get meta from STP
   static INT32 _stpqGetMeta( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      BSONObj argument, result ;
      string shmKey ;
      stpMetaData meta ;
      DPS_LSN metaLSN ;

      // run get meta command
      rc = _stpqRunCommand( client, _stpqGetTaskCommand( STPQ_TASK_GETMETA ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETMETA ), rc ) ;
         goto error ;
      }

      // parse BSON to get meta
      try
      {
         BSONElement subElement ;

         // get shared memory key
         subElement = result.getField( STP_FIELD_NAME_META_SHMKEY ) ;
         if ( String != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_META_SHMKEY ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         shmKey = subElement.str() ;

         // get meta data
         subElement = result.getField( STP_FIELD_NAME_META_DATA ) ;
         if ( Object != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_META_DATA ) ;
            rc = SDB_SYS ;
            goto error ;
         }

         // parse meta data
         rc = meta.fromBSON( subElement.embeddedObject() ) ;
         if ( SDB_OK != rc )
         {
            ossPrintf( "Error: Failed to parse BSONObj for BSON, "
                       "rc: %d"OSS_NEWLINE, rc ) ;
            goto error ;
         }

         // get meta LSN
         subElement = result.getField( STP_FIELD_NAME_META_LSN ) ;
         if ( Object != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_META_LSN ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            // parse meta LSN
            BSONObj lsnObject = subElement.embeddedObject() ;
            BSONElement lsnElement ;

            // get offset
            lsnElement = lsnObject.getField( STP_FIELD_NAME_META_OFFSET ) ;
            if ( NumberLong != lsnElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"
                          OSS_NEWLINE, STP_FIELD_NAME_META_OFFSET ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            metaLSN.offset = (DPS_LSN_OFFSET)( lsnElement.numberLong() ) ;

            // get version
            lsnElement = lsnObject.getField( STP_FIELD_NAME_META_VERSION ) ;
            if ( NumberInt != lsnElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"
                          OSS_NEWLINE, STP_FIELD_NAME_META_VERSION ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            metaLSN.version = (DPS_LSN_VER)( lsnElement.numberInt() ) ;
         }
      }
      catch ( exception &e )
      {
         ossPrintf( "Error: Failed to parse BSON for meta, "
                    "error: %s"OSS_NEWLINE, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // print meta
      rc = _stpqOutputMeta( shmKey, meta, metaLSN ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output meta, rc: %d"OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output servers
   static INT32 _stpqOutputServers( UINT32 version,
                                    const STP_SERVER_LIST &serverList,
                                    const stpServerNode &primaryNode )
   {
      ossPrintf( "Servers:"OSS_NEWLINE ) ;
      // print version
      ossPrintf( "   %-7s : %u"OSS_NEWLINE, STP_FIELD_NAME_VERSION, version ) ;
      // print servers
      for ( STP_SERVER_LIST::const_iterator iter = serverList.begin() ;
            iter != serverList.end() ;
            ++ iter )
      {
         // print host and service names
         ossPrintf( "   %-7s : %s:%s"OSS_NEWLINE, "Server",
                    iter->getHostName(), iter->getServiceName() ) ;
      }
      // print primary
      if ( 0 != ossStrcmp( STP_UNKNOWN_HOST_NAME,
                           primaryNode.getHostName() ) &&
           0 != ossStrcmp( STP_UNKNOWN_SERVICE_NAME,
                           primaryNode.getServiceName() ) )
      {
         // known primary
         ossPrintf( "   %-7s : %s:%s"OSS_NEWLINE, "Primary",
                    primaryNode.getHostName(), primaryNode.getServiceName() ) ;
      }
      else
      {
         // unknown primary
         ossPrintf( "   %-7s : %s"OSS_NEWLINE, "Primary",
                    STP_UNKNOWN_HOST_NAME ) ;
      }

      return SDB_OK ;
   }

   // get servers from STP
   static INT32 _stpqGetServers( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      BSONObj argument, result ;

      UINT32 version = STP_GROUP_INVALID_VERSION ;
      STP_SERVER_LIST serverList ;
      stpServerNode primaryNode ;

      // run get server command
      rc = _stpqRunCommand( client, _stpqGetTaskCommand( STPQ_TASK_GETSERVERS ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETSERVERS ), rc ) ;
         goto error ;
      }

      // parse meta from BSON
      try
      {
         BSONElement subElement ;

         // get version
         subElement = result.getField( STP_FIELD_NAME_VERSION ) ;
         if ( NumberInt != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_VERSION ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         version = (UINT32)( subElement.numberInt() ) ;

         // get server group
         subElement = result.getField( STP_FIELD_NAME_GROUP ) ;
         if ( Array != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_GROUP ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            // parse servers
            BSONObj serverInfo = subElement.embeddedObject() ;
            BSONObjIterator serverEleIter( serverInfo ) ;
            while ( serverEleIter.more() )
            {
               stpServerNode server ;
               BSONElement serverElement = serverEleIter.next() ;
               if ( Object != serverElement.type() )
               {
                  ossPrintf( "Error: Unknown format for server: %s"OSS_NEWLINE,
                             serverElement.toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               rc = server.fromBSON( serverElement.embeddedObject(), TRUE ) ;
               if ( SDB_OK != rc )
               {
                  ossPrintf( "Error: Failed to parse BSONObj for server, "
                             "rc: %d"OSS_NEWLINE, rc ) ;
                  goto error ;
               }
               serverList.push_back( server ) ;
            }
         }

         // parse primary
         subElement = result.getField( STP_FIELD_NAME_PRIMARY ) ;
         if ( Object != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_PRIMARY ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            BSONObj primaryObject = subElement.embeddedObject() ;
            BSONElement primaryElement ;

            // get host name
            primaryElement = primaryObject.getField( STP_FIELD_NAME_HOST ) ;
            if ( String != primaryElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                          STP_FIELD_NAME_HOST ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            primaryNode.setHostName( primaryElement.valuestr() ) ;

            // get service name
            primaryElement = primaryObject.getField( STP_FIELD_NAME_SERVICE ) ;
            if ( String != primaryElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                          STP_FIELD_NAME_SERVICE ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            primaryNode.setServiceName( primaryElement.valuestr() ) ;
         }
      }
      catch ( exception &e )
      {
         ossPrintf( "Error: Failed to parse BSON for servers, "
                    "error: %s"OSS_NEWLINE, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // print servers
      rc = _stpqOutputServers( version, serverList, primaryNode ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output servers, rc: %d"OSS_NEWLINE,
                    rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output synchronize clients
   static INT32 _stpqOutputSyncClients( const CHAR *sourceAddr,
                                        const STP_CLIENT_LIST &clientList )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != sourceAddr, "source address is invalid" ) ;

      // print source
      ossPrintf( "Synchronize Source: %s"OSS_NEWLINE, sourceAddr ) ;

      // print synchronize client in below fields
      // address: address of client
      // role: role of client, SERVER or CLIENT
      // port: port used to synchronize, default is 9622
      // status: synchronize status of client, IntervalCheck, etc.
      // timeError: current and maximum allowed time error
      //            ( in microseconds )
      // passed: microseconds after last synchronize
      ossPrintf( "Synchronize Clients:"OSS_NEWLINE ) ;

      if ( clientList.size() > 0 )
      {
         list< string > addressList, timeErrorList ;
         UINT64 addressLen = 0, roleLen = 0,portLen = 0,
                statusLen = 0, countLen = 0, intervalLen = 0,
                timeErrorLen = 0, passedLen = 0 ;

         // calculate maximum length to print each field for each client
         // fields should be align
         for ( STP_CLIENT_LIST::const_iterator clientIter = clientList.begin() ;
               clientIter != clientList.end() ;
               ++ clientIter )
         {
            // get address of client
            CHAR address[ OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 2 ] = { '\0' } ;
            const stpClientNode &client = ( *clientIter ) ;

            ossSnprintf( address, OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 1,
                         "%s:%s", client.getHostName(), client.getServiceName() ) ;

            // length of address
            addressLen = _stpqGetFormatLen( addressLen, "Client", address ) ;

            try
            {
               addressList.push_back( address ) ;
            }
            catch ( exception &e )
            {
               ossPrintf( "Error: Failed to add address list, "
                          "error: %s"OSS_NEWLINE, e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // length of role
            roleLen = _stpqGetFormatLen( roleLen, "Role",
                                         stpGetRoleName( client.getRole() ) ) ;

            // length of port
            portLen = _stpqGetFormatLen( portLen, "Port",
                                         (UINT64)( client.getSyncPort() ) ) ;

            // length of status
            statusLen = _stpqGetFormatLen( statusLen, "Status",
                                           stpGetSyncStatusName( client.getStatus() ) ) ;

            // length of count
            countLen = _stpqGetFormatLen( countLen, "Count", client.getSyncCount() ) ;

            // length of interval
            intervalLen = _stpqGetFormatLen( intervalLen, "Interval",
                                             (UINT64)( client.getSyncInterval() ) ) ;

            // get time error in format "( current / maximum )"
            stringstream timeErrorSS ;
            timeErrorSS << STP_NANOSEC_TO_MICROSEC( client.getTimeError() )
                        << "/"
                        << STP_NANOSEC_TO_MICROSEC( client.getMaxTimeError() ) ;
            string timeErrorOutput = timeErrorSS.str() ;
            // length of time error
            timeErrorLen = _stpqGetFormatLen( timeErrorLen, "TimeError",
                                              timeErrorOutput.c_str() ) ;

            try
            {
               timeErrorList.push_back( timeErrorOutput ) ;
            }
            catch ( exception &e )
            {
               ossPrintf( "Error: Failed to add time error list, "
                          "error: %s"OSS_NEWLINE, e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // length of passed
            passedLen = _stpqGetFormatLen( passedLen, "Passed",
                                           STP_MILLISEC_TO_MICROSEC( client.getLastSyncTick() ) ) ;
         }

         // header
         stringstream headerSS ;
         headerSS << "   "
                  << "%-" << addressLen << "s "
                  << "%-" << roleLen << "s "
                  << "%-" << portLen << "s "
                  << "%-" << statusLen << "s "
                  << "%-" << countLen << "s "
                  << "%-" << intervalLen << "s "
                  << "%-" << timeErrorLen << "s "
                  << "%-" << passedLen << "s "
                  << OSS_NEWLINE ;
         string headerOutput = headerSS.str() ;
         ossPrintf( headerOutput.c_str(), "Client", "Role", "Port",
                    "Status", "Count", "Interval", "TimeError", "Passed" ) ;

         // output fields
         stringstream outputSS ;
         outputSS << "   "
                  << "%-" << addressLen << "s "
                  << "%-" << roleLen << "s "
                  << "%-" << portLen << "llu "
                  << "%-" << statusLen << "s "
                  << "%-" << countLen << "llu "
                  << "%-" << intervalLen << "llu "
                  << "%-" << timeErrorLen << "s "
                  << "%-" << passedLen << "llu "
                  << OSS_NEWLINE ;
         string output = outputSS.str() ;

         // print each client
         STP_CLIENT_LIST::const_iterator clientIter ;
         list< string >::iterator addressIter, timeErrorIter ;

         for ( clientIter = clientList.begin(),
               addressIter = addressList.begin(),
               timeErrorIter = timeErrorList.begin() ;
               clientList.end() != clientIter &&
               addressList.end() != addressIter &&
               timeErrorList.end() != timeErrorIter ;
               ++ clientIter,
               ++ addressIter,
               ++ timeErrorIter )
         {
            const stpClientNode &client = ( *clientIter ) ;
            ossPrintf( output.c_str(),
                       addressIter->c_str(),
                       stpGetRoleName( client.getRole() ),
                       client.getSyncPort(),
                       stpGetSyncStatusName( client.getStatus() ),
                       client.getSyncCount(),
                       client.getSyncInterval(),
                       timeErrorIter->c_str(),
                       STP_MILLISEC_TO_MICROSEC( client.getLastSyncTick() ) ) ;
         }
      }
      else
      {
         // no client, print "-" for each field instead
         UINT32 addressLen = _stpqGetFormatLen( 0, "Client", "-" ) ;
         UINT32 roleLen = _stpqGetFormatLen( 0, "Role", "-" ) ;
         UINT32 portLen = _stpqGetFormatLen( 0, "Port", "-" ) ;
         UINT32 statusLen = _stpqGetFormatLen( 0, "Status", "-" ) ;
         UINT32 countLen = _stpqGetFormatLen( 0, "Count", "-" ) ;
         UINT32 intervalLen = _stpqGetFormatLen( 0, "Interval", "-" ) ;
         UINT32 timeErrorLen = _stpqGetFormatLen( 0, "TimeError", "-" ) ;
         UINT32 passedLen = _stpqGetFormatLen( 0, "Passed", "-" ) ;

         stringstream headerSS ;
         headerSS << "   "
                  << "%-" << addressLen << "s "
                  << "%-" << roleLen << "s "
                  << "%-" << portLen << "s "
                  << "%-" << statusLen << "s "
                  << "%-" << countLen << "s "
                  << "%-" << intervalLen << "s "
                  << "%-" << timeErrorLen << "s "
                  << "%-" << passedLen << "s "
                  << OSS_NEWLINE ;
         string headerOutput = headerSS.str() ;
         ossPrintf( headerOutput.c_str(), "Client", "Role", "Port",
                    "Status", "Count", "Interval", "TimeError", "Passed" ) ;
         ossPrintf( headerOutput.c_str(), "-", "-", "-", "-", "-", "-", "-",
                    "-" ) ;
      }

      // print total count
      ossPrintf( "   Total: %llu"OSS_NEWLINE,
                 (UINT64)( clientList.size() ) ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   // get synchronize clients from STP
   static INT32 _stpqGetSyncClients( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      BSONObj argument, result ;
      STP_CLIENT_LIST clientList ;
      CHAR sourceAddr[ OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 2 ] = { '\0' } ;


      // run get synchronize clients command
      rc = _stpqRunCommand( client,
                            _stpqGetTaskCommand( STPQ_TASK_GETSYNCCLIENTS ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETSYNCCLIENTS ), rc ) ;
         goto error ;
      }

      // parse clients from BSON
      try
      {
         BSONElement subElement ;

         subElement = result.getField( STP_FIELD_NAME_SYNC_SOURCE ) ;
         if ( Object != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_SYNC_SOURCE ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            BSONObj sourceObj = subElement.embeddedObject() ;
            BSONElement sourceElement ;
            const CHAR *hostName = NULL ;
            const CHAR *serviceName = NULL ;

            sourceElement = sourceObj.getField( STP_FIELD_NAME_HOST ) ;
            if ( String != sourceElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                          STP_FIELD_NAME_HOST ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            hostName = sourceElement.valuestr() ;

            sourceElement = sourceObj.getField( STP_FIELD_NAME_SERVICE ) ;
            if ( String != sourceElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                          STP_FIELD_NAME_SERVICE ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            serviceName = sourceElement.valuestr() ;

            ossSnprintf( sourceAddr, OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 1,
                         "%s:%s", hostName, serviceName ) ;
         }

         // get clients
         subElement = result.getField( STP_FIELD_NAME_SYNC_CLIENTS ) ;
         if ( Array != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_SYNC_CLIENTS ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         else
         {
            // parse clients
            BSONObj clientInfo = subElement.embeddedObject() ;
            BSONObjIterator clientEleIter( clientInfo ) ;
            while ( clientEleIter.more() )
            {
               stpClientNode client ;
               BSONElement clientElement = clientEleIter.next() ;
               if ( Object != clientElement.type() )
               {
                  ossPrintf( "Error: Unknown format for client: %s"OSS_NEWLINE,
                             clientElement.toString().c_str() ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }
               rc = client.fromBSON( clientElement.embeddedObject(), TRUE ) ;
               if ( SDB_OK != rc )
               {
                  ossPrintf( "Error: Failed to parse BSONObj for "
                             "synchronize client, rc: %d"OSS_NEWLINE, rc ) ;
                  goto error ;
               }
               clientList.push_back( client ) ;
            }
         }
      }
      catch ( exception &e )
      {
         ossPrintf( "Error: Failed to parse BSONObj, error: %s"OSS_NEWLINE,
                    e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // print synchronize clients
      rc = _stpqOutputSyncClients( sourceAddr, clientList ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output synchronize clients, "
                    "rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output synchronize status
   static INT32 _stpqOutputSyncStatus( const CHAR *roleName,
                                       const CHAR *statusName,
                                       BOOLEAN isPrimary,
                                       BOOLEAN hasSource,
                                       const stpSourceNode &source )
   {
      // print synchronize status in below fields
      // role: role of this STP node
      // primary: if this STP node is primary
      // status: synchronize status of this STP node, IntervalCheck etc.
      // source: address of source
      // count: counts of synchronize ( valid / total )
      // delay: delays of synchronize in microseconds
      //        formatted in ( minimum delay / maximum delay / last delay )
      // offset: offsets of synchronize in microseconds
      //         formatted in ( minimum negative offset ,
      //         maximum negative offset / minimum positive offset, maximum
      //         positive offset / last offset )
      // passed: microseconds after last synchronize

      // print synchronize history in below fields
      // requestID: request ID of this synchronize request
      // valid: if this synchronize result is valid ( not delay too much )
      // status: synchronize status for this synchronize request
      // delay: delay of this synchronize request in microseconds
      // offset: offset of this synchronize request in microseconds
      // passed: microseconds after this synchronize request

      // print header
      ossPrintf( "Synchronize Status:"OSS_NEWLINE ) ;

      if ( isPrimary || !hasSource )
      {
         // for primary or not synchronized, print "-" for each fields
         stringstream ss ;
         ss << "   "
            << "%-" << _stpqGetFormatLen( 0, "Role", roleName ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Primary",
                                          isPrimary ? "TRUE" : "FALSE" ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Status", statusName ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Source", "-" ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Count", "-" ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Delay", "-" ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Offset", "-" ) << "s "
            << "%-" << _stpqGetFormatLen( 0, "Passed", "-" ) << "s "
            << OSS_NEWLINE ;
         string output = ss.str() ;

         ossPrintf( output.c_str(), "Role", "Primary", "Status", "Source",
                    "Count", "Delay", "Offset", "Passed" ) ;

         ossPrintf( output.c_str(), roleName, isPrimary ? "TRUE" : "FALSE",
                    statusName, "-", "-", "-", "-", "-", "-", "-", "-", "-",
                    "-", "-", "-" ) ;
      }
      else
      {
         CHAR address[ OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 2 ] = { '\0' } ;
         const stpSyncStats &curStats = source.getCurStats() ;
         const STP_SYNC_REC_LIST &histList = curStats.getHistList() ;

         // format address
         ossSnprintf( address, OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 1,
                      "%s:%s", source.getHostName(),
                      source.getServiceName() ) ;

         // format counts
         stringstream countSS ;
         countSS << curStats.getValidCount() << "/"
                 << curStats.getSyncCount() ;
         string countOutput = countSS.str() ;

         // format delays
         stringstream delaySS ;
         delaySS << STP_NANOSEC_TO_MICROSEC( curStats.getMinDelay() ) << "/"
                 << STP_NANOSEC_TO_MICROSEC( curStats.getMaxDelay() ) << "/"
                 << STP_NANOSEC_TO_MICROSEC( curStats.getLastDelay() ) ;
         string delayOutput = delaySS.str() ;

         // format offsets
         stringstream offsetSS ;
         offsetSS << "["
                  << STP_NANOOFFSET_TO_MICROOFFSET( curStats.getMinNegOffset() ) << ","
                  << STP_NANOOFFSET_TO_MICROOFFSET( curStats.getMaxNegOffset() )
                  << "]/["
                  << STP_NANOOFFSET_TO_MICROOFFSET( curStats.getMinPosOffset() ) << ","
                  << STP_NANOOFFSET_TO_MICROOFFSET( curStats.getMaxPosOffset() )
                  << "]/"
                  << STP_NANOOFFSET_TO_MICROOFFSET( curStats.getLastOffset() ) ;
         string offsetOutput = offsetSS.str() ;

         // format header
         stringstream headerSS ;
         headerSS << "   "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Role", roleName )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Primary", "FALSE" )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Status", statusName )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Source", address )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Count", countOutput.c_str() )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Delay", delayOutput.c_str() )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen( 0, "Offset", offsetOutput.c_str() )
                  << "s "
                  << "%-"
                  << _stpqGetFormatLen(
                              0, "Passed",
                              STP_MILLISEC_TO_MICROSEC(
                                          curStats.getUpdateTick() ) )
                  << "s "
                  << OSS_NEWLINE ;

         string headerOutput = headerSS.str() ;

         // format source
         stringstream ss ;
         ss << "   "
            << "%-"
            << _stpqGetFormatLen( 0, "Role", roleName )
            << "s "
            << "%-"
            << _stpqGetFormatLen( 0, "Primary", "FALSE" )
            << "s "
            << "%-"
            << _stpqGetFormatLen( 0, "Status", statusName )
            << "s "
            << "%-"
            << _stpqGetFormatLen( 0, "Source", address )
            << "s "
            << "%-"
            << _stpqGetFormatLen( 0, "Count", countOutput.c_str() )
            << "s "
            << "%-"
            << _stpqGetFormatLen( 0, "Delay", delayOutput.c_str() )
            << "s "
            << "%-"
            << _stpqGetFormatLen( 0, "Offset", offsetOutput.c_str() )
            << "s "
            << "%-"
            << _stpqGetFormatLen(
                        0, "Passed", STP_MILLISEC_TO_MICROSEC(
                                    curStats.getUpdateTick() ) )
            << "llu "
            << OSS_NEWLINE ;

         string output = ss.str() ;

         // print header
         ossPrintf( headerOutput.c_str(), "Role", "Primary", "Status", "Source",
                    "Count", "Delay", "Offset", "Passed" ) ;

         // print source
         ossPrintf( output.c_str(), roleName, "FALSE", statusName,
                    address, countOutput.c_str(), delayOutput.c_str(),
                    offsetOutput.c_str(),
                    STP_MILLISEC_TO_MICROSEC( curStats.getUpdateTick() ) ) ;

         // print history if has
         if ( histList.size() > 0 )
         {
            // calculate maximum lenght of each fields
            // fields should be align
            UINT32 reqIDLen = 0, validLen = 0, statusLen = 0,
                   delayLen = 0, offsetLen = 0, passedLen = 0 ;
            for ( STP_SYNC_REC_LIST::const_reverse_iterator iter = histList.rbegin() ;
                  histList.rend() != iter ;
                  ++ iter )
            {
               const stpSyncRecord &record = ( *iter ) ;
               reqIDLen = _stpqGetFormatLen( reqIDLen, "RequestID",
                                             record.getRequestID() ) ;
               validLen = _stpqGetFormatLen( validLen, "Valid",
                                             record.isValid() ? "TRUE" : "FALSE" ) ;
               statusLen = _stpqGetFormatLen( statusLen, "Status",
                                              stpGetSyncStatusName( record.getStatus() ) ) ;
               delayLen = _stpqGetFormatLen( delayLen, "Delay",
                                             STP_NANOOFFSET_TO_MICROOFFSET( record.getDelay() ) ) ;
               offsetLen = _stpqGetFormatLen( offsetLen, "Offset",
                                              STP_NANOOFFSET_TO_MICROOFFSET( record.getOffset() ) ) ;
               passedLen = _stpqGetFormatLen( passedLen, "Passed",
                                              STP_MILLISEC_TO_MICROSEC( record.getSyncTick() ) ) ;
            }

            // format header
            stringstream histHeaderSS ;
            histHeaderSS << "   "
                         << "%-" << reqIDLen << "s "
                         << "%-" << validLen << "s "
                         << "%-" << statusLen << "s "
                         << "%-" << delayLen << "s "
                         << "%-" << offsetLen << "s "
                         << "%-" << passedLen << "s "
                         << OSS_NEWLINE ;

            string histHeaderOutput = histHeaderSS.str() ;

            // format output
            stringstream histOutputSS ;
            histOutputSS << "   "
                         << "%-" << reqIDLen << "llu "
                         << "%-" << validLen << "s "
                         << "%-" << statusLen << "s "
                         << "%-" << delayLen << "lld "
                         << "%-" << offsetLen << "lld "
                         << "%-" << passedLen << "llu "
                         << OSS_NEWLINE ;
            string histOutput = histOutputSS.str() ;

            // print header
            ossPrintf( "Synchronize history:"OSS_NEWLINE ) ;
            ossPrintf( histHeaderOutput.c_str(), "RequestID", "Valid",
                       "Status", "Delay", "Offset", "Passed" ) ;

            // print each history record
            for ( STP_SYNC_REC_LIST::const_iterator iter = histList.begin() ;
                  histList.end() != iter ;
                  ++ iter )
            {
               const stpSyncRecord &record = ( *iter ) ;
               ossPrintf( histOutput.c_str(), record.getRequestID(),
                          record.isValid() ? "TRUE" : "FALSE",
                          stpGetSyncStatusName( record.getStatus() ),
                          STP_NANOOFFSET_TO_MICROOFFSET( record.getDelay() ),
                          STP_NANOOFFSET_TO_MICROOFFSET( record.getOffset() ),
                          STP_MILLISEC_TO_MICROSEC( record.getSyncTick() ) ) ;
            }
         }
      }

      return SDB_OK ;
   }

   // get synchronize status from STP
   static INT32 _stpqGetSyncStatus( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      const CHAR *roleName = STP_ROLE_MANE_UNKNOWN ;
      const CHAR *statusName = STP_SYNC_STATUS_NAME_UNKNOWN ;
      BOOLEAN isPrimary = FALSE, hasSource = FALSE ;
      stpSourceNode source ;

      BSONObj argument, result ;

      // run get synchronize clients command
      rc = _stpqRunCommand( client,
                            _stpqGetTaskCommand( STPQ_TASK_GETSYNCSTATUS ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETSYNCSTATUS ), rc ) ;
         goto error ;
      }

      // parse source from BSON
      try
      {
         BSONElement subElement ;

         // parse role
         subElement = result.getField( STP_FIELD_NAME_ROLE ) ;
         if ( String != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_ROLE ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         roleName = subElement.valuestr() ;

         // parse primary
         subElement = result.getField( STP_FIELD_NAME_IS_PRIMARY ) ;
         if ( Bool != subElement.type() )
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_IS_PRIMARY ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         isPrimary = subElement.boolean() ;

         if ( isPrimary )
         {
            statusName = "-" ;
         }
         else
         {
            BSONObj subObject ;

            subElement = result.getField( STP_FIELD_NAME_SYNC_STATUS ) ;
            if ( String != subElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                          STP_FIELD_NAME_SYNC_STATUS ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            statusName = subElement.valuestr() ;

            // get source
            subElement = result.getField( STP_FIELD_NAME_SYNC_SOURCE ) ;
            if ( Object != subElement.type() )
            {
               ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                          STP_FIELD_NAME_SYNC_SOURCE ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            subObject = subElement.embeddedObject() ;
            if ( subObject.nFields() > 0 )
            {
               rc = source.fromBSON( subElement.embeddedObject(), TRUE, TRUE ) ;
               if ( SDB_OK != rc )
               {
                  ossPrintf( "Error: Failed to parse BSONObj for "
                             "synchronize source, rc: %d"OSS_NEWLINE, rc ) ;
                  goto error ;
               }

               hasSource = TRUE ;
            }
         }
      }
      catch ( exception &e )
      {
         ossPrintf( "Error: Failed to parse BSONObj, error: %s"OSS_NEWLINE,
                    e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // print synchronize status
      rc = _stpqOutputSyncStatus( roleName, statusName, isPrimary,
                                  hasSource, source ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output synchronize status, "
                    "rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   // output synchronize history
   static INT32 _stpqOutputSyncHistory( const STP_SOURCE_LIST &sourceList )
   {
      INT32 rc = SDB_OK ;

      // print synchronize history in below fields
      // source: address of source
      // count: counts of synchronize ( valid / total )
      // delay: delays of synchronize in microseconds
      //        formatted in ( minimum delay / maximum delay / last delay )
      // offset: offsets of synchronize in microseconds
      //         formatted in ( minimum negative offset ,
      //         maximum negative offset / minimum positive offset, maximum
      //         positive offset / last offset )
      // passed: microseconds after last synchronize

      // print header
      ossPrintf( "Synchronize History:"OSS_NEWLINE ) ;

      if ( sourceList.size() > 0 )
      {
         // calculate maximum length of each fields
         // fields should be align
         UINT32 addressLen = 0, countLen = 0, delayLen = 0,
                offsetLen = 0, passedLen = 0 ;
         list< string > addressList, countList, delayList, offsetList ;

         // format fields
         for ( STP_SOURCE_LIST::const_iterator iter = sourceList.begin() ;
               sourceList.end() != iter ;
               ++ iter )
         {
            CHAR address[ OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 2 ] = { '\0' } ;
            const stpSourceNode &source = ( *iter ) ;
            const stpSyncStats &histStats = source.getHistStats() ;

            // format address
            ossSnprintf( address, OSS_MAX_HOSTNAME + OSS_MAX_SERVICENAME + 1,
                         "%s:%s", source.getHostName(),
                         source.getServiceName() ) ;

            // get length of address
            addressLen = _stpqGetFormatLen( addressLen, "Source", address ) ;

            try
            {
               addressList.push_back( address ) ;
            }
            catch ( exception &e )
            {
               ossPrintf( "Error: Failed to add address list, "
                          "error: %s"OSS_NEWLINE, e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // format counts
            stringstream countSS ;
            countSS << histStats.getValidCount() << "/"
                    << histStats.getSyncCount() ;
            string countOutput = countSS.str() ;
            // get length of count
            countLen = _stpqGetFormatLen( countLen, "Count",
                                          countOutput.c_str() ) ;

            try
            {
               countList.push_back( countOutput ) ;
            }
            catch ( exception &e )
            {
               ossPrintf( "Error: Failed to add count list, "
                          "error: %s"OSS_NEWLINE, e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // format delay
            stringstream delaySS ;
            delaySS << STP_NANOSEC_TO_MICROSEC( histStats.getMinDelay() ) << "/"
                    << STP_NANOSEC_TO_MICROSEC( histStats.getMaxDelay() ) << "/"
                    << STP_NANOSEC_TO_MICROSEC( histStats.getLastDelay() ) ;
            string delayOutput = delaySS.str() ;
            // get length of delay
            delayLen = _stpqGetFormatLen( delayLen, "Delay",
                                          delayOutput.c_str() ) ;

            try
            {
               delayList.push_back( delayOutput ) ;
            }
            catch ( exception &e )
            {
               ossPrintf( "Error: Failed to add delay list, "
                          "error: %s"OSS_NEWLINE, e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // format offset
            stringstream offsetSS ;
            offsetSS << "["
                     << STP_NANOOFFSET_TO_MICROOFFSET( histStats.getMinNegOffset() ) << ","
                     << STP_NANOOFFSET_TO_MICROOFFSET( histStats.getMaxNegOffset() )
                     << "]/["
                     << STP_NANOOFFSET_TO_MICROOFFSET( histStats.getMinPosOffset() ) << ","
                     << STP_NANOOFFSET_TO_MICROOFFSET( histStats.getMaxPosOffset() )
                     << "]/"
                     << STP_NANOOFFSET_TO_MICROOFFSET( histStats.getLastOffset() ) ;
            string offsetOutput = offsetSS.str() ;

            // get length of offset
            offsetLen = _stpqGetFormatLen( offsetLen, "Offset",
                                           offsetOutput.c_str() ) ;

            try
            {
               offsetList.push_back( offsetOutput ) ;
            }
            catch ( exception &e )
            {
               ossPrintf( "Error: Failed to add offset list, "
                          "error: %s"OSS_NEWLINE, e.what() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            // get length of passed
            passedLen = _stpqGetFormatLen( passedLen, "Passed",
                                           STP_MILLISEC_TO_MICROSEC( histStats.getUpdateTick() ) ) ;
         }

         // format header
         stringstream headerSS ;
         headerSS << "   "
                  << "%-" << addressLen << "s "
                  << "%-" << countLen << "s "
                  << "%-" << delayLen << "s "
                  << "%-" << offsetLen << "s "
                  << "%-" << passedLen << "s "
                  << OSS_NEWLINE ;
         string headerOutput = headerSS.str() ;

         // print header
         ossPrintf( headerOutput.c_str(), "Source", "Count", "Delay", "Offset",
                    "Passed" ) ;

         // format output
         stringstream outputSS ;
         outputSS << "   "
                  << "%-" << addressLen << "s "
                  << "%-" << countLen << "s "
                  << "%-" << delayLen << "s "
                  << "%-" << offsetLen << "s "
                  << "%-" << passedLen << "llu "
                  << OSS_NEWLINE ;
         string output = outputSS.str() ;

         // print each synchronize history source
         STP_SOURCE_LIST::const_iterator sourceIter ;
         list< string >::iterator addressIter, countIter, delayIter,
                                  offsetIter ;

         for ( sourceIter = sourceList.begin(),
               addressIter = addressList.begin(),
               countIter = countList.begin(),
               delayIter = delayList.begin(),
               offsetIter = offsetList.begin() ;
               sourceList.end() != sourceIter &&
               addressList.end() != addressIter &&
               countList.end() != countIter &&
               delayList.end() != delayIter &&
               offsetList.end() != offsetIter ;
               ++ sourceIter,
               ++ addressIter,
               ++ countIter,
               ++ delayIter,
               ++ offsetIter )
         {
            const stpSyncStats &histStats = sourceIter->getHistStats() ;
            ossPrintf( output.c_str(),
                       addressIter->c_str(),
                       countIter->c_str(),
                       delayIter->c_str(),
                       offsetIter->c_str(),
                       STP_MILLISEC_TO_MICROSEC( histStats.getUpdateTick() ) ) ;
         }
      }
      else
      {
         UINT32 addressLen = _stpqGetFormatLen( 0, "Source", "-" ) ;
         UINT32 countLen = _stpqGetFormatLen( 0, "Count", "-" ) ;
         UINT32 delayLen = _stpqGetFormatLen( 0, "Delay", "-" ) ;
         UINT32 offsetLen = _stpqGetFormatLen( 0, "Offset", "-" ) ;
         UINT32 passedLen = _stpqGetFormatLen( 0, "Passed", "-" ) ;

         stringstream headerSS ;
         headerSS << "   "
                  << "%-" << addressLen << "s "
                  << "%-" << countLen << "s "
                  << "%-" << delayLen << "s "
                  << "%-" << offsetLen << "s "
                  << "%-" << passedLen << "s "
                  << OSS_NEWLINE ;
         string headerOutput = headerSS.str() ;
         ossPrintf( headerOutput.c_str(), "Source", "Count", "Delay", "Offset",
                    "Passed" ) ;
         ossPrintf( headerOutput.c_str(), "-", "-", "-", "-", "-" ) ;
      }

      // print total count
      ossPrintf( "   Total: %llu"OSS_NEWLINE,
                 (UINT64)( sourceList.size() ) ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   // get synchronize history from STP
   static INT32 _stpqGetSyncHistory( stpClient &client )
   {
      INT32 rc = SDB_OK ;

      STP_SOURCE_LIST sourceList ;
      BSONObj argument, result ;

      // run get synchronize clients command
      rc = _stpqRunCommand( client,
                            _stpqGetTaskCommand( STPQ_TASK_GETSYNCHISTORY ),
                            argument, result ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to execute [%s] command, rc: %d"OSS_NEWLINE,
                    _stpqGetTaskCommand( STPQ_TASK_GETSYNCHISTORY ), rc ) ;
         goto error ;
      }

      // parse source from BSON
      try
      {
         BSONElement element ;

         element = result.getField( STP_FIELD_NAME_SYNC_SOURCES ) ;
         if ( Array == element.type() )
         {
            BSONObjIterator iter( element.embeddedObject() ) ;
            while ( iter.more() )
            {
               stpSourceNode source ;
               string address ;
               BSONElement subElement = iter.next() ;

               if ( Object != subElement.type() )
               {
                  ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                             STP_FIELD_NAME_SYNC_SOURCES ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               rc = source.fromBSON( subElement.embeddedObject(), FALSE, TRUE ) ;
               if ( SDB_OK != rc )
               {
                  ossPrintf( "Error: Failed to parse BSONObj for "
                             "synchronize source, rc: %d"OSS_NEWLINE, rc ) ;
                  goto error ;
               }

               address = source.getHostName() ;
               address += ":" ;
               address += source.getServiceName() ;

               sourceList.push_back( source ) ;
            }
         }
         else
         {
            ossPrintf( "Error: Failed to get [%s] field from BSON"OSS_NEWLINE,
                       STP_FIELD_NAME_SYNC_SOURCE ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      catch ( exception &e )
      {
         ossPrintf( "Error: Failed to parse BSONObj, error: %s"OSS_NEWLINE,
                    e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // output synchronize history
      rc = _stpqOutputSyncHistory( sourceList ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to output synchronize history, "
                    "rc: %d"OSS_NEWLINE, rc ) ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   static INT32 mainEntry ( INT32 argc, CHAR **argv )
   {
      INT32 rc = SDB_OK ;

      po::options_description desc( "Command options" ) ;
      po::options_description all( "Command options" ) ;
      po::variables_map vm ;

      string hostName, serviceName ;
      list<STPQ_TASK_TYPE> taskList ;
      UINT32 count = STPQ_DFT_COUNT, delay = STPQ_DFT_DELAY_SEC ;
      UINT32 iterateCount = 0 ;

      stpClient client ;

      // resolve arguments
      _stpqInitArgument( desc, all ) ;
      rc = _stpqResolveArgument( desc, all, vm, argc, argv,
                                 hostName, serviceName, taskList,
                                 count, delay ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_PMD_HELP_ONLY != rc && SDB_PMD_VERSION_ONLY != rc )
         {
            ossPrintf( "Error: Invalid argument: %d"OSS_NEWLINE, rc ) ;
            _stpqDisplayArgument( desc ) ;
         }
         else
         {
            rc = SDB_OK ;
         }
         goto done ;
      }

      // connect to STP
      rc = client.connect( hostName.c_str(), serviceName.c_str() ) ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Error: Failed to connect to STP %s:%s, rc: %d"OSS_NEWLINE,
                    hostName.c_str(), serviceName.c_str(), rc ) ;
         goto error ;
      }

      // run commands
      while ( 0 == count || iterateCount < count )
      {
         for ( list<STPQ_TASK_TYPE>::iterator iter = taskList.begin() ;
               iter != taskList.end() ;
               ++ iter )
         {
            STPQ_TASK_TYPE taskType = (*iter) ;

            switch ( taskType )
            {
               case STPQ_TASK_GETTIME :
               {
                  // get time
                  rc = _stpqGetTime( client ) ;
                  break ;
               }
               case STPQ_TASK_GETTIMEUS :
               {
                  // get time in microsecond
                  rc = _stpqGetTimeUS( client ) ;
                  break ;
               }
               case STPQ_TASK_GETCONF :
               {
                  // get config
                  rc = _stpqGetConf( client, FALSE ) ;
                  break ;
               }
               case STPQ_TASK_GETCONFFULL :
               {
                  // get full config
                  rc = _stpqGetConf( client, TRUE ) ;
                  break ;
               }
               case STPQ_TASK_GETMETA :
               {
                  // get meta
                  rc = _stpqGetMeta( client ) ;
                  break ;
               }
               case STPQ_TASK_GETSERVERS :
               {
                  // get servers
                  rc = _stpqGetServers( client ) ;
                  break ;
               }
               case STPQ_TASK_GETSYNCCLIENTS :
               {
                  // get synchronize clients
                  rc = _stpqGetSyncClients( client ) ;
                  break ;
               }
               case STPQ_TASK_GETSYNCSTATUS :
               {
                  // get synchronize status
                  rc = _stpqGetSyncStatus( client ) ;
                  break ;
               }
               case STPQ_TASK_GETSYNCHISTORY :
               {
                  // get synchronize history
                  rc = _stpqGetSyncHistory( client ) ;
                  break ;
               }
               default:
               {
                  ossPrintf( "Error: Failed to run unknown command" ) ;
                  rc = SDB_SYS ;
               }
            }

            if ( SDB_OK != rc )
            {
               // command calls will print error message itself,
               // rno need to print duplicated error message here
               goto error ;
            }
            ossPrintf( OSS_NEWLINE ) ;
         }

         ++ iterateCount ;

         // take a break if needed
         // - zero means not stop
         // - > 1 means print output in multiple iterations
         if ( ( count > 1 && iterateCount < count ) ||
              0 == count )
         {
            ossPrintf( OSS_NEWLINE ) ;
            ossSleep( OSS_ONE_SEC * delay ) ;
         }
      }

   done:
      client.disconnect() ;

      if ( SDB_PMD_HELP_ONLY == rc ||
           SDB_PMD_VERSION_ONLY == rc )
      {
         return 0 ;
      }
      return ( SDB_OK != rc ? SDB_SRC_INVALIDARG : 1 ) ;

   error:
      goto done ;
   }
}

INT32 main ( INT32 argc, CHAR **argv )
{
   return engine::mainEntry( argc, argv ) ;
}
