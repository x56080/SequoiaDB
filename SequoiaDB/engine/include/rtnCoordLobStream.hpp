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

   Source File Name = rtnCoordLobStream.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/10/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_COORDLOBSTREAM_
#define RTN_COORDLOBSTREAM_

#include "rtnLobStream.hpp"
#include "netDef.hpp"
#include "coordDef.hpp"
#include "msg.h"
#include "netMultiRouteAgent.hpp"
#include "utilMap.hpp"

namespace engine
{
   class _rtnCoordLobStream : public _rtnLobStream
   {
   public:
      _rtnCoordLobStream() ;
      virtual ~_rtnCoordLobStream() ;

   public:
      virtual _dmsStorageUnit *getSU()
      {
         return NULL ;
      }

   private:
      virtual INT32 _prepare( const CHAR *fullName,
                              const bson::OID &oid,
                              INT32 mode,
                              _pmdEDUCB *cb ) ;

      virtual INT32 _queryLobMeta( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta ) ;

      virtual INT32 _ensureLob( _pmdEDUCB *cb,
                                _dmsLobMeta &meta,
                                BOOLEAN &isNew ) ;

      virtual INT32 _getLobPageSize( INT32 &pageSize ) ;

      virtual INT32 _write( const _rtnLobTuple &tuple,
                            _pmdEDUCB *cb ) ;

      virtual INT32 _writev( const RTN_LOB_TUPLES &tuples,
                             _pmdEDUCB *cb ) ;

      virtual INT32 _readv( const RTN_LOB_TUPLES &tuples,
                            _pmdEDUCB *cb ) ;

      virtual INT32 _completeLob( const _rtnLobTuple &tuple,
                                  _pmdEDUCB *cb ) ;
 
      virtual INT32 _rollback( _pmdEDUCB *cb ) ;

      virtual INT32 _queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                 _dmsLobMeta &meta ) ;

      virtual INT32 _removev( const RTN_LOB_TUPLES &tuples,
                              _pmdEDUCB *cb ) ;

      virtual INT32 _close( _pmdEDUCB *cb ) ;

   private:
      struct subStream
      {
         SINT64 contextID ;
         MsgRouteID id ;

         subStream()
         :contextID( -1 )
         {
            id.value = MSG_INVALID_ROUTEID ;
         }

         subStream( SINT64 context, MsgRouteID route )
         :contextID( context ),
          id( route )
         {
         }
      } ;

      struct dataGroup
      {
         SINT64 contextID ;
         MsgRouteID id ;
         netIOVec body ;
         UINT32 bodyLen ;
         std::list<ossValuePtr> tuples ;

         dataGroup()
         :contextID( -1 ),
          bodyLen( 0 )
         {
            id.value = MSG_INVALID_ROUTEID ;
         }

         BOOLEAN hasData() const
         {
            return 0 < bodyLen ;
         }

         void addData( const MsgLobTuple &tuple,
                       const void *data )
         {
            body.push_back( netIOV( tuple.data, sizeof( tuple ) ) ) ;
            bodyLen += sizeof( tuple ) ;
            tuples.push_back( ( ossValuePtr )( &tuple ) ) ;
            if ( NULL != data )
            {
               body.push_back( netIOV( data, tuple.columns.len ) ) ;
               bodyLen += tuple.columns.len ;
            }
            return ;
         }

         void clearData()
         {
            body.clear() ;
            bodyLen = 0 ;
            tuples.clear() ;
         }
      } ;


      enum RETRY_TAG
      {
         RETRY_TAG_NULL = 0,
         RETRY_TAG_RETRY = 0x00001,
         RETRY_TAG_REOPEN = 0x00002,
      } ;

      typedef _utilMap<UINT32, subStream, 20 >  SUB_STREAMS ;
      typedef std::set<ossValuePtr>             DONE_LST ;
      typedef _utilMap<UINT32, dataGroup, 20 >  DATA_GROUPS ;

   private:
      INT32 _openSubStreams( const CHAR *fullName,
                             const bson::OID &oid,
                             INT32 mode,
                             _pmdEDUCB *cb ) ;

      INT32 _openMainStream( const CHAR *fullName,
                             const bson::OID &oid,
                             INT32 mode,
                             _pmdEDUCB *cb ) ;

      INT32 _openOtherStreams( const CHAR *fullName,
                               const bson::OID &oid,
                               INT32 mode,
                               _pmdEDUCB *cb ) ;

      INT32 _extractMeta( const MsgOpReply *header,
                          bson::BSONObj &obj,
                          _rtnLobDataPool::tuple &dataTuple ) ;

      INT32 _closeSubStreams( _pmdEDUCB *cb, BOOLEAN exceptMeta ) ;

      INT32 _closeSubStreamsWithException( _pmdEDUCB *cb ) ;

      INT32 _push2Pool( const MsgOpReply *header ) ;

      INT32 _getReply( const MsgOpLob &header,
                       _pmdEDUCB *cb,
                       BOOLEAN canRetry,
                       BOOLEAN nodeSpecified,
                       INT32 &tag,
                       set< INT32 > *pIgoreErr = NULL ) ;

      void _clearMsgData() ;

      INT32 _reopenSubStreams( _pmdEDUCB *cb ) ;

      INT32 _addSubStreamsFromReply() ;

      INT32 _updateCataInfo( BOOLEAN refresh,
                             _pmdEDUCB *cb ) ;

      INT32 _shardData( const MsgOpLob &header,
                        const RTN_LOB_TUPLES &tuples,
                        BOOLEAN isWrite,
                        const DONE_LST &doneLst ) ;

      INT32 _handleReadResults( _pmdEDUCB *cb, DONE_LST &doneLst ) ;

      INT32 _add2DoneLstFromReply( DONE_LST &doneLst ) ;

      INT32 _add2DoneLst( UINT32 groupID, DONE_LST &doneLst ) ;

      INT32 _removeClosedSubStreams() ;

      void _add2Subs( UINT32 groupID, SINT64 contextID, MsgRouteID id ) ;

      void _pushLobHeader( const MsgOpLob *header,
                           const BSONObj &obj,
                           netIOVec &iov ) ;

      void _pushLobData( const void *data,
                         UINT32 len,
                         netIOVec &iov ) ;

      void _initHeader( MsgOpLob &header,
                        INT32 opCode,
                        INT32 bsonLen,
                        INT64 contextID,
                        INT32 msgLen = -1 ) ;
   private:
      CoordCataInfoPtr _cataInfo ;
      CoordGroupMap    _mapGroupInfo ;
      UINT32           _pageSize ;

      std::vector<MsgOpReply *> _results ;
      REQUESTID_MAP     _sendMap ;
      DATA_GROUPS       _dataGroups ;

      SUB_STREAMS       _subs;
      bson::BSONObj     _metaObj ;
      CoordGroupInfoPtr _metaGroupInfo ;
      UINT32            _metaGroup ;

      UINT32            _alignBuf ;
   } ;
   typedef class _rtnCoordLobStream rtnCoordLobStream ;
}
#endif

