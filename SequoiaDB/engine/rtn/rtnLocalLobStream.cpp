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

   Source File Name = rtnLocalLobStream.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLocalLobStream.hpp"
#include "rtnTrace.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "rtnLob.hpp"

namespace engine
{
   _rtnLocalLobStream::_rtnLocalLobStream()
   :_mbContext( NULL ),
    _su( NULL ),
    _dmsCB( NULL ),
    _writeDMS( FALSE )
   {
   }

   _rtnLocalLobStream::~_rtnLocalLobStream()
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      _closeInner( cb ) ;
   }

   void _rtnLocalLobStream::_closeInner( _pmdEDUCB *cb )
   {
      if ( _mbContext && _su )
      {
         _su->data()->releaseMBContext( _mbContext ) ;
         _mbContext = NULL ;
      }
      if ( _su && _dmsCB )
      {
         sdbGetDMSCB()->suUnlock ( _su->CSID() ) ;
         _su = NULL ;
      }
      if ( _writeDMS )
      {
         _dmsCB->writeDown( cb ) ;
         _writeDMS = FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__PREPARE, "_rtnLocalLobStream::_prepare" )
   INT32 _rtnLocalLobStream::_prepare( const CHAR *fullName,
                                       const bson::OID &oid,
                                       INT32 mode,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__PREPARE ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *clName = NULL ;
      _dmsCB = sdbGetDMSCB() ;

      if ( SDB_LOB_MODE_R != mode )
      {
         rc = _dmsCB->writable( cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "database is not writable, rc = %d", rc ) ;
            goto error ;
         }
         _writeDMS = TRUE ;
      }

      rc = rtnResolveCollectionNameAndLock( fullName, _dmsCB,
                                            &_su, &clName, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to resolve collection name:%s",
                 fullName ) ;
         goto error ;
      }

      rc = _su->data()->getMBContext( &_mbContext, clName, -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to resolve collection name:%s",
                 clName ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__PREPARE, rc ) ;
      return rc ;
   error:
      _closeInner( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__QUERYLOBMETA, "_rtnLocalLobStream::_queryLobMeta" )
   INT32 _rtnLocalLobStream::_queryLobMeta(  _pmdEDUCB *cb,
                                             _dmsLobMeta &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__QUERYLOBMETA ) ;
      UINT32 len = _su->getLobPageSize() ;
      UINT32 readLen = 0 ;
      CHAR *buf = NULL ;
      dmsLobRecord record ;

      _getPool().clear() ;

      rc = _getPool().allocate( len, &buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to alloc buffer[%u], rc: %d",
                 len, rc ) ;
         goto error ;
      }

      record.set( &getOID(), DMS_LOB_META_SEQUENCE, 0, len, NULL ) ;

      /// read whole the meta page
      rc = _su->lob()->read( record, _mbContext, cb, buf, readLen ) ;
      if ( SDB_OK == rc )
      {
         if ( readLen < sizeof( _dmsLobMeta ) )
         {
            PD_LOG( PDERROR, "Read lob[%s]'s meta page len is less than "
                    "meta size[%u]", getOID().str().c_str(),
                    sizeof( _dmsLobMeta ) ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         /// copy data
         ossMemcpy( (void*)&meta, buf, sizeof( meta ) ) ;
         if ( !meta.isDone() )
         {
            PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                    getOID().str().c_str(), meta.toString().c_str() ) ;
            rc = SDB_LOB_IS_NOT_AVAILABLE ;
            goto error ;
         }
         /// if meta page has data, push the data to pool
         if ( meta._version >= DMS_LOB_CURRENT_VERSION &&
              meta._lobLen > 0 &&
              readLen > DMS_LOB_META_LENGTH )
         {
            rc = _getPool().push( buf + DMS_LOB_META_LENGTH,
                                  ( meta._lobLen <= readLen-DMS_LOB_META_LENGTH ?
                                  meta._lobLen : readLen-DMS_LOB_META_LENGTH ),
                                  0 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to push data to pool, rc:%d", rc ) ;
               goto error ;
            }
            _getPool().pushDone() ;
         }
         goto done ;
      }
      else
      {
         if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
         {
            rc = SDB_FNE ;
         }
         else if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to get meta of lob, rc:%d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__QUERYLOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__ENSURELOB, "_rtnLocalLobStream::_ensureLob" )
   INT32 _rtnLocalLobStream::_ensureLob( _pmdEDUCB *cb,
                                         _dmsLobMeta &meta,
                                         BOOLEAN &isNew )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__ENSURELOB ) ;

      rc = _mbContext->mbLock( EXCLUSIVE ) ;
      PD_RC_CHECK( rc, PDERROR, "dms mb context lock failed, rc: %d", rc ) ;

      rc = _su->lob()->getLobMeta( getOID(), _mbContext,
                                   cb, meta ) ;
      if ( SDB_OK == rc )
      {
         if ( !meta.isDone() )
         {
            PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                    getOID().str().c_str(), meta.toString().c_str() ) ;
            rc = SDB_LOB_IS_NOT_AVAILABLE ;
            goto error ;
         }
         isNew = FALSE ;
         goto done ;
      }
      else if ( SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to get meta of lob, rc:%d", rc ) ;
         goto error ;
      }
      else
      {
         rc = SDB_OK ;
         if ( !dmsAccessAndFlagCompatiblity ( _mbContext->mb()->_flag,
                                              DMS_ACCESS_TYPE_INSERT ) )
         {
            PD_LOG ( PDERROR, "Incompatible collection mode: %d",
                     _mbContext->mb()->_flag ) ;
            rc = SDB_DMS_INCOMPATIBLE_MODE ;
            goto error ;
         }

         meta.clear() ;
         rc = _su->lob()->writeLobMeta( getOID(), _mbContext,
                                        cb, meta, TRUE, _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write lob meta, rc:%d", rc ) ;
            goto error ;
         }
         isNew = TRUE ;
      }

   done:
      _mbContext->mbUnlock() ;

      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__ENSURELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__COMPLETELOB, "_rtnLocalLobStream::_completeLob" )
   INT32 _rtnLocalLobStream::_completeLob( const _rtnLobTuple &tuple,
                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__COMPLETELOB ) ;
      dmsLobRecord record ;
      const MsgLobTuple &t = tuple.tuple ;

      record.set( &getOID(), t.columns.sequence, t.columns.offset,
                  t.columns.len, ( const CHAR * )tuple.data ) ;

      rc = _su->lob()->update( record, _mbContext, cb, _getDPSCB() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write to lob:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__COMPLETELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_getLobPageSize( INT32 &pageSize )
   {
      SDB_ASSERT( NULL != _su, "can not be null" ) ;
      pageSize = _su->getLobPageSize() ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__WRITEV, "_rtnLocalLobStream::_writev" )
   INT32 _rtnLocalLobStream::_writev( const RTN_LOB_TUPLES &tuples,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__WRITEV ) ;
      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr ) 
      {
         SDB_ASSERT( !itr->empty(), "can not be empty" ) ;
         if ( cb->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }

         rc = _write( *itr, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__WRITEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }


   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__WRITE, "_rtnLocalLobStream::_write" )
   INT32 _rtnLocalLobStream::_write( const _rtnLobTuple &tuple,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__WRITE ) ;
      _dmsLobRecord record ;

      record.set( &getOID(),
                  tuple.tuple.columns.sequence,
                  tuple.tuple.columns.offset,
                  tuple.tuple.columns.len,
                  tuple.data ) ;
      rc = _su->lob()->write( record, _mbContext, cb,
                              _getDPSCB() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob[%s],"
                 "sequence:%d, rc:%d", record._oid->str().c_str(),
                 record._sequence, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__READV, "_rtnLocalLobStream::_readv" )
   INT32 _rtnLocalLobStream::_readv( const RTN_LOB_TUPLES &tuples,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__READV ) ;
      SDB_ASSERT( !tuples.empty(), "can not be empty" ) ;

      CHAR *buf = NULL ;
      SINT64 readSize = 0 ;
      INT32 pageSize =  _su->getLobPageSize() ;
      UINT32 needLen = 0 ;
      _getPool().clear() ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         needLen += itr->tuple.columns.len ;
      }

      rc = _getPool().allocate( needLen, &buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to allocate buf:%d", rc ) ;
         goto error ;
      }

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         rc = _read( *itr, cb, buf + readSize ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         readSize += itr->tuple.columns.len ;
      }

      SDB_ASSERT( readSize == needLen, "impossible" ) ;
      rc = _getPool().push( buf, readSize,
                            RTN_LOB_GET_OFFSET_OF_LOB(
                                pageSize,
                                tuples.begin()->tuple.columns.sequence,
                                tuples.begin()->tuple.columns.offset,
                                _getMeta()._version >= DMS_LOB_CURRENT_VERSION ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
         goto error ;
      }

      _getPool().pushDone() ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__READV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_read( const _rtnLobTuple &tuple,
                                    _pmdEDUCB *cb,
                                    CHAR *buf )
   {
      INT32 rc = SDB_OK ;
      UINT32 len = 0 ;
      dmsLobRecord record ;
      record.set( &getOID(),
                  tuple.tuple.columns.sequence,
                  tuple.tuple.columns.offset,
                  tuple.tuple.columns.len,
                  tuple.data ) ;
      rc = _su->lob()->read( record, _mbContext, cb,
                             buf, len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob[%s], sequence[%d], rc:%d",
                 record._oid->str().c_str(), record._sequence, rc ) ;
         goto error ;
      }

      SDB_ASSERT( len == record._dataLen, "impossible" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__ROLLBACK, "_rtnLocalLobStream::_rooback" )
   INT32 _rtnLocalLobStream::_rollback( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__ROLLBACK ) ;
      dmsLobRecord piece ;
      INT32 num = 0 ;

      RTN_LOB_GET_SEQUENCE_NUM( curOffset(), _getPageSz(),
                                _getMeta()._version >= DMS_LOB_CURRENT_VERSION,
                                num ) ;

      while ( 0 < num )
      {
         --num ;
         piece.set( &getOID(), num, 0, 0, NULL ) ;
         rc = _su->lob()->remove( piece, _mbContext, cb,
                                  _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to remove lob[%s],"
                    "sequence:%d, rc:%d", piece._oid->str().c_str(),
                    piece._sequence, rc ) ;
            if ( SDB_LOB_SEQUENCE_NOT_EXIST != rc )
            {
               /// include SDB_DMS_CS_DELETING
               goto error ;
            }
            rc = SDB_OK ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__ROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__QUERYANDINVALIDATEMETADATA, "_rtnLocalLobStream::_queryAndInvalidateMetaData" )
   INT32 _rtnLocalLobStream::_queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                          _dmsLobMeta &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__QUERYANDINVALIDATEMETADATA ) ;
      rc = rtnQueryAndInvalidateLob( getFullName(), getOID(),
                                     cb, 1, _getDPSCB(), meta ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to invalidate lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__QUERYANDINVALIDATEMETADATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_close( _pmdEDUCB *cb )
   {
      _closeInner( cb ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__REMOVEV, "_rtnLocalLobStream::_removev" )
   INT32 _rtnLocalLobStream::_removev( const RTN_LOB_TUPLES &tuples,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__REMOVEV ) ;
      dmsLobRecord record ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         record.set( &getOID(),
                     itr->tuple.columns.sequence,
                     itr->tuple.columns.offset,
                     itr->tuple.columns.len,
                     itr->data ) ;
         rc = _su->lob()->remove( record, _mbContext, cb,
                                  _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove lob[%s],"
                    "sequence:%d, rc:%d", record._oid->str().c_str(),
                    record._sequence, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__REMOVEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

}

