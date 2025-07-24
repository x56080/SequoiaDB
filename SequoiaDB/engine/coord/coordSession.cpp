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

   Source File Name = coordSession.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of runtime component. This file contains code logic for
   common functions for coordinator node.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================

   Last Changed =

*******************************************************************************/
#include "coordSession.hpp"
#include "pmdDef.hpp"
#include "pdTrace.hpp"
#include "coordTrace.hpp"
#include "pmd.hpp"
#include "rtnCoordCommon.hpp"
#include "msgMessageFormat.hpp"
#include "../bson/bson.h"

using namespace bson ;

namespace engine
{

   /*
      CoordSession implement
    */
   CoordSession::CoordSession( pmdEDUCB *pEduCB )
   : _rtnSessionProperty()
   {
      pmdOptionsCB * optionCB = pmdGetKRCB()->getOptionCB() ;
      setInstanceOption( optionCB->getPrefInstStr(),
                         optionCB->getPrefInstModeStr(),
                         optionCB->getPreferedPeriod(),
                         PREFER_INSTANCE_TYPE_MASTER ) ;
      _pEduCB = pEduCB ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDSN_DISCONN, "CoordSession::disConnect" )
   INT32 CoordSession::disConnect( const MsgRouteID &routeID )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_COORDSN_DISCONN );
      BOOLEAN hasSession = FALSE;
      {
         ossScopedLock _lock( &_mutex ) ;
         COORD_SUBSESSION_MAP::iterator it;
         it = _subSessionMap.find( routeID.value );
         if ( it != _subSessionMap.end() )
         {
            hasSession = TRUE;
            it->second.isConnected = FALSE;
         }
      }

      if ( hasSession )
      {
         MsgOpReply *pMsg = (MsgOpReply *)SDB_OSS_MALLOC( sizeof(MsgOpReply) );
         if ( NULL != pMsg )
         {
            pMsg->header.messageLength = sizeof(MsgOpReply);
            pMsg->header.opCode = MSG_COM_REMOTE_DISC;
            pMsg->header.routeID = routeID;
            pMsg->header.requestID = pmdGetKRCB()->getCoordCB(
               )->getRouteAgent()->reqIDNew();
            pMsg->flags = SDB_COORD_REMOTE_DISC;
            pMsg->numReturned = 0;
            pMsg->contextID = -1 ;
            pMsg->startFrom = 0 ;
            _pEduCB->postEvent ( pmdEDUEvent( PMD_EDU_EVENT_MSG,
                                              PMD_EDU_MEM_ALLOC,
                                              pMsg ) ) ;
         }
         else
         {
            rc = SDB_OOM;
            PD_LOG( PDERROR, "malloc failed(size=%d)", sizeof(MsgHeader) );
         }
      }

      PD_TRACE_EXITRC ( SDB_COORDSN_DISCONN, rc );
      return rc;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDSN_SESSIONINIT, "CoordSession::sessionInit" )
   INT32 CoordSession::sessionInit( const MsgRouteID & routeID,
                                    const CHAR *pRemoteIP,
                                    UINT16 remotePort )
   {
      INT32 rc = SDB_OK;
      PD_TRACE_ENTRY ( SDB_COORDSN_SESSIONINIT ) ;
      CoordCB *pCoordCB = pmdGetKRCB()->getCoordCB() ;
      netMultiRouteAgent *pRouteAgent = pCoordCB->getRouteAgent() ;
      SDB_ASSERT( _pEduCB, "_pEduCB can't be NULL!" ) ;
      BSONObj objInfo ;
      CHAR *pBuff = NULL ;
      MsgComSessionInitReq *pInitReq = NULL ;
      UINT32 msgLength = sizeof( MsgComSessionInitReq ) ;

      REQUESTID_MAP requestIdMap ;
      REPLY_QUE replyQue ;

      UINT32 mask = 0 ;
      UINT32 configMask = 0 ;

      pdGetCurAuditMask( AUDIT_LEVEL_USER, mask, configMask ) ;

      /// construct info
      try
      {
         objInfo = BSON( SDB_AUTH_USER << _pEduCB->getUserName() <<
                         SDB_AUTH_PASSWD << _pEduCB->getPassword() <<
                         FIELD_NAME_HOST << pmdGetKRCB()->getHostName() <<
                         PMD_OPTION_SVCNAME << pmdGetOptionCB()->getServiceAddr() <<
                         FIELD_NAME_REMOTE_IP << pRemoteIP <<
                         FIELD_NAME_REMOTE_PORT << (INT32)remotePort <<
                         FIELD_NAME_AUDIT_MASK << (INT32)mask <<
                         FIELD_NAME_AUDIT_CONFIG_MASK << (INT32)configMask ) ;
         msgLength += objInfo.objsize() ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// allocate memory
      rc = _pEduCB->allocBuff( msgLength, &pBuff, NULL ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Alloc memory failed, size: %u, rc: %d",
                 msgLength, rc ) ;
         goto error ;
      }
      pInitReq = (MsgComSessionInitReq*)pBuff ;

      /// init message
      pInitReq->header.messageLength = msgLength ;
      pInitReq->header.opCode = MSG_COM_SESSION_INIT_REQ ;
      pInitReq->header.requestID = 0 ;
      pInitReq->header.routeID.value = 0 ;
      pInitReq->header.TID = _pEduCB->getTID() ;
      pInitReq->dstRouteID.value = routeID.value ;
      pInitReq->srcRouteID.value = pCoordCB->netWork()->localID().value ;
      pInitReq->localIP = _netFrame::getLocalAddress() ;
      pInitReq->peerIP = 0 ;
      pInitReq->localPort = pmdGetOptionCB()->getServicePort() ;
      pInitReq->peerPort = 0 ;
      pInitReq->localTID = _pEduCB->getTID() ;
      pInitReq->localSessionID = _pEduCB->getID() ;
      ossMemset( pInitReq->reserved, 0, sizeof( pInitReq->reserved ) ) ;
      ossMemcpy( pInitReq->data, objInfo.objdata(), objInfo.objsize() ) ;

      /// send message to peer
      rc = rtnCoordSendRequestToNodeWithoutCheck( (void *)pBuff, routeID,
                                                  pRouteAgent, _pEduCB,
                                                  requestIdMap ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to send the message to the node[%s], rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      /// get reply
      rc = rtnCoordGetReply( _pEduCB, requestIdMap, replyQue,
                             MSG_COM_SESSION_INIT_RSP ) ;
      PD_RC_CHECK( rc, PDERROR,
                   "Failed to get reply from node[%s], rc: %d",
                   routeID2String( routeID ).c_str(), rc ) ;

      /// analyse the reply
      while ( !replyQue.empty() )
      {
         MsgOpReply *pReply = NULL;
         pReply = (MsgOpReply *)(replyQue.front()) ;
         SDB_ASSERT( pReply, "pReply can't be NULL!" );
         replyQue.pop();
         rc = rc ? rc : pReply->flags ;
         SDB_OSS_FREE( pReply ) ;
      }

   done:
      if ( pBuff && _pEduCB )
      {
         _pEduCB->releaseBuff( pBuff ) ;
         pBuff = NULL ;
      }
      PD_TRACE_EXIT ( SDB_COORDSN_SESSIONINIT );
      return rc ;
   error:
      rtnCoordClearRequest( _pEduCB, requestIdMap ) ;
      goto done ;
   }

   void CoordSession::addSubSessionWithoutCheck( const MsgRouteID &routeID )
   {
      subSessionInfo subSession;
      subSession.routeID = routeID;
      subSession.isConnected = TRUE;
      ossScopedLock _lock( &_mutex ) ;
      _subSessionMap[routeID.value] = subSession;
   }

   //Note: addSubSession and delSubSession must be called by the same thread
   INT32 CoordSession::addSubSession( const MsgRouteID &routeID,
                                      ISession *pSession )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pRemoteIP = "" ;
      UINT16 remotePort = 0 ;
      COORD_SUBSESSION_MAP::iterator iterMap ;

      iterMap = _subSessionMap.find( routeID.value );
      if ( iterMap != _subSessionMap.end() &&
           TRUE == iterMap->second.isConnected )
      {
         goto done;
      }

      /// get remote info
      if ( pSession )
      {
         IClient *pClient = pSession->getClient() ;
         if ( pClient )
         {
            pRemoteIP = pClient->getFromIPAddr() ;
            remotePort = pClient->getFromPort() ;
         }
      }

      {
         subSessionInfo subSession;
         subSession.routeID = routeID;
         subSession.isConnected = TRUE;
         ossScopedLock _lock( &_mutex ) ;
         _subSessionMap[routeID.value] = subSession;
      }
      rc = sessionInit( routeID, pRemoteIP, remotePort );
      if ( rc )
      {
         {
            ossScopedLock _lock( &_mutex ) ;
            iterMap = _subSessionMap.find( routeID.value );
            if ( iterMap != _subSessionMap.end() )
            {
               iterMap->second.isConnected = FALSE;
            }
         }
         PD_LOG( PDERROR, "Init session with node[%s] failed, rc: %d",
                 routeID2String( routeID ).c_str(), rc ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   //Note: addSubSession and delSubSession must be called by the same thread
   BOOLEAN CoordSession::delSubSession( const MsgRouteID & routeID )
   {
      UINT32 num = 0;
      {
         ossScopedLock _lock( &_mutex ) ;
         num = _subSessionMap.erase( routeID.value );
      }
      if ( 0 == num )
      {
         return FALSE;
      }
      else
      {
         return TRUE;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDSN_GETALLSR, "CoordSession::getAllSessionRoute" )
   void CoordSession::getAllSessionRoute( ROUTE_SET & routeSet )
   {
      PD_TRACE_ENTRY ( SDB_COORDSN_GETALLSR );
      ossScopedLock _lock( &_mutex ) ;
      COORD_SUBSESSION_MAP::iterator it = _subSessionMap.begin();
      while ( it != _subSessionMap.end() )
      {
         if ( it->second.isConnected )
         {
            routeSet.insert( it->second.routeID.value );
         }
         ++it;
      }
      PD_TRACE_EXIT ( SDB_COORDSN_GETALLSR );
   }

   void CoordSession::postEvent( pmdEDUEvent const & data )
   {
      _pEduCB->postEvent( data );
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDSN_ISSUBSNCONN, "CoordSession::isSubsessionConnected" )
   BOOLEAN CoordSession::isSubsessionConnected( const MsgRouteID &routeID )
   {
      PD_TRACE_ENTRY ( SDB_COORDSN_ISSUBSNCONN );
      BOOLEAN hasConnected = FALSE;
      ossScopedLock _lock( &_mutex ) ;
      COORD_SUBSESSION_MAP::iterator it;
      it = _subSessionMap.find( routeID.value );
      if ( it != _subSessionMap.end() )
      {
         hasConnected = it->second.isConnected;
      }
      PD_TRACE_EXIT ( SDB_COORDSN_ISSUBSNCONN );
      return hasConnected;
   }

   INT32 CoordSession::addLastNode( const MsgRouteID &nodeID,
                                    BOOLEAN primaryRequest )
   {
      INT32 rc = SDB_OK ;

      try
      {
         COORD_LASTNODE_MAP::iterator iter =
                              _lastNodeMap.find( nodeID.columns.groupID ) ;
         if ( iter != _lastNodeMap.end() && !primaryRequest )
         {
            // found group
            coordLastNodeStatus &lastNodeStatus = iter->second ;
            if ( lastNodeStatus._nodeID.value != nodeID.value )
            {
               // not the same node, need update node ID
               lastNodeStatus._nodeID.value = nodeID.value ;
               // new node is used, save current tick, then make it
               // timeout after a preferred period
               lastNodeStatus._addTick = pmdGetDBTick() ;
            }
         }
         else
         {
            // if not found or primary required (write request),
            // add last node directly
            coordLastNodeStatus lastNodeStatus ;
            // set node ID
            lastNodeStatus._nodeID.value = nodeID.value ;
            // new node is used, save current tick, then make it
            // timeout after a preferred period
            lastNodeStatus._addTick = pmdGetDBTick() ;
            // save to the last node map
            _lastNodeMap[ nodeID.columns.groupID ] = lastNodeStatus ;
         }
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to save the last node for group [%u] "
                 "with node [%u], error: %s", nodeID.columns.groupID,
                 nodeID.columns.nodeID, e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   void CoordSession::removeLastNode( UINT32 groupID )
   {
      _lastNodeMap.erase( groupID ) ;
   }

   void CoordSession::removeLastNode( const MsgRouteID &nodeID )
   {
      COORD_LASTNODE_MAP::iterator it = _lastNodeMap.find( nodeID.columns.groupID ) ;
      if ( it != _lastNodeMap.end() &&
           it->second._nodeID.columns.nodeID == nodeID.columns.nodeID )
      {
         _lastNodeMap.erase( it ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_COORDSN_GETLASTND, "CoordSession::getLastNode" )
   UINT64 CoordSession::getLastNode( UINT32 groupID )
   {
      MsgRouteID nodeID ;

      PD_TRACE_ENTRY ( SDB_COORDSN_GETLASTND );

      COORD_LASTNODE_MAP::iterator iter = _lastNodeMap.find( groupID ) ;

      nodeID.value = MSG_INVALID_ROUTEID ;

      if ( iter != _lastNodeMap.end() )
      {
         nodeID.value = iter->second._nodeID.value ;
         // check if timeout against preferred period
         // - if period is zero, always timeout
         // - if added before a period (in second), make it timeout
         // NOTE: if period is -1, always not timeout
         if ( ( _instanceOption.getPreferedPeriod() == 0 ) ||
              ( _instanceOption.getPreferedPeriod() > 0 &&
                pmdGetTickSpanTime( iter->second._addTick ) >
                      ( (UINT64)( _instanceOption.getPreferedPeriod() ) *
                        OSS_ONE_SEC ) ) )
         {
            nodeID.value = MSG_INVALID_ROUTEID ;
            // remote node now
            _lastNodeMap.erase( iter ) ;
         }
      }

      PD_TRACE_EXIT ( SDB_COORDSN_GETLASTND ) ;

      return nodeID.value ;
   }

   void CoordSession::addRequest( const UINT64 reqID,
                                  const MsgRouteID &routeID,
                                  NET_HANDLE handle )
   {
      _requestMap[ reqID ] = coordRequestInfo( routeID, handle ) ;
   }

   void CoordSession::delRequest( const UINT64 reqID )
   {
      _requestMap.erase( reqID );
   }

   /*void CoordSession::delRequest( const MsgRouteID &routeID )
   {
      REQUESTID_MAP::iterator iterMap = _requestMap.begin();
      while( iterMap != _requestMap.end() )
      {
         if ( iterMap->second.value == routeID.value )
         {
            _requestMap.erase( iterMap++ );
            continue;
         }
         ++iterMap;
      }
   }*/

   void CoordSession::clearRequest()
   {
      _requestMap.clear();
   }

   BOOLEAN CoordSession::isValidResponse( const UINT64 reqID )
   {
      if ( _requestMap.find( reqID ) != _requestMap.end() )
      {
         return TRUE;
      }
      return FALSE;
   }

   BOOLEAN  CoordSession::isValidResponse( const MsgRouteID &routeID,
                                           const UINT64 reqID )
   {
      COORD_REQINFO_MAP_IT iterMap = _requestMap.begin();
      while( iterMap != _requestMap.end() )
      {
         if ( iterMap->second._id.value == routeID.value &&
              iterMap->first <= reqID )
         {
            return TRUE;
         }
         ++iterMap;
      }
      return FALSE;
   }

   BOOLEAN CoordSession::isValidResponse( const NET_HANDLE &handle,
                                          const UINT64 reqID )
   {
      COORD_REQINFO_MAP_IT iterMap = _requestMap.begin();
      while( iterMap != _requestMap.end() )
      {
         if ( iterMap->second._handle == handle &&
              iterMap->first <= reqID )
         {
            return TRUE;
         }
         ++iterMap;
      }
      return FALSE;
   }

   void CoordSession::_onSetInstance ()
   {
      _lastNodeMap.clear() ;
   }

}
