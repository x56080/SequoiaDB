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

   Source File Name = tpOptions.cpp

   Descriptive Name = SequoiaDB Time Protocol Service

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains structure for SequoiaDB
   Time Protocol Service.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/30/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "tpOptions.hpp"
#include "ossVer.h"
#include "pmdEnv.hpp"

using namespace std ;
using namespace po ;

namespace engine
{

   #define PMD_TP_OPTION_BREAKTIME_DFT          (7000)
   #define PMD_TP_OPTION_STARTSHIFTTIME_DFT     (600)

   #define FILE_OPTIONS \
         ( PMD_OPTION_PORT, value<string>(), "sdbtp listening port, default is 9622" ) \
         ( PMD_TP_OPTION_SERVERLIST, value<string>(), "sdbtp server list" ) \
         ( PMD_OPTION_ROLE, value<string>(), "sdbtp role, default is standalone" ) \
         ( PMD_OPTION_WEIGHT, value<INT32>(), "sdbtp vote weight" ) \
         ( PMD_TP_OPTION_SYNCINTERVAL, value<INT32>(), "sdbtp synchronize interval" ) \
         ( PMD_TP_OPTION_MAXTIMEERROR, value<INT32>(), "sdbtp max time error" ) \
         ( PMD_OPTION_DIAGLEVEL, value<INT32>(), "sdbtp dialog level, default is 3" ) \
         ( PMD_OPTION_SHARINGBRK, value<INT32>(), "The timeout period for heartbeat in each replica group ( in ms ), default:7000, value range:[5000,300000] " ) \
         ( PMD_OPTION_START_SHIFT_TIME, value<INT32>(), "Nodes starting shift time(sec), default:600, value range:[0,7200]" )

   #define COMMANDS_OPTIONS \
         ( PMD_COMMANDS_STRING( PMD_OPTION_PORT, ",p" ), value<string>(), "sdbtp listening port, default is 9622" ) \
         ( PMD_TP_OPTION_SERVERLIST, value<string>(), "sdbtp server list" ) \
         ( PMD_OPTION_ROLE, value<string>(), "sdbtp role, default is standalone" ) \
         ( PMD_OPTION_WEIGHT, value<INT32>(), "sdbtp vote weight" ) \
         ( PMD_TP_OPTION_SYNCINTERVAL, value<INT32>(), "sdbtp synchronize interval" ) \
         ( PMD_TP_OPTION_MAXTIMEERROR, value<INT32>(), "sdbtp max time error" ) \
         ( PMD_OPTION_DIAGLEVEL, value<INT32>(), "sdbtp dialog level, default is 3" ) \
         ( PMD_OPTION_SHARINGBRK, value<INT32>(), "The timeout period for heartbeat in each replica group ( in ms ), default:7000, value range:[5000,300000] " ) \
         ( PMD_OPTION_START_SHIFT_TIME, value<INT32>(), "Nodes starting shift time(sec), default:600, value range:[0,7200]" ) \
         ( PMD_COMMANDS_STRING( PMD_OPTION_HELP, ",h" ), "help" ) \
         ( PMD_OPTION_VERSION, "version" ) \
         ( PMD_COMMANDS_STRING( PMD_OPTION_CONFPATH, ",c" ), value<string>(), "sdbtp configuration file path" ) \
         ( PMD_OPTION_FORCE, "force to start without configuration file" )

   /*
      _tpOptions implement
    */
   _tpOptions::_tpOptions()
   : _weight( 0 ),
     _syncInterval( TP_DEF_SYNC_INTERVAL ),
     _maxTimeErrorUS( TP_MAX_TIME_ERROR_US ),
     _diagLevel( PDWARNING ),
     _sharingBreakTime( PMD_TP_OPTION_BREAKTIME_DFT ),
     _startShiftTime( PMD_TP_OPTION_STARTSHIFTTIME_DFT ),
     _port( TP_DEF_PORT ),
     _role( TP_ROLE_STANDALONE )
   {
      _cfgFileName[ 0 ] = '\0' ;
      _localCfgPath[ 0 ] = '\0' ;
      _serverListString[ 0 ] = '\0' ;
      ossSnprintf( _roleString, PMD_MAX_SHORT_STR_LEN,
                   TP_ROLE_NAME_CLIENT ) ;
      ossSnprintf( _serviceName, OSS_MAX_SERVICENAME, "%d", _port ) ;
   }

   _tpOptions::~_tpOptions()
   {
   }

   INT32 _tpOptions::initialize( INT32 argc,
                                 CHAR **argv,
                                 const CHAR *rootPath )
   {
      INT32 rc = SDB_OK ;

      BOOLEAN force = TRUE ;
      options_description desc( "Command options" ) ;
      variables_map vmFile, vmCommand ;

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
         if ( NULL == ossGetRealPath(
                     vmCommand[ PMD_OPTION_CONFPATH ].as<string>().c_str(),
                     _localCfgPath, OSS_MAX_PATHSIZE ) )
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
         PD_CHECK( NULL != rootPath, SDB_INVALIDARG, error, PDERROR,
                   "Root path is empty" ) ;

         // build 'conf' file path
         rc = utilBuildFullPath( rootPath, SDBTP_ROOT_PATH, OSS_MAX_PATHSIZE,
                                 _localCfgPath ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build local path for root "
                      "path %s, rc: %d", rootPath, rc ) ;
      }

      if ( vmCommand.count( PMD_OPTION_FORCE ) )
      {
         // force to start
         force = TRUE ;
      }

      // build sdbtp config file path
      rc = utilBuildFullPath( _localCfgPath, SDBTP_CFG_FILE_NAME,
                              OSS_MAX_PATHSIZE, _cfgFileName ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build config path for root "
                   "path %s, rc: %d", rootPath, rc ) ;

      PMD_ADD_PARAM_OPTIONS_BEGIN( desc )
         FILE_OPTIONS
      PMD_ADD_PARAM_OPTIONS_END

      // read config from file
      rc = utilReadConfigureFile( _cfgFileName, desc, vmFile ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_FNE == rc && force )
         {
            // File or dir not exist
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

      vmCommand.erase( PMD_OPTION_CONFPATH ) ;
      vmCommand.erase( PMD_OPTION_FORCE ) ;

      rc = pmdCfgRecord::init( &vmFile, &vmCommand ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to initialize configurations, rc: %d",
                   rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _tpOptions::save()
   {
      INT32 rc = SDB_OK ;

      string line ;

      rc = pmdCfgRecord::toString( line, PMD_CFG_MASK_SKIP_UNFIELD ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get the line string, rc: %d", rc ) ;
         goto error ;
      }

      rc = utilWriteConfigFile( _cfgFileName, line.c_str(), FALSE ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to write config file [%s], rc: %d",
                   _cfgFileName, rc ) ;

   done:
      return rc ;

   error:
      goto done ;
   }

   void _tpOptions::setServiceName( const CHAR *serviceName )
   {
      if ( NULL != serviceName && '\0' != serviceName[ 0 ] )
      {
         ossGetPort( serviceName, _port ) ;
      }
   }

   void _tpOptions::setServerList( const CHAR *serverList )
   {
      if ( NULL != serverList && '\0' != serverList[ 0 ] )
      {
         parseAddressLine( serverList, _serverList ) ;
      }
   }

   void _tpOptions::setRole( const CHAR *role )
   {
      if ( 0 == ossStrcmp( role, TP_ROLE_NAME_CLIENT ) )
      {
         _role = TP_ROLE_CLIENT ;
      }
      else if ( 0 == ossStrcmp( role, TP_ROLE_NAME_SERVER ) )
      {
         _role = TP_ROLE_SERVER ;
      }
      ossStrncpy( _roleString, role, PMD_MAX_SHORT_STR_LEN ) ;
   }

   void _tpOptions::setRole( TP_ROLE role )
   {
      _role = role ;
      ossStrncpy( _roleString, tpGetRoleName( role ),
                  PMD_MAX_SHORT_STR_LEN ) ;
   }

   void _tpOptions::logOptions()
   {
      string configs ;
      toString( configs ) ;
      PD_LOG( PDEVENT, "All configs:\n%s\nLimit info:\n%s",
              configs.c_str(), pmdGetLimit()->str().c_str() ) ;
   }

   void _tpOptions::formatServerList()
   {
      string serverListString = makeAddressLine( _serverList ) ;
      ossStrncpy( _serverListString, serverListString.c_str(),
                  PMD_MAX_LONG_STR_LEN ) ;
      _serverListString[ PMD_MAX_LONG_STR_LEN ] = '\0' ;
   }

   INT32 _tpOptions::doDataExchange( pmdCfgExchange *ex )
   {
      resetResult() ;

      // --port
      rdxString( ex, PMD_OPTION_PORT, _serviceName,
                 sizeof( _serviceName ), FALSE, PMD_CFG_CHANGE_FORBIDDEN,
                 _serviceName ) ;

      // --serverlist
      rdxString( ex, PMD_TP_OPTION_SERVERLIST, _serverListString,
                 sizeof( _serverListString ), FALSE, PMD_CFG_CHANGE_RUN,
                 _serverListString ) ;

      // --role
      rdxString( ex, PMD_OPTION_ROLE, _roleString, sizeof( _roleString ),
                 FALSE, PMD_CFG_CHANGE_RUN, _roleString ) ;

      // --weight
      rdxUInt( ex, PMD_OPTION_WEIGHT, _weight, FALSE, PMD_CFG_CHANGE_RUN,
               _weight ) ;

      // --syncinterval
      rdxUInt( ex, PMD_TP_OPTION_SYNCINTERVAL, _syncInterval, FALSE,
               PMD_CFG_CHANGE_RUN, _syncInterval ) ;

      // --maxtimeerror
      rdxUInt( ex, PMD_TP_OPTION_MAXTIMEERROR, _maxTimeErrorUS, FALSE,
               PMD_CFG_CHANGE_RUN, _maxTimeErrorUS ) ;

      // --diaglevel
      rdxUShort( ex, PMD_OPTION_DIAGLEVEL, _diagLevel, FALSE,
              PMD_CFG_CHANGE_RUN, _diagLevel ) ;
      rdvMinMax( ex, _diagLevel, PDSEVERE, PDDEBUG, TRUE ) ;

      // --sharingBreak
      rdxUInt( ex, PMD_OPTION_SHARINGBRK, _sharingBreakTime, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_TP_OPTION_BREAKTIME_DFT, TRUE ) ;
      rdvMinMax( ex, _sharingBreakTime, 5000, 300000, TRUE ) ;

      // --startshifttime
      rdxUInt( ex, PMD_OPTION_START_SHIFT_TIME, _startShiftTime, FALSE,
               PMD_CFG_CHANGE_RUN, PMD_TP_OPTION_STARTSHIFTTIME_DFT, TRUE ) ;
      rdvMinMax( ex, _startShiftTime, 0, 7200, TRUE ) ;

      return getResult () ;
   }

   INT32 _tpOptions::postLoaded( PMD_CFG_STEP step )
   {
      INT32 rc = SDB_OK ;

      // make sure directory exist
      rc = ossMkdir( getLocalCfgPath() ) ;
      if ( rc && SDB_FE != rc )
      {
         PD_LOG( PDERROR, "Failed to create dir: %s, rc: %d",
                 getLocalCfgPath(), rc ) ;
         goto error ;
      }
      rc = SDB_OK ;

      // port
      rc = ossGetPort( _serviceName, _port ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse port from service name [%s], "
                   "rc: %d", _serviceName, _port ) ;
      pmdSetLocalPort( _port ) ;

      // server list
      _serverList.clear() ;
      rc = parseAddressLine( _serverListString, _serverList ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse server list [%s], rc: %d",
                   _serverListString, rc ) ;

      // role
      if ( 0 == ossStrcmp( _roleString, TP_ROLE_NAME_STANDALONE ) )
      {
         _role = TP_ROLE_STANDALONE ;
      }
      else if ( 0 == ossStrcmp( _roleString, TP_ROLE_NAME_CLIENT ) )
      {
         _role = TP_ROLE_CLIENT ;
      }
      else if ( 0 == ossStrcmp( _roleString, TP_ROLE_NAME_SERVER ) )
      {
         _role = TP_ROLE_SERVER ;
      }
      else
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse role, unknown role [%s]", _roleString ) ;
      }

      if ( _serverList.empty() )
      {
         PD_LOG( PDEVENT, "Server list is empty, change to standalone" ) ;
         _role = TP_ROLE_STANDALONE ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _tpOptions::preSaving()
   {
      INT32 rc = SDB_OK ;

      formatServerList() ;

      /// make sure hasField
      if ( '\0' != _serverListString[ 0 ] )
      {
         _addToFieldMap( PMD_TP_OPTION_SERVERLIST, _serverListString, TRUE,
                         TRUE ) ;
      }

      ossStrncpy( _roleString, tpGetRoleName( _role ), PMD_MAX_SHORT_STR_LEN ) ;
      _roleString[ PMD_MAX_SHORT_STR_LEN ] = '\0' ;
      _addToFieldMap( PMD_OPTION_ROLE, _roleString, TRUE, TRUE ) ;

      return rc ;
   }

   INT32 _tpOptions::_initArguments( INT32 argc,
                                     CHAR **argv,
                                     variables_map &vm )
   {
      INT32 rc = SDB_OK ;
      options_description desc( "Command options" ) ;

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

      /// read cmd first
      if ( vm.count( PMD_OPTION_HELP ) )
      {
         _displayArguments( desc ) ;
         rc = SDB_PMD_HELP_ONLY ;
      }
      else if ( vm.count( PMD_OPTION_VERSION ) )
      {
         ossPrintVersion( "SequoiaDB TP version" ) ;
         rc = SDB_PMD_VERSION_ONLY ;
      }

   done:
      return rc ;
   }

   void _tpOptions::_displayArguments( const options_description &desc ) const
   {
      cout << "Usage:  sdbtp [OPTION]" << endl ;
      cout << desc << endl ;
   }

   void _tpOptions::_displayVersion() const
   {
      ossPrintVersion( "SequoiaDB TP version" ) ;
   }

}
