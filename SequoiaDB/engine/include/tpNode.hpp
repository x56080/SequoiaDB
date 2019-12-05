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

   Source File Name = tpNode.hpp

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

#ifndef TP_NODE_HPP__
#define TP_NODE_HPP__

#include "tpCBCommon.hpp"
#include "msgDef.hpp"
#include "netDef.hpp"
#include "utilPooledObject.hpp"
#include "ossMemPool.hpp"
#include "tpSyncStats.hpp"
#include "../bson/bson.hpp"

namespace engine
{

   /*
      _tpNode define
    */
   class _tpNode ;
   typedef class _tpNode tpNode ;

   class _tpNode : public utilPooledObject
   {
   public:
      _tpNode() ;
      _tpNode( const tpNode &node ) ;
      ~_tpNode() ;

   public:
      tpNode &operator =( const tpNode &node ) ;

   public:
      OSS_INLINE TP_ROLE getRole() const
      {
         return _role ;
      }

      OSS_INLINE void setRole( TP_ROLE role )
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
      OSS_INLINE BOOLEAN isValidRoute() const
      {
         return ( MSG_INVALID_ROUTEID != _routeID.value &&
                  INVALID_GROUPID != _routeID.columns.groupID &&
                  INVALID_NODEID != _routeID.columns.nodeID &&
                  MSG_ROUTE_LOCAL_SERVICE == _routeID.columns.serviceID ) ;
      }

      OSS_INLINE UINT32 getGroupID() const
      {
         return _routeID.columns.groupID ;
      }

      OSS_INLINE UINT16 getNodeID() const
      {
         return _routeID.columns.nodeID ;
      }

      OSS_INLINE UINT16 getServiceID() const
      {
         return _routeID.columns.serviceID ;
      }

      OSS_INLINE UINT64 getRouteIDValue() const
      {
         return _routeID.value ;
      }

   protected:
      TP_ROLE        _role ;
      MsgRouteID     _routeID ;
      ossPoolString  _hostName ;
      ossPoolString  _serviceName ;
   } ;

   /*
      _tpServerNode
    */
   class _tpServerNode ;
   typedef class _tpServerNode tpServerNode ;
   typedef ossPoolList< tpServerNode > TP_SERVER_LIST ;

   class _tpServerNode : public tpNode
   {
   public:
      _tpServerNode() ;
      _tpServerNode( const tpServerNode &server ) ;
      ~_tpServerNode() ;

   public:
      tpServerNode &operator =( const tpServerNode &server ) ;
      BOOLEAN operator ==( const tpServerNode &server ) const ;
      BOOLEAN operator ==( const MsgRouteID &routeID ) const ;

   public:
      ossPoolString toString() const ;

      INT32 fromBSON( const bson::BSONObj &nodeObject ) ;
      INT32 toBSON( bson::BSONObjBuilder &nodeBuilder ) const ;
      INT32 toRouteNode( netRouteNode &node ) const ;

      OSS_INLINE BOOLEAN isValidRole() const
      {
         return ( TP_ROLE_SERVER == _role ) ;
      }
   } ;

   /*
      _tpSourceNode define
    */
   class _tpSourceNode ;
   typedef class _tpSourceNode tpSourceNode ;
   typedef ossPoolMap< MsgRouteID,
                       tpSourceNode,
                       MsgRouteIDComp > TP_SOURCE_MAP ;
   typedef ossPoolVector< tpSourceNode > TP_SOURCE_LIST ;

   class _tpSourceNode : public tpServerNode
   {
   public:
      _tpSourceNode() ;
      _tpSourceNode( const tpSourceNode &source ) ;
      ~_tpSourceNode() ;

   public:
      tpSourceNode &operator =( const tpSourceNode &source ) ;
      BOOLEAN operator <( const tpSourceNode &source ) const ;

   public:
      OSS_INLINE UINT64 getLastSyncTick() const
      {
         return _lastSyncTick ;
      }

      OSS_INLINE const tpSyncStats &getCurStats() const
      {
         return _curStats ;
      }

      OSS_INLINE tpSyncStats &getCurStats()
      {
         return _curStats ;
      }

   public:
      void onRegister() ;
      void onPreSync() ;
      void onPostSync( const tpSyncRecord &record, BOOLEAN isValid ) ;
      void mergeStats() ;

      INT32 toBSON( bson::BSONObjBuilder &nodeBuilder,
                    BOOLEAN current ) const ;

   protected:
      UINT64         _lastSyncTick ;
      tpSyncStats    _curStats ;
      tpSyncStats    _histStats ;
   } ;

   /*
      _tpClientNode define
    */
   class _tpClientNode ;
   typedef class _tpClientNode tpClientNode ;
   typedef ossPoolMap< MsgRouteID,
                       tpClientNode,
                       MsgRouteIDComp > TP_CLIENT_MAP ;

   class _tpClientNode : public tpNode
   {
   public:
      _tpClientNode() ;
      _tpClientNode( const tpClientNode &client ) ;
      ~_tpClientNode() ;

   public:
      tpClientNode &operator =( const tpClientNode &client ) ;

   public:
      OSS_INLINE const bson::OID &getOID() const
      {
         return _oid ;
      }

      OSS_INLINE void setOID( const bson::OID &oid )
      {
         _oid = oid ;
      }

      OSS_INLINE TP_SYNC_STATUS getStatus() const
      {
         return _status ;
      }

      OSS_INLINE void setStatus( TP_SYNC_STATUS status )
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
      ossPoolString toString() const ;
      INT32 toBSON( bson::BSONObjBuilder &nodeBuilder ) const ;

      OSS_INLINE BOOLEAN isValidRole() const
      {
         return ( TP_ROLE_SERVER == _role ||
                  TP_ROLE_CLIENT == _role ) ;
      }

      OSS_INLINE BOOLEAN isValidOID() const
      {
         return _oid.isSet() ;
      }

      OSS_INLINE void generateOID()
      {
         _oid = bson::OID::gen() ;
      }

      OSS_INLINE void resetOID()
      {
         _oid.clear() ;
      }

      void increaseTimeError() ;
      void decreaseTimeError() ;
      void onSyncReq( TP_SYNC_STATUS status ) ;
      void onSyncRsp() ;

      static UINT32 getIncTimeError( UINT32 timeError,
                                     UINT32 maxTimeError,
                                     UINT32 step ) ;
      static UINT32 getDecTimeError( UINT32 timeError,
                                     UINT32 minTimeError,
                                     UINT32 step ) ;

   protected:
      bson::OID            _oid ;
      TP_SYNC_STATUS       _status ;
      UINT32               _syncInterval ;
      UINT32               _maxTimeError ;
      UINT32               _timeError ;
      UINT64               _lastSyncTick ;
      UINT64               _syncCount ;
   } ;

}

#endif // TP_NOTE_HPP__
