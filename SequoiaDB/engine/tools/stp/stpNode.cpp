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

   Source File Name = stpNode.cpp

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
#include "stpNode.hpp"
#include "pdTrace.hpp"
#include "stpTrace.hpp"
#include "pmd.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _stpNode implement
    */
   _stpNode::_stpNode()
   : _role( STP_ROLE_SERVER )
   {
      _routeID.value = MSG_INVALID_ROUTEID ;
   }

   _stpNode::_stpNode( const stpNode &node )
   : _role( node._role ),
     _hostName( node._hostName ),
     _serviceName( node._serviceName )
   {
      _routeID.value = node._routeID.value ;
   }

   _stpNode::~_stpNode()
   {
   }

   stpNode &_stpNode::operator =( const stpNode &node )
   {
      _role = node._role ;
      _routeID.value = node._routeID.value ;
      _hostName = node._hostName ;
      _serviceName = node._serviceName ;

      return ( *this ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODE__FROMBSON, "_stpNode::_fromBSON" )
   INT32 _stpNode::_fromBSON( const BSONObj &nodeObject,
                              BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODE__FROMBSON ) ;

      STP_ROLE role = STP_ROLE_SERVER ;
      MsgRouteID routeID ;
      UINT32 groupID = INVALID_GROUPID ;
      UINT16 nodeID = INVALID_NODEID ;
      const CHAR * hostName = NULL ;
      const CHAR * serviceName = NULL ;

      try
      {
         BSONElement element ;

         if ( forDisplay )
         {
            // get role field
            element = nodeObject.getField( STP_FIELD_NAME_ROLE ) ;
            // role field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not found", STP_FIELD_NAME_ROLE ) ;
            // role field should be string
            PD_CHECK( String == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not a string", STP_FIELD_NAME_ROLE ) ;
            // get role
            role = stpGetRoleByName( element.valuestr() ) ;
         }
         else
         {
            // get role field
            element = nodeObject.getField( STP_FIELD_NAME_ROLE ) ;
            // role field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not found", STP_FIELD_NAME_ROLE ) ;
            // role field should be integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not an integer", STP_FIELD_NAME_ROLE ) ;
            // get role
            role = (STP_ROLE)( element.numberInt() ) ;

            // get group ID field
            element = nodeObject.getField( STP_FIELD_NAME_GROUPID ) ;
            // group ID field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not found", STP_FIELD_NAME_GROUPID ) ;
            // group ID field should be integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not an integer", STP_FIELD_NAME_GROUPID ) ;
            // get group ID
            groupID = (UINT32)( element.numberInt() ) ;

            // get node ID field
            element = nodeObject.getField( STP_FIELD_NAME_NODEID ) ;
            // node ID field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not found", STP_FIELD_NAME_NODEID ) ;
            // node ID field should be integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse node object, "
                      "field [%s] is not an integer", STP_FIELD_NAME_NODEID ) ;
            // get node ID
            nodeID = (UINT16)( element.numberInt() ) ;
         }

         // get host name field
         element = nodeObject.getField( STP_FIELD_NAME_HOST ) ;
         // host name field should not be empty
         PD_CHECK( EOO != element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "field [%s] is not found", STP_FIELD_NAME_HOST ) ;
         // host name field should be string type
         PD_CHECK( String == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "field [%s] is not a string", STP_FIELD_NAME_HOST ) ;
         // get host name
         hostName = element.valuestrsafe() ;

         // get service name field
         element = nodeObject.getField( STP_FIELD_NAME_SERVICE ) ;
         // service name field should not be empty
         PD_CHECK( EOO != element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "field [%s] is not found", STP_FIELD_NAME_SERVICE ) ;
         // service name field should be string type
         PD_CHECK( String == element.type(),
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "field [%s] is not a string", STP_FIELD_NAME_SERVICE ) ;
         // get service name
         serviceName = element.valuestrsafe() ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      if ( !forDisplay )
      {
         // check if role is valid
         PD_CHECK( stpCheckRole( role ), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "role [%d] is invalid", role ) ;

         // check if group ID is valid
         PD_CHECK( INVALID_GROUPID != groupID,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "group ID [%u] is invalid", groupID ) ;

         // check if node ID is valid
         PD_CHECK( INVALID_NODEID != nodeID,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "node ID [%u] is invalid", nodeID ) ;

         // check if host name is valid
         PD_CHECK( NULL != hostName && '\0' != hostName,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "host name is invalid" ) ;

         // check if service name is valid
         PD_CHECK( NULL != serviceName && '\0' != serviceName,
                   SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse node object, "
                   "service name is invalid" ) ;

         // set route ID
         // NOTE: always use local service as service ID
         routeID.columns.groupID = groupID ;
         routeID.columns.nodeID = nodeID ;
         routeID.columns.serviceID = MSG_ROUTE_LOCAL_SERVICE ;

         setRouteID( routeID ) ;
      }

      // set fields of node
      setRole( role ) ;
      setHostName( hostName ) ;
      setServiceName( serviceName ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPNODE__FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPNODE__TOBSON, "_stpNode::_toBSON" )
   INT32 _stpNode::_toBSON( BSONObjBuilder &nodeBuilder,
                            BOOLEAN forDisplay ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPNODE__TOBSON ) ;

      try
      {
         if ( forDisplay )
         {
            // to display, we use name of role
            nodeBuilder.append( STP_FIELD_NAME_ROLE,
                                stpGetRoleName( getRole() ) ) ;
         }
         else
         {
            // we use value of role for internal usage
            nodeBuilder.append( STP_FIELD_NAME_ROLE, (INT32)( getRole() ) ) ;

            // we need group ID and node ID for internal usage
            nodeBuilder.append( STP_FIELD_NAME_GROUPID,
                                (INT32)( getGroupID() ) ) ;
            nodeBuilder.append( STP_FIELD_NAME_NODEID,
                                (INT32)( getNodeID() ) ) ;
         }
         // build host name and service name
         nodeBuilder.append( STP_FIELD_NAME_HOST, getHostName() ) ;
         nodeBuilder.append( STP_FIELD_NAME_SERVICE, getServiceName() ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPNODE__TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpServerNode implement
    */
   _stpServerNode::_stpServerNode()
   : stpNode()
   {
      _role = STP_ROLE_SERVER ;
   }

   _stpServerNode::_stpServerNode( const stpServerNode &server )
   : stpNode( server )
   {
   }

   _stpServerNode::~_stpServerNode ()
   {
   }

   stpServerNode &_stpServerNode::operator =( const stpServerNode &server )
   {
      _stpNode::operator =( server ) ;
      return ( *this ) ;
   }

   BOOLEAN _stpServerNode::operator ==( const stpServerNode &server ) const
   {
      // only check value of route ID
      // if route ID is the same, they are the same server
      return ( server.getRouteIDValue() == _routeID.value ) ;
   }

   BOOLEAN _stpServerNode::operator ==( const MsgRouteID &routeID ) const
   {
      // only check value of route ID
      // if route ID is the same, they are the same server
      return ( routeID.value == _routeID.value ) ;
   }

   ossPoolString _stpServerNode::toString() const
   {
      StringBuilder ss ;
      ss << "{ GroupID: " << _routeID.columns.groupID <<
            ", NodeID: " << _routeID.columns.nodeID <<
            ", Host: " << _hostName <<
            ", Service: " << _serviceName << " )" ;
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVERNODE_FROMBSON, "_stpServerNode::fromBSON" )
   INT32 _stpServerNode::fromBSON( const BSONObj &nodeObject,
                                   BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVERNODE_FROMBSON ) ;

      // parse BSON object
      rc = _fromBSON( nodeObject, forDisplay ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON object of server node, "
                   "rc: %d", rc ) ;

      // check role
      PD_CHECK( STP_ROLE_SERVER == getRole(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse BSON object of server node, "
                "expected [%s] role, given [%s] role",
                stpGetRoleName( STP_ROLE_SERVER ),
                stpGetRoleName( getRole() ) ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVERNODE_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVERNODE_TOBSON, "_stpServerNode::toBSON" )
   INT32 _stpServerNode::toBSON( BSONObjBuilder &nodeBuilder,
                                 BOOLEAN forDisplay ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVERNODE_TOBSON ) ;

      rc = _toBSON( nodeBuilder, forDisplay ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build server node into "
                   "BSON object, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__STPSERVERNODE_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSERVERNODE_TOROUTENODE, "_stpServerNode::toRouteNode" )
   INT32 _stpServerNode::toRouteNode( netRouteNode &node ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSERVERNODE_TOROUTENODE ) ;

      // set host
      ossStrncpy( node._host, _hostName.c_str(), OSS_MAX_HOSTNAME ) ;
      node._host[ OSS_MAX_HOSTNAME ] = '\0' ;

      // set service
      node._service[ MSG_ROUTE_LOCAL_SERVICE ] = _serviceName.c_str() ;

      // set route ID
      node._id.value = _routeID.value ;

      PD_TRACE_EXITRC( SDB__STPSERVERNODE_TOROUTENODE, rc ) ;

      return rc ;
   }

   /*
      _stpSourceNode implement
    */
   _stpSourceNode::_stpSourceNode()
   : stpServerNode()
   {
   }

   _stpSourceNode::_stpSourceNode( const stpSourceNode &source )
   : stpServerNode( source ),
     _curStats( source._curStats ),
     _histStats( source._histStats )
   {
   }

   _stpSourceNode::~_stpSourceNode()
   {
   }

   stpSourceNode &_stpSourceNode::operator =( const stpSourceNode &source )
   {
      stpServerNode::operator =( source ) ;

      _curStats = source._curStats ;

      return ( *this ) ;
   }

   BOOLEAN _stpSourceNode::operator <( const stpSourceNode &source ) const
   {
      // sort by last synchronize time
      return getUpdateTick() < source.getUpdateTick() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSOURCENODE_ONREG, "_stpSourceNode::onRegister" )
   void _stpSourceNode::onRegister()
   {
      PD_TRACE_ENTRY( SDB__STPSOURCENODE_ONREG ) ;

      // on register, means this node becomes synchronize source again
      // merge last synchronize statistics into all history statistics
      mergeStats() ;

      PD_TRACE_EXIT( SDB__STPSOURCENODE_ONREG ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSOURCENODE_ONPRESYNC, "_stpSourceNode::onPreSync" )
   void _stpSourceNode::onPreSync()
   {
      PD_TRACE_ENTRY( SDB__STPSOURCENODE_ONPRESYNC ) ;

      // update synchronize count
      _curStats.incSyncCount() ;

      PD_TRACE_EXIT( SDB__STPSOURCENODE_ONPRESYNC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSOURCENODE_ONPOSTSYNC, "_stpSourceNode::onPostSync" )
   void _stpSourceNode::onPostSync( const stpSyncRecord &record,
                                    BOOLEAN isValid,
                                    UINT32 maxSyncHist )
   {
      PD_TRACE_ENTRY( SDB__STPSOURCENODE_ONPOSTSYNC ) ;

      // update current statistics
      _curStats.updateStats( record, isValid, maxSyncHist ) ;

      PD_TRACE_EXIT( SDB__STPSOURCENODE_ONPOSTSYNC ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSOURCENODE_MERGESTATS, "_stpSourceNode::mergeStats" )
   void _stpSourceNode::mergeStats()
   {
      PD_TRACE_ENTRY( SDB__STPSOURCENODE_MERGESTATS ) ;

      // merge current statistics into history statistics
      _histStats.updateStats( _curStats ) ;
      // clear current statistics
      _curStats.reset() ;
      // set update tick
      _curStats.setUpdateTick( pmdGetDBTick() ) ;

      PD_TRACE_EXIT( SDB__STPSOURCENODE_MERGESTATS ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSOURCENODE_TOBSON, "_stpSourceNode::toBSON" )
   INT32 _stpSourceNode::toBSON( BSONObjBuilder &nodeBuilder,
                                 BOOLEAN isCurrent,
                                 BOOLEAN forDisplay ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSOURCENODE_TOBSON ) ;

      try
      {
         // build server fields into BSON format
         // NOTE: no need to display role, always SERVER for source
         rc = stpServerNode::toBSON( nodeBuilder, forDisplay ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to build source node %s, "
                      "rc: %d", toString().c_str(), rc ) ;

         if ( isCurrent && forDisplay )
         {
            // only current statistics is needed, build current statistics
            // into BSON format
            rc = _curStats.toBSON( nodeBuilder, TRUE ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to build current synchronize "
                         "statistics for source node %s, rc: %d",
                         toString().c_str(), rc ) ;
         }
         else if ( forDisplay )
         {
            // need history statistics, build history statistics
            // into BSON format
            rc = _histStats.toBSON( nodeBuilder, FALSE ) ;
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
      PD_TRACE_EXITRC( SDB__STPSOURCENODE_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPSOURCENODE_FROMBSON, "_stpSourceNode::fromBSON" )
   INT32 _stpSourceNode::fromBSON( const BSONObj &object,
                                   BOOLEAN isCurrent,
                                   BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPSOURCENODE_FROMBSON ) ;

      try
      {
         // parse server fields from BSON
         rc = stpServerNode::fromBSON( object, forDisplay ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse source node from BSON, "
                      "rc: %d", rc ) ;

         if ( isCurrent )
         {
            // parse current statistics
            rc = _curStats.fromBSON( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse synchronize statistics "
                         "from BSON, rc: %d", rc ) ;
         }
         else
         {
            // parse history statistics
            rc = _histStats.fromBSON( object ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to parse synchronize statistics "
                         "from BSON, rc: %d", rc ) ;
         }

      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to parse source node from BSON object, "
                 "occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPSOURCENODE_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   /*
      _stpClientNode implement
    */
   _stpClientNode::_stpClientNode()
   : stpNode(),
     _status( STP_SYNC_NOSOURCE ),
     _syncInterval( STP_DEF_SYNC_INTERVAL ),
     _maxTimeError( STP_MAX_TIME_ERROR ),
     _timeError( STP_DEF_TIME_ERROR ),
     _lastSyncTick(),
     _syncCount( 0LL ),
     _syncPort( STP_INVALID_SYNCPORT )
   {
   }

   _stpClientNode::_stpClientNode( const stpClientNode &client )
   : _stpNode( client ),
     _oid( client._oid ),
     _status( client._status ),
     _syncInterval( client._syncInterval ),
     _maxTimeError( client._maxTimeError ),
     _timeError( client._timeError ),
     _lastSyncTick( client._lastSyncTick ),
     _syncCount( client._syncCount ),
     _syncPort( client._syncPort )
   {
   }

   _stpClientNode::~_stpClientNode()
   {
   }

   stpClientNode &_stpClientNode::operator =( const stpClientNode &client )
   {
      stpNode::operator =( client ) ;

      _oid = client._oid ;
      _status = client._status ;
      _syncInterval = client._syncInterval ;
      _maxTimeError = client._maxTimeError ;
      _timeError = client._timeError ;
      _lastSyncTick = client._lastSyncTick ;
      _syncCount = client._syncCount ;
      _syncPort = client._syncPort ;

      return ( *this ) ;
   }

   ossPoolString _stpClientNode::toString() const
   {
      StringBuilder ss ;
      ss << "{ GroupID: " << _routeID.columns.groupID <<
            ", NodeID: " << _routeID.columns.nodeID <<
            ", Host: " << _hostName <<
            ", Service: " << _serviceName <<
            ", Role: " << stpGetRoleName( _role ) <<
            ", OID: " << _oid.toString() << " )" ;
      return ss.poolStr() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTNODE_FROMBSON, "_stpClientNode::fromBSON" )
   INT32 _stpClientNode::fromBSON( const BSONObj &nodeObject,
                                   BOOLEAN forDisplay )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTNODE_FROMBSON ) ;

      OID oid ;
      STP_SYNC_STATUS syncStatus = STP_SYNC_NOSOURCE ;
      UINT32 syncInterval = STP_DEF_SYNC_INTERVAL ;
      UINT32 timeError = STP_DEF_TIME_ERROR ;
      UINT32 maxTimeError = STP_MAX_TIME_ERROR ;
      UINT16 syncPort = STP_INVALID_SYNCPORT ;
      UINT64 syncCount = 0LL ;
      UINT64 lastSyncTime = 0LL ;

      rc = _fromBSON( nodeObject, forDisplay ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to parse BSON object of client node, "
                   "rc: %d", rc ) ;

      try
      {
         BSONElement element ;

         // get OID field
         element = nodeObject.getField( STP_FIELD_NAME_NODE_OID ) ;
         // OID field should not be empty
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not found", STP_FIELD_NAME_NODE_OID ) ;
         // OID field should be OID type
         PD_CHECK( jstOID == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not an OID", STP_FIELD_NAME_NODE_OID ) ;
         // get OID
         oid = element.OID() ;

         if ( forDisplay )
         {
            // get synchronize status field
            element = nodeObject.getField( STP_FIELD_NAME_SYNC_STATUS ) ;
            // synchronize status field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not found", STP_FIELD_NAME_SYNC_STATUS ) ;
            // synchronize status field should be integer type
            PD_CHECK( String == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not a string",
                      STP_FIELD_NAME_SYNC_STATUS ) ;
            // get synchronize status
            syncStatus = stpGetSyncStatusByName( element.valuestr() ) ;

            // get synchronize port
            element = nodeObject.getField( STP_FIELD_NAME_SYNC_PORT ) ;
            // synchronize port field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not found", STP_FIELD_NAME_SYNC_PORT ) ;
            // synchronize port field should be an integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not an integer",
                      STP_FIELD_NAME_SYNC_PORT ) ;
            syncPort = element.numberInt() ;

            // get synchronize count
            element = nodeObject.getField( STP_FIELD_NAME_SYNC_COUNT ) ;
            // synchronize count field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not found", STP_FIELD_NAME_SYNC_COUNT ) ;
            // synchronize count field should be an long type
            PD_CHECK( NumberLong == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not a long integer",
                      STP_FIELD_NAME_SYNC_COUNT ) ;
            syncCount = (UINT64)( element.numberLong() ) ;

            // get last synchronize time
            element = nodeObject.getField( STP_FIELD_NAME_SYNC_PASSED ) ;
            // last synchronize time field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not found",
                      STP_FIELD_NAME_SYNC_PASSED ) ;
            // last synchronize time field should be an long type
            PD_CHECK( NumberLong == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not a long integer",
                      STP_FIELD_NAME_SYNC_PASSED ) ;
            lastSyncTime = (UINT64)( element.numberLong() ) ;
         }
         else
         {
            // get synchronize status field
            element = nodeObject.getField( STP_FIELD_NAME_SYNC_STATUS ) ;
            // synchronize status field should not be empty
            PD_CHECK( EOO != element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not found", STP_FIELD_NAME_SYNC_STATUS ) ;
            // synchronize status field should be integer type
            PD_CHECK( NumberInt == element.type(),
                      SDB_INVALIDARG, error, PDERROR,
                      "Failed to parse client node object, "
                      "field [%s] is not an integer",
                      STP_FIELD_NAME_SYNC_STATUS ) ;
            // get synchronize status
            syncStatus = (STP_SYNC_STATUS)( element.numberInt() ) ;
         }

         // get synchronize interval field
         element = nodeObject.getField( STP_FIELD_NAME_SYNC_INTERVAL ) ;
         // synchronize interval field should not be empty
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not found", STP_FIELD_NAME_SYNC_INTERVAL ) ;
         // synchronize interval field should be integer type
         PD_CHECK( NumberInt == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not an integer",
                   STP_FIELD_NAME_SYNC_INTERVAL ) ;
         // get synchronize interval
         syncInterval = (UINT32)( element.numberInt() ) ;

         // get time error field
         element = nodeObject.getField( STP_FIELD_NAME_TIME_ERROR ) ;
         // time error field should not be empty
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not found", STP_FIELD_NAME_TIME_ERROR ) ;
         // time error field should be integer type
         PD_CHECK( NumberInt == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not an integer",
                   STP_FIELD_NAME_TIME_ERROR ) ;
         // get time error
         timeError = (UINT32)( element.numberInt() ) ;

         // get max time error field
         element = nodeObject.getField( STP_FIELD_NAME_MAX_TIME_ERROR ) ;
         // max time error field should not be empty
         PD_CHECK( EOO != element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not found", STP_FIELD_NAME_MAX_TIME_ERROR ) ;
         // max time error field should be integer type
         PD_CHECK( NumberInt == element.type(), SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse client node object, "
                   "field [%s] is not an integer",
                   STP_FIELD_NAME_MAX_TIME_ERROR ) ;
         // get max time error
         maxTimeError = (UINT32)( element.numberInt() ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to parse client node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

      // check OID
      PD_CHECK( oid.isSet(), SDB_INVALIDARG, error, PDERROR,
                "Failed to parse client node object, OID is invalid" ) ;

      // check synchronize status
      PD_CHECK( stpCheckSyncStatus( syncStatus ),
                SDB_INVALIDARG, error, PDERROR,
                "Failed to parse client node object, "
                "synchronize status is invalid" ) ;

      // set fields of client node
      setOID( oid ) ;
      setStatus( syncStatus ) ;
      setSyncInterval( syncInterval ) ;
      setTimeError( timeError ) ;
      setMaxTimeError( maxTimeError ) ;

      if ( forDisplay )
      {
         setSyncPort( syncPort ) ;
         setSyncCount( syncCount ) ;
         setLastSyncTime( lastSyncTime ) ;
      }
      else
      {
         // not parsed from BSON, set to default value
         setSyncCount( 0LL ) ;
         setSyncPort( STP_INVALID_SYNCPORT ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTNODE_FROMBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTNODE_TOBSON, "_stpClientNode::toBSON" )
   INT32 _stpClientNode::toBSON( BSONObjBuilder &nodeBuilder,
                                 BOOLEAN forDisplay ) const
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__STPCLIENTNODE_TOBSON ) ;

      rc = _toBSON( nodeBuilder, forDisplay ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to build client node into "
                   "BSON object, rc: %d", rc ) ;

      try
      {
         // build client fields into BSON format
         nodeBuilder.append( STP_FIELD_NAME_NODE_OID, getOID() ) ;
         nodeBuilder.append( STP_FIELD_NAME_SYNC_INTERVAL,
                             (INT32)getSyncInterval() ) ;
         nodeBuilder.append( STP_FIELD_NAME_TIME_ERROR,
                             (INT32)getTimeError() ) ;
         nodeBuilder.append( STP_FIELD_NAME_MAX_TIME_ERROR,
                             (INT32)getMaxTimeError() ) ;
         if ( forDisplay )
         {
            // calculate time after last synchronize ( in milliseconds )
            INT64 syncPassed = pmdGetTickSpanTime( getLastSyncTick() ) ;
            nodeBuilder.append( STP_FIELD_NAME_SYNC_PASSED, syncPassed ) ;
            nodeBuilder.append( STP_FIELD_NAME_SYNC_COUNT,
                                (INT64)getSyncCount() ) ;
            // output name of synchronize status to display
            nodeBuilder.append( STP_FIELD_NAME_SYNC_STATUS,
                                stpGetSyncStatusName( getStatus() ) ) ;
            nodeBuilder.append( STP_FIELD_NAME_SYNC_PORT,
                                (UINT32)( getSyncPort() ) ) ;
         }
         else
         {
            // output value of synchronize status for internal usage
            nodeBuilder.append( STP_FIELD_NAME_SYNC_STATUS,
                                (INT32)( getStatus() ) ) ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "Failed to build client node object, "
                 "occurred unexpected error: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__STPCLIENTNODE_TOBSON, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTNODE_INCTIMEERROR, "_stpClientNode::incTimeError" )
   void _stpClientNode::incTimeError()
   {
      PD_TRACE_ENTRY( SDB__STPCLIENTNODE_INCTIMEERROR ) ;

      // increase time error by 1 step ( 10% )
      _timeError = getIncTimeError( _timeError, _maxTimeError, 1 ) ;

      PD_TRACE_EXIT( SDB__STPCLIENTNODE_INCTIMEERROR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTNODE_DECTIMEERROR, "_stpClientNode::decTimeError" )
   void _stpClientNode::decTimeError()
   {
      PD_TRACE_ENTRY( SDB__STPCLIENTNODE_DECTIMEERROR ) ;

      // decrease time error by 1 step ( 10% )
      _timeError = getDecTimeError( _timeError, STP_MIN_TIME_ERROR, 1 ) ;

      PD_TRACE_EXIT( SDB__STPCLIENTNODE_DECTIMEERROR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__STPCLIENTNODE_ONSYNC, "_stpClientNode::onSync" )
   void _stpClientNode::onSync( STP_SYNC_STATUS status )
   {
      PD_TRACE_ENTRY( SDB__STPCLIENTNODE_ONPRESYNC ) ;

      // update status
      _status = status ;
      // update synchronize tick
      _lastSyncTick = pmdGetDBTick() ;
      // increase synchronize count
      _syncCount ++ ;

      PD_TRACE_EXIT( SDB__STPCLIENTNODE_ONPRESYNC ) ;
   }

   UINT32 _stpClientNode::getIncTimeError( UINT32 timeError,
                                           UINT32 maxTimeError,
                                           UINT32 step )
   {
      // increase by step * 10%
      FLOAT64 incFraction = 1.0 + (FLOAT64)step * STP_TIME_ERROR_ADJUST_STEP ;
      // calculate new time error
      UINT32 newTimeError = (UINT32)( (FLOAT64)( timeError ) * incFraction ) ;
      // cut by maximum time error
      return OSS_MIN( newTimeError, maxTimeError ) ;
   }


   UINT32 _stpClientNode::getDecTimeError( UINT32 timeError,
                                           UINT32 minTimeError,
                                           UINT32 step )
   {
      // decrease by step * 10%
      FLOAT64 decFraction = 1.0 - (FLOAT64)step * STP_TIME_ERROR_ADJUST_STEP ;
      // calculate new time error
      UINT32 newTimeError = (UINT32)( (FLOAT64)( timeError ) * decFraction ) ;
      // cut by minimum time error
      return OSS_MAX( newTimeError, minTimeError ) ;
   }

}
