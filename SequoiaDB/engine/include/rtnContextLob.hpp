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

   Source File Name = rtnContextLob.hpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef RTN_CONTEXTLOB_HPP_
#define RTN_CONTEXTLOB_HPP_

#include "rtnContext.hpp"
#include "monInterface.hpp"

using namespace bson ;

namespace engine
{
   class _rtnLobStream ;
   class _rtnLobFetcher ;

   /*
      _rtnContextLob define
   */
   class _rtnContextLob : public _rtnContextBase, public _IMonSubmitEvent
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextLob )
   public:
      _rtnContextLob( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextLob() ;

   public:
      virtual const CHAR*        name() const ;
      virtual RTN_CONTEXT_TYPE   getType() const { return RTN_CONTEXT_LOB ; }
      virtual _dmsStorageUnit*   getSU () ;
      virtual BOOLEAN            isWrite() const ;

      virtual const CHAR *getProcessName() const ;

      virtual UINT32 getSULogicalID() const
      {
         return _suLogicalID ;
      }

   public:
      /*
         Note: The pStream will be takeover in cases both failed and succed
      */
      INT32 open( const BSONObj &lob,
                  INT32 flags,
                  _pmdEDUCB *cb,
                  SDB_DPSCB *dpsCB,
                  _rtnLobStream *pStream ) ;

      INT32 read( UINT32 len,
                  SINT64 offset,
                  _pmdEDUCB *cb ) ;

      INT32 write( UINT32 len,
                   const CHAR *buf,
                   INT64 lobOffset,
                   _pmdEDUCB *cb ) ;

      INT32 lock( _pmdEDUCB *cb,
                  INT64 offset,
                  INT64 length ) ;

      INT32 getRTDetail( _pmdEDUCB *cb, BSONObj &detail, const BSONObj &option = BSONObj() ) ;

      INT32 explain( _pmdEDUCB *cb, BSONObj &detail, const BSONObj &option = BSONObj() ) ;

      INT32 close( _pmdEDUCB *cb ) ;

      INT32 getLobMetaData( BSONObj &meta ) ;

      INT32 mode() const ;

      INT32 flags() const ;

      INT64 getLobLength() const ;

      const bson::OID& getOID() const ;

      virtual void     getErrorInfo( INT32 rc,
                                     _pmdEDUCB *cb,
                                     rtnContextBuf &buffObj ) ;
   public:
      virtual void onSubmit( const monAppCB &delta ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual BOOLEAN   _canPrefetch () const { return TRUE ; }
      virtual void  _toString( stringstream &ss ) ;

   private:
      INT32 _createLobID( bson::OID &oid ) ;

   private:
      _rtnLobStream     *_stream ;
      UINT32            _suLogicalID ;
      SINT64            _offset ;
      UINT32            _readLen ;
      bson::OID         _oid ;
      INT32             _flags ;
   } ;
   typedef class _rtnContextLob rtnContextLob ;

   /*
      _rtnContextLobFetcher define
   */
   class _rtnContextLobFetcher : public rtnContextBase
   {
      DECLARE_RTN_CTX_AUTO_REGISTER( _rtnContextLobFetcher )
      public:
         _rtnContextLobFetcher( INT64 contextID, UINT64 eduID ) ;
         virtual ~_rtnContextLobFetcher() ;

         INT32 open( _rtnLobFetcher *pFetcher,
                     const CHAR *fullName,
                     BOOLEAN onlyMetaPage ) ;

         /*
            Forbidden the function
         */
         virtual INT32     getMore( INT32 maxNumToReturn,
                                    rtnContextBuf &buffObj,
                                    _pmdEDUCB *cb ) ;

         _rtnLobFetcher*   getLobFetcher() ;

      public:
         virtual const CHAR*      name() const ;
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () ;

         virtual UINT32 getSULogicalID() const
         {
            return _suLogicalID ;
         }

      protected:
         virtual INT32     _prepareData( _pmdEDUCB *cb ) { return SDB_OK ; }
         virtual void      _toString( stringstream &ss ) ;

      private:
         _rtnLobFetcher    *_pFetcher ;
         UINT32            _suLogicalID ;

   } ;
   typedef _rtnContextLobFetcher rtnContextLobFetcher ;

}

#endif

