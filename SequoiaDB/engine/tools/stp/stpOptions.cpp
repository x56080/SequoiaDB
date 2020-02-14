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

namespace po = boost::program_options ;

using namespace std ;

namespace engine
{

   #define PMD_STP_OPTION_BREAKTIME_DFT          (7000)
   #define PMD_STP_OPTION_STARTSHIFTTIME_DFT     (600)

   #define FILE_OPTIONS \
         ( PMD_OPTION_PORT, po::value<string>(), "sdbtp listening port, default is 9622" ) \
         ( PMD_STP_OPTION_SERVERLIST, po::value<string>(), "sdbtp server list" ) \
         ( PMD_OPTION_ROLE, po::value<string>(), "sdbtp role, default is standalone" ) \
         ( PMD_OPTION_WEIGHT, po::value<INT32>(), "sdbtp vote weight" ) \
         ( PMD_STP_OPTION_SYNCINTERVAL, po::value<INT32>(), "sdbtp synchronize interval" ) \
         ( PMD_STP_OPTION_MAXTIMEERROR, po::value<INT32>(), "sdbtp max time error" ) \
         ( PMD_OPTION_DIAGLEVEL, po::value<INT32>(), "sdbtp dialog level, default is 3" ) \
         ( PMD_OPTION_SHARINGBRK, po::value<INT32>(), "The timeout period for heartbeat in each replica group ( in ms ), default:7000, value range:[5000,300000] " ) \
         ( PMD_OPTION_START_SHIFT_TIME, po::value<INT32>(), "Nodes starting shift time(sec), default:600, value range:[0,7200]" )

   #define COMMANDS_OPTIONS \
         ( PMD_COMMANDS_STRING( PMD_OPTION_PORT, ",p" ), po::value<string>(), "sdbtp listening port, default is 9622" ) \
         ( PMD_STP_OPTION_SERVERLIST, po::value<string>(), "sdbtp server list" ) \
         ( PMD_OPTION_ROLE, po::value<string>(), "sdbtp role, default is standalone" ) \
         ( PMD_OPTION_WEIGHT, po::value<INT32>(), "sdbtp vote weight" ) \
         ( PMD_STP_OPTION_SYNCINTERVAL, po::value<INT32>(), "sdbtp synchronize interval" ) \
         ( PMD_STP_OPTION_MAXTIMEERROR, po::value<INT32>(), "sdbtp max time error" ) \
         ( PMD_OPTION_DIAGLEVEL, po::value<INT32>(), "sdbtp dialog level, default is 3" ) \
         ( PMD_OPTION_SHARINGBRK, po::value<INT32>(), "The timeout period for heartbeat in each replica group ( in ms ), default:7000, value range:[5000,300000] " ) \
         ( PMD_OPTION_START_SHIFT_TIME, po::value<INT32>(), "Nodes starting shift time(sec), default:600, value range:[0,7200]" ) \
         ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" ) \
         ( PMD_OPTION_VERSION, "version" ) \
         ( PMD_COMMANDS_STRING( PMD_OPTION_CONFPATH, ",c" ), po::value<string>(), "sdbtp configuration file path" ) \
         ( PMD_OPTION_FORCE, "force to start without configuration file" )

   /*
      _tpOptions implement
    */
   _stpOptions::_stpOptions()
   : _weight( 0 ),
     _syncInterval( STP_DEF_SYNC_INTERVAL ),
     _maxTimeErrorUS( STP_MAX_TIME_ERROR_US ),
     _diagLevel( PDWARNING ),
     _sharingBreakTime( PMD_STP_OPTION_BREAKTIME_DFT ),
     _startShiftTime( PMD_STP_OPTION_STARTSHIFTTIME_DFT ),
     _port( STP_DEF_PORT ),
     _role( STP_ROLE_STANDALONE )
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
                                  const CHAR *rootPath )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN force = TRUE ;
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

      if ( vmCommand.count( PMD_OPTION_CONFPATH ) )
      {
         // if config path is given, check if exists
         if ( NULL == ossGetRealPath(
                     vmCommand[ PMD_OPTION_CONFPATH ].as<string>().c_str(),
                     _stpPath, OSS_MAX_PATHSIZE ) )
         {
            cerr << "ERROR: Failed to get real path for " <<
                    vmCommand[ PMD_OPTION_CONFPATH ].as<string>().c_str() <<
                    endl ;
            rc = SDB_INVALIDPATH ;
            goto error;
         }
         force = FALSE ;
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

      if ( vmCommand.count( PMD_OPTION_FORCE ) )
      {
         // force to start
         force = TRUE ;
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
         if ( SDB_FNE == rc && force )
         {
            // file or dir not exist
            PD_LOG( PDWARNING, "Failed to read missing configurations [%s], "
                    "use default configurations", _cfgFileName ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to read configurations from file [%s], "
                    "rc: %d", _cfgFileName, rc ) ;
            goto error ;
         }
      }

      // remove options should no saved into config file
      vmCommand.erase( PMD_OPTION_CONFPATH ) ;
      vmCommand.erase( PMD_OPTION_FORCE ) ;

      // initialze config record
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
         if ( 0 == ossStrcmp( role, STP_ROLE_NAME_CLIENT ) )
         {
            _role = STP_ROLE_CLIENT ;
         }
         else if ( 0 == ossStrcmp( role, STP_ROLE_NAME_SERVER ) )
         {
            _role = STP_ROLE_SERVER ;
         }
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
      rdxString( ex, PMD_OPTION_PORT, _serviceName,
                 sizeof( _serviceName ), FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 _serviceName ) ;

      // --serverlist
      rdxString( ex, PMD_STP_OPTION_SERVERLIST, _serverListString,
                 sizeof( _serverListString ), FALSE, PMD_CFG_CHANGE_RUN,
                 _serverListString ) ;

      // --role
      rdxString( ex, PMD_OPTION_ROLE, _roleString, sizeof( _roleString ),
                 FALSE, PMD_CFG_CHANGE_RUN, _roleString ) ;

      // --weight
      rdxUInt( ex, PMD_OPTION_WEIGHT, _weight, FALSE, PMD_CFG_CHANGE_RUN,
               _weight ) ;

      // --syncinterval
      rdxUInt( ex, PMD_STP_OPTION_SYNCINTERVAL, _syncInterval, FALSE,
               PMD_CFG_CHANGE_RUN, _syncInterval ) ;

      // --maxtimeerror
      rdxUInt( ex, PMD_STP_OPTION_MAXTIMEERROR, _maxTimeErrorUS, FALSE,
               PMD_CFG_CHANGE_RUN, _maxTimeErrorUS ) ;

      // --diaglevel
      rdxUShort( ex, PMD_OPTION_DIAGLEVEL, _diagLevel, FALSE,
              PMD_CFG_CHANGE_RUN, _diagLevel ) ;
      rdvMinMax( ex, _diagLevel, PDSEVERE, PDDEBUG, TRUE ) ;

      // --sharingBreak
      rdxUInt( ex, PMD_OPTION_SHARINGBRK, _sharingBreakTime, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_STP_OPTION_BREAKTIME_DFT, TRUE ) ;
      rdvMinMax( ex, _sharingBreakTime, 5000, 300000, TRUE ) ;

      // --startshifttime
      rdxUInt( ex, PMD_OPTION_START_SHIFT_TIME, _startShiftTime, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_STP_OPTION_STARTSHIFTTIME_DFT, TRUE ) ;
      rdvMinMax( ex, _startShiftTime, 0, 7200, TRUE ) ;

      return getResult () ;
   }

   INT32 _stpOptions::postLoaded( PMD_CFG_STEP step )
   {
      INT32 rc = SDB_OK ;

      // make sure directory exist
      rc = ossMkdir( _stpPath ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Failed to create dir: %s, rc: %d", _stpPath, rc ) ;
         goto error ;
      }
      rc = SDB_OK ;

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
      if ( 0 == ossStrcmp( _roleString, STP_ROLE_NAME_STANDALONE ) )
      {
         _role = STP_ROLE_STANDALONE ;
      }
      else if ( 0 == ossStrcmp( _roleString, STP_ROLE_NAME_CLIENT ) )
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
         PD_LOG( PDEVENT, "Server list is empty, change to standalone" ) ;
         _role = STP_ROLE_STANDALONE ;
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
         _addToFieldMap( PMD_STP_OPTION_SERVERLIST, _serverListString, TRUE,
                         TRUE ) ;
      }

      // format role
      ossStrncpy( _roleString, stpGetRoleName( _role ), PMD_MAX_SHORT_STR_LEN ) ;
      _roleString[ PMD_MAX_SHORT_STR_LEN ] = '\0' ;
      _addToFieldMap( PMD_OPTION_ROLE, _roleString, TRUE, TRUE ) ;

      return rc ;
   }

   INT32 _stpOptions::_initArguments( INT32 argc,
                                      CHAR **argv,
                                      po::variables_map &vm )
   {
      INT32 rc = SDB_OK ;
      po::options_description desc( "Command options" ) ;

      // initialize options
      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         COMMANDS_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      // validate arguments
      rc = utilReadCommandLine( argc, argv, desc, vm ) ;
      if ( SDB_OK != rc )
      {
         cout << "Invalid arguments: " << rc << endl ;
         _displayArguments( desc ) ;
         goto done ;
      }

      // handle help or version
      if ( vm.count( PMD_OPTION_HELP ) )
      {
         _displayArguments( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
      }
      else if ( vm.count( PMD_OPTION_VERSION ) )
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

}
