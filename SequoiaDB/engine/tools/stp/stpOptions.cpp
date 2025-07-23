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

   Source File Name = stpOptions.cpp

   Descriptive Name = Serial Time Protocol

   When/how to use: this program may be used on binary and text-formatted
   versions of STP component. This file contains structure for Serial Time
   Protocol.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/
#include "stpOptions.hpp"
#include "ossVer.h"
#include "pmdEnv.hpp"
#include "stpToolUtil.hpp"

namespace po = boost::program_options ;

using namespace std ;

namespace engine
{

   // default sharing break time in 7 seconds
   #define STP_OPTION_BREAKTIME_DFT          ( 7000 )
   // default start shift time in 600 seconds
   #define STP_OPTION_STARTSHIFTTIME_DFT     ( 600 )
   // default value for maximum synchronize ports is 1
   #define STP_OPTION_MAXSYNCPORTS_DFT       ( 1 )
   // default value for default synchronize clients per port is 10
   #define STP_OPTION_DEFCLIENTSPERPORT_DFT  ( 10 )

   // minimum value of maxtimeerror option ( 1000 microseconds )
   #define STP_OPTION_MAXTIMEERROR_MIN       ( STP_MIN_TIME_ERROR_US )
   // maximum value for maxtimeerror option, 10 seconds in microseconds
   #define STP_OPTION_MAXTIMEERROR_MAX       ( 10000000 )
   // default value for maxtimeerror option ( 50000 microseconds )
   #define STP_OPTION_MAXTIEMERROR_DEF       ( STP_MAX_TIME_ERROR_US )

   #define FILE_OPTIONS \
      ( STP_OPTION_PORT, \
            po::value<string>(), \
            "STP listening port, default is 9622" ) \
      ( STP_OPTION_SERVERLIST, \
            po::value<string>(), \
            "STP server list, if not specified, " \
            "will use host name of this machine" ) \
      ( STP_OPTION_ROLE, \
            po::value<string>(), \
            "STP role, default is \"server\"" ) \
      ( STP_OPTION_WEIGHT, \
            po::value<INT32>(), \
            "STP vote weight, default is 0" ) \
      ( STP_OPTION_SYNCINTERVAL, \
            po::value<INT32>(), \
            "STP synchronize interval in seconds, default is 60" ) \
      ( STP_OPTION_MAXTIMEERROR, \
            po::value<INT32>(), \
            "STP max time error in microseconds, default is 50000, " \
            "value range is [ 1000, 10000000 ]" ) \
      ( STP_OPTION_MAXSYNCHIST, \
            po::value<INT32>(), \
            "STP save history records of synchronize for statistics, " \
            "default is 20, range is [ 0, 200 ]" ) \
      ( STP_OPTION_MAXSYNCPORTS, \
            po::value<INT32>(), \
            "maximum UDP ports used to synchronize time, default is 1, " \
            "means only use default port to synchronize, the extra ports " \
            "will start from <port> + 1, maximum is 128" ) \
      ( STP_OPTION_DEFCLIENTSPERPORT, \
            po::value<INT32>(), \
            "default synchronize clients could be assigned to a " \
            "synchronize UDP port, default is 10" ) \
      ( STP_OPTION_PREOPENPORTS, \
            "indicates whether to open all synchronize UDP ports during " \
            "start of STP node, default is false" ) \
      ( STP_OPTION_SYNCWITHSYSPORT, \
            "indicates whether to allow synchronize only on system port, " \
            "default is true" ) \
      ( STP_OPTION_DIAGLEVEL, \
            po::value<INT32>(), \
            "STP dialog level, default is 3" ) \
      ( STP_OPTION_SHARINGBRK, \
            po::value<INT32>(), \
            "the timeout period for heartbeat in each replica group " \
            "( in ms ), default is 7000, value range is [ 5000, 300000 ]" ) \
      ( STP_OPTION_STARTSHIFTTIME, \
            po::value<INT32>(), \
            "nodes starting shift time ( in seconds ), " \
            "default is 600, value range is [ 0, 7200 ]" ) \
      ( STP_OPTION_TESTMODE, \
            "start STP in test mode" )

   #define COMMANDS_OPTIONS \
      ( PMD_COMMANDS_STRING( STP_OPTION_PORT, ",p" ), \
            po::value<string>(), \
            "STP listening port, default is 9622" ) \
      ( STP_OPTION_SERVERLIST, \
            po::value<string>(), \
            "STP server list, if not specified, " \
            "will use host name of this machine" ) \
      ( STP_OPTION_ROLE, \
            po::value<string>(), \
            "STP role, default is server" ) \
      ( STP_OPTION_SYNCINTERVAL, \
            po::value<INT32>(), \
            "STP synchronize interval in seconds, default is 60" ) \
      ( STP_OPTION_MAXTIMEERROR, \
            po::value<INT32>(), \
            "STP max time error in microseconds, default is 50000, " \
            "value range is [ 1000, 10000000 ]" ) \
      ( STP_OPTION_DIAGLEVEL, \
            po::value<INT32>(), \
            "STP dialog level, default is 3" ) \
      ( STP_OPTION_DAEMON, \
            "Start STP in daemon mode" ) \
      ( PMD_COMMANDS_STRING( STP_OPTION_HELP, ",h" ), \
            "help" ) \
      ( STP_OPTION_VERSION, \
            "version" ) \
      ( PMD_COMMANDS_STRING( STP_OPTION_CONFPATH, ",c" ), \
            po::value<string>(), \
            "STP configuration file path" )

   #define COMMANDS_HIDE_OPTIONS \
      ( STP_OPTION_HELPFULL, \
            "help all configs" ) \
      ( STP_OPTION_TESTMODE, \
            "start STP in test mode" ) \
      ( STP_OPTION_MAXSYNCHIST, \
            po::value<INT32>(), \
            "STP save history records of synchronize for statistics, " \
            "default is 20, range is [ 0, 200 ]" ) \
      ( STP_OPTION_MAXSYNCPORTS, \
            po::value<INT32>(), \
            "maximum UDP ports used to synchronize time, default is 1, " \
            "means only use default port to synchronize, the extra ports " \
            "will start from <port> + 1, maximum is 128" ) \
      ( STP_OPTION_DEFCLIENTSPERPORT, \
            po::value<INT32>(), \
            "default synchronize clients could be assigned to a " \
            "synchronize UDP port, default is 10" ) \
      ( STP_OPTION_PREOPENPORTS, \
            "indicates whether to open all synchronize UDP ports during " \
            "start of STP node, default is false" ) \
      ( STP_OPTION_SYNCWITHSYSPORT, \
            "indicates whether to allow synchronize only on system port, " \
            "default is true" ) \
      ( STP_OPTION_WEIGHT, \
            po::value<INT32>(), \
            "STP vote weight, default is 0" ) \
      ( STP_OPTION_SHARINGBRK, \
            po::value<INT32>(), \
            "the timeout period for heartbeat in each replica group " \
            "( in ms ), default is 7000, value range is [ 5000, 300000 ]" ) \
      ( STP_OPTION_STARTSHIFTTIME, \
            po::value<INT32>(), \
            "nodes starting shift time ( in seconds ), " \
            "default is 600, value range is [ 0, 7200 ]" ) \
      ( STP_OPTION_CURUSER, \
            "use current user to start STP node" )

   /*
      _tpOptions implement
    */
   _stpOptions::_stpOptions()
   : _weight( 0 ),
     _syncInterval( STP_DEF_SYNC_INTERVAL ),
     _maxTimeErrorUS( STP_OPTION_MAXTIEMERROR_DEF ),
     _maxSyncHist( STP_DEF_SYNC_HIST_SIZE ),
     _maxSyncPorts( STP_OPTION_MAXSYNCPORTS_DFT ),
     _defClientsPerPort( STP_OPTION_DEFCLIENTSPERPORT_DFT ),
     _preOpenPorts( FALSE ),
     _syncWithSysPort( TRUE ),
     _diagLevel( PDWARNING ),
     _sharingBreakTime( STP_OPTION_BREAKTIME_DFT ),
     _startShiftTime( STP_OPTION_STARTSHIFTTIME_DFT ),
     _port( STP_DEF_PORT ),
     _role( STP_ROLE_SERVER ),
     _testMode( FALSE )
   {
      _cfgFileName[ 0 ] = '\0' ;
      _stpPath[ 0 ] = '\0' ;
      _serverListString[ 0 ] = '\0' ;
      ossSnprintf( _roleString, PMD_MAX_SHORT_STR_LEN, STP_ROLE_NAME_CLIENT ) ;
      ossSnprintf( _serviceName, OSS_MAX_SERVICENAME, "%d", _port ) ;
   }

   _stpOptions::~_stpOptions()
   {
   }

   INT32 _stpOptions::initialize( INT32 argc,
                                  CHAR **argv,
                                  const CHAR *rootPath,
                                  BOOLEAN &daemonMode )
   {
      INT32 rc = SDB_OK ;

      po::options_description desc( "Command options" ) ;
      po::variables_map vmFile, vmCommand ;

      // initialize arguments
      rc = _initArguments( argc, argv, vmCommand ) ;
      if ( SDB_PMD_HELP_ONLY == rc ||
           SDB_PMD_VERSION_ONLY == rc )
      {
         goto done ;
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }

      /// change user
      if ( !vmCommand.count( STP_OPTION_CURUSER ) )
      {
         UTIL_CHECK_AND_CHG_USER() ;
      }

      if ( vmCommand.count( STP_OPTION_CONFPATH ) )
      {
         // if config path is given, check if exists
         if ( NULL == ossGetRealPath(
                     vmCommand[ STP_OPTION_CONFPATH ].as<string>().c_str(),
                     _stpPath, OSS_MAX_PATHSIZE ) )
         {
            cerr << "ERROR: Failed to get real path for " <<
                    vmCommand[ STP_OPTION_CONFPATH ].as<string>().c_str() <<
                    endl ;
            rc = SDB_INVALIDPATH ;
            goto error;
         }
      }
      else
      {
         // if not given, construct config path from root path
         PD_CHECK( NULL != rootPath, SDB_INVALIDARG, error, PDERROR,
                   "Root path is empty" ) ;

         // build 'conf' file path
         rc = utilBuildFullPath( rootPath, STP_ROOT_PATH, OSS_MAX_PATHSIZE,
                                 _stpPath ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build local path for root "
                      "path %s, rc: %d", rootPath, rc ) ;
      }

      if ( vmCommand.count( STP_OPTION_DAEMON ) )
      {
         string options ;

         // remove options should no saved into config file
         vmCommand.erase( STP_OPTION_CONFPATH ) ;
         vmCommand.erase( STP_OPTION_DAEMON ) ;

         rc = _toCommandLine( vmCommand, options ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to generate command line for "
                      "daemon mode, rc: %d", rc ) ;

         rc = stpStartNode( rootPath, _stpPath, options ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to start STP node, rc: %d", rc ) ;

         daemonMode = TRUE ;

         goto done ;
      }

      // build stp config file path
      rc = utilBuildFullPath( _stpPath, STP_CFG_FILE_NAME,
                              OSS_MAX_PATHSIZE, _cfgFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build config path for root "
                   "path %s, rc: %d", rootPath, rc ) ;

      // initialize options
      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         FILE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      // read config from file
      rc = utilReadConfigureFile( _cfgFileName, desc, vmFile ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc )
         {
            // file or dir not exist
            PD_LOG( PDWARNING, "Failed to read missing configurations [%s], "
                    "use default configurations", _cfgFileName ) ;
         }
      }

      // remove options should no saved into config file
      vmCommand.erase( STP_OPTION_CONFPATH ) ;
      vmCommand.erase( STP_OPTION_DAEMON ) ;
      vmCommand.erase( STP_OPTION_CURUSER ) ;

      // initialize config record
      rc = pmdCfgRecord::init( &vmFile, &vmCommand ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize configurations, rc: %d",
                   rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _stpOptions::save()
   {
      INT32 rc = SDB_OK ;

      string line ;

      // format into string
      rc = pmdCfgRecord::toString( line, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get the line string, rc: %d", rc ) ;
         goto error ;
      }

      // write options to config file
      rc = utilWriteConfigFile( _cfgFileName, line.c_str(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write config file [%s], rc: %d",
                   _cfgFileName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _stpOptions::setServiceName( const CHAR *serviceName )
   {
      // set service name, and format into port
      if ( NULL != serviceName && '\0' != serviceName[ 0 ] )
      {
         // parse port
         ossGetPort( serviceName, _port ) ;
         // copy string
         ossStrncpy( _serviceName, serviceName, OSS_MAX_SERVICENAME ) ;
         _serviceName[ OSS_MAX_SERVICENAME ] = '\0' ;
      }
   }

   void _stpOptions::setServerList( const CHAR *serverList )
   {
      // set service list string and format into addresses
      if ( NULL != serverList && '\0' != serverList[ 0 ] )
      {
         // parse addresses
         parseAddressLine( serverList, _serverList ) ;
         // copy string
         ossStrncpy( _serverListString, serverList, PMD_MAX_LONG_STR_LEN ) ;
         _serverListString[ PMD_MAX_LONG_STR_LEN ] = '\0' ;
      }
   }

   void _stpOptions::setRole( const CHAR *role )
   {
      // set role string and format into role
      if ( NULL != role && '\0' != role[ 0 ] )
      {
         // parse role
         _role = stpGetRoleByName( role ) ;
         // copy string
         ossStrncpy( _roleString, role, PMD_MAX_SHORT_STR_LEN ) ;
         _roleString[ PMD_MAX_SHORT_STR_LEN ] = '\0' ;
      }
   }

   void _stpOptions::setRole( STP_ROLE role )
   {
      // set role and format into string
      _role = role ;
      ossStrncpy( _roleString, stpGetRoleName( role ),
                  PMD_MAX_SHORT_STR_LEN ) ;
      _roleString[ PMD_MAX_SHORT_STR_LEN ] = '\0' ;
   }

   void _stpOptions::logOptions()
   {
      // save options into diagnostic log
      string configs ;
      toString( configs ) ;
      PD_LOG( PDEVENT, "All configs:\n%s\nLimit info:\n%s",
              configs.c_str(), pmdGetLimit()->str().c_str() ) ;
   }

   void _stpOptions::formatServerList()
   {
      // format server list from addresses into string
      string serverListString = makeAddressLine( _serverList ) ;
      ossStrncpy( _serverListString, serverListString.c_str(),
                  PMD_MAX_LONG_STR_LEN ) ;
      _serverListString[ PMD_MAX_LONG_STR_LEN ] = '\0' ;
   }

   INT32 _stpOptions::doDataExchange( pmdCfgExchange *ex )
   {
      resetResult() ;

      // --port
      rdxString( ex, STP_OPTION_PORT, _serviceName,
                 sizeof( _serviceName ), FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 _serviceName ) ;

      // --serverlist
      rdxString( ex, STP_OPTION_SERVERLIST, _serverListString,
                 sizeof( _serverListString ), FALSE, PMD_CFG_CHANGE_RUN,
                 _serverListString ) ;

      // --role
      rdxString( ex, STP_OPTION_ROLE, _roleString, sizeof( _roleString ),
                 FALSE, PMD_CFG_CHANGE_RUN, _roleString ) ;

      // --testmode
      rdxBooleanS( ex, STP_OPTION_TESTMODE, _testMode, FALSE,
                   PMD_CFG_CHANGE_REBOOT, _testMode, TRUE ) ;

      // --weight
      rdxUInt( ex, STP_OPTION_WEIGHT, _weight, FALSE, PMD_CFG_CHANGE_RUN,
               _weight ) ;

      // --syncinterval
      rdxUInt( ex, STP_OPTION_SYNCINTERVAL, _syncInterval, FALSE,
               PMD_CFG_CHANGE_RUN, _syncInterval ) ;

      // --maxtimeerror
      rdxUInt( ex, STP_OPTION_MAXTIMEERROR, _maxTimeErrorUS, FALSE,
               PMD_CFG_CHANGE_RUN, _maxTimeErrorUS ) ;
      rdvMinMax( ex, _maxTimeErrorUS, STP_OPTION_MAXTIMEERROR_MIN,
                 STP_OPTION_MAXTIMEERROR_MAX, TRUE ) ;

      // --maxsynchist
      rdxUInt( ex, STP_OPTION_MAXSYNCHIST, _maxSyncHist, FALSE,
               PMD_CFG_CHANGE_RUN, _maxSyncHist, TRUE ) ;
      rdvMinMax( ex, _maxSyncHist, 0, 200, TRUE ) ;

      // --maxsyncports
      rdxUInt( ex, STP_OPTION_MAXSYNCPORTS, _maxSyncPorts, FALSE,
               PMD_CFG_CHANGE_REBOOT, _maxSyncPorts, TRUE ) ;
      rdvMinMax( ex, _maxSyncPorts, 1, 128, TRUE ) ;

      // --defclientsperport
      rdxUInt( ex, STP_OPTION_DEFCLIENTSPERPORT, _defClientsPerPort, FALSE,
               PMD_CFG_CHANGE_REBOOT, _defClientsPerPort, TRUE ) ;
      rdvMinMax( ex, _defClientsPerPort, 1, 128, TRUE ) ;

      // --preopenports
      rdxBooleanS( ex, STP_OPTION_PREOPENPORTS, _preOpenPorts, FALSE,
                   PMD_CFG_CHANGE_REBOOT, FALSE, TRUE ) ;

      // --syncwithsysport
      rdxBooleanS( ex, STP_OPTION_SYNCWITHSYSPORT, _syncWithSysPort,
                   FALSE, PMD_CFG_CHANGE_REBOOT, TRUE, TRUE ) ;

      // --diaglevel
      rdxUShort( ex, STP_OPTION_DIAGLEVEL, _diagLevel, FALSE,
              PMD_CFG_CHANGE_RUN, _diagLevel ) ;
      rdvMinMax( ex, _diagLevel, PDSEVERE, PDDEBUG, TRUE ) ;

      // --sharingBreak
      rdxUInt( ex, STP_OPTION_SHARINGBRK, _sharingBreakTime, FALSE,
               PMD_CFG_CHANGE_RUN, STP_OPTION_BREAKTIME_DFT, TRUE ) ;
      rdvMinMax( ex, _sharingBreakTime, 5000, 300000, TRUE ) ;

      // --startshifttime
      rdxUInt( ex, STP_OPTION_STARTSHIFTTIME, _startShiftTime, FALSE,
               PMD_CFG_CHANGE_RUN, STP_OPTION_STARTSHIFTTIME_DFT, TRUE ) ;
      rdvMinMax( ex, _startShiftTime, 0, 7200, TRUE ) ;

      return getResult () ;
   }

   INT32 _stpOptions::postLoaded( PMD_CFG_STEP step )
   {
      INT32 rc = SDB_OK ;

      if ( '\0' != _stpPath[ 0 ] )
      {
         // make sure directory exist
         rc = ossMkdir( _stpPath ) ;
         if ( rc && SDB_FE != rc )
         {
            PD_LOG( PDERROR, "Failed to create dir: %s, rc: %d",
                    _stpPath, rc ) ;
            goto error ;
         }
         rc = SDB_OK ;
      }

      // parse port
      rc = ossGetPort( _serviceName, _port ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse port from service name [%s], "
                   "rc: %d", _serviceName, _port ) ;
      pmdSetLocalPort( _port ) ;

      // parse server list into addresses
      _serverList.clear() ;
      rc = parseAddressLine( _serverListString, _serverList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse server list [%s], rc: %d",
                   _serverListString, rc ) ;

      // parse role
      if ( 0 == ossStrcmp( _roleString, STP_ROLE_NAME_CLIENT ) )
      {
         _role = STP_ROLE_CLIENT ;
      }
      else if ( 0 == ossStrcmp( _roleString, STP_ROLE_NAME_SERVER ) )
      {
         _role = STP_ROLE_SERVER ;
      }
      else
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse role, unknown role [%s]", _roleString ) ;
      }

      // reset role
      if ( _serverList.empty() )
      {
         PD_LOG( PDEVENT, "Server list is empty, change to server" ) ;
         _role = STP_ROLE_SERVER ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _stpOptions::preSaving()
   {
      INT32 rc = SDB_OK ;

      // format server list
      formatServerList() ;
      // make sure has field
      if ( '\0' != _serverListString[ 0 ] )
      {
         _addToFieldMap( STP_OPTION_SERVERLIST, _serverListString, TRUE,
                         TRUE ) ;
      }

      // format role
      ossStrncpy( _roleString, stpGetRoleName( _role ), PMD_MAX_SHORT_STR_LEN ) ;
      _roleString[ PMD_MAX_SHORT_STR_LEN ] = '\0' ;
      _addToFieldMap( STP_OPTION_ROLE, _roleString, TRUE, TRUE ) ;

      return rc ;
   }

   INT32 _stpOptions::_initArguments( INT32 argc,
                                      CHAR **argv,
                                      po::variables_map &vm )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc( "Command options" ) ;
      po::options_description all( "Command options" ) ;

      // initialize options
      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      PMD_ADD_PARAM_OPTIONS_BEGIN( all )
         COMMANDS_OPTIONS
         COMMANDS_HIDE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      // validate arguments
      rc = utilReadCommandLine( argc, argv, all, vm ) ;
      if ( SDB_OK != rc )
      {
         cout << "Invalid arguments: " << rc << endl ;
         _displayArguments( desc ) ;
         goto done ;
      }

      // handle help or version
      if ( vm.count( STP_OPTION_HELP ) )
      {
         _displayArguments( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
      }
      else if ( vm.count( STP_OPTION_HELPFULL ) )
      {
         _displayArguments( all ) ;
         rc = SDB_PMD_HELP_ONLY ;
      }
      else if ( vm.count( STP_OPTION_VERSION ) )
      {
         ossPrintVersion( "SequoiaDB STP version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
      }

   done:
      return rc ;
   }

   void _stpOptions::_displayArguments(
                                 const po::options_description &desc ) const
   {
      cout << "Usage:  stp [OPTION]" << endl ;
      cout << desc << endl ;
   }

   void _stpOptions::_displayVersion() const
   {
      ossPrintVersion( "Serial Time Protocol version" ) ;
   }

   INT32 _stpOptions::_toCommandLine( const po::variables_map &vm,
                                      string &options )
   {
      INT32 rc = SDB_OK ;

      try
      {
         stringstream ss ;

         // merge variables into command line
         for ( po::variables_map::const_iterator iter = vm.begin() ;
               iter != vm.end() ;
               ++ iter )
         {
            ss << " --" << iter->first << " " << iter->second.as<string>() ;
         }

         options = ss.str() ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to generate command line, error: %s",
                 e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

}
