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

   Source File Name = tpOptions.hpp

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

#ifndef TP_OPTIONS_HPP__
#define TP_OPTIONS_HPP__

#include "tpCBCommon.hpp"
#include "pmdOptionsMgr.hpp"
#include "tpToolCommon.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _tpOptions define
    */
   class _tpOptions : public pmdCfgRecord
   {
   public:
      _tpOptions() ;
      virtual ~_tpOptions() ;

      INT32 initialize( INT32 argc, CHAR **argv, const CHAR *rootPath ) ;
      INT32 save() ;

      OSS_INLINE const CHAR *getCfgFileName() const
      {
         return _cfgFileName ;
      }

      OSS_INLINE const CHAR *getLocalCfgPath() const
      {
         return _localCfgPath ;
      }

      void setServiceName( const CHAR *serviceName ) ;

      OSS_INLINE const CHAR *getServiceName() const
      {
         return _serviceName ;
      }

      OSS_INLINE UINT16 getPort() const
      {
         return _port ;
      }

      void setServerList( const CHAR *serverList ) ;

      OSS_INLINE const vector< pmdAddrPair > &getServerList() const
      {
         return _serverList ;
      }

      OSS_INLINE const CHAR *getServerListString() const
      {
         return _serverListString ;
      }

      void setRole( const CHAR *role ) ;
      void setRole( TP_ROLE role ) ;

      OSS_INLINE TP_ROLE getRole() const
      {
         return _role ;
      }

      OSS_INLINE const CHAR *getRoleString() const
      {
         return _roleString ;
      }

      OSS_INLINE void setWeight( UINT32 weight )
      {
         _weight = weight ;
      }

      OSS_INLINE UINT32 getWeight() const
      {
         return _weight ;
      }

      OSS_INLINE void setSyncInterval( UINT32 interval )
      {
         _syncInterval = interval ;
      }

      OSS_INLINE UINT32 getSyncInterval() const
      {
         return _syncInterval ;
      }

      OSS_INLINE void setMaxTimeErrorUS( UINT32 maxTimeErrorUS )
      {
         _maxTimeErrorUS = maxTimeErrorUS ;
      }

      OSS_INLINE UINT32 getMaxTimeErrorUS() const
      {
         return _maxTimeErrorUS ;
      }

      OSS_INLINE UINT32 getMaxTimeError() const
      {
         return TP_MICROSEC_TO_NANOSEC( _maxTimeErrorUS ) ;
      }

      OSS_INLINE PDLEVEL getDiagLevel() const
      {
         return (PDLEVEL)_diagLevel ;
      }

      OSS_INLINE UINT32 getSharingBreakTime() const
      {
         return _sharingBreakTime ;
      }

      OSS_INLINE UINT32 getStartShiftTime() const
      {
         return _startShiftTime ;
      }

      OSS_INLINE void clearServerList()
      {
         _serverList.clear() ;
      }

      OSS_INLINE void addServerAddress( const CHAR *hostName,
                                        const CHAR *serviceName )
      {
         SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;
         SDB_ASSERT( NULL != serviceName, "service name is invalid" ) ;
         _serverList.push_back( pmdAddrPair( hostName, serviceName ) ) ;
      }

      void logOptions() ;
      void formatServerList() ;

   protected:
      virtual INT32 doDataExchange( pmdCfgExchange *ex ) ;
      virtual INT32 postLoaded( PMD_CFG_STEP step ) ;
      virtual INT32 preSaving() ;

      INT32 _initArguments( INT32 argc, CHAR **argv, po::variables_map &vm ) ;

      void _displayArguments( const po::options_description &desc ) const ;
      void _displayVersion() const ;

   protected :
      CHAR _cfgFileName[ OSS_MAX_PATHSIZE + 1 ] ;
      CHAR _localCfgPath[ OSS_MAX_PATHSIZE + 1 ] ;

      CHAR           _serviceName[ OSS_MAX_SERVICENAME + 1 ] ;
      CHAR           _serverListString[ PMD_MAX_LONG_STR_LEN + 1 ] ;
      CHAR           _roleString[ PMD_MAX_SHORT_STR_LEN + 1 ] ;
      UINT32         _weight ;
      UINT32         _syncInterval ;
      UINT32         _maxTimeErrorUS ;
      UINT16         _diagLevel ;
      UINT32         _sharingBreakTime ;
      UINT32         _startShiftTime ;

      UINT16                  _port ;
      vector< pmdAddrPair >   _serverList ;
      TP_ROLE                 _role ;
   } ;

   typedef class _tpOptions tpOptions ;

}

#endif // TP_OPTIONS_HPP__
