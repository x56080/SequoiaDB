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

   Source File Name = rtnContextShdOfLob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnContextShdOfLob.hpp"
#include "pmdEDU.hpp"
#include "rtnTrace.hpp"
#include "rtnLob.hpp"
#include "clsMgr.hpp"

using namespace bson ;

namespace engine
{
   _rtnContextShdOfLob::_rtnContextShdOfLob( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _mode( SDB_LOB_MODE_R ),
    _flags( 0 ),
    _isMainShd( FALSE ),
    _w( 1 ),
    _version( 0 ),
    _dpsCB( NULL ),
    _closeWithException( TRUE ),
    _buf( NULL ),
    _bufLen( 0 ),
    _su( NULL ),
    _mbContext( NULL ),
    _dmsCB( NULL ),
    _writeDMS( FALSE )
   {
      _pData = NULL ;
      _dataLen = 0 ;
      _offset = 0 ;
   }

   _rtnContextShdOfLob::~_rtnContextShdOfLob()
   {
      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( _closeWithException &&
           SDB_LOB_MODE_CREATEONLY == _mode )
      {
         SDB_ASSERT( cb->getID() == eduID(), "impossible" ) ;
         _rollback( cb ) ;
      }

      close( cb ) ;

      SAFE_OSS_FREE( _buf ) ;
   }

   _dmsStorageUnit* _rtnContextShdOfLob::getSU ()
   {
      return _su ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_OPEN, "_rtnContextShdOfLob::open" )
   INT32 _rtnContextShdOfLob::open( const BSONObj &lob,
                                    SINT32 flag,
                                    SINT32 version,
                                    SINT16 w,
                                    SDB_DPSCB *dpsCB,
                                    _pmdEDUCB *cb,
                                    const CHAR **data,
                                    UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_OPEN ) ;
      _dmsCB = sdbGetDMSCB() ;
      const CHAR *clName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      BSONElement ele = lob.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid full name type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _fullName.assign( ele.valuestr() ) ;

      rc = rtnResolveCollectionNameAndLock( _fullName.c_str(),
                                            _dmsCB, &_su,
                                            &clName, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get cs lock: %s, rc: %d",
                 _fullName.c_str(), rc ) ;
         goto error ;
      }

      /// get mb context
      rc = _su->data()->getMBContext( &_mbContext, clName, -1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get collection[%s] mb context, rc: %d",
                 _fullName.c_str(), rc ) ;
         goto error ;
      }

      ele = lob.getField( FIELD_NAME_LOB_OPEN_MODE ) ;
      if ( NumberInt != ele.type() )
      {
         PD_LOG( PDERROR, "invalid mode type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _mode = ele.Int() ;

      ele = lob.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid oid type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossMemcpy( &_oid, &( ele.__oid() ), sizeof( _oid ) ) ;

      ele = lob.getField( FIELD_NAME_LOB_IS_MAIN_SHD ) ;
      if ( Bool != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid \"isMainShd\" type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _isMainShd = ele.Bool() ;

      ele = lob.getField( FIELD_NAME_LOB_META_DATA ) ;
      if ( Object == ele.type() )
      {
         _metaObj = ele.embeddedObject() ;
      }
      else if ( !ele.eoo() )
      {
         PD_LOG( PDERROR, "invalid meta obj type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _w = w ;
      _dpsCB = dpsCB ;
      _version = version ;
      _flags = flag ;

      if ( SDB_LOB_MODE_R != _mode )
      {
         rc = _dmsCB->writable( cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "database is not writable, rc = %d", rc ) ;
            goto error ;
         }
         _writeDMS = TRUE ;
      }

      rc = _open( cb, data, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob:%d", rc ) ;
         goto error ;
      }
      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      /// write down
      if ( _writeDMS )
      {
         _dmsCB->writeDown( cb ) ;
         _writeDMS = FALSE ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_OPEN, rc ) ;
      return rc ;
   error:
      close( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_WRITE, "_rtnContextShdOfLob::write" )
   INT32 _rtnContextShdOfLob::write( UINT32 sequence,
                                     UINT32 offset,
                                     UINT32 len,
                                     const CHAR *data,
                                     _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_WRITE ) ;
      rc = rtnWriteLob( _fullName.c_str(),
                        _oid, sequence,
                        offset, len, data, cb,
                        _w, _dpsCB, _su, _mbContext ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }

      _written.insert( sequence ) ;
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_UPDATE, "_rtnContextShdOfLob::update" )
   INT32 _rtnContextShdOfLob::update( UINT32 sequence,
                                      UINT32 offset,
                                      UINT32 len,
                                      const CHAR *data,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_UPDATE ) ;
      rc = rtnUpdateLob( _fullName.c_str(),
                         _oid, sequence,
                         offset, len, data, cb,
                         _w, _dpsCB, _su, _mbContext ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_UPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextShdOfLob::_prepareData( _pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   void _rtnContextShdOfLob::_toString( stringstream &ss )
   {
      ss << ",Name:" << _fullName.c_str()
         << ",OID:" << _oid.toString().c_str()
         << ",Mode:" << _mode
         << ",IsMainShard:" << _isMainShd
         << ",BuffLen:" << _bufLen ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB__OPEN, "_rtnContextShdOfLob::_open" )
   INT32 _rtnContextShdOfLob::_open( _pmdEDUCB *cb,
                                     const CHAR **data,
                                     UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB__OPEN ) ;
      if ( _isMainShd && SDB_LOB_MODE_R == _mode )
      {
         UINT32 readLen = 0 ;
         UINT32 len = _su->getLobPageSize() ;
         dmsLobRecord record ;

         record.set( &_oid, DMS_LOB_META_SEQUENCE, 0, len, NULL ) ;

         rc = _extendBuf( len * 2 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf[%u], rc:%d", len * 2, rc ) ;
            goto error ;
         }
         /// read the whole page
         rc = _su->lob()->read( record, _mbContext, cb, _buf + len, readLen ) ;
         if ( SDB_OK == rc )
         {
            if ( readLen < sizeof( _meta ) )
            {
               PD_LOG( PDERROR, "Read lob[%s]'s meta page len is less than "
                       "meta size[%u]", getOID().str().c_str(),
                       sizeof( _meta ) ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            /// copy data
            ossMemcpy( (void*)&_meta, _buf + len, sizeof( _meta ) ) ;
            if ( !_meta.isDone() )
            {
               PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                       getOID().str().c_str(), _meta.toString().c_str() ) ;
               rc = SDB_LOB_IS_NOT_AVAILABLE ;
               goto error ;
            }
            /// if meta page has data
            if ( _meta._version >= DMS_LOB_CURRENT_VERSION &&
                 _meta._lobLen > 0 &&
                 readLen > DMS_LOB_META_LENGTH )
            {
               _pData = _buf + len + DMS_LOB_META_LENGTH ;
               _dataLen = readLen - DMS_LOB_META_LENGTH ;
               if ( _dataLen > _meta._lobLen )
               {
                  _dataLen = _meta._lobLen ;
               }
               _offset = 0 ;
               /// add msg tuple
               _pData -= sizeof( MsgLobTuple ) ;
               _dataLen += sizeof( MsgLobTuple ) ;
               MsgLobTuple *rt = (MsgLobTuple*)_pData ;
               rt->columns.sequence = DMS_LOB_META_SEQUENCE ;
               rt->columns.len = _dataLen - sizeof( MsgLobTuple ) ;
               rt->columns.offset = DMS_LOB_META_LENGTH ;
            }
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
      }
      else if ( SDB_LOB_MODE_CREATEONLY == _mode && _isMainShd )
      {
         rc = rtnCreateLob( _fullName.c_str(),
                            _oid, cb, _w, _dpsCB,
                            _su, _mbContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to create lob:%d", rc ) ;
            goto error ;
         }

         _written.insert( DMS_LOB_META_SEQUENCE ) ;
      }
      else if ( _isMainShd && SDB_LOB_MODE_REMOVE == _mode )
      {
         rc = rtnQueryAndInvalidateLob( _fullName.c_str(),
                                        _oid, cb, _w,
                                        _dpsCB, _meta,
                                        _su, _mbContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to invalidate lob:%d", rc ) ;
            goto error ;
         }
      }

      if ( _isMainShd )
      {
         _meta2Obj( _metaObj ) ;

         if ( _pData )
         {
            if ( _flags & FLG_LOBOPEN_WITH_RETURNDATA )
            {
               UINT32 tmpLen = _metaObj.objsize() ;
               tmpLen = ossAlign4( tmpLen ) ;
               _pData -= tmpLen ;
               _dataLen += tmpLen ;
               ossMemcpy( (CHAR*)_pData, _metaObj.objdata(),
                          _metaObj.objsize() ) ;
               *data = _pData ;
               read = _dataLen ;

               _pData = NULL ;
               _dataLen = 0 ;
               goto done ;
            }
            else
            {
                ossMemmove( _buf, _pData, _dataLen ) ;
                _pData = _buf ;
            }
         }
      }

      /// we need to send back pagesize when this node
      ///  is not the main shard.
      *data = _metaObj.objdata() ;
      read = _metaObj.objsize() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB__OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_READV, "_rtnContextShdOfLob::readv" )
   INT32 _rtnContextShdOfLob::readv( const MsgLobTuple *tuples,
                                     UINT32 cnt,
                                     _pmdEDUCB *cb,
                                     const CHAR **data,
                                     UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_READV ) ;
      SDB_ASSERT( NULL != tuples && 0 < cnt, "can not be null" ) ;
      UINT32 totalRead = 0 ;
      UINT32 i = 0 ;
      UINT32 len = 0 ;

      /// calc total buff size
      for ( i = 0 ; i < cnt ; ++i )
      {
         const MsgLobTuple &t = tuples[i] ;
         len += sizeof( MsgLobTuple ) ;
         len += t.columns.len ;
      }

      /// extend buf
      rc = _extendBuf( len ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to extend buf[%u], rc: %d", len, rc ) ;
         goto error ;
      }

      /// read all tuples
      for ( i = 0; i < cnt; ++i )
      {
         UINT32 onceRead = 0 ;
         CHAR *dataOfTuple = NULL ;
         MsgLobTuple *rt = NULL ;
         const MsgLobTuple &t = tuples[i] ;

         dataOfTuple = _buf + totalRead ;
         rt = ( MsgLobTuple * )dataOfTuple ;
         dataOfTuple += sizeof( MsgLobTuple ) ;

         if ( _pData && _dataLen > 0 &&
              rt->columns.sequence == t.columns.sequence &&
              rt->columns.offset == t.columns.offset )
         {
            onceRead = rt->columns.len ;
         }
         else
         {
            rc = rtnReadLob( _fullName.c_str(),
                             _oid, t.columns.sequence,
                             t.columns.offset, t.columns.len,
                             cb, dataOfTuple, onceRead,
                             _su, _mbContext ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to read lob[%s:%d], rc:%d",
                       _oid.str().c_str(), t.columns.sequence, rc ) ;
               goto error ;
            }
            rt->columns.sequence = t.columns.sequence ;
            rt->columns.offset = t.columns.offset ;
            rt->columns.len = onceRead ;
         }
         _pData = NULL ;
         _dataLen = 0 ;
         onceRead += sizeof( MsgLobTuple ) ; /// | tuple | data | tuple | data |
         totalRead += onceRead ;
      }

      *data = _buf ;
      read = totalRead ;
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_READV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextShdOfLob::remove( UINT32 sequence,
                                      _pmdEDUCB *cb )
   {
      return rtnRemoveLobPiece( _fullName.c_str(),
                                _oid, sequence, cb,
                                _w, _dpsCB, _su, _mbContext ) ;
   }

   INT32 _rtnContextShdOfLob::close( _pmdEDUCB *cb )
   {
      _isOpened = FALSE ;
      _closeWithException = FALSE ;

      if ( _mbContext && _su )
      {
         _su->data()->releaseMBContext( _mbContext ) ;
         _mbContext = NULL ;
      }
      if ( _su && _dmsCB )
      {
         _dmsCB->suUnlock( _su->CSID() ) ;
         _su = NULL ;
      }
      if ( _writeDMS )
      {
         _dmsCB->writeDown( cb ) ;
         _writeDMS = FALSE ;
      }
      return SDB_OK ; 
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB__ROLLBACK, "_rtnContextShdOfLob::_rollback" )
   INT32 _rtnContextShdOfLob::_rollback( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB__ROLLBACK ) ;
      UINT64 sucNum = 0 ;
      std::set<UINT32>::reverse_iterator itr = _written.rbegin() ;
      for ( ; itr != _written.rend(); ++itr )
      {
         if ( !sdbGetReplCB()->primaryIsMe() )
         {
            PD_LOG( PDERROR, "we are not primary any more, stop to rollback" ) ;
            break ;
         }

         rc = rtnRemoveLobPiece( _fullName.c_str(),
                                 _oid, *itr, cb,
                                 _w, _dpsCB, _su, _mbContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove piece[%d] of lob, rc:%d",
                    *itr, rc ) ;
            if ( SDB_DMS_CS_DELETING == rc )
            {
               break ;
            }
            rc = SDB_OK ;
            /// do not goto error. try to rollback all pieces.
         }
         else
         {
            ++sucNum ;
         }
      }

      PD_LOG( PDEVENT, "rollback[%s]: we removed %d pieces, failed:%d",
              _oid.str().c_str(),
              _written.size(),  _written.size() - sucNum ) ;
      _written.clear() ;

      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB__ROLLBACK, rc ) ;
      return rc ;
   }

   void _rtnContextShdOfLob::_meta2Obj( bson::BSONObj &obj )
   {
      BSONObjBuilder builder ;
      builder.append( FIELD_NAME_LOB_SIZE, (long long)_meta._lobLen ) ;
      builder.append( FIELD_NAME_LOB_PAGE_SIZE,
                      NULL != _su ? _su->getLobPageSize() : 0 ) ;
      builder.append( FIELD_NAME_VERSION, (INT32)_meta._version ) ;
      builder.append( FIELD_NAME_LOB_CREATTIME, (long long)_meta._createTime ) ;
      obj = builder.obj() ;
   }

   INT32 _rtnContextShdOfLob::_extendBuf( UINT32 len )
   {
      INT32 rc = SDB_OK ;
      if ( _bufLen < len )
      {
         len = ossRoundUpToMultipleX( len, _su->getLobPageSize() ) ;
         CHAR *buf = _buf ;
         _buf = ( CHAR * )SDB_OSS_REALLOC( _buf, len ) ;
         if ( NULL == _buf )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            _buf = buf ;
            goto error ;
         }
         _bufLen = len ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

