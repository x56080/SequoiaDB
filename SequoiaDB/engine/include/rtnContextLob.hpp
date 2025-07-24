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

using namespace bson ;

namespace engine
{
   class _rtnLobStream ;
   class _rtnLobFetcher ;

   class _rtnContextLob : public _rtnContextBase
   {
   public:
      _rtnContextLob( INT64 contextID, UINT64 eduID ) ;
      virtual ~_rtnContextLob() ;

   public:
      virtual RTN_CONTEXT_TYPE getType() const { return RTN_CONTEXT_LOB ; }
      virtual _dmsStorageUnit*  getSU () ;

   public:
      INT32 open( const BSONObj &lob,
                  BOOLEAN isLocal,
                  INT32 flags,
                  _pmdEDUCB *cb,
                  SDB_DPSCB *dpsCB ) ;

      INT32 read( UINT32 len,
                  SINT64 offset,
                  _pmdEDUCB *cb ) ;

      INT32 write( UINT32 len,
                   const CHAR *buf,
                   _pmdEDUCB *cb ) ;

      INT32 close( _pmdEDUCB *cb ) ;

      INT32 getLobMetaData( BSONObj &meta ) ;

   protected:
      virtual INT32 _prepareData( _pmdEDUCB *cb ) ;
      virtual BOOLEAN   _canPrefetch () const { return TRUE ; }
      virtual void  _toString( stringstream &ss ) ;

   private:
      rtnLobStream *_stream ;
      SINT64 _offset ;
      UINT32 _readLen ;
   } ;
   typedef class _rtnContextLob rtnContextLob ;

   /*
      _rtnContextLobFetcher define
   */
   class _rtnContextLobFetcher : public rtnContextBase
   {
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
         virtual RTN_CONTEXT_TYPE getType () const ;
         virtual _dmsStorageUnit* getSU () ;

      protected:
         virtual INT32     _prepareData( _pmdEDUCB *cb ) { return SDB_OK ; }
         virtual void      _toString( stringstream &ss ) ;

      private:
         _rtnLobFetcher    *_pFetcher ;

   } ;
   typedef _rtnContextLobFetcher rtnContextLobFetcher ;

}

#endif

