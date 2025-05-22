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

   Source File Name = rtnContextListLob.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnContextListLob.hpp"
#include "rtnTrace.hpp"
#include "rtnLob.hpp"
#include "rtnLobPieces.hpp"
#include "rtnLobMetricsSubmitor.hpp"
#include "dmsCB.hpp"
#include "pmdEnv.hpp"

using namespace bson ;

namespace engine
{

   /*
      Tool functions
   */
   static void _mergeOIDSet( ossPoolSet<OID> &dest, ossPoolSet<OID> &source, BOOLEAN &isNull )
   {
      if ( source.empty() )
      {
         /// do nothing
      }
      else if ( dest.empty() )
      {
         dest = source ;
      }
      else
      {
         ossPoolSet<OID>::iterator itSet = dest.begin() ;
         while( itSet != dest.end() )
         {
            if ( source.find( *itSet ) == source.end() )
            {
               dest.erase( itSet++ ) ;
            }
            else
            {
               ++itSet ;
            }
         }

         if ( dest.empty() )
         {
            isNull = TRUE ;
         }
      }
   }

   static void _mergeGroupSet( ossPoolSet<UINT32> &dest, ossPoolSet<UINT32> &source, BOOLEAN &isNull )
   {
      if ( source.empty() )
      {
         /// do nothing
      }
      else if ( dest.empty() )
      {
         dest = source ;
      }
      else
      {
         ossPoolSet<UINT32>::iterator itSet = dest.begin() ;
         while( itSet != dest.end() )
         {
            if ( source.find( *itSet ) == source.end() )
            {
               dest.erase( itSet++ ) ;
            }
            else
            {
               ++itSet ;
            }
         }

         if ( dest.empty() )
         {
            isNull = TRUE ;
         }
      }
   }

   /*
      _rtnContextListLob implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextListLob, RTN_CONTEXT_LIST_LOB, "LIST_LOB") ;

   _rtnContextListLob::_rtnContextListLob( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _suLogicalID( DMS_INVALID_LOGICCSID ),
    _buf( NULL ),
    _bufLen( 0 ),
    _fetchLobHead( TRUE ),
    _skip( 0 ),
    _returnNum( -1 )
   {
      _totalDeltaMonApp.reset() ;
   }

   _rtnContextListLob::~_rtnContextListLob()
   {
   	pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      _close( cb ) ;

      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
         _bufLen = 0 ;
      }
   }

   _dmsStorageUnit* _rtnContextListLob::getSU()
   {
      return _fetcher.getSu() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB_OPEN, "_rtnContextListLob::open" )
   INT32 _rtnContextListLob::open( const BSONObj &query,
                                   const BSONObj &selector, const BSONObj &hint,
                                   INT64 skip, INT64 returnNum, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB_OPEN ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;
      BSONElement fullName, pieces ;

      ossPoolSet<OID> queryOIDs ;
      ossPoolSet<OID> hintOIDs ;
      ossPoolSet<UINT32> queryGroups ;
      ossPoolSet<UINT32> hintGroups ;
      BOOLEAN isNullCond = FALSE ;

      _selectorObj = selector.getOwned() ;
      _hint = hint.getOwned() ;
      _skip = skip ;
      _returnNum = returnNum ;

      /// parse query
      rc = _parseInfoFromQuery( query.getOwned(), queryOIDs, queryGroups, isNullCond, _query ) ;
      if ( rc )
      {
         goto error ;
      }

      /// parse hint
      rc = _parseInfoFromHint( _hint, hintOIDs, hintGroups ) ;
      if ( rc )
      {
         goto error ;
      }

      /// merge oid
      _mergeOIDSet( hintOIDs, queryOIDs, isNullCond ) ;
      /// merge groupid
      _mergeGroupSet( hintGroups, queryGroups, isNullCond ) ;

      if ( !_selectorObj.isEmpty() )
      {
         rc = _selector.loadPattern ( _selectorObj ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load selector pattern[%s], rc: %d",
                      _selectorObj.toString().c_str(), rc ) ;
      }

      rc = _matchTree.loadPattern( _query ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load matchTree pattern[%s], rc: %d",
                   _query.toString().c_str(), rc ) ;

      try
      {
         fullName = _hint.getField( FIELD_NAME_COLLECTION ) ;
         if ( String != fullName.type() )
         {
            PD_LOG_MSG( PDERROR, "invalid collection name in hint(%s)",
                        _hint.toString( FALSE, TRUE ).c_str() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         pieces = _hint.getField( FIELD_NAME_LOB_LIST_PIECES_MODE ) ;
         if ( pieces.isBoolean() )
         {
            _fetchLobHead = pieces.boolean() ? FALSE : TRUE ;
         }
         else if ( pieces.isNumber() )
         {
            _fetchLobHead = ( 0 != pieces.numberInt() ) ? FALSE : TRUE ;
         }
         else if ( !pieces.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Param[%s] shoud be boolean", FIELD_NAME_LOB_LIST_PIECES_MODE ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

      rc = _fetcher.init( fullName.valuestr(), _fetchLobHead, &hintOIDs ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init lob fetcher, rc: %d", rc ) ;
         goto error ;
      }
      if ( NULL != _fetcher.getSu() )
      {
         _suLogicalID = _fetcher.getSu()->LogicalCSID() ;
      }

      DMS_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_LIST, 1 ) ;

      _fullName.assign( fullName.valuestr() ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      if ( isNullCond ||
           ( !hintGroups.empty() &&
             hintGroups.find( pmdGetNodeID().columns.groupID ) == hintGroups.end() )
          )
      {
         _hitEnd = TRUE ;
         _returnNum = 0 ;
      }
      else if ( _fetchLobHead && !hintOIDs.empty() )
      {
         if ( _returnNum < 0 || _returnNum > (INT64)hintOIDs.size() )
         {
            _returnNum = hintOIDs.size() ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextListLob::onSubmit( const monAppCB & delta )
   {
      _totalDeltaMonApp += delta ;
      getMonCB()->incMetrics( delta ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__PREPAGEDATA, "_rtnContextListLob::_prepareData" )
   INT32 _rtnContextListLob::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__PREPAGEDATA ) ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;
      BSONObj obj ;
      INT32 returnObjNum = 0 ;

      while ( returnObjNum < 1000 && 0 != _returnNum )
      {
         BOOLEAN isMatch = FALSE ;
         rc = _fetchLobHead ?_getMetaInfo( cb, obj ) :
                             _getSequenceInfo( cb, obj ) ;
         if ( SDB_OK == rc )
         {
            rc = _matchTree.matches( obj, isMatch ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to matches obj[%s], rc: %d",
                         obj.toString().c_str(), rc ) ;
            if ( isMatch )
            {
               BSONObj selObj ;
               if ( _skip > 0 )
               {
                  --_skip ;
                  continue ;
               }

               if ( _returnNum > 0 )
               {
                  --_returnNum ;
               }

               if ( _selector.isInitialized() )
               {
                  rc = _selector.select( obj, selObj ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to select obj[%s], rc: %d",
                               obj.toString().c_str(), rc ) ;
               }
               else
               {
                  selObj = obj ;
               }

               rc = append( selObj ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append data to context, rc: %d",
                            rc ) ;
               returnObjNum++ ;
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            goto error ;
         }
         else if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
         {
            // Ignore the concurrency during deleteLob and listLobs operations
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get lob data, rc: %d", rc ) ;
            goto error ;
         }

         if ( 0 == _returnNum )
         {
            _hitEnd = TRUE ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__PREPAGEDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextListLob::_toString( stringstream &ss )
   {
      ss << ",Name:" << _fullName.c_str()
         << ",BuffLen:" << _bufLen ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__GETMETAINFO, "_rtnContextListLob::_getMetaInfo" )
   INT32 _rtnContextListLob::_getMetaInfo( _pmdEDUCB *cb, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__GETMETAINFO ) ;
      _dmsLobInfoOnPage info ;
      UINT32 read = 0 ;
      const _dmsLobMeta *meta = NULL ;
      UINT64 modificationTime = 0 ;

      _builder.reset() ;

      rc = _fetcher.fetch( cb, info ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to fetch lob, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( isCountMode() && _matchTree.isMatchesAll() && _selectorObj.isEmpty() )
      {
         obj = BSONObj() ;
         goto done ;
      }

      rc = _reallocate( info._len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to reallocate buf, rc: %d", rc ) ;
         goto error ;
      }

      rc = rtnReadLob( _fullName.c_str(), info._oid, info._sequence, 0,
                       info._len, cb, _buf, read, _fetcher.getSu(),
                       _fetcher.getMBContext() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to read lob[%s], rc: %d",
                 info._oid.str().c_str(), rc ) ;
         goto error ;
      }

      SDB_ASSERT( read == info._len, "impossible" ) ;

      meta = ( const _dmsLobMeta* )_buf ;
      modificationTime = meta->_modificationTime ;
      if ( 0 == modificationTime )
      {
         modificationTime = meta->_createTime ;
      }

      try
      {
         _builder.append( FIELD_NAME_LOB_SIZE, meta->_lobLen ) ;
         _builder.appendOID( FIELD_NAME_LOB_OID, &( info._oid ) ) ;
         _builder.appendTimestamp( FIELD_NAME_LOB_CREATETIME,
                                   meta->_createTime,
                                   ( meta->_createTime % 1000 ) * 1000) ;
         _builder.appendTimestamp( FIELD_NAME_LOB_MODIFICATION_TIME,
                                   modificationTime,
                                   (modificationTime % 1000 ) * 1000) ;
         _builder.appendBool( FIELD_NAME_LOB_AVAILABLE, meta->isDone() ) ;

   #ifdef _DEBUG
         _builder.appendBool( FIELD_NAME_LOB_HAS_PIECESINFO, meta->hasPiecesInfo() ) ;
         if ( meta->hasPiecesInfo() && info._len >= DMS_LOB_META_LENGTH )
         {
            BSONArray array ;
            _rtnLobPiecesInfo piecesInfo ;

            INT32 length = meta->_piecesInfoNum * (INT32)sizeof( _rtnLobPieces ) ;
            const CHAR* piecesInfoBuf = (const CHAR*)
                                        ( _buf + DMS_LOB_META_LENGTH - length ) ;

            rc = piecesInfo.readFrom( piecesInfoBuf, length ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to read pieces info of lob[%s], rc: %d",
                       info._oid.str().c_str(), rc ) ;
               goto error ;
            }

            rc = piecesInfo.saveTo( array ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to save pieces info of lob[%s], rc: %d",
                       info._oid.str().c_str(), rc ) ;
               goto error ;
            }

            _builder.append( FIELD_NAME_LOB_PIECESINFONUM, meta->_piecesInfoNum ) ;
            _builder.appendArray( FIELD_NAME_LOB_PIECESINFO, array ) ;
         }
   #endif

         if ( SDB_ROLE_STANDALONE != pmdGetDBRole() )
         {
            _builder.append( FIELD_NAME_GROUPID, (INT32)pmdGetNodeID().columns.groupID ) ;
         }

         obj = _builder.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__GETMETAINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__GETSEQUENCEINFO, "_rtnContextListLob::_getSequenceInfo" )
   INT32 _rtnContextListLob::_getSequenceInfo( _pmdEDUCB *cb, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__GETSEQUENCEINFO ) ;
      _dmsLobInfoOnPage info ;

      _builder.reset() ;

      rc = _fetcher.fetch( cb, info, NULL ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "Failed to fetch lob, rc: %d", rc ) ;
         }
         goto error ;
      }

      if ( isCountMode() && _matchTree.isMatchesAll() && _selectorObj.isEmpty() )
      {
         obj = BSONObj() ;
         goto done ;
      }

      try
      {
         _builder.appendOID( FIELD_NAME_LOB_OID, &( info._oid ) ) ;
         _builder.append( FIELD_NAME_SEQUENCE, info._sequence ) ;
         _builder.append( FIELD_NAME_LOB_LENGTH, (INT32)info._len ) ;

         if ( SDB_ROLE_STANDALONE != pmdGetDBRole() )
         {
            _builder.append( FIELD_NAME_GROUPID, (INT32)pmdGetNodeID().columns.groupID ) ;
         }

         obj = _builder.done() ;
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__GETSEQUENCEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextListLob::_reallocate( UINT32 len )
   {
      INT32 rc = SDB_OK ;
      if ( len <= _bufLen )
      {
         goto done ;
      }
      else if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _bufLen = 0 ;
         _buf = NULL ;
      }

      _buf = ( CHAR * )SDB_OSS_MALLOC( len ) ;
      if ( NULL == _buf )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }
   done:
      return rc;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__CLOSE, "_rtnContextListLob::_close" )
   void _rtnContextListLob::_close( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__CLOSE ) ;

      INT32 rc = SDB_OK ;
      dmsStorageUnitID suID ;
      _dmsStorageUnit *su = NULL ;
      _dmsMBContext *mbContext = NULL ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      const CHAR *clName = NULL ;
      _monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( _fullName.empty() )
      {
         goto done ;
      }

      // submit snapshot changes to sdb/svctask monitor
      if ( pMonAppCB && pMonAppCB->mondbcb )
      {
         pMonAppCB->mondbcb->incMetrics( _totalDeltaMonApp ) ;
      }
      if ( pMonAppCB && pMonAppCB->getSvcTaskInfo() )
      {
         pMonAppCB->getSvcTaskInfo()->incMetrics( _totalDeltaMonApp ) ;
      }

      // get MBContext, and submit snapshot changes to cl monitor
      rc = rtnResolveCollectionNameAndLock( _fullName.c_str(), dmsCB,
                                            &su, &clName, suID ) ;
      if ( SDB_OK != rc || !su )
      {
         PD_LOG( PDERROR, "Resolve collection[%s] failed, rc: %d",
                 _fullName.c_str(), rc ) ;
         goto error ;
      }

      rc = su->data()->getMBContext( &mbContext, clName, SHARED ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get collection[%s] mb-context failed, rc: %d",
                 clName, rc ) ;
         goto error ;
      }

      if ( mbContext && mbContext->mbStat() )
      {
         // submit to cl
         mbContext->mbStat()->_crudCB.incMetrics( _totalDeltaMonApp ) ;
      }

      _isOpened = FALSE ;

   done:
      if ( NULL != mbContext && NULL != su )
      {
         su->data()->releaseMBContext( mbContext ) ;
         mbContext = NULL ;
      }
      if ( NULL != su )
      {
         dmsCB->suUnlock ( suID ) ;
         su = NULL ;
         suID = DMS_INVALID_CS ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__CLOSE, rc ) ;
      return ;
   error:
      goto done ;
   }

   INT32 _rtnContextListLob::_parseInfoFromQuery( const BSONObj &match,
                                                  ossPoolSet<OID> &oids,
                                                  ossPoolSet<UINT32> &groups,
                                                  BOOLEAN &isNullCond,
                                                  BSONObj &newMatch )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder( match.objsize() ) ;
      BOOLEAN isModify = FALSE ;

      try
      {
         BSONObjIterator itr( match ) ;
         while( itr.more() && !isNullCond )
         {
            BSONElement e = itr.next() ;

            // $and:[{Oid:{$oid:"xxx"}}, {Oid:{$oid:"yyyy"}}]
            if ( Array == e.type() && 0 == ossStrcmp( e.fieldName(), "$and" ) )
            {
               INT32 curBBLen = builder.bb().len() ;
               INT32 curReserved = builder.bb().getReserveBytes() ;
               BSONArrayBuilder sub( builder.subarrayStart( e.fieldName() ) ) ;
               BSONObj tmpNew ;

               BSONObjIterator tmpItr( e.embeddedObject() ) ;
               while( tmpItr.more() && !isNullCond )
               {
                  ossPoolSet<OID> tmpOids ;
                  ossPoolSet<UINT32> tmpGroups ;

                  BSONElement tmpE = tmpItr.next() ;
                  BSONObj tmpObj ;

                  if ( Object != tmpE.type() )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Parse matcher(%s) failed, rc: %d",
                                 match.toPoolString().c_str(), rc ) ;
                     goto error ;
                  }

                  tmpObj = tmpE.embeddedObject() ;
                  rc = _parseInfoFromQuery( tmpObj, tmpOids, tmpGroups, isNullCond, tmpNew ) ;
                  if ( rc )
                  {
                     goto error ;
                  }
                  /// merge set
                  _mergeOIDSet( oids, tmpOids, isNullCond ) ;
                  _mergeGroupSet( groups, tmpGroups, isNullCond ) ;
                  /// build new matcher
                  if ( tmpNew.objdata() != tmpObj.objdata() )
                  {
                     isModify = TRUE ;

                     if ( !tmpNew.isEmpty() )
                     {
                        sub.append( tmpNew ) ;
                     }
                  }
                  else
                  {
                     sub.append( tmpObj ) ;
                  }
               }

               if ( sub.isEmpty() )
               {
                  sub.abandon() ;
                  builder.bb().setlen( curBBLen ) ;
                  builder.bb().setReserveBytes( curReserved ) ;
               }
               else
               {
                  sub.done() ;
               }
            }
            // Oid
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_LOB_OID ) )
            {
               ossPoolSet<OID> tmpOids ;
               if ( _parseOID( e, tmpOids, isNullCond, FALSE ) )
               {
                  _mergeOIDSet( oids, tmpOids, isNullCond ) ;
                  isModify = TRUE ;
               }
               else
               {
                  builder.append( e ) ;
               }
            }
            else if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_GROUPID ) )
            {
               ossPoolSet<UINT32> tmpGroups ;
               if ( _parseGroupID( e, tmpGroups, isNullCond, FALSE ) )
               {
                  _mergeGroupSet( groups, tmpGroups, isNullCond ) ;
                  isModify = TRUE ;
               }
               else
               {
                  builder.append( e ) ;
               }
            }
            else
            {
               builder.append( e ) ;
            }
         }

         if ( !isNullCond && isModify )
         {
            newMatch = builder.obj() ;
         }
         else
         {
            newMatch = match ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnContextListLob::_parseOID( const BSONElement &e,
                                          ossPoolSet<OID> &oids,
                                          BOOLEAN &isNullCond,
                                          BOOLEAN onlyOID )
   {
      BOOLEAN hasParsed = FALSE ;

      /// Oid:{}
      if ( jstOID == e.type() )
      {
         oids.insert( e.OID() ) ;
         hasParsed = TRUE ;
      }
      else if ( !onlyOID && e.isABSONObj() )
      {
         BSONElement subE = e.embeddedObject().firstElement() ;

         /// Oid:{$et:{}}
         if ( 0 == ossStrcmp( subE.fieldName(), "$et" ) )
         {
            _parseOID( subE, oids, isNullCond, TRUE ) ;
            hasParsed = TRUE ;
         }
         /// Oid:{$in:[]}
         else if ( 0 == ossStrcmp( subE.fieldName(), "$in" ) && Array == subE.type() )
         {
            BOOLEAN tmpIsNullCond = FALSE ;
            BSONObjIterator itr( subE.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement tmpE = itr.next() ;
               _parseOID( tmpE, oids, tmpIsNullCond, TRUE ) ;
            }
            hasParsed = TRUE ;
            if ( oids.empty() )
            {
               isNullCond = TRUE ;
            }
         }
      }
      else
      {
         isNullCond = TRUE ;
      }

      return hasParsed ;
   }

   BOOLEAN _rtnContextListLob::_parseGroupID( const BSONElement &e,
                                              ossPoolSet<UINT32> &groups,
                                              BOOLEAN &isNullCond,
                                              BOOLEAN onlyNumber )
   {
      BOOLEAN hasParsed = FALSE ;

      /// GroupID:xxx
      if ( e.isNumber() )
      {
         groups.insert( e.numberInt() ) ;
         hasParsed = TRUE ;
      }
      else if ( !onlyNumber && e.isABSONObj() )
      {
         BSONElement subE = e.embeddedObject().firstElement() ;

         /// GroupID:{$et:xxx}
         if ( 0 == ossStrcmp( subE.fieldName(), "$et" ) )
         {
            _parseGroupID( subE, groups, isNullCond, TRUE ) ;
            hasParsed = TRUE ;
         }
         /// GroupID:{$in:[]}
         else if ( 0 == ossStrcmp( subE.fieldName(), "$in" ) && Array == subE.type() )
         {
            BOOLEAN tmpIsNullCond = FALSE ;
            BSONObjIterator itr( subE.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement tmpE = itr.next() ;
               _parseGroupID( tmpE, groups, tmpIsNullCond, TRUE ) ;
            }
            hasParsed = TRUE ;
            if ( groups.empty() )
            {
               isNullCond = TRUE ;
            }
         }
      }
      else
      {
         isNullCond = TRUE ;
      }

      return hasParsed ;
   }

   INT32 _rtnContextListLob::_parseInfoFromHint( const BSONObj &hint,
                                                 ossPoolSet<OID> &oids,
                                                 ossPoolSet<UINT32> &groups )
   {
      INT32 rc = SDB_OK ;

      try
      {
         /// OID
         BSONElement e = hint.getField( FIELD_NAME_LOB_OID ) ;
         if ( jstOID == e.type() )
         {
            oids.insert( e.OID() ) ;
         }
         else if ( String == e.type() )
         {
            if ( !utilIsValidOID( e.valuestr() ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG_MSG( PDERROR, "Hint param[%s] is invalid lob Oid string",
                           FIELD_NAME_LOB_OID ) ;
               goto error ;
            }
            else
            {
               OID tmpOID ;
               tmpOID.init( e.valuestr() ) ;
               oids.insert( tmpOID ) ;
            }
         }
         else if ( Array == e.type() )
         {
            BSONObjIterator itr( e.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement tmpE = itr.next() ;
               if ( jstOID == tmpE.type() )
               {
                  oids.insert( tmpE.OID() ) ;
               }
               else if ( String == tmpE.type() )
               {
                  if ( !utilIsValidOID( tmpE.valuestr() ) )
                  {
                     rc = SDB_INVALIDARG ;
                     PD_LOG_MSG( PDERROR, "Hint param[%s] is invalid lob Oid string",
                                 FIELD_NAME_LOB_OID ) ;
                     goto error ;
                  }
                  else
                  {
                     OID tmpOID ;
                     tmpOID.init( tmpE.valuestr() ) ;
                     oids.insert( tmpOID ) ;
                  }
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Hint param[%s] must be Oid or Oid array",
                              FIELD_NAME_LOB_OID ) ;
                  goto error ;
               }
            }
         }
         else if ( !e.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Hint param[%s] must be Oid/String or Oid/String array",
                        FIELD_NAME_LOB_OID ) ;
            goto error ;
         }

         /// GroupID
         e = hint.getField( FIELD_NAME_GROUPID ) ;
         if ( e.isNumber() )
         {
            groups.insert( e.numberInt() ) ;
         }
         else if ( Array == e.type() )
         {
            BSONObjIterator itr( e.embeddedObject() ) ;
            while( itr.more() )
            {
               BSONElement tmpE = itr.next() ;
               if ( tmpE.isNumber() )
               {
                  groups.insert( tmpE.numberInt() ) ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  PD_LOG_MSG( PDERROR, "Hint param[%s] must be int or int array",
                              FIELD_NAME_GROUPID ) ;
                  goto error ;
               }
            }
         }
         else if ( !e.eoo() )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG_MSG( PDERROR, "Hint param[%s] must be int or int array",
                        FIELD_NAME_GROUPID ) ;
            goto error ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

}

