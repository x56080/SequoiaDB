/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnCoordLobStream.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/10/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnCoordLobStream.hpp"
#include "rtnTrace.hpp"
#include "msgDef.hpp"
#include "rtnCoordCommon.hpp"
#include "pmd.hpp"
#include "msgMessage.hpp"
#include "coordSession.hpp"

using namespace bson ;

#define RTN_COORD_LOB_GET_SUBSTREAM( groupID, s ) \
        do\
        {\
           SUB_STREAMS::iterator itr = _subs.find( groupID ) ;\
           if ( _subs.end() == itr )\
           {\
              PD_LOG( PDERROR, "group:%d is not in sub streams", groupID ) ;\
              rc = SDB_SYS ;\
              goto error ;\
           }\
           s = &( itr->second ) ;\
        } while ( FALSE )

namespace engine
{

   #define LOB_MAX_RETRYTIMES                ( 5 )

   static BOOLEAN _isRetry( UINT32 times )
   {
      return times == 0 ? TRUE : FALSE ;
   }

   static BOOLEAN _canRetry( UINT32 times )
   {
      return times < LOB_MAX_RETRYTIMES ? TRUE : FALSE ;
   }

   _rtnCoordLobStream::_rtnCoordLobStream()
   :_metaGroup( 0 ),
    _alignBuf( 0 )
   {
      _pageSize = 0 ;
   }

   _rtnCoordLobStream::~_rtnCoordLobStream()
   {

   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__PREPARE, "_rtnCoordLobStream::_prepare" )
   INT32 _rtnCoordLobStream::_prepare( const CHAR *fullName,
                                       const bson::OID &oid,
                                       INT32 mode,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__PREPARE ) ;

      rc = _updateCataInfo( FALSE, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to update catalog info of:%s, rc:%d",
                 fullName, rc ) ;
         goto error ;
      }

      rc = _openSubStreams( fullName, oid, mode, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open sub streams:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__PREPARE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__UPDATECATAINFO, "_rtnCoordLobStream::_openSubStreams" )
   INT32 _rtnCoordLobStream::_updateCataInfo( BOOLEAN refresh,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__UPDATECATAINFO ) ;
      rc = rtnCoordGetCataInfo( cb, getFullName(), refresh, _cataInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get catalog info of:%s, rc:%d",
                 getFullName(), rc ) ;
         goto error ;
      }

      if ( _cataInfo->isMainCL() )
      {
         PD_LOG( PDERROR, "can not open a lob in main cl" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( _cataInfo->isRangeSharded() )
      {
         PD_LOG( PDERROR, "can not open a lob in range sharded cl" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else
      {
         rc = _cataInfo->getLobGroupID( getOID(),
                                        DMS_LOB_META_SEQUENCE,
                                        _metaGroup ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get meta group:%d", rc ) ;
            goto error ;
         }
         // get group info
         rc = rtnCoordGetGroupInfo( cb, _metaGroup, FALSE,
                                    _metaGroupInfo, NULL ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Get meta group info[%u] failed, rc: %d",
                    _metaGroup, rc ) ;
            goto error ;
         }
         _mapGroupInfo[ _metaGroup ] = _metaGroupInfo ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__UPDATECATAINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__OPENSUBSTREAMS, "_rtnCoordLobStream::_openSubStreams" )
   INT32 _rtnCoordLobStream::_openSubStreams( const CHAR *fullName,
                                              const bson::OID &oid,
                                              INT32 mode,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__OPENSUBSTREAMS ) ;
      rc = _openMainStream( fullName, oid, mode, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open main stream:%d", rc ) ;
         goto error ;
      }

      rc = _openOtherStreams( fullName, oid, mode, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open other streams:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__OPENSUBSTREAMS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__OPENOTHERSTREAMS, "_rtnCoordLobStream::_openOtherStreams" )
   INT32 _rtnCoordLobStream::_openOtherStreams( const CHAR *fullName,
                                                const bson::OID &oid,
                                                INT32 mode,
                                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      INT32 rcTmp = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__OPENOTHERSTREAMS ) ;

      MsgOpLob header ;
      CoordGroupList gpLst ;
      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      BSONObjBuilder builder ;
      BSONObj obj ;
      netIOVec iov ;
      UINT32 retryTime = 0 ;
      CoordGroupMap::iterator itMap ;

      builder.append( FIELD_NAME_COLLECTION, fullName )
             .append( FIELD_NAME_LOB_OID, oid )
             .append( FIELD_NAME_LOB_OPEN_MODE, mode )
             .appendBool( FIELD_NAME_LOB_IS_MAIN_SHD, FALSE ) ;
      if ( SDB_LOB_MODE_R == mode )
      {
         /// send meta data to every group.
         builder.append( FIELD_NAME_LOB_META_DATA, _metaObj ) ;
      }

      obj = builder.obj() ;

      _initHeader( header,
                   MSG_BS_LOB_OPEN_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      _pushLobHeader( &header, obj, iov ) ;

      do
      {
         INT32 tag = RETRY_TAG_NULL ;
         _clearMsgData() ;
         _cataInfo->getGroupLst( gpLst ) ;
         SDB_ASSERT( 1 == gpLst.count( _metaGroup ), "impossible" ) ;

         rc = rtnGroupList2GroupPtr( cb, gpLst, _mapGroupInfo, FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Get groups info failed, rc: %d", rc ) ;
            goto error ;
         }

         header.version = _cataInfo->getVersion() ;

         for ( CoordGroupList::const_iterator itr = gpLst.begin();
               itr != gpLst.end();
               ++itr )
         {
            if ( 0 < _subs.count( itr->first ) )
            {
               continue ;
            }

            itMap = _mapGroupInfo.find( itr->first ) ;
            if ( _mapGroupInfo.end() == itMap )
            {
               SDB_ASSERT( FALSE, "Group info is not exist" ) ;
               rc = SDB_COOR_NO_NODEGROUP_INFO ;
               goto error ;
            }

            rc = rtnCoordSendRequestToNodeGroup( &( header.header ),
                                                 itMap->second,
                                                 SDB_LOB_MODE_R != mode,
                                                 routeAgent,
                                                 cb, iov,
                                                 _sendMap,
                                                 _isRetry( retryTime ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to send open msg to group:%d, rc:%d",
                       itr->first, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), FALSE, tag ) ;
         /// need to add succeed nodes to subs whether successful or not,
         /// because it can close all opened contexts that on data node
         /// when some nodes failed
         rcTmp = _addSubStreamsFromReply() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }
         if ( SDB_OK != rcTmp )
         {
            rc = rcTmp ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else if ( RETRY_TAG_REOPEN & tag )
         {
            rc = _closeSubStreams( cb, TRUE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to close sub streams: %d", rc ) ;
               goto error ;
            }
         }
         else
         {
            continue ;
         }
      } while ( TRUE ) ;

   done:
      _clearMsgData() ; 
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__OPENOTHERSTREAMS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__OPENMAINSTREAM, "_rtnCoordLobStream::_openMainStream" )
   INT32 _rtnCoordLobStream::_openMainStream( const CHAR *fullName,
                                              const bson::OID &oid,
                                              INT32 mode,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__OPENMAINSTREAM ) ;

      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      MsgOpLob header ;
      const MsgOpReply *reply = NULL ;
      BSONObj obj ;
      BSONObjBuilder builder ;
      netIOVec iov ;
      UINT32 retryTime = 0 ;
      _rtnLobDataPool::tuple dataTuple ;

      builder.append( FIELD_NAME_COLLECTION, fullName )
             .append( FIELD_NAME_LOB_OID, oid )
             .append( FIELD_NAME_LOB_OPEN_MODE, mode )
             .appendBool( FIELD_NAME_LOB_IS_MAIN_SHD, TRUE ) ;
      obj = builder.obj() ;

      _initHeader( header, MSG_BS_LOB_OPEN_REQ,
                   ossRoundUpToMultipleX( obj.objsize(), 4 ),
                   -1 ) ;
      _pushLobHeader( &header, obj, iov ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = rtnCoordSendRequestToNodeGroup( &( header.header ),
                                              _metaGroupInfo,
                                              SDB_LOB_MODE_R != mode,
                                              routeAgent,
                                              cb, iov,
                                              _sendMap,
                                              _isRetry( retryTime ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to send open msg to group:%d, rc:%d",
                    _metaGroup, rc ) ;
            goto error ;
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), FALSE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }
         else if ( RETRY_TAG_NULL == tag )
         {
            SDB_ASSERT( 1 == _results.size(), "impossible" ) ;
            reply = _results.empty() ? NULL : *( _results.begin() ) ;
            break ;
         }
         else
         {
            continue ;
         }
      } while ( TRUE ) ;

      _add2Subs( reply->header.routeID.columns.groupID,
                 reply->contextID, reply->header.routeID ) ;

      rc = _extractMeta( reply, _metaObj, dataTuple ) ;
      if ( dataTuple.data && dataTuple.len > 0 )
      {
         /// push data to pool
         rc = _getPool().push( dataTuple.data,
                               dataTuple.len,
                               dataTuple.offset ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Push data to pool failed, rc:%d", rc ) ;
            goto error ;
         }
         _getPool().pushDone() ;

         _getPool().entrust( ( CHAR * )( *_results.begin() ) ) ;
         _results.erase( _results.begin() ) ;
      }
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract meta data from reply msg:%d",
                 rc ) ;
         goto error ;
      }

   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__OPENMAINSTREAM, rc ) ;
      return rc ;
   error:
      _getPool().clear() ;
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__QUERYMETA, "_rtnCoordLobStream::_queryLobMeta" )
   INT32 _rtnCoordLobStream::_queryLobMeta( _pmdEDUCB *cb,
                                            _dmsLobMeta &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__QUERYMETA ) ;

      try
      { 
         BSONElement ele = _metaObj.getField( FIELD_NAME_LOB_SIZE ) ;
         if ( NumberLong != ele.type() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         meta._lobLen = ele.Long() ;

         ele = _metaObj.getField( FIELD_NAME_LOB_CREATTIME ) ;
         if ( NumberLong != ele.type() )
         {
            PD_LOG( PDERROR, "invalid meta obj:%s",
                    _metaObj.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         meta._createTime = ele.Long() ;

         ele = _metaObj.getField( FIELD_NAME_VERSION ) ;
         if ( NumberInt == ele.type() )
         {
            meta._version = (UINT8)ele.numberInt() ;
         }

         meta._status = DMS_LOB_COMPLETE ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__QUERYMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCoordLobStream::_ensureLob( _pmdEDUCB *cb,
                                         _dmsLobMeta &meta,
                                         BOOLEAN &isNew )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( SDB_LOB_MODE_CREATEONLY == _getMode(), "should not hit here" ) ;
      isNew = TRUE ;
      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__GETLOBPAGESIZE, "_rtnCoordLobStream::_getLobPageSize" )
   INT32 _rtnCoordLobStream::_getLobPageSize( INT32 &pageSize )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__GETLOBPAGESIZE ) ;

      if ( 0 == _pageSize )
      {
         try
         {
            BSONElement ele = _metaObj.getField( FIELD_NAME_LOB_PAGE_SIZE ) ;
            if ( NumberInt != ele.type() )
            {
               PD_LOG( PDERROR, "invalid meta obj:%s",
                       _metaObj.toString( FALSE, TRUE ).c_str() ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            _pageSize = ele.Int() ;
         }
         catch ( std::exception &e )
         {
            PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }
      pageSize = _pageSize ;

   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__GETLOBPAGESIZE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__WRITE, "_rtnCoordLobStream::_write" )
   INT32 _rtnCoordLobStream::_write( const _rtnLobTuple &tuple,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__WRITE ) ;
      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      MsgOpLob header ;
      UINT32 groupID = 0 ;
      const subStream *sub = NULL ;
      netIOVec iov ;
      UINT32 retryTime = 0 ;

      _initHeader( header, MSG_BS_LOB_WRITE_REQ,
                   0, -1,
                   sizeof( header ) +
                   sizeof( _MsgLobTuple ) +
                   tuple.tuple.columns.len ) ;
      _pushLobHeader( &header, BSONObj(), iov ) ;
      _pushLobData( tuple.tuple.data, sizeof( tuple.tuple ), iov ) ;
      _pushLobData( tuple.data, tuple.tuple.columns.len, iov ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         rc = _cataInfo->getLobGroupID( getOID(),
                                        tuple.tuple.columns.sequence,
                                        groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
            goto error ;
         }

         RTN_COORD_LOB_GET_SUBSTREAM( groupID, sub ) ;
         header.contextID = sub->contextID ;
         rc = rtnCoordSendRequestToNode( &( header.header ),
                                         sub->id,
                                         routeAgent,
                                         cb,
                                         iov,
                                         _sendMap ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to send msg to node[%d:%hd], rc:%d",
                    sub->id.columns.groupID, sub->id.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), TRUE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else 
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__WRITEV, "_rtnCoordLobStream::_writev" )
   INT32 _rtnCoordLobStream::_writev( const RTN_LOB_TUPLES &tuples,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__WRITEV ) ;

      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      DONE_LST doneLst ;
      BOOLEAN reshard = TRUE ;
      MsgOpLob header ;
      UINT32 retryTime = 0 ;

      /// will reassign length
      _initHeader( header, MSG_BS_LOB_WRITE_REQ,
                   0, -1 ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         if ( reshard )
         {
            rc = _shardData( header, tuples, TRUE, doneLst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard pieces:%d", rc ) ;
               goto error ;
            }

            reshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;

            if ( !dg.hasData() )
            {
               continue ;
            }

            /// we have pushed part of header to body.
            header.header.messageLength = sizeof( MsgHeader ) +
                                           dg.bodyLen ;
            header.contextID = dg.contextID ;

            rc = rtnCoordSendRequestToNode( &( header.header ),
                                            dg.id,
                                            routeAgent,
                                            cb,
                                            dg.body,
                                            _sendMap ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to send msg to node[%d:%hd], rc:%d",
                    dg.id.columns.groupID, dg.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), TRUE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLstFromReply( doneLst ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
            reshard = TRUE ;
         }
      } while ( TRUE ) ;

   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__WRITEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__READV, "_rtnCoordLobStream::_readv" )
   INT32 _rtnCoordLobStream::_readv( const RTN_LOB_TUPLES &tuples,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__READV ) ;
      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      DONE_LST doneLst ; 
      BOOLEAN needReshard = TRUE ;
      MsgOpLob header ;
      UINT32 retryTime = 0 ;
      _initHeader( header, MSG_BS_LOB_READ_REQ,
                   0, -1 ) ;
      
      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         if ( needReshard )
         {
            rc = _shardData( header, tuples, FALSE, doneLst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard pieces:%d", rc ) ;
               goto error ;
            }
            needReshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;
            if ( !dg.hasData() )
            {
               continue ;
            }

            header.header.messageLength = sizeof( MsgHeader ) +
                                           dg.bodyLen ;
            header.contextID = dg.contextID ;

            rc = rtnCoordSendRequestToNode( &( header.header ),
                                            dg.id,
                                            routeAgent,
                                            cb,
                                            dg.body,
                                            _sendMap ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to send msg to node[%d:%hd], rc:%d",
                    dg.id.columns.groupID, dg.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), FALSE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }

         rc = _handleReadResults( cb, doneLst ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            _getPool().pushDone() ;
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
            needReshard = TRUE ;
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__READV, rc ) ;
      return rc ;
   error:
      _getPool().clear() ;
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__PUSH2POOL, "_rtnCoordLobStream::_push2Pool" )
   INT32 _rtnCoordLobStream::_push2Pool( const MsgOpReply *header )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__PUSH2POOL ) ;

      const MsgLobTuple *begin = NULL ;
      UINT32 tupleSz = 0 ;
      const MsgLobTuple *curTuple = NULL ;
      const CHAR *data = NULL ;
      BOOLEAN got = FALSE ;
      INT32 pageSz = 0 ;
      rc = _getLobPageSize( pageSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get page size of lob:%d", rc ) ;
         goto error ;
      }

      rc = msgExtractReadResult( header, &begin, &tupleSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to extract read result:%d", rc ) ;
         goto error ;
      }

      while ( TRUE )
      {
         rc = msgExtractTuplesAndData( &begin, &tupleSz,
                                       &curTuple, &data, &got ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to extract tuple from msg:%d", rc ) ;
            goto error ;
         }
         else if ( got )
         {
            if ( _getMeta()._version <  DMS_LOB_CURRENT_VERSION &&
                 0 == curTuple->columns.sequence )
            {
               PD_LOG( PDERROR, "we should not get sequence 0" ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            rc = _getPool().push( data, curTuple->columns.len,
                                  RTN_LOB_GET_OFFSET_OF_LOB(
                                         pageSz,
                                         curTuple->columns.sequence,
                                         curTuple->columns.offset,
                                         _getMeta()._version >= DMS_LOB_CURRENT_VERSION ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            break ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__PUSH2POOL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__COMPLETELOB, "_rtnCoordLobStream::_completeLob" )
   INT32 _rtnCoordLobStream::_completeLob( const _rtnLobTuple &tuple,
                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__COMPLETELOB ) ;
      const _MsgLobTuple &t = tuple.tuple ;
      const subStream *sub = NULL ;
      netIOVec iov ;
      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      UINT32 retryTime = 0 ;
      MsgOpLob header ;
      _initHeader( header,
                   MSG_BS_LOB_UPDATE_REQ,
                   0, -1,
                   sizeof( header ) +
                   sizeof( _MsgLobTuple ) +
                   t.columns.len ) ;
      _pushLobHeader( &header, BSONObj(), iov ) ;
      _pushLobData( &t, sizeof( _MsgLobTuple ), iov ) ;
      _pushLobData( tuple.data, t.columns.len, iov ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         RTN_COORD_LOB_GET_SUBSTREAM( _metaGroup, sub ) ;

         header.contextID = sub->contextID ;

         rc = rtnCoordSendRequestToNode( &( header.header ),
                                         sub->id,
                                         routeAgent,
                                         cb,
                                         iov,
                                         _sendMap ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to send msg to node[%d:%hd], rc:%d",
                    sub->id.columns.groupID, sub->id.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), TRUE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
         }
      } while ( TRUE ) ;
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__COMPLETELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCoordLobStream::_close( _pmdEDUCB *cb )
   {
      return _closeSubStreams( cb, FALSE ) ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__ROLLBACK, "_rtnCoordLobStream::_rollback" )
   INT32 _rtnCoordLobStream::_rollback( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__ROLLBACK ) ;
      rc = _closeSubStreamsWithException( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "got error when rollback lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__ROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__CLOSESUBSTREAMS, "_rtnCoordLobStream::_closeSubStreams" )
   INT32 _rtnCoordLobStream::_closeSubStreams( _pmdEDUCB *cb,
                                               BOOLEAN exceptMeta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__CLOSESUBSTREAMS ) ;

      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      UINT32 retryTime = 0 ;
      MsgOpLob header ;
      _initHeader( header, MSG_BS_LOB_CLOSE_REQ,
                   0, -1, sizeof( header ) ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;

         for ( SUB_STREAMS::iterator itr = _subs.begin();
               itr != _subs.end();
               ++itr )
         {
            if ( exceptMeta && _metaGroup == itr->second.id.columns.groupID )
            {
               continue ;
            }
            header.contextID = itr->second.contextID ;
            rc = rtnCoordSendRequestToNode( &( header.header ),
                                            itr->second.id,
                                            routeAgent,
                                            cb,
                                            _sendMap ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to send msg to node[%d:%hd], rc:%d",
                    itr->second.id.columns.groupID,
                    itr->second.id.columns.nodeID, rc ) ;
               goto error ;
            }
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), TRUE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }

         rc = _removeClosedSubStreams() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close sub streams:%d", rc ) ;
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;            
         }
         else
         {
            /// here we only need to close opened sub streams.
            /// do not reopen new streams.
            continue ;
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__CLOSESUBSTREAMS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__CLOSESSWITHEXCEP, "_rtnCoordLobStream::__closeSubStreamsWithException" )
   INT32 _rtnCoordLobStream::_closeSubStreamsWithException( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__CLOSESSWITHEXCEP ) ;
      REQUESTID_MAP sendMap ;
      REPLY_QUE q ;
      netMultiRouteAgent *route = pmdGetKRCB()->getCoordCB(
                                  )->getRouteAgent() ;
      MsgOpKillContexts killMsg ;
      killMsg.header.messageLength = sizeof ( MsgOpKillContexts ) ;
      killMsg.header.opCode = MSG_BS_KILL_CONTEXT_REQ ;
      killMsg.header.TID = cb->getTID() ;
      killMsg.header.routeID.value = 0;
      killMsg.ZERO = 0;
      killMsg.numContexts = 1 ;

      SUB_STREAMS::const_iterator itr = _subs.begin() ;
      for ( ; itr != _subs.end(); ++itr )
      {
         killMsg.contextIDs[0] = itr->second.contextID ;
         rc = rtnCoordSendRequestToNode( &killMsg, itr->second.id,
                                         route, cb, sendMap ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to kill sub context on node[%d:%hd], rc:%d",
                    itr->second.id.columns.groupID,
                    itr->second.id.columns.nodeID, rc ) ;
            /// try to rollback all substreams, so do not goto error.
         }
      }

      rc = rtnCoordGetReply( cb, sendMap, q, MSG_BS_KILL_CONTEXT_RES,
                             TRUE, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get reply:%d", rc ) ;
         goto error ;
      }
   done:
      while ( !q.empty() )
      {
         CHAR *p = q.front();
         q.pop();
         SAFE_OSS_FREE( p ) ;
      }
      _subs.clear() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__CLOSESSWITHEXCEP, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__EXTRACTMETA, "_rtnCoordLobStream::_extractMeta" )   
   INT32 _rtnCoordLobStream::_extractMeta( const MsgOpReply *header,
                                           bson::BSONObj &metaObj,
                                           _rtnLobDataPool::tuple &dataTuple )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__EXTRACTMETA ) ;
      const CHAR *metaRaw = NULL ;
      UINT32 dataOffset = sizeof( MsgOpReply ) ;

      dataTuple.clear() ;

      if ( NULL == header )
      {
         PD_LOG( PDERROR, "header is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( ( UINT32 )header->header.messageLength <
           sizeof( MsgOpReply ) + 5 )
      {
         PD_LOG( PDERROR, "invalid msg length:%d",
                 header->header.messageLength ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      metaRaw = ( const CHAR * )header + sizeof( MsgOpReply ) ;
      try
      {
         metaObj = BSONObj( metaRaw ).getOwned() ;
         dataOffset += ossAlign4( (UINT32)metaObj.objsize() ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      /// If have data, need add to pool
      if ( (UINT32)header->header.messageLength >
           dataOffset + sizeof( MsgLobTuple ) )
      {
         MsgLobTuple *rt = ( MsgLobTuple* )( (const CHAR*)header+dataOffset ) ;
         dataOffset += sizeof( MsgLobTuple ) ;

         SDB_ASSERT( DMS_LOB_META_SEQUENCE == rt->columns.sequence &&
                     DMS_LOB_META_LENGTH == rt->columns.offset,
                     "Must be the first sequence page" ) ;

         dataTuple.data = ( const CHAR* )header + dataOffset ;
         dataTuple.len = rt->columns.len ;
         dataTuple.offset = 0 ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__EXTRACTMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCoordLobStream::_queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                          _dmsLobMeta &meta )
   {
      return _queryLobMeta( cb, meta ) ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__REMOVEV, "_rtnCoordLobStream::_removev" )
   INT32 _rtnCoordLobStream::_removev( const RTN_LOB_TUPLES &tuples,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__REMOVEV ) ;

      netMultiRouteAgent *routeAgent = pmdGetKRCB()->getCoordCB(
                                       )->getRouteAgent() ;
      BOOLEAN reshard = TRUE ;
      DONE_LST doneLst ;
      MsgOpLob header ;
      UINT32 retryTime = 0 ;

      _initHeader( header,
                   MSG_BS_LOB_REMOVE_REQ,
                   0, -1, sizeof( header ) ) ;

      do
      {
         _clearMsgData() ;
         INT32 tag = RETRY_TAG_NULL ;
         header.version = _cataInfo->getVersion() ;
         if ( reshard )
         {
            rc = _shardData( header, tuples, TRUE, doneLst ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to shard pieces:%d", rc ) ;
               goto error ;
            }

            reshard = FALSE ;
         }

         for ( DATA_GROUPS::iterator itr = _dataGroups.begin();
               itr != _dataGroups.end();
               ++itr )
         {
            const dataGroup &dg = itr->second ;
            if ( !dg.hasData() )
            {
               continue ;
            }

            header.contextID = dg.contextID ;
            header.header.messageLength = sizeof( MsgHeader ) + itr->second.bodyLen ;
            rc = rtnCoordSendRequestToNode( &( header.header ),
                                            dg.id,
                                            routeAgent,
                                            cb,
                                            dg.body,
                                            _sendMap ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to send msg to node[%d:%hd], rc:%d",
                       dg.id.columns.groupID,
                       dg.id.columns.nodeID, rc ) ;
               goto error ;
            }      
         }

         rc = _getReply( header, cb, _canRetry( retryTime++ ), TRUE, tag ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLstFromReply( doneLst ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         if ( RETRY_TAG_NULL == tag )
         {
            break ;
         }
         else
         {
            rc = _reopenSubStreams( cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to reopen sub streams:%d", rc ) ;
               goto error ;
            }
            reshard = TRUE ;
         }
      } while ( TRUE ) ;
   done:
      _clearMsgData() ;
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__REMOVEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCoordLobStream::_addSubStreamsFromReply()
   {
      INT32 rc = SDB_OK ;
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         if ( SDB_OK != ( *itr )->flags )
         {
            rc = ( *itr )->flags ;
            PD_LOG( PDERROR, "failed to open lob on node[%d:%d], rc:%d",
                    ( *itr )->header.routeID.columns.groupID,
                    ( *itr )->header.routeID.columns.nodeID, rc ) ;
            continue ;
         }

         if ( -1 == ( *itr )->contextID )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "invalid context id" ) ;
            continue ;
         }

         _add2Subs( ( *itr )->header.routeID.columns.groupID,
                    ( *itr )->contextID,
                    ( *itr )->header.routeID ) ;
      }

      return rc ;
   }

   //PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__GETREPLY, "_rtnCoordLobStream::_getReply" )
   INT32 _rtnCoordLobStream::_getReply( const MsgOpLob &header,
                                        _pmdEDUCB *cb,
                                        BOOLEAN canRetry,
                                        BOOLEAN nodeSpecified,
                                        INT32 &tag )
   {
      INT32 rc = SDB_OK ;
      INT32 flags = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__GETREPLY ) ;
      INT32 replyType = MAKE_REPLY_TYPE( header.header.opCode ) ;
      REPLY_QUE replyQueue ;
      tag = ( INT32 )RETRY_TAG_NULL ;

      rc = rtnCoordGetReply( cb, _sendMap, replyQueue, replyType,
                             TRUE, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get reply msg:%d", rc ) ;
         goto error ;
      }

      while ( !replyQueue.empty() )
      {
         MsgOpReply *replyHeader = ( MsgOpReply * )( replyQueue.front() ) ;
         replyQueue.pop() ;
         flags = replyHeader->flags ;
         MsgRouteID id = replyHeader->header.routeID ;

         if ( SDB_OK == flags )
         {
            /// replyHeader will be released by _clearMsgData()    
            _results.push_back( replyHeader ) ;
            continue ;
         }

         PD_LOG( PDWARNING, "Node[%d.%d] return failed, flags: %d, "
                 "new primary: %d", id.columns.groupID, id.columns.nodeID,
                 flags, replyHeader->startFrom ) ;

         // get group info
         CoordGroupMap::iterator it = _mapGroupInfo.find( id.columns.groupID ) ;
         if ( it == _mapGroupInfo.end() )
         {
            SDB_ASSERT( FALSE, "Group info is not exist" ) ;
            flags = SDB_COOR_NO_NODEGROUP_INFO ;
         }
         else if ( !nodeSpecified &&
                   rtnCoordGroupReplyCheck( cb, flags, canRetry, id,
                                            it->second, NULL, TRUE,
                                            replyHeader->startFrom,
                                            isReadonly() ) )
         {
            tag |= RETRY_TAG_RETRY ;
            flags = SDB_OK ;

            /// if meta group has update, so need to update meta group
            if ( _metaGroup == it->first )
            {
               _metaGroupInfo = it->second ;
            }
         }
         // then catalog
         else if ( rtnCoordCataReplyCheck( cb, flags, canRetry, _cataInfo,
                                           NULL, FALSE ) &&
                   SDB_OK == ( flags = _updateCataInfo( TRUE, cb ) ) )
         {
            tag |= ( RETRY_TAG_RETRY | RETRY_TAG_REOPEN ) ;
            flags = SDB_OK ;
         }

         SDB_OSS_FREE( replyHeader ) ;
         rc = flags ? flags : rc ;
      }

      if ( rc )
      {
         goto error ;
      }

   done:
      while ( !replyQueue.empty() )
      {
         CHAR *p = replyQueue.front();
         replyQueue.pop();
         SAFE_OSS_FREE( p ) ;
      }
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__GETREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnCoordLobStream::_clearMsgData()
   {
      std::vector<MsgOpReply *>::iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         SAFE_OSS_FREE( *itr ) ;
      }

      _results.clear() ;
      _sendMap.clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__REOPENSS, "_rtnCoordLobStream::_reopenSubStreams" ) 
   INT32 _rtnCoordLobStream::_reopenSubStreams( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__REOPENSS ) ;
      rc = _closeSubStreams( cb, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close sub streams:%d", rc ) ;
         goto error ;
      }

      /// main stream was opened before, the meta piece may be synced
      ///  to other group. we open all streams as normal.
      rc = _openOtherStreams( getFullName(), getOID(), _getMode(), cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open sub streams:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__REOPENSS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__SHARDDATA, "_rtnCoordLobStream::_shardData" )
   INT32 _rtnCoordLobStream::_shardData( const MsgOpLob &header,
                                         const RTN_LOB_TUPLES &tuples,
                                         BOOLEAN isWrite,
                                         const DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__SHARDDATA ) ;
      _dataGroups.clear() ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         const subStream *sub = NULL ;
         UINT32 groupID = 0 ;
         dataGroup *dg = NULL ;
         const MsgLobTuple *tuple = ( const MsgLobTuple * )(&( *itr )) ;

         if ( 0 < doneLst.count( (ossValuePtr)tuple ) )
         {
            continue ;
         }

         rc = _cataInfo->getLobGroupID( getOID(),
                                        tuple->columns.sequence,
                                        groupID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get destination:%d", rc ) ;
            goto error ;
         }

         RTN_COORD_LOB_GET_SUBSTREAM( groupID, sub ) ;

         dg = &( _dataGroups[groupID] ) ;
         if ( !dg->hasData() )
         {
            _pushLobHeader( &header, BSONObj(), dg->body ) ;
            dg->bodyLen += sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
         }
         dg->addData( *tuple,
                      isWrite ? itr->data : NULL ) ;
         dg->contextID = sub->contextID ;
         dg->id = sub->id ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__SHARDDATA, rc ) ;
      return rc ;
   error:
      _dataGroups.clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__HANDLEREADRES, "_rtnCoordLobStream::_handleReadResults" )
   INT32 _rtnCoordLobStream::_handleReadResults( _pmdEDUCB *cb,
                                                 DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__HANDLEREADRES ) ;

      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         if ( SDB_OK != ( *itr )->flags )
         {
            rc = ( *itr )->flags ;
            PD_LOG( PDERROR, "failed to read lob on node[%d:%hd], rc:%d",
                    ( *itr )->header.routeID.columns.groupID,
                    ( *itr )->header.routeID.columns.nodeID, rc ) ;
            goto error ;
         }

         rc = _push2Pool( *itr ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
            goto error ;
         }

         rc = _add2DoneLst( ( *itr )->header.routeID.columns.groupID,
                            doneLst ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add tuples to done list:%d", rc ) ;
            goto error ;
         }
      }

      /// we need to keep reply msg in memory.
      {
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         _getPool().entrust( ( CHAR * )( *itr ) ) ;
      }
      }
      _results.clear() ;
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__HANDLEREADRES, rc ) ;
      return rc ;
   error:
      _getPool().clear() ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__ADD2DONELSTFROMREPLY, "_rtnCoordLobStream::_add2DoneLstFromReply" )
   INT32 _rtnCoordLobStream::_add2DoneLstFromReply( DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__ADD2DONELSTFROMREPLY ) ;
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         SDB_ASSERT( SDB_OK == ( *itr )->flags, "impossible" ) ; 
         rc = _add2DoneLst( ( *itr )->header.routeID.columns.groupID,
                            doneLst ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to add group to done list:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__ADD2DONELSTFROMREPLY, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB_RTNCOORDLOBSTREAM__ADD2DONELST, "_rtnCoordLobStream::_add2DoneLst" )
   INT32 _rtnCoordLobStream::_add2DoneLst( UINT32 groupID, DONE_LST &doneLst )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNCOORDLOBSTREAM__ADD2DONELST ) ;
      DATA_GROUPS::iterator itr = _dataGroups.find( groupID ) ;
      if ( _dataGroups.end() == itr )
      {
         PD_LOG( PDERROR, "we can not find group:%d", groupID ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      {
      dataGroup &dg = itr->second ;
      for ( list<ossValuePtr>::const_iterator titr =
                                               dg.tuples.begin() ;
            titr != dg.tuples.end() ;
            ++titr )
      {
         if ( !doneLst.insert( *titr ).second )
         {
            PD_LOG( PDERROR, "we already pushed tuple to pool[%d:%d:%lld]",
                    (( MsgLobTuple *)( *titr ))->columns.len,
                    (( MsgLobTuple *)( *titr ))->columns.sequence,
                    (( MsgLobTuple *)( *titr ))->columns.offset ) ;
            rc = SDB_SYS ;
         }
      }

      dg.clearData() ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNCOORDLOBSTREAM__ADD2DONELST, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnCoordLobStream::_removeClosedSubStreams()
   {
      INT32 rc = SDB_OK ;
      std::vector<MsgOpReply *>::const_iterator itr = _results.begin() ;
      for ( ; itr != _results.end(); ++itr )
      {
         SDB_ASSERT( 1 == _subs.count( ( *itr )->header.routeID.columns.groupID ), "impossible" ) ;
         _subs.erase( ( *itr )->header.routeID.columns.groupID ) ;
      }
      return rc ;
   }

   void _rtnCoordLobStream::_pushLobHeader( const MsgOpLob *header,
                                            const BSONObj &obj,
                                            netIOVec &iov )
   {
      const CHAR *off = ( const CHAR * )header + sizeof( MsgHeader ) ;
      UINT32 len = sizeof( MsgOpLob ) - sizeof( MsgHeader ) ;
      iov.push_back( netIOV( off, len ) ) ;

      if ( !obj.isEmpty() )
      {
         iov.push_back( netIOV( obj.objdata(), obj.objsize() ) ) ;
	      UINT32 alignedLen = ossRoundUpToMultipleX( obj.objsize(), 4 ) ;
         if ( ( UINT32 )obj.objsize() < alignedLen )
         {
            iov.push_back( netIOV( &_alignBuf, alignedLen - obj.objsize() ) ) ;
         }
      }
      return ;
   }

   void _rtnCoordLobStream::_pushLobData( const void *data,
                                          UINT32 len,
                                          netIOVec &iov )
   {
      iov.push_back( netIOV( data, len ) ) ;
   }

   void _rtnCoordLobStream::_initHeader( MsgOpLob &header,
                                         INT32 opCode,
                                         INT32 bsonLen,
                                         SINT64 contextID,
                                         INT32 msgLen )
   {
      ossMemset( &header, 0, sizeof( header ) ) ;
      header.header.opCode = opCode ;
      header.bsonLen = bsonLen ;
      header.contextID = contextID ;
      header.flags = _getFlags() ;
      header.header.messageLength = msgLen < 0 ?
                                    sizeof( header ) + bsonLen :
                                    msgLen ;
      return ;
   }

   void _rtnCoordLobStream::_add2Subs( UINT32 groupID,
                                       SINT64 contextID,
                                       MsgRouteID id )
   {
      SDB_ASSERT( 0 == _subs.count( groupID ), "impossible" ) ;
      _subs[groupID] = subStream( contextID, id ) ;
      return ;
   }
}

