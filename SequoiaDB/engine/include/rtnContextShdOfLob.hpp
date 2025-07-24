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

   Source File Name = rtnContextShdOfLob.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXTSHDOFLOB_HPP_
#define RTN_CONTEXTSHDOFLOB_HPP_

#include "rtnContext.hpp"

namespace engine
{
   class _dmsMBContext ;
   class _dmsStorageUnit ;
   class _SDB_DMSCB ;

   class _rtnContextShdOfLob : public _rtnContextBase
   {
   public:
      _rtnContextShdOfLob( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextShdOfLob() ;

   public:
      virtual RTN_CONTEXT_TYPE getType() const { return RTN_CONTEXT_SHARD_OF_LOB ; }
      virtual _dmsStorageUnit*  getSU () ;

   public:
      INT32 open( const bson::BSONObj &lob,
                  SINT32 flag,
                  SINT32 version,
                  SINT16 w,
                  SDB_DPSCB *dpsCB,
                  _pmdEDUCB *cb,
                  const CHAR **data,
                  UINT32 &read ) ;

      INT32 write( UINT32 sequence,
                   UINT32 offset,
                   UINT32 len,
                   const CHAR *data,
                   _pmdEDUCB *cb ) ;

      INT32 readv( const MsgLobTuple *tuples,
                   UINT32 cnt,
                   _pmdEDUCB *cb,
                   const CHAR **data,
                   UINT32 &read ) ;

      INT32 remove( UINT32 sequence,
                    _pmdEDUCB *cb ) ;

      INT32 update( UINT32 sequence,
                    UINT32 offset,
                    UINT32 len,
                    const CHAR *data,
                    _pmdEDUCB *cb ) ;

      INT32 close( _pmdEDUCB *cb ) ;

   public:
      const CHAR *getFullName() const
      {
         return _fullName.c_str() ;
      }

      INT32 getMode() const
      {
         return _mode ;
      }

      SINT32 getVersion() const
      {
         return _version ;
      }

      INT16 getW() const
      {
         return _w ;
      }

      const bson::OID &getOID() const
      {
         return _oid ;
      }

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _open( _pmdEDUCB *cb,
                   const CHAR **data,
                   UINT32 &read ) ;

      void _meta2Obj( bson::BSONObj &obj ) ;

      INT32 _rollback( _pmdEDUCB *cb ) ;

      INT32 _extendBuf( UINT32 len ) ;

   private:
      std::string    _fullName ;
      bson::OID      _oid ;
      BSONObj        _metaObj ;
      INT32          _mode ;
      INT32          _flags ;
      BOOLEAN        _isMainShd ;
      SINT16         _w ;
      SINT32         _version ;
      _dmsLobMeta    _meta ;
      SDB_DPSCB      *_dpsCB ;
      BOOLEAN        _closeWithException ;
      CHAR           *_buf ;
      UINT32         _bufLen ;
      const CHAR*    _pData ;
      UINT32         _dataLen ;
      INT64          _offset ;
      std::set<UINT32> _written ;

      _dmsStorageUnit   *_su ;
      _dmsMBContext     *_mbContext ;
      _SDB_DMSCB        *_dmsCB ;
      BOOLEAN           _writeDMS ;

   } ;
   typedef class _rtnContextShdOfLob rtnContextShdOfLob ;
}

#endif

