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

   Source File Name = rtnLocalLobStream.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_LOCALLOBSTREAM_HPP_
#define RTN_LOCALLOBSTREAM_HPP_

#include "rtnLobStream.hpp"
#include "rtnLobAccessManager.hpp"

namespace engine
{
   class _dmsMBContext ;
   class _dmsStorageUnit ;
   class _dpsLogWrapper ;
   class _SDB_DMSCB ;

   class _rtnLocalLobStream : public _rtnLobStream
   {
   public:
      _rtnLocalLobStream() ;
      virtual ~_rtnLocalLobStream() ;

   public:
      virtual _dmsStorageUnit *getSU()
      {
         return _su ;
      }

   private:
      virtual INT32 _prepare( _pmdEDUCB *cb ) ;

      virtual INT32 _queryLobMeta( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta,
                                   BOOLEAN allowUncompleted = FALSE,
                                   _rtnLobPiecesInfo* piecesInfo = NULL ) ;

      virtual INT32 _ensureLob( _pmdEDUCB *cb,
                                _dmsLobMeta &meta,
                                BOOLEAN &isNew ) ;

      virtual INT32 _getLobPageSize( INT32 &pageSize ) ;

      virtual INT32 _write( const _rtnLobTuple &tuple,
                            _pmdEDUCB *cb,
                            BOOLEAN orUpdate = FALSE ) ;

      virtual INT32 _writev( const RTN_LOB_TUPLES &tuples,
                             _pmdEDUCB *cb,
                             BOOLEAN orUpdate = FALSE ) ;

      virtual INT32 _update( const _rtnLobTuple &tuple,
                             _pmdEDUCB *cb ) ;

      virtual INT32 _updatev( const RTN_LOB_TUPLES &tuples,
                             _pmdEDUCB *cb ) ;

      virtual INT32 _completeLob( const _rtnLobTuple &tuple,
                                  _pmdEDUCB *cb ) ;

      virtual INT32 _rollback( _pmdEDUCB *cb ) ;

      virtual INT32 _readv( const RTN_LOB_TUPLES &tuples,
                            _pmdEDUCB *cb,
                            const _rtnLobPiecesInfo* piecesInfo = NULL ) ;

      virtual INT32 _queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                 _dmsLobMeta &meta ) ;

      virtual INT32 _removev( const RTN_LOB_TUPLES &tuples,
                              _pmdEDUCB *cb ) ;

      virtual INT32 _lock( _pmdEDUCB *cb,
                           INT64 offset,
                           INT64 length ) ;

      virtual INT32 _close( _pmdEDUCB *cb ) ;

      virtual INT32 _getRTDetail( _pmdEDUCB *cb,
                                  const RTN_LOB_TUPLES &tuples,
                                  bson::BSONObj &detail,
                                  const _rtnLobPiecesInfo* piecesInfo = NULL,
                                  const bson::BSONObj &option = BSONObj() ) ;

      virtual INT32 _explain( _pmdEDUCB *cb,
                              const RTN_LOB_TUPLES &tuples,
                              bson::BSONObj &detail,
                              const _rtnLobPiecesInfo* piecesInfo = NULL,
                              const bson::BSONObj &option = BSONObj() ) ;

      INT32 _read( const _rtnLobTuple &tuple,
                   _pmdEDUCB *cb,
                   CHAR *buf ) ;

      INT32 _getAccessPrivilege( const CHAR *fullName,
                                 const bson::OID &oid,
                                 INT32 mode ) ;

      virtual void _onIncreaseMetrics( const monAppCB &delta ) ;

   private:
      void        _closeInner( _pmdEDUCB *cb ) ;
      INT32       _queryLobMeta4Write( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta,
                                   _rtnLobPiecesInfo* piecesInfo ) ;

   private:
      _dmsMBContext*       _mbContext ;
      _dmsStorageUnit*     _su ;
      _SDB_DMSCB*          _dmsCB ;
      _rtnLobAccessInfo*   _accessInfo ;
      BOOLEAN              _hasLobPrivilege ;
   } ;
   typedef class _rtnLocalLobStream rtnLocalLobStream ;
}

#endif

