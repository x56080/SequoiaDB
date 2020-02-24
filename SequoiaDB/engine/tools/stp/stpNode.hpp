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

   Source File Name = stpNode.hpp

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

#ifndef STP_NODE_HPP__
#define STP_NODE_HPP__

#include "stpCBCommon.hpp"
#include "msgDef.hpp"
#include "netDef.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "stpSyncStats.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _stpNode define
    */
   class _stpNode ;
   typedef class _stpNode stpNode ;

   // _stpNode is based class for STP nodes including client, source and
   // server
   class _stpNode : public utilPooledObject
   {
   public:
      // constructor and destructor
      _stpNode() ;
      _stpNode( const stpNode &node ) ;
      ~_stpNode() ;

   public:
      // operators
      stpNode &operator =( const stpNode &node ) ;

   public:
      // get and set functions
      OSS_INLINE STP_ROLE getRole() const
      {
         return _role ;
      }

      OSS_INLINE void setRole( STP_ROLE role )
      {
         _role = role ;
      }

      OSS_INLINE const MsgRouteID &getRouteID() const
      {
         return _routeID ;
      }

      OSS_INLINE void setRouteID( const MsgRouteID &routeID )
      {
         _routeID.value = routeID.value ;
      }

      OSS_INLINE const CHAR *getHostName() const
      {
         return _hostName.c_str() ;
      }

      OSS_INLINE void setHostName( const CHAR *hostName )
      {
         SDB_ASSERT( NULL != hostName, "host name is invalid" ) ;
         _hostName.assign( hostName ) ;
      }

      OSS_INLINE const CHAR *getServiceName() const
      {
         return _serviceName.c_str() ;
      }

      OSS_INLINE void setServiceName( const CHAR *serviceName )
      {
         SDB_ASSERT( NULL != serviceName, "service name is invalid" ) ;
         _serviceName.assign( serviceName ) ;
      }

   public:
      // check if route ID is valid
      OSS_INLINE BOOLEAN isValidRoute() const
      {
         // NOTE: always use local service as service ID
         return ( MSG_INVALID_ROUTEID != _routeID.value &&
                  INVALID_GROUPID != _routeID.columns.groupID &&
                  INVALID_NODEID != _routeID.columns.nodeID &&
                  MSG_ROUTE_LOCAL_SERVICE == _routeID.columns.serviceID ) ;
      }

      // get group ID from route ID
      OSS_INLINE UINT32 getGroupID() const
      {
         return _routeID.columns.groupID ;
      }

      // get node ID from route ID
      OSS_INLINE UINT16 getNodeID() const
      {
         return _routeID.columns.nodeID ;
      }

      // get service ID from route ID
      OSS_INLINE UINT16 getServiceID() const
      {
         return _routeID.columns.serviceID ;
      }

      // get value of route ID
      OSS_INLINE UINT64 getRouteIDValue() const
      {
         return _routeID.value ;
      }

   protected:
      INT32 _fromBSON( const bson::BSONObj &nodeObject ) ;
      INT32 _toBSON( bson::BSONObjBuilder &nodeBuilder,
                     BOOLEAN toDisplay ) const ;

   protected:
      // role of node
      STP_ROLE       _role ;
      // route ID of node
      MsgRouteID     _routeID ;
      // host name of node
      ossPoolString  _hostName ;
      // service name of node
      ossPoolString  _serviceName ;
   } ;

   /*
      _stpServerNode
    */
   class _stpServerNode ;
   typedef class _stpServerNode stpServerNode ;
   typedef ossPoolList< stpServerNode > STP_SERVER_LIST ;

   // _stpServerNode represents for server node in STP
   class _stpServerNode : public stpNode
   {
   public:
      // constructor and destructor
      _stpServerNode() ;
      _stpServerNode( const stpServerNode &server ) ;
      ~_stpServerNode() ;

   public:
      // operators
      stpServerNode &operator =( const stpServerNode &server ) ;
      BOOLEAN operator ==( const stpServerNode &server ) const ;
      BOOLEAN operator ==( const MsgRouteID &routeID ) const ;

   public:
      // format server node into string format
      ossPoolString toString() const ;

      // parse server node from BSON format
      INT32 fromBSON( const bson::BSONObj &nodeObject ) ;
      // format server node into BSON format
      // NOTE:
      // - forDisplay: TRUE for output to STP shell commands
      //               FALSE for internal messages
      INT32 toBSON( bson::BSONObjBuilder &nodeBuilder,
                    BOOLEAN forDisplay ) const ;

      // format server node into route node format ( used for replica )
      INT32 toRouteNode( netRouteNode &node ) const ;

      // check if it is valid route
      OSS_INLINE BOOLEAN isValidRole() const
      {
         return ( STP_ROLE_SERVER == _role ) ;
      }
   } ;

   /*
      _stpSourceNode define
    */
   class _stpSourceNode ;
   typedef class _stpSourceNode stpSourceNode ;
   typedef ossPoolMap< MsgRouteID,
                       stpSourceNode,
                       MsgRouteIDComp > STP_SOURCE_MAP ;
   typedef ossPoolVector< stpSourceNode > STP_SOURCE_LIST ;

   class _stpSourceNode : public stpServerNode
   {
   public:
      // constructor and destructor
      _stpSourceNode() ;
      _stpSourceNode( const stpSourceNode &source ) ;
      ~_stpSourceNode() ;

   public:
      // operators
      stpSourceNode &operator =( const stpSourceNode &source ) ;
      // NOTE: implement < operator to sort source by last synchronize time
      //       which could be used for get synchronize history queries
      BOOLEAN operator <( const stpSourceNode &source ) const ;

   public:
      // get functions
      OSS_INLINE UINT64 getLastSyncTick() const
      {
         return _lastSyncTick ;
      }

      OSS_INLINE const stpSyncStats &getCurStats() const
      {
         return _curStats ;
      }

      OSS_INLINE stpSyncStats &getCurStats()
      {
         return _curStats ;
      }

      OSS_INLINE const stpSyncStats &getHistStats() const
      {
         return _histStats ;
      }

   public:
      // on event register this source
      void onRegister() ;
      // on event sending synchronize request to this source
      void onPreSync() ;
      // on event receiving synchronize response from this source
      void onPostSync( const stpSyncRecord &record, BOOLEAN isValid ) ;


      void mergeStats() ;

      // format source node into BSON format
      // - forDisplay: TRUE for output to STP shell commands
      //               FALSE for internal messages
      INT32 toBSON( bson::BSONObjBuilder &nodeBuilder,
                    BOOLEAN current,
                    BOOLEAN forDisplay ) const ;

   protected:
      // last tick to synchronize with
      UINT64         _lastSyncTick ;
      // current synchronize statistics from this source
      // NOTE: will not always synchronize with only one source since the
      //       primary switch
      stpSyncStats _curStats ;
      // all history statistics from this source
      stpSyncStats _histStats ;
   } ;

   /*
      _stpClientNode define
    */
   class _stpClientNode ;
   typedef class _stpClientNode stpClientNode ;
   typedef ossPoolMap< MsgRouteID,
                       stpClientNode,
                       MsgRouteIDComp > STP_CLIENT_MAP ;

   class _stpClientNode : public stpNode
   {
   public:
      // constructor and destructor
      _stpClientNode() ;
      _stpClientNode( const stpClientNode &client ) ;
      ~_stpClientNode() ;

   public:
      // operators
      stpClientNode &operator =( const stpClientNode &client ) ;

   public:
      // get and set functions
      OSS_INLINE const bson::OID &getOID() const
      {
         return _oid ;
      }

      OSS_INLINE void setOID( const bson::OID &oid )
      {
         _oid = oid ;
      }

      OSS_INLINE STP_SYNC_STATUS getStatus() const
      {
         return _status ;
      }

      OSS_INLINE void setStatus( STP_SYNC_STATUS status )
      {
         _status = status ;
      }

      OSS_INLINE UINT32 getSyncInterval() const
      {
         return _syncInterval ;
      }

      OSS_INLINE void setSyncInterval( UINT32 syncInterval )
      {
         _syncInterval = syncInterval ;
      }

      OSS_INLINE UINT32 getMaxTimeError() const
      {
         return _maxTimeError ;
      }

      OSS_INLINE void setMaxTimeError( UINT32 maxTimeError )
      {
         _maxTimeError = maxTimeError ;
         // need cut time error
         if ( _timeError > maxTimeError )
         {
            _timeError = maxTimeError ;
         }
      }

      OSS_INLINE UINT32 getTimeError() const
      {
         return _timeError ;
      }

      OSS_INLINE void setTimeError( UINT32 timeError )
      {
         _timeError = timeError ;
      }

      OSS_INLINE UINT64 getLastSyncTick() const
      {
         return _lastSyncTick ;
      }

      OSS_INLINE void setLastSyncTime( UINT64 syncTime )
      {
         _lastSyncTick = syncTime ;
      }

      OSS_INLINE UINT64 getSyncCount() const
      {
         return _syncCount ;
      }

      OSS_INLINE void setSyncCount( UINT64 syncCount )
      {
         _syncCount = syncCount ;
      }

   public:
      // format client node into string format
      ossPoolString toString() const ;

      // parse client node from BSON format
      INT32 fromBSON( const bson::BSONObj &nodeObject ) ;
      // format client node into BSON format
      INT32 toBSON( bson::BSONObjBuilder &nodeBuilder,
                    BOOLEAN toDisplay ) const ;

      // check if role is valid
      OSS_INLINE BOOLEAN isValidRole() const
      {
         return ( STP_ROLE_SERVER == _role ||
                  STP_ROLE_CLIENT == _role ) ;
      }

      // check if OID is valid
      OSS_INLINE BOOLEAN isValidOID() const
      {
         return _oid.isSet() ;
      }

      // generate OID
      OSS_INLINE void generateOID()
      {
         _oid = bson::OID::gen() ;
      }

      // reset OID to invalid valie
      OSS_INLINE void resetOID()
      {
         _oid.clear() ;
      }

      // increase time error by step
      void incTimeError() ;
      // decrease time error by step
      void decTimeError() ;
      // on event receiving synchronize request from this client
      // with given status
      void onSync( STP_SYNC_STATUS status ) ;

      // calculate to increase time error by step
      static UINT32 getIncTimeError( UINT32 timeError,
                                     UINT32 maxTimeError,
                                     UINT32 step ) ;
      // calculate to decrease time error by step
      static UINT32 getDecTimeError( UINT32 timeError,
                                     UINT32 minTimeError,
                                     UINT32 step ) ;

   protected:
      // OID of client, used to verify if client reconnects or reboots
      bson::OID         _oid ;
      // synchronize status of client
      STP_SYNC_STATUS   _status ;
      // synchronize interval of client ( from in client's configs )
      UINT32            _syncInterval ;
      // maximum time error ( in nanoseconds ) allowed of client
      // ( from in client's configs )
      UINT32            _maxTimeError ;
      // current time error ( in nanoseconds ) of client
      UINT32            _timeError ;
      // last synchronize tick of client
      UINT64            _lastSyncTick ;
      // counts of synchronize from client
      UINT64            _syncCount ;
   } ;

}

#endif // STP_NOTE_HPP__
