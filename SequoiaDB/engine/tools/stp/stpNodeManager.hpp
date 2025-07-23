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

   Source File Name = stpNodeManager.hpp

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
#ifndef STP_NODE_MANAGER_HPP__
#define STP_NODE_MANAGER_HPP__

#include "stpCBCommon.hpp"
#include "stpModule.hpp"
#include "stpNode.hpp"
#include "stpMsg.hpp"

namespace engine
{

   /*
      _stpNodeManager define
    */
   // _stpNodeManager manages node information including servers, local node
   // and primary server
   class _stpNodeManager : public stpManagerBase
   {
      DECLARE_OBJ_MSG_MAP()

   public:
      // constructor and destructor
      _stpNodeManager( STPCB *stpCB ) ;
      virtual ~_stpNodeManager() ;

   public:
      // override functions for STP module
      // get name of module
      OSS_INLINE virtual const CHAR *getModuleName() const
      {
         return STP_NODE_MANAGER_NAME ;
      }

      // on timer callback
      virtual void onTimer( UINT64 timerID, UINT32 interval ) ;

      // process message callback
      virtual INT32 processMessage( NET_HANDLE handle, MsgHeader *message ) ;

   protected:
      // override protected functions of STP module

      // initialize module
      virtual INT32 _initialize() ;

      // on event post activate
      virtual INT32 _postActivate() ;

      // on event after change primary
      virtual INT32 _afterChangePrimary( const MsgRouteID &primaryRID,
                                         BOOLEAN isLocalPrimary ) ;

   protected:
      // handle server request
      INT32 _handleServerReq( NET_HANDLE handle,
                              const stpServerReq *request ) ;
      // handle server response
      INT32 _handleServerRsp( NET_HANDLE handle,
                              const stpServerRsp *response ) ;

      // handle add server request
      INT32 _handleAddServer( NET_HANDLE handle,
                              const MsgRouteID &routeID ) ;
      // handle remove server request
      INT32 _handleRemoveServer( NET_HANDLE handle,
                                 const MsgRouteID &routeID ) ;

      // send server request
      INT32 _sendServerReq( const MsgRouteID &routeID,
                            STP_SERVER_REQ_TYPE type ) ;
      // send server response
      INT32 _sendServerRsp( NET_HANDLE handle,
                            const stpServerReq *request,
                            const bson::BSONObj &object,
                            INT32 returnCode ) ;

   public:
      // get catalog status
      OSS_INLINE STP_NODE_STATUS getStatus()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _getStatus() ;
      }

      // set catalog status
      OSS_INLINE void setStatus( STP_NODE_STATUS status )
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
      OSS_INLINE stpClientNode getLocal()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local ;
      }

      // update local information
      INT32 setLocal( const stpClientNode &local ) ;

      // check whether self is primary server
      OSS_INLINE BOOLEAN isPrimaryServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _isPrimaryServer() ;
      }

      // check whether self is primary
      // NOTE: it could be a test mode node
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
                  STP_ROLE_SERVER == _local.getRole() ) ;
      }

      // check whether self is a server
      OSS_INLINE BOOLEAN isServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _isServer() ;
      }

      // check whether self is a synchronize client
      OSS_INLINE BOOLEAN isSyncClient()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( _hasPrimary() && !_isLocalPrimary() ) ;
      }

      // check whether need to check servers ( send server request )
      OSS_INLINE BOOLEAN needServerCheck()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( !_hasPrimary() ||
                  !_isVersionValid() ||
                  STP_NODE_QUERYSERVERS == _status ||
                  STP_NODE_ADDSERVER == _status ||
                  STP_NODE_REMOVESERVER == _status ) ;
      }

      // check whether need to query and update server information
      OSS_INLINE BOOLEAN needQueryServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( !_hasPrimary() ||
                  !_isVersionValid() ||
                  STP_NODE_QUERYSERVERS == _status ) ;
      }

      // check whether need to add self into servers
      OSS_INLINE BOOLEAN needAddServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( STP_NODE_ADDSERVER == _status ) ;
      }

      // check whether need to remove self from servers
      OSS_INLINE BOOLEAN needRemoveServer()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return ( STP_NODE_REMOVESERVER == _status ) ;
      }

      // reset version of servers
      OSS_INLINE void resetVersion()
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _version = STP_GROUP_INVALID_VERSION ;
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

      // get route ID value of local node
      OSS_INLINE UINT64 getLocalRIDValue()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local.getRouteIDValue() ;
      }

      // get local node and version of servers
      OSS_INLINE void getLocalAndVersion( stpClientNode & local,
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
      OSS_INLINE STP_ROLE getLocalRole()
      {
         ossScopedRWLock lock( &_mutex, SHARED ) ;
         return _local.getRole() ;
      }

      // update OID and synchronize port of local node
      OSS_INLINE void updateLocalSyncInfo( const MsgRouteID &routeID,
                                           const bson::OID & oid,
                                           UINT16 port )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _local.setRouteID( routeID ) ;
         _local.setOID( oid ) ;
         _local.setSyncPort( port ) ;
      }

      // update time error of local node
      // NOTE: update by time synchronization
      OSS_INLINE void updateLocalTimeError( UINT32 timeError )
      {
         ossScopedRWLock lock( &_mutex, EXCLUSIVE ) ;
         _local.setTimeError( OSS_MIN( _local.getMaxTimeError(),
                                       timeError ) ) ;
      }

      // check whether version of servers is expired
      // if expired, set the status to query servers
      void checkExpiredVersion( UINT32 expiredVersion ) ;

      // launch periodic server checks
      INT32 launchServerCheck() ;

      // get route ID of a server to send message
      // NOTE: if primary is preferred, try to get route ID of primary
      //       if current route ID is given and primary is not found,
      //       choose server next to given route ID
      INT32 chooseServerRID( const MsgRouteID &curRouteID,
                             BOOLEAN preferPrimary,
                             MsgRouteID &routeID ) ;

      // get server information by route ID
      INT32 getServer( const MsgRouteID &routeID, stpServerNode &server ) ;

      // dump information of all servers
      INT32 dumpServers( STP_SERVER_LIST &servers ) ;

      // remove all servers
      INT32 removeServers() ;

      // get information of servers
      INT32 getServers( UINT32 &version, STP_SERVER_LIST &servers ) ;

      // get information of servers as BSON object
      INT32 getServers( bson::BSONObj &object, BOOLEAN forDisplay ) ;

      // update information of servers from BSON object
      INT32 setServers( const bson::BSONObj &object ) ;

      // update catalog by configs
      INT32 updateConfigs() ;

   protected:
      // set status of catalog in lock
      OSS_INLINE void _setStatus( STP_NODE_STATUS status )
      {
         // only update between non-normal status and normal status
         // NOTE: if current status is non-normal, and given status is also
         //       non-normal, ignore it
         if ( STP_NODE_NORMAL == _status ||
              STP_NODE_NORMAL == status )
         {
            _status = status ;
         }
      }

      // get status of catalog in lock
      OSS_INLINE STP_NODE_STATUS _getStatus() const
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

      // check whether self is primary server in lock
      OSS_INLINE BOOLEAN _isPrimaryServer() const
      {
         return ( _hasPrimary() &&
                  _isLocalPrimary() &&
                  STP_ROLE_SERVER == _local.getRole() ) ;
      }

      // check whether version of servers is valid in lock
      OSS_INLINE BOOLEAN _isVersionValid() const
      {
         return STP_GROUP_INVALID_VERSION != _version ;
      }

      // check whether self is a server in lock
      OSS_INLINE BOOLEAN _isServer() const
      {
         return ( STP_ROLE_SERVER == _local.getRole() ) ;
      }

      // initialize local node
      INT32 _initLocal() ;
      // initialize servers
      INT32 _initServers() ;
      // update local from configs
      INT32 _updateLocalConfigs( BOOLEAN &roleChanged ) ;
      // update servers from configs
      INT32 _updateServerConfigs( BOOLEAN &changed ) ;
      // check local and servers ( whether a server is missing )
      INT32 _checkLocalAndServers() ;
      // save information of servers into configs
      INT32 _saveServers() ;

      // get information of servers
      void _getServers( UINT32 &version,
                        STP_SERVER_LIST &servers,
                        MsgRouteID &primaryRID ) ;

      // set information of servers
      void _setServers( UINT32 version,
                        const STP_SERVER_LIST &servers,
                        const MsgRouteID &primaryRID,
                        STP_SERVER_LIST &removedServers ) ;

      // build BSON object for version of servers
      INT32 _buildVersion( bson::BSONObjBuilder &builder,
                           UINT32 version ) ;
      // build BSON object for server nodes
      INT32 _buildServers( bson::BSONObjBuilder &builder,
                           const STP_SERVER_LIST &servers,
                           BOOLEAN forDisplay ) ;
      // build BSON object for primary node
      INT32 _buildPrimaryNode( bson::BSONObjBuilder &builder,
                               const STP_SERVER_LIST &servers,
                               const MsgRouteID &primaryRID,
                               BOOLEAN forDisplay ) ;
      // parse version of servers from BSON object
      INT32 _parseVersion( const bson::BSONObj &object,
                           UINT32 &version ) ;
      // parse server nodes from BSON object
      INT32 _parseServers( const bson::BSONObj &object,
                           STP_SERVER_LIST &servers ) ;
      // parse primary node from BSON object
      INT32 _parsePrimaryNode( const bson::BSONObj &object,
                               MsgRouteID &primaryRID ) ;

      // add given server node into servers
      INT32 _addServer( const stpServerNode &server,
                        BOOLEAN increaseVersion ) ;
      // remove given route ID from servers
      INT32 _removeServer( const MsgRouteID &routeID,
                           BOOLEAN increaseVersion ) ;
      // update servers by server list from configs
      INT32 _updateServers( const vector< pmdAddrPair > &serverList ) ;

   protected:
      // lock to protect catalog information
      ossRWMutex        _mutex ;
      // status of catalog
      STP_NODE_STATUS   _status ;
      // version of servers
      // NOTE: version is used to verify if the group is updated
      //       adding or removing server will increase version
      UINT32            _version ;
      // primary route ID of servers
      MsgRouteID        _primaryRID ;
      // list of server nodes
      STP_SERVER_LIST   _servers ;
      // information of local node
      stpClientNode     _local ;
   } ;

}

#endif // STP_NODE_MANAGER_HPP__
