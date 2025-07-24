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

   Source File Name = rtnLobStream.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/08/2014  YW Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOBSTREAM_HPP_
#define RTN_LOBSTREAM_HPP_

#include "dmsLobDef.hpp"
#include "rtnLobWindow.hpp"
#include "rtnLobDataPool.hpp"
#include "msgDef.hpp"
#include "../bson/bson.hpp"

namespace engine
{
#define SDB_LOB_MODE_REMOVE 0x00000010

   class _pmdEDUCB ;
   class _dmsStorageUnit ;
   class _dpsLogWrapper ;
   class _rtnContextBase ;

   class _rtnLobStream : public SDBObject
   {
   public:
      _rtnLobStream() ;
      virtual ~_rtnLobStream() ;

   public:

      INT32 open( const CHAR *fullName,
                  const bson::OID &oid,
                  INT32 mode,
                  INT32 flags,
                  _rtnContextBase *context,
                  _pmdEDUCB *cb ) ;

      INT32 close( _pmdEDUCB *cb ) ;

      INT32 write( UINT32 len,
                   const CHAR *buf,
                   _pmdEDUCB *cb ) ;

      INT32 read( UINT32 len,
                  _rtnContextBase *context,
                  _pmdEDUCB *cb,
                  UINT32 &read ) ;
 
      /// buf may be invalid when do next read.
      /// copy data to your own buf if necessary.
      INT32 next( _pmdEDUCB *cb,
                  const CHAR **buf,
                  UINT32 &len ) ;

      INT32 seek( SINT64 offset,
                  _pmdEDUCB *cb ) ;

      INT32 truncate( SINT64 len,
                      _pmdEDUCB *cb ) ;

      INT32 closeWithException( _pmdEDUCB *cb ) ;

      INT32 getMetaData( bson::BSONObj &meta ) ;

      OSS_INLINE const bson::OID &getOID() const
      {
         return _oid ;
      }

      OSS_INLINE BOOLEAN isOpened() const
      {
         return _opened ;
      }

      OSS_INLINE SINT64 curOffset() const
      {
         return _offset ; 
      }

      virtual _dmsStorageUnit *getSU() = 0 ;

      OSS_INLINE void setDPSCB( _dpsLogWrapper *dpsCB )
      {
         _dpsCB = dpsCB ;
         return ;
      }

      OSS_INLINE const CHAR *getFullName() const
      {
         return _fullName ;
      }

      OSS_INLINE BOOLEAN isReadonly() const
      {
         return SDB_LOB_MODE_R == _mode ? TRUE : FALSE ;
      }

   protected:
      OSS_INLINE _dmsLobMeta &_getMeta()
      {
         return _meta ;
      }

      OSS_INLINE INT32 _getPageSz() const
      {
         return _lobPageSz ;
      }

   protected:
      _dpsLogWrapper *_getDPSCB()
      {
         return _dpsCB ;
      }

      _rtnLobDataPool &_getPool()
      {
         return _pool ;
      }

      INT32 _getMode() const
      {
         return _mode ;
      }

      INT32 _getFlags() const
      {
         return _flags ;
      }

   private:
      virtual INT32 _prepare( const CHAR *fullName,
                              const bson::OID &oid,
                              INT32 mode,
                              _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _queryLobMeta( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta ) = 0 ;

      virtual INT32 _ensureLob( _pmdEDUCB *cb,
                                _dmsLobMeta &meta,
                                BOOLEAN &isNew ) = 0 ;

      virtual INT32 _getLobPageSize( INT32 &pageSize ) = 0 ;

      virtual INT32 _write( const _rtnLobTuple &tuple,
                            _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _writev( const RTN_LOB_TUPLES &tuples,
                             _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _readv( const RTN_LOB_TUPLES &tuples,
                            _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _completeLob( const _rtnLobTuple &tuple,
                                  _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _close( _pmdEDUCB *cb ) = 0 ;

      virtual INT32 _rollback( _pmdEDUCB *cb ) { return SDB_SYS ; }

      virtual INT32 _queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                 _dmsLobMeta &meta ) = 0 ;

      virtual INT32 _removev( const RTN_LOB_TUPLES &tuples,
                              _pmdEDUCB *cb ) = 0 ;

   private:
      INT32 _readFromPool( UINT32 len,
                           _rtnContextBase *context,
                           _pmdEDUCB *cb,
                           UINT32 &readLen ) ;

      INT32 _open4Read( _pmdEDUCB *cb ) ;

      INT32 _open4Create( _pmdEDUCB *cb ) ;

      INT32 _open4Remove( _pmdEDUCB *cb ) ;

      bson::BSONObj _meta2Obj( const _dmsLobMeta &meta ) const ;

   private:
      CHAR                 _fullName[ DMS_COLLECTION_SPACE_NAME_SZ +
                                      DMS_COLLECTION_NAME_SZ + 2 ] ;
      _dpsLogWrapper       *_dpsCB ;
      bson::OID            _oid ;
      _dmsLobMeta          _meta ;
      bson::BSONObj        _metaObj ;
      _rtnLobDataPool      _pool ;
      BOOLEAN              _opened ;
      _rtnLobWindow        _lw ;
      UINT32               _mode ;
      INT32                _flags ;
      SINT32               _lobPageSz ;
      SINT64               _offset ;
   } ;
   typedef class _rtnLobStream rtnLobStream ;
}

#endif

