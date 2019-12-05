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

   Source File Name = tpNode.cpp

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

#include "tpNode.hpp"
#include "pdTrace.hpp"
#include "tpTrace.hpp"
#include "pmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _tpNode implement
    */
   _tpNode::_tpNode()
   : _role( TP_ROLE_STANDALONE )
   {
      _routeID.value = MSG_INVALID_ROUTEID ;
   }

   _tpNode::_tpNode( const tpNode &node )
   : _role( node._role ),
     _hostName( node._hostName ),
     _serviceName( node._serviceName )
   {
      _routeID.value = node._routeID.value ;
   }

   _tpNode::~_tpNode()
   {
   }

   tpNode &_tpNode::operator =( const tpNode &node )
   {
      _role = node._role ;
      _routeID.value = node._routeID.value ;
      _hostName = node._hostName ;
      _serviceName = node._serviceName ;

      return ( *this ) ;
   }

   /*
      _tpServerNode implement
    */
   _tpServerNode::_tpServerNode()
   : tpNode()
   {
      _role = TP_ROLE_SERVER ;
   }

   _tpServerNode::_tpServerNode( const tpServerNode &server )
   : _tpNode( server )
   {
   }

   _tpServerNode::~_tpServerNode ()
   {
   }

   tpServerNode &_tpServerNode::operator =( const tpServerNode &server )
   {
      _tpNode::operator =( server ) ;
      return ( *this ) ;
   }

   BOOLEAN _tpServerNode::operator ==( const tpServerNode &server ) const
   {
      return ( server.getRouteIDValue() == _routeID.value ) ;
   }

   BOOLEAN _tpServerNode::operator ==( const MsgRouteID &routeID ) const
   {
      return ( routeID.value == _routeID.value ) ;
   }

   ossPoolString _tpServerNode::toString() const
   {
      StringBuilder ss ;
      ss << "{ GroupID: " << _routeID.columns.groupID <<
            ", NodeID: " << _routeID.columns.nodeID <<
            ", Host: " << _hostName <<
            ", Service: " << _serviceName << " )" ;
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVERNODE_FROMBSON, "_tpServerNode::fromBSON" )
   INT32 _tpServerNode::fromBSON( const BSONObj &nodeObject )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVERNODE_FROMBSON ) ;

      MsgRouteID routeID ;
      UINT32 groupID = INVALID_GROUPID ;
      UINT16 nodeID = INVALID_NODEID ;
      const CHAR * hostName = NULL ;
      const CHAR * serviceName = NULL ;

      try
      {
         BSONElement element ;

         // group ID
         element = nodeObject.getField( TP_FIELD_NAME_GROUPID ) ;
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not found", TP_FIELD_NAME_GROUPID ) ;
         PD_CHECK( NumberInt == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not integer", TP_FIELD_NAME_GROUPID ) ;
         groupID = (UINT32)( element.numberInt() ) ;

         // node ID
         element = nodeObject.getField( TP_FIELD_NAME_NODEID ) ;
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not found", TP_FIELD_NAME_NODEID ) ;
         PD_CHECK( NumberInt == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not integer", TP_FIELD_NAME_NODEID ) ;
         nodeID = (UINT16)( element.numberInt() ) ;

         // host name
         element = nodeObject.getField( TP_FIELD_NAME_HOST ) ;
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not found", TP_FIELD_NAME_HOST ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not integer", TP_FIELD_NAME_HOST ) ;
         hostName = element.valuestrsafe() ;

         // service name
         element = nodeObject.getField( TP_FIELD_NAME_SERVICE ) ;
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not found", TP_FIELD_NAME_SERVICE ) ;
         PD_CHECK( String == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse server node object, "
                   "field [%s] is not integer", TP_FIELD_NAME_SERVICE ) ;
         serviceName = element.valuestrsafe() ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse server node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      PD_CHECK( INVALID_GROUPID != groupID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse server node object, "
                "group ID [%u] is invalid", groupID ) ;

      PD_CHECK( INVALID_NODEID != nodeID,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse server node object, "
                "node ID [%u] is invalid", nodeID ) ;

      PD_CHECK( NULL != hostName && '\0' != hostName,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse server node object, "
                "host name is invalid" ) ;

      PD_CHECK( NULL != serviceName && '\0' != serviceName,
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse server node object, "
                "service name is invalid" ) ;

      routeID.columns.groupID = groupID ;
      routeID.columns.nodeID = nodeID ;
      routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

      setRouteID( routeID ) ;
      setHostName( hostName ) ;
      setServiceName( serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__TPSERVERNODE_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVERNODE_TOBSON, "_tpServerNode::toBSON" )
   INT32 _tpServerNode::toBSON( BSONObjBuilder &nodeBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVERNODE_TOBSON ) ;

      try
      {
         nodeBuilder.append( TP_FIELD_NAME_GROUPID, (INT32)( getGroupID() ) ) ;
         nodeBuilder.append( TP_FIELD_NAME_NODEID, (INT32)( getNodeID() ) ) ;
         nodeBuilder.append( TP_FIELD_NAME_HOST, getHostName() ) ;
         nodeBuilder.append( TP_FIELD_NAME_SERVICE, getServiceName() ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build server node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSERVERNODE_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSERVERNODE_TOROUTENODE, "_tpServerNode::toRouteNode" )
   INT32 _tpServerNode::toRouteNode( netRouteNode &node ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSERVERNODE_TOROUTENODE ) ;

      // set host
      ossStrncpy( node._host, _hostName.c_str(), OSS_MAX_HOSTNAME ) ;
      node._host[ OSS_MAX_HOSTNAME ] = '\0' ;

      // set service
      node._service[ MSG_ROUTE_LOCAL_SERVICE ] = _serviceName.c_str() ;

      // set route ID
      node._id.value = _routeID.value ;

      PD_TRACE_EXITRC( SDB__TPSERVERNODE_TOROUTENODE, rc ) ;

      return rc ;
   }

   /*
      _tpSourceNode implement
    */
   _tpSourceNode::_tpSourceNode()
   : tpServerNode(),
     _lastSyncTick( 0LL )
   {
   }

   _tpSourceNode::_tpSourceNode( const tpSourceNode &source )
   : tpServerNode( source ),
     _lastSyncTick( source._lastSyncTick ),
     _curStats( source._curStats ),
     _histStats( source._histStats )
   {
   }

   _tpSourceNode::~_tpSourceNode()
   {
   }

   tpSourceNode &_tpSourceNode::operator =( const tpSourceNode &source )
   {
      tpServerNode::operator =( source ) ;

      _curStats = source._curStats ;
      _lastSyncTick = source._lastSyncTick ;

      return ( *this ) ;
   }

   BOOLEAN _tpSourceNode::operator <( const tpSourceNode &source ) const
   {
      return _lastSyncTick < source._lastSyncTick ;
   }

   void _tpSourceNode::onRegister()
   {
      mergeStats() ;
   }

   void _tpSourceNode::onPreSync()
   {
      _curStats.updateSync() ;
   }

   void _tpSourceNode::onPostSync( const tpSyncRecord &record,
                                   BOOLEAN isValid )
   {
      _curStats.updateStats( record, isValid ) ;
      _lastSyncTick = pmdGetDBTick() ;
   }

   void _tpSourceNode::mergeStats()
   {
      _histStats.updateStats( _curStats ) ;
      _curStats.reset() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPSOURCENODE_TOBSON, "_tpSourceNode::toBSON" )
   INT32 _tpSourceNode::toBSON( BSONObjBuilder &nodeBuilder,
                                BOOLEAN current ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPSOURCENODE_TOBSON ) ;

      try
      {
         rc = tpServerNode::toBSON( nodeBuilder ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build source node %s, "
                      "rc: %d", toString().c_str(), rc ) ;

         if ( current )
         {
            rc = _curStats.toBSON( nodeBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build current synchronize "
                         "statistics for source node %s, rc: %d",
                         toString().c_str(), rc ) ;
         }
         else
         {
            rc = _histStats.toBSON( nodeBuilder ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build history synchronize "
                         "statistics for source node %s, rc: %d",
                         toString().c_str(), rc ) ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build source node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPSOURCENODE_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _tpClientNode implement
    */
   _tpClientNode::_tpClientNode()
   : tpNode(),
     _status( TP_SYNC_NOSOURCE ),
     _syncInterval( TP_DEF_SYNC_INTERVAL ),
     _maxTimeError( TP_MAX_TIME_ERROR ),
     _timeError( TP_DEF_TIME_ERROR ),
     _lastSyncTick(),
     _syncCount( 0LL )
   {
   }

   _tpClientNode::_tpClientNode( const tpClientNode &client )
   : _tpNode( client ),
     _oid( client._oid ),
     _status( client._status ),
     _syncInterval( client._syncInterval ),
     _maxTimeError( client._maxTimeError ),
     _timeError( client._timeError ),
     _lastSyncTick( client._lastSyncTick ),
     _syncCount( client._syncCount )
   {
   }

   _tpClientNode::~_tpClientNode()
   {
   }

   tpClientNode &_tpClientNode::operator =( const tpClientNode &client )
   {
      tpNode::operator =( client ) ;

      _oid = client._oid ;
      _status = client._status ;
      _syncInterval = client._syncInterval ;
      _maxTimeError = client._maxTimeError ;
      _timeError = client._timeError ;
      _lastSyncTick = client._lastSyncTick ;
      _syncCount = client._syncCount ;

      return ( *this ) ;
   }

   ossPoolString _tpClientNode::toString() const
   {
      StringBuilder ss ;
      ss << "{ GroupID: " << _routeID.columns.groupID <<
            ", NodeID: " << _routeID.columns.nodeID <<
            ", Host: " << _hostName <<
            ", Service: " << _serviceName <<
            ", Role: " << tpGetRoleName( _role ) <<
            ", OID: " << _oid.toString() << " )" ;
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__TPCLIENTNODE_TOBSON, "_tpClientNode::toBSON" )
   INT32 _tpClientNode::toBSON( BSONObjBuilder &nodeBuilder ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__TPCLIENTNODE_TOBSON ) ;

      try
      {
         nodeBuilder.append( TP_FIELD_NAME_GROUPID, (INT32)( getGroupID() ) ) ;
         nodeBuilder.append( TP_FIELD_NAME_NODEID, (INT32)( getNodeID() ) ) ;
         nodeBuilder.append( TP_FIELD_NAME_HOST, getHostName() ) ;
         nodeBuilder.append( TP_FIELD_NAME_SERVICE, getServiceName() ) ;
         nodeBuilder.append( TP_FIELD_NAME_NODE_OID, getOID() ) ;
         nodeBuilder.append( TP_FIELD_NAME_SYNC_STATUS,
                             tpGetSyncStatusName( getStatus() ) ) ;
         nodeBuilder.append( TP_FIELD_NAME_SYNC_INTERVAL,
                             (INT32)getSyncInterval() ) ;
         nodeBuilder.append( TP_FIELD_NAME_TIME_ERROR,
                             (INT32)getTimeError() ) ;
         nodeBuilder.append( TP_FIELD_NAME_MAX_TIME_ERROR,
                             (INT32)getMaxTimeError() ) ;
         nodeBuilder.append( TP_FIELD_NAME_LAST_SYNC_PASSED,
                        (INT64)( pmdGetTickSpanTime( getLastSyncTick() ) ) ) ;
         nodeBuilder.append( TP_FIELD_NAME_SYNC_COUNT, (INT64)getSyncCount() ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build client node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__TPCLIENTNODE_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   void _tpClientNode::increaseTimeError()
   {
      _timeError = getIncTimeError( _timeError, _maxTimeError, 1 ) ;
   }

   void _tpClientNode::decreaseTimeError()
   {
      _timeError = getDecTimeError( _timeError, TP_MIN_TIME_ERROR, 1 ) ;
   }

   void _tpClientNode::onSyncReq( TP_SYNC_STATUS status )
   {
      _status = status ;
      _lastSyncTick = pmdGetDBTick() ;
      _syncCount ++ ;
   }

   void _tpClientNode::onSyncRsp()
   {
      _lastSyncTick = pmdGetDBTick() ;
      _syncCount ++ ;
   }

   UINT32 _tpClientNode::getIncTimeError( UINT32 timeError,
                                          UINT32 maxTimeError,
                                          UINT32 step )
   {
      double incFraction = 1.0 + (double)step * TP_TIME_ERROR_ADJUST_STEP ;
      UINT32 newTimeError = (UINT32)( (double)( timeError ) * incFraction ) ;
      return OSS_MIN( newTimeError, maxTimeError ) ;
   }


   UINT32 _tpClientNode::getDecTimeError( UINT32 timeError,
                                          UINT32 minTimeError,
                                          UINT32 step )
   {
      double decFraction = 1.0 - (double)step * TP_TIME_ERROR_ADJUST_STEP ;
      UINT32 newTimeError = (UINT32)( (double)( timeError ) * decFraction ) ;
      return OSS_MAX( newTimeError, minTimeError ) ;
   }

}
