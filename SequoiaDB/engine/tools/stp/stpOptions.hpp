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

   Source File Name = stpOptions.hpp

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

#ifndef STP_OPTIONS_HPP__
#define STP_OPTIONS_HPP__

#include "stpCBCommon.hpp"
#include "pmdOptionsMgr.hpp"
#include "stpToolCommon.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _stpOptions define
    */
   // _stpOptions manages configuration options of STP
   class _stpOptions : public pmdCfgRecord
   {
   public:
      // constructor and destructor
      _stpOptions() ;
      virtual ~_stpOptions() ;

      // initialize options
      INT32 initialize( INT32 argc, CHAR **argv, const CHAR *rootPath,
                        BOOLEAN &daemonMode ) ;

      // save options to config file
      INT32 save() ;

      // get config file name
      OSS_INLINE const CHAR *getCfgFileName() const
      {
         return _cfgFileName ;
      }

      // get path to stp files
      OSS_INLINE const CHAR *getStpPath() const
      {
         return _stpPath ;
      }

      // set service name
      void setServiceName( const CHAR *serviceName ) ;

      // get service name
      OSS_INLINE const CHAR *getServiceName() const
      {
         return _serviceName ;
      }

      // get port
      OSS_INLINE UINT16 getPort() const
      {
         return _port ;
      }

      // set server list by string format
      void setServerList( const CHAR *serverList ) ;

      // get server list
      OSS_INLINE const vector< pmdAddrPair > &getServerList() const
      {
         return _serverList ;
      }

      // get server list in string format
      OSS_INLINE const CHAR *getServerListString() const
      {
         return _serverListString ;
      }

      // set role by string format
      void setRole( const CHAR *role ) ;
      // set role
      void setRole( STP_ROLE role ) ;

      // get role
      OSS_INLINE STP_ROLE getRole() const
      {
         return _role ;
      }

      // get role in string format
      OSS_INLINE const CHAR *getRoleString() const
      {
         return _roleString ;
      }

      OSS_INLINE void setTestMode( BOOLEAN testMode )
      {
         _testMode = testMode ;
      }

      OSS_INLINE BOOLEAN isTestMode() const
      {
         return _testMode ;
      }

      // set vote weight
      OSS_INLINE void setWeight( UINT32 weight )
      {
         _weight = weight ;
      }

      // get vote weight
      OSS_INLINE UINT32 getWeight() const
      {
         return _weight ;
      }

      // set synchronize interval
      OSS_INLINE void setSyncInterval( UINT32 interval )
      {
         _syncInterval = interval ;
      }

      // get synchronize interval
      OSS_INLINE UINT32 getSyncInterval() const
      {
         return _syncInterval ;
      }

      // set max time error in milliseconds
      OSS_INLINE void setMaxTimeErrorUS( UINT32 maxTimeErrorUS )
      {
         _maxTimeErrorUS = maxTimeErrorUS ;
      }

      // get max time error in milliseconds
      OSS_INLINE UINT32 getMaxTimeErrorUS() const
      {
         return _maxTimeErrorUS ;
      }

      // get max time error in nanoseconds
      OSS_INLINE UINT32 getMaxTimeErrorNS() const
      {
         return STP_MICROSEC_TO_NANOSEC( _maxTimeErrorUS ) ;
      }

      // get diagnostic log level
      OSS_INLINE PDLEVEL getDiagLevel() const
      {
         return (PDLEVEL)_diagLevel ;
      }

      // get sharing break time
      OSS_INLINE UINT32 getSharingBreakTime() const
      {
         return _sharingBreakTime ;
      }

      // get start shift time
      OSS_INLINE UINT32 getStartShiftTime() const
      {
         return _startShiftTime ;
      }

      // clear server list
      OSS_INLINE void clearServerList()
      {
         _serverList.clear() ;
      }

      // add server into server list
      OSS_INLINE void addServerAddress( const CHAR *hostName,
                                        const CHAR *serviceName )
      {
         SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;
         SDB_ASSERT( NULL != serviceName, "service name is invalid" ) ;
         _serverList.push_back( pmdAddrPair( hostName, serviceName ) ) ;
      }

      // output options into diagnostic log
      void logOptions() ;
      // format server list into string
      void formatServerList() ;

   protected:
      // override functions of config record
      // exchange data
      virtual INT32 doDataExchange( pmdCfgExchange *ex ) ;
      // event post loading data
      virtual INT32 postLoaded( PMD_CFG_STEP step ) ;
      // event previous to saving data
      virtual INT32 preSaving() ;

      // initialize arguments
      INT32 _initArguments( INT32 argc, CHAR **argv,
                            boost::program_options::variables_map &vm ) ;

      // display arguments for help
      void _displayArguments(
            const boost::program_options::options_description &desc ) const ;

      // display version
      void _displayVersion() const ;

   protected:
      // name of config file ( with full path )
      CHAR           _cfgFileName[ OSS_MAX_PATHSIZE + 1 ] ;
      // path name of stp directory
      CHAR           _stpPath[ OSS_MAX_PATHSIZE + 1 ] ;

      // service name ( port )
      CHAR           _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;
      // server list in string format ( split by comma )
      CHAR           _serverListString[ PMD_MAX_LONG_STR_LEN + 1 ] ;
      // role in string format
      CHAR           _roleString[ PMD_MAX_SHORT_STR_LEN + 1 ] ;
      // vote weight
      UINT32         _weight ;
      // synchronize time interval
      UINT32         _syncInterval ;
      // max time error allowed in microseconds
      UINT32         _maxTimeErrorUS ;
      // level of diagnostic log
      UINT16         _diagLevel ;
      // sharing break time for vote
      UINT32         _sharingBreakTime ;
      // start shift time for vote
      UINT32         _startShiftTime ;

      // port
      UINT16                  _port ;
      // server list ( parsed into address )
      vector< pmdAddrPair >   _serverList ;
      // role
      STP_ROLE                _role ;
      // test mode
      BOOLEAN                 _testMode ;
   } ;

   typedef class _stpOptions stpOptions ;

}

#endif // STP_OPTIONS_HPP__
