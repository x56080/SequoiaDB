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

   Source File Name = tpCatalogManager.hpp

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

#ifndef TP_CATALOG_MANAGER_HPP__
#define TP_CATALOG_MANAGER_HPP__

#include "tpCBCommon.hpp"
#include "tpModule.hpp"
#include "tpNode.hpp"
#include "msgTp.hpp"

namespace engine
{

   /*
      _tpCatalogManager define
    */
   // _tpCatalogManager manages catalog information including servers,
   // local node and primary server
   class _tpCatalogManager : public tpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      _tpCatalogManager( SDB_TPCB *tpCB ) ;
      virtual ~_tpCatalogManager() ;

   public:
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return TP_CATALOG_MANAGER_NAME ;
      }

      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      virtual INT32 _initialize() ;
      virtual INT32 _postActivate() ;
      virtual INT32 _onChangePrimary( const MsgRouteID &primaryRID,
                                      BOOLEAN isLocalPrimary ) ;

   protected:
      // handle server request
      INT32 _handleServerReq( NET_HANDLE handle,
                              const MsgTpServerReq *request ) ;
      // handle server response
      INT32 _handleServerRsp( NET_HANDLE handle,
                              const MsgTpServerRsp *response ) ;

      // handle add server request
      INT32 _handleAddServer( NET_HANDLE handle,
                              const MsgRouteID &routeID ) ;
      // handle remove server request
      INT32 _handleRemoveServer( NET_HANDLE handle,
                                 const MsgRouteID &routeID ) ;

      // send server request
      INT32 _sendServerReq( const MsgRouteID &routeID,
                            MSG_TP_SERVER_REQ_TYPE type ) ;
      // send server response
      INT32 _sendServerRsp( NET_HANDLE handle,
                            const MsgTpServerReq *request,
                            const bson::BSONObj &object,
                            INT32 returnCode ) ;

   public:
      // get catalog status
      OSS_INLINE TP_CATALOG_STATUS getStatus()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _getStatus() ;
      }

      // set catalog status
      OSS_INLINE void setStatus( TP_CATALOG_STATUS status )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _status = status ;
      }

      // get version of servers
      OSS_INLINE UINT32 getVersion()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _version ;
      }

      // set version of servers
      OSS_INLINE void setVersion( UINT32 version )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _version = version ;
      }

      // get route ID of primary server
      OSS_INLINE MsgRouteID getPrimaryRID()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _primaryRID ;
      }

      // set route ID of primary server
      OSS_INLINE void setPrimaryRID( const MsgRouteID &routeID )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _primaryRID.value = routeID.value ;
      }

      // get local information
      OSS_INLINE tpClientNode getLocal()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local ;
      }

      // update local information
      INT32 setLocal( const tpClientNode &local ) ;

      // check whether self is primary server
      OSS_INLINE BOOLEAN isPrimaryServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( _hasPrimary() &&
                  _isLocalPrimary() &&
                  TP_ROLE_SERVER == _local.getRole() ) ;
      }

      // check whether self is primary
      // NOTE: it could be a standalone node
      OSS_INLINE BOOLEAN isPrimary()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( _hasPrimary() &&
                  _isLocalPrimary() ) ;
      }

      // check whether self is secondary server
      OSS_INLINE BOOLEAN isSecondaryServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( _hasPrimary() &&
                  !_isLocalPrimary() &&
                  TP_ROLE_SERVER == _local.getRole() ) ;
      }

      // check whether self is a sychronize client
      OSS_INLINE BOOLEAN isSyncClient()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( _hasPrimary() && !_isLocalPrimary() ) ;
      }

      // check whether need to query and update server information
      OSS_INLINE BOOLEAN needQueryServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( !_hasPrimary() ||
                  !_isVersionValid() ||
                  TP_CATALOG_QUERY == _status ) ;
      }

      // check whether need to add self into servers
      OSS_INLINE BOOLEAN needAddServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( TP_CATALOG_ADDSERVER == _status ) ;
      }

      // check whether need to remove self from servers
      OSS_INLINE BOOLEAN needRemoveServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( TP_CATALOG_REMOVESERVER == _status ) ;
      }

      // reset version of servers
      OSS_INLINE void resetVersion()
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _version = TP_GROUP_INVALID_VERSION ;
      }

      // check whether version of servers is valid
      OSS_INLINE BOOLEAN isVersionValid()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _isVersionValid() ;
      }

      // check whether version of servers is invalid
      OSS_INLINE BOOLEAN isVersionInvalid()
      {
         return !isVersionValid() ;
      }

      // increase version of servers
      OSS_INLINE void increaseVersion()
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         ++ _version ;
      }

      // reset primary node
      OSS_INLINE void resetPrimary()
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _primaryRID.value = MSG_INVALID_ROUTEID ;
      }

      // check whether there is a known primary
      OSS_INLINE BOOLEAN hasPrimary()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _hasPrimary() ;
      }

      // check whether there is not a known primary
      OSS_INLINE BOOLEAN hasNoPrimary()
      {
         return !hasPrimary() ;
      }

      // get route ID of local node
      OSS_INLINE MsgRouteID getLocalRID()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local.getRouteID() ;
      }

      // get local node and version of servers
      OSS_INLINE void getLocalAndVersion( tpClientNode & local,
                                          UINT32 &version )
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         local = _local ;
         version = _version ;
      }

      // get time error of local node
      OSS_INLINE UINT32 getLocalTimeError()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local.getTimeError() ;
      }

      // get role of local node
      OSS_INLINE TP_ROLE getLocalRole()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local.getRole() ;
      }

      // get OID of local node
      OSS_INLINE void setLocalOID( const bson::OID & oid )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _local.setOID( oid ) ;
      }

      // update time error of local node
      // NOTE: update by time synchronization
      OSS_INLINE void syncLocal( UINT32 timeError )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _local.setTimeError( OSS_MIN( _local.getMaxTimeError(), timeError ) ) ;
         _local.onSyncRsp() ;
      }

      // check whether version of servers is expired
      // if expired, set the status to query servers
      void checkExpiredVersion( UINT32 expiredVersion ) ;

      // launch periodic server checks
      INT32 launchServerCheck() ;

      // get route ID of a server
      // if primary is preferred, try to get route ID of primary
      INT32 getServerRID( MsgRouteID &routeID, BOOLEAN preferPrimary ) ;

      // get server information by route ID
      INT32 getServer( const MsgRouteID &routeID, tpServerNode &server ) ;

      // dump information of all servers
      INT32 dumpServers( TP_SERVER_LIST &servers ) ;

      // remove all servers
      INT32 removeServers() ;

      // get information of servers
      INT32 getServers( UINT32 &version, TP_SERVER_LIST &servers ) ;

      // get information of servers as BSON object
      INT32 getServers( bson::BSONObj &object ) ;

      // update information of servers from BSON object
      INT32 setServers( const bson::BSONObj &object ) ;

      // update catalog by configs
      INT32 updateConfigs() ;

   protected:
      // set status of catalog in lock
      OSS_INLINE void _setStatus( TP_CATALOG_STATUS status )
      {
         if ( TP_CATALOG_NORMAL == _status ||
              TP_CATALOG_NORMAL == status )
         {
            _status = status ;
         }
      }

      // get status of catalog in lock
      OSS_INLINE TP_CATALOG_STATUS _getStatus() const
      {
         return _status ;
      }

      // check whether self is primary in lock
      OSS_INLINE BOOLEAN _isLocalPrimary() const
      {
         return ( MSG_INVALID_ROUTEID != _primaryRID.value &&
                  _local.getRouteIDValue() == _primaryRID.value ) ;
      }

      // check whether there is a known primary in lock
      OSS_INLINE BOOLEAN _hasPrimary() const
      {
         return MSG_INVALID_ROUTEID != _primaryRID.value ;
      }

      // check whether version of servers is valid in lock
      OSS_INLINE BOOLEAN _isVersionValid() const
      {
         return TP_GROUP_INVALID_VERSION != _version ;
      }

      // update route ID in net agent for given host name and service name
      INT32 _updateRouteID( const MsgRouteID &routeID,
                            const CHAR *hostName,
                            const CHAR *serviceName ) ;

      // delete route ID from net agent
      INT32 _deleteRouteID( const MsgRouteID &routeID ) ;

      // initialize local node
      INT32 _initLocal() ;
      // initialize servers
      INT32 _initServers() ;
      // update local from configs
      INT32 _updateLocalConfigs( BOOLEAN &changed ) ;
      // update servers from configs
      INT32 _updateServerConfigs( BOOLEAN &changed ) ;
      // check local and servers ( whether a server is missing )
      INT32 _checkLocalAndServers() ;
      // save information of servers into configs
      INT32 _saveServers() ;

      // get information of servers
      void _getServers( UINT32 &version,
                        TP_SERVER_LIST &servers,
                        MsgRouteID &primaryRID ) ;

      // set information of servers
      void _setServers( UINT32 version,
                        const TP_SERVER_LIST &servers,
                        const MsgRouteID &primaryRID,
                        TP_SERVER_LIST &removedServers ) ;

      // build BSON object for version of servers
      INT32 _buildVersion( bson::BSONObjBuilder &builder,
                           UINT32 version ) ;
      // build BSON object for server nodes
      INT32 _buildServers( bson::BSONObjBuilder &builder,
                           const TP_SERVER_LIST &servers ) ;
      // build BSON object for primary node
      INT32 _buildPrimaryNode( bson::BSONObjBuilder &builder,
                               const TP_SERVER_LIST &servers,
                               const MsgRouteID &primaryRID ) ;
      // parse version of servers from BSON object
      INT32 _parseVersion( const bson::BSONObj &object,
                           UINT32 &version ) ;
      // parse server nodes from BSON object
      INT32 _parseServers( const bson::BSONObj &object,
                           TP_SERVER_LIST &servers ) ;
      // parse primary node from BSON object
      INT32 _parsePrimaryNode( const bson::BSONObj &object,
                               MsgRouteID &primaryRID ) ;

      // add given server node into servers
      INT32 _addServer( const tpServerNode &server,
                        BOOLEAN increaseVersion ) ;
      // remove given route ID from servers
      INT32 _removeServer( const MsgRouteID &routeID,
                           BOOLEAN increaseVersion ) ;
      // update servers by server list from configs
      INT32 _updateServers( const vector< pmdAddrPair > &serverList ) ;

   protected:
      // get route ID by host name and server name
      static INT32 _getRouteID( const CHAR *hostName,
                                const CHAR *serviceName,
                                MsgRouteID &routeID ) ;
      // get route ID by remote end point
      static INT32 _getRouteID( const netUDPEndPoint &endPoint,
                                MsgRouteID &routeID ) ;

   protected:
      // lock to protect catalog information
      ossRWMutex        _mutex ;
      // status of catalog
      TP_CATALOG_STATUS _status ;
      // version of servers
      UINT32            _version ;
      // primary route ID of servers
      MsgRouteID        _primaryRID ;
      // list of server nodes
      TP_SERVER_LIST    _servers ;
      // information of local node
      tpClientNode      _local ;
   } ;

}

#endif // TP_CATALOG_MANAGER_HPP__
